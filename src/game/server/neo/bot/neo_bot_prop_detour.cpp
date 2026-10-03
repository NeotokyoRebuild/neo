#include "cbase.h"
#include "nav_mesh.h"
#include "NextBot.h"
#include "NextBot/Path/NextBotPathFollow.h"
#include "NextBotLocomotionInterface.h"
#include "NextBotBodyInterface.h"
#include "neo_bot_prop_detour.h"
#include "tier1/utlpriorityqueue.h"
#include "vphysics_interface.h"

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

// How often the path ahead is checked, so a prop that is pushed or animates is followed
static const float REPLAN_INTERVAL = 0.25f;

// How far along the path props are looked for, and how much further the detour may rejoin it
static const float LOOK_AHEAD_RANGE = 256.0f;
static const float REJOIN_EXTRA_RANGE = 128.0f;

// The detour rejoins the path this far past the last prop in the way
static const float REJOIN_PAST_PROP = 32.0f;

// Floor around the path and the props that a detour may use
static const float GRID_MARGIN = 48.0f;
static const float GRID_CELL_SIZE = 16.0f;
static const int GRID_MAX_CELLS_PER_SIDE = 64;
static const int SEARCH_MAX_EXPANSIONS = 4096;

// A detour stays on floor this close in height to the bot's feet or to where it rejoins the path
static const float FLOOR_HEIGHT_TOLERANCE = 40.0f;

// Stepping through a cell a prop occupies costs as much as this many free cells:
// the search crosses a prop only to get out from under one, and a route that has to cross one is no detour
static const float OCCUPIED_CELL_COST = 1000.0f;

static const float WAYPOINT_REACHED_RANGE = 12.0f;

// A moving prop is kept clear of where it will be within this time, at its current velocity
static const float MOTION_PREDICT_TIME = 1.0f;
static const float MOVING_PROP_MIN_SPEED = 10.0f;

// Props are looked for up to this high above the path, so one coming down is seen in time
static const float PROP_QUERY_HEADROOM = 512.0f;
static const int PROP_QUERY_MAX_ENTITIES = 256;

// A prop smaller than this in every direction (a can, a bottle) is pushed aside, not walked around
static const Vector SMALL_PROP_SIZE( 16.0f, 16.0f, 40.0f );

static const float DIAGONAL_STEP_LENGTH = 1.41421356f;

// The body box is this much narrower than the hull, so a prop the bot only brushes is not in its way
static const float BODY_CLEARANCE = 1.0f;


//----------------------------------------------------------------------------------------------------------------
namespace
{
	struct PropObstacle_t
	{
		CBaseEntity *entity;
		Vector sweep;			// how far it moves within MOTION_PREDICT_TIME
		Vector2D lo, hi;		// footprint over that time, grown by a hull half-width
	};

	// The bot's body as a box: from a step over the floor to its standing height
	struct BodyBox_t
	{
		Vector mins, maxs;
	};

	// Return true if the line from a to b crosses the box, with the entry and exit fractions
	bool SegmentCrossesBox( const Vector2D &a, const Vector2D &b, const Vector2D &lo, const Vector2D &hi, float *enter, float *exit )
	{
		float t0 = 0.0f;
		float t1 = 1.0f;
		const Vector2D delta = b - a;
		for ( int axis = 0; axis < 2; ++axis )
		{
			if ( fabsf( delta[ axis ] ) < 1e-4f )
			{
				if ( a[ axis ] < lo[ axis ] || a[ axis ] > hi[ axis ] )
				{
					return false;
				}

				continue;
			}

			float ta = ( lo[ axis ] - a[ axis ] ) / delta[ axis ];
			float tb = ( hi[ axis ] - a[ axis ] ) / delta[ axis ];
			if ( ta > tb )
			{
				V_swap( ta, tb );
			}

			t0 = MAX( t0, ta );
			t1 = MIN( t1, tb );
			if ( t0 > t1 )
			{
				return false;
			}
		}

		*enter = t0;
		*exit = t1;
		return true;
	}
}


//----------------------------------------------------------------------------------------------------------------
// Props the nav mesh cannot account for: physics props and physboxes move when pushed or shot,
// and bone followers move with an animated prop's bones (fixed props belong to the mesh)
static bool IsMovableProp( CBaseEntity *entity )
{
	return FClassnameIs( entity, "prop_physics*" ) || FClassnameIs( entity, "func_physbox*" ) || FClassnameIs( entity, "phys_bone_follower" );
}


//----------------------------------------------------------------------------------------------------------------
static bool IsSolidToPlayers( CBaseEntity *entity )
{
	if ( !entity->IsSolid() || entity->IsSolidFlagSet( FSOLID_TRIGGER ) )
	{
		return false;
	}

	return g_pGameRules->ShouldCollide( COLLISION_GROUP_PLAYER_MOVEMENT, entity->GetCollisionGroup() );
}


//----------------------------------------------------------------------------------------------------------------
static Vector GetPropVelocity( CBaseEntity *entity )
{
	IPhysicsObject *physics = entity->VPhysicsGetObject();
	if ( !physics || physics->IsAsleep() )
	{
		return vec3_origin;
	}

	Vector velocity;
	physics->GetVelocity( &velocity, NULL );
	return velocity.IsLengthGreaterThan( MOVING_PROP_MIN_SPEED ) ? velocity : vec3_origin;
}


//----------------------------------------------------------------------------------------------------------------
// The bot can see the prop: no world geometry between its eyes and the prop's middle
static bool IsInSight( INextBot *bot, CBaseEntity *entity )
{
	trace_t result;
	CTraceFilterWorldOnly filter;
	UTIL_TraceLine( bot->GetBodyInterface()->GetEyePosition(), entity->WorldSpaceCenter(), MASK_BLOCKLOS, &filter, &result );
	return !result.DidHit();
}


//----------------------------------------------------------------------------------------------------------------
static BodyBox_t GetBodyBox( INextBot *bot )
{
	const float halfWidth = 0.5f * bot->GetBodyInterface()->GetHullWidth() - BODY_CLEARANCE;

	BodyBox_t box;
	box.mins.Init( -halfWidth, -halfWidth, bot->GetLocomotionInterface()->GetStepHeight() );
	box.maxs.Init( halfWidth, halfWidth, bot->GetBodyInterface()->GetStandHullHeight() );
	return box;
}


//----------------------------------------------------------------------------------------------------------------
// Return true if a body moving from 'from' to 'to' touches the prop, now or where it will be
static bool BodyMeetsProp( const BodyBox_t &body, const Vector &from, const Vector &to, const PropObstacle_t &obstacle, float *fraction )
{
	*fraction = 1.0f;

	// moving the body back along the prop's motion is the same as moving the prop forward
	const int tests = obstacle.sweep.IsZero() ? 1 : 2;
	for ( int i = 0; i < tests; ++i )
	{
		const Vector shift = ( i == 0 ) ? vec3_origin : -obstacle.sweep;

		Ray_t ray;
		ray.Init( from + shift, to + shift, body.mins, body.maxs );

		trace_t result;
		enginetrace->ClipRayToEntity( ray, MASK_PLAYERSOLID, obstacle.entity, &result );
		if ( result.startsolid )
		{
			*fraction = 0.0f;
			return true;
		}

		*fraction = MIN( *fraction, result.fraction );
	}

	return *fraction < 1.0f;
}


//----------------------------------------------------------------------------------------------------------------
// The path ahead as a line from the bot's feet, over walked segments only, up to the range,
// with the segment each point of the line belongs to
static void GetPathAhead( const PathFollower &path, const Vector &feet, float range, CUtlVector< Vector > *line, CUtlVector< const Path::Segment * > *segments )
{
	line->AddToTail( feet );
	segments->AddToTail( path.GetCurrentGoal() );

	float length = 0.0f;
	for ( const Path::Segment *seg = path.GetCurrentGoal(); seg && length < range; seg = path.NextSegment( seg ) )
	{
		if ( seg->ladder || seg->type != Path::ON_GROUND )
		{
			break;
		}

		const Vector leg = seg->pos - line->Tail();
		const float legLength = leg.Length2D();
		segments->AddToTail( seg );
		if ( length + legLength > range )
		{
			line->AddToTail( line->Tail() + leg * ( ( range - length ) / legLength ) );
			break;
		}

		length += legLength;
		line->AddToTail( seg->pos );
	}
}


//----------------------------------------------------------------------------------------------------------------
// Return the point the given distance along the line, and the index of the leg's end
static Vector GetPointAlong( const CUtlVector< Vector > &line, float distance, int *legEnd )
{
	for ( int i = 0; i + 1 < line.Count(); ++i )
	{
		const float legLength = ( line[ i + 1 ] - line[ i ] ).Length2D();
		if ( distance <= legLength && legLength > 0.0f )
		{
			*legEnd = i + 1;
			return line[ i ] + ( line[ i + 1 ] - line[ i ] ) * ( distance / legLength );
		}

		distance -= legLength;
	}

	*legEnd = line.Count() - 1;
	return line.Tail();
}


//----------------------------------------------------------------------------------------------------------------
// Movable props the bot can see that stand, or will soon stand, in its body space over the floor in the box
static void CollectProps( INextBot *bot, const Vector &floorLo, const Vector &floorHi, CUtlVector< PropObstacle_t > *obstacles )
{
	const BodyBox_t body = GetBodyBox( bot );
	const float halfWidth = 0.5f * bot->GetBodyInterface()->GetHullWidth();

	const Vector queryLo = floorLo - Vector( GRID_MARGIN, GRID_MARGIN, FLOOR_HEIGHT_TOLERANCE );
	const Vector queryHi = floorHi + Vector( GRID_MARGIN, GRID_MARGIN, PROP_QUERY_HEADROOM );

	CBaseEntity *list[ PROP_QUERY_MAX_ENTITIES ];
	const int count = UTIL_EntitiesInBox( list, ARRAYSIZE( list ), queryLo, queryHi, 0 );
	for ( int i = 0; i < count; ++i )
	{
		CBaseEntity *entity = list[ i ];
		if ( !IsMovableProp( entity ) || !IsSolidToPlayers( entity ) )
		{
			continue;
		}

		Vector propLo, propHi;
		entity->CollisionProp()->WorldSpaceAABB( &propLo, &propHi );

		const Vector size = propHi - propLo;
		if ( size.x < SMALL_PROP_SIZE.x && size.y < SMALL_PROP_SIZE.y && size.z < SMALL_PROP_SIZE.z )
		{
			continue;
		}

		const Vector sweep = GetPropVelocity( entity ) * MOTION_PREDICT_TIME;
		Vector sweptLo = propLo;
		Vector sweptHi = propHi;
		VectorMin( sweptLo, propLo + sweep, sweptLo );
		VectorMax( sweptHi, propHi + sweep, sweptHi );

		// overhead or underfoot all over the box
		if ( sweptLo.z > floorHi.z + body.maxs.z || sweptHi.z < floorLo.z + body.mins.z )
		{
			continue;
		}

		if ( !IsInSight( bot, entity ) )
		{
			continue;
		}

		PropObstacle_t obstacle;
		obstacle.entity = entity;
		obstacle.sweep = sweep;
		obstacle.lo.Init( sweptLo.x - halfWidth, sweptLo.y - halfWidth );
		obstacle.hi.Init( sweptHi.x + halfWidth, sweptHi.y + halfWidth );
		obstacles->AddToTail( obstacle );
	}
}


//----------------------------------------------------------------------------------------------------------------
// Cells over the nav mesh around the path ahead, each off the mesh, free, or occupied by a prop:
// a search over them finds the way past the props that stays on the mesh
class CPropDetourGrid
{
public:
	CPropDetourGrid( INextBot *bot, const Vector2D &lo, const Vector2D &hi, float floorLo, float floorHi );

	void MarkProps( const CUtlVector< PropObstacle_t > &obstacles );
	bool FindRoute( const Vector &from, const Vector &to, CUtlVector< int > *route ) const;
	void Straighten( const CUtlVector< int > &route, CUtlVector< Vector > *waypoints ) const;

	bool IsOccupied( int cell ) const { return m_state[ cell ] == CELL_OCCUPIED; }

private:
	enum CellState
	{
		CELL_OFF_MESH,
		CELL_FREE,
		CELL_OCCUPIED,
	};

	struct OpenCell_t
	{
		float estimate;
		int cell;
	};

	static bool IsFartherFromGoal( const OpenCell_t &a, const OpenCell_t &b ) { return a.estimate > b.estimate; }

	int GetCell( const Vector &pos ) const;
	int GetNearestOnMesh( int cell ) const;
	Vector2D GetCellCenter( int cell ) const;
	Vector GetCellFloor( int cell ) const;
	bool CanStep( int from, int to ) const;
	bool IsLineClear( int from, int to ) const;

	INextBot *m_bot;
	Vector2D m_origin;
	float m_cellSize;
	int m_width;
	int m_height;

	CUtlVector< unsigned char > m_state;
	CUtlVector< float > m_floor;
	CUtlVector< const CNavArea * > m_area;
};


//----------------------------------------------------------------------------------------------------------------
CPropDetourGrid::CPropDetourGrid( INextBot *bot, const Vector2D &lo, const Vector2D &hi, float floorLo, float floorHi )
{
	m_bot = bot;
	m_origin = lo;

	// coarser cells for a big region, so the search stays cheap
	m_cellSize = GRID_CELL_SIZE;
	while ( ( hi.x - lo.x ) / m_cellSize > GRID_MAX_CELLS_PER_SIDE || ( hi.y - lo.y ) / m_cellSize > GRID_MAX_CELLS_PER_SIDE )
	{
		m_cellSize *= 1.5f;
	}

	m_width = (int)ceilf( ( hi.x - lo.x ) / m_cellSize );
	m_height = (int)ceilf( ( hi.y - lo.y ) / m_cellSize );

	const int cellCount = m_width * m_height;
	m_state.SetCount( cellCount );
	m_floor.SetCount( cellCount );
	m_area.SetCount( cellCount );

	const int team = bot->GetEntity()->GetTeamNumber();
	const float floorTop = floorHi + FLOOR_HEIGHT_TOLERANCE;
	const float floorRange = floorTop - ( floorLo - FLOOR_HEIGHT_TOLERANCE );
	for ( int cell = 0; cell < cellCount; ++cell )
	{
		const Vector2D center = GetCellCenter( cell );
		const CNavArea *area = TheNavMesh->GetNavArea( Vector( center.x, center.y, floorTop ), floorRange );

		m_area[ cell ] = area;
		m_state[ cell ] = ( area && !area->IsBlocked( team ) ) ? CELL_FREE : CELL_OFF_MESH;
		m_floor[ cell ] = area ? area->GetZ( center.x, center.y ) : 0.0f;
	}
}


//----------------------------------------------------------------------------------------------------------------
// A cell is occupied when the bot's body standing there would touch a prop, now or soon
void CPropDetourGrid::MarkProps( const CUtlVector< PropObstacle_t > &obstacles )
{
	const BodyBox_t body = GetBodyBox( m_bot );

	for ( int cell = 0; cell < m_state.Count(); ++cell )
	{
		if ( m_state[ cell ] != CELL_FREE )
		{
			continue;
		}

		const Vector at = GetCellFloor( cell );

		FOR_EACH_VEC( obstacles, i )
		{
			const PropObstacle_t &obstacle = obstacles[ i ];
			if ( at.x < obstacle.lo.x || at.x > obstacle.hi.x || at.y < obstacle.lo.y || at.y > obstacle.hi.y )
			{
				continue;
			}

			float fraction;
			if ( BodyMeetsProp( body, at, at, obstacle, &fraction ) )
			{
				m_state[ cell ] = CELL_OCCUPIED;
				break;
			}
		}
	}
}


//----------------------------------------------------------------------------------------------------------------
int CPropDetourGrid::GetCell( const Vector &pos ) const
{
	const int x = clamp( (int)( ( pos.x - m_origin.x ) / m_cellSize ), 0, m_width - 1 );
	const int y = clamp( (int)( ( pos.y - m_origin.y ) / m_cellSize ), 0, m_height - 1 );
	return y * m_width + x;
}


//----------------------------------------------------------------------------------------------------------------
Vector2D CPropDetourGrid::GetCellCenter( int cell ) const
{
	const int x = cell % m_width;
	const int y = cell / m_width;
	return Vector2D( m_origin.x + ( x + 0.5f ) * m_cellSize, m_origin.y + ( y + 0.5f ) * m_cellSize );
}


//----------------------------------------------------------------------------------------------------------------
// The floor at the middle of the cell
Vector CPropDetourGrid::GetCellFloor( int cell ) const
{
	const Vector2D center = GetCellCenter( cell );
	return Vector( center.x, center.y, m_floor[ cell ] );
}


//----------------------------------------------------------------------------------------------------------------
// The given cell if it is on the mesh, else the nearest one around it that is (-1 if none)
int CPropDetourGrid::GetNearestOnMesh( int cell ) const
{
	const int searchRadius = 2;
	const int cx = cell % m_width;
	const int cy = cell / m_width;
	for ( int r = 0; r <= searchRadius; ++r )
	{
		for ( int dy = -r; dy <= r; ++dy )
		{
			for ( int dx = -r; dx <= r; ++dx )
			{
				const int x = cx + dx;
				const int y = cy + dy;
				if ( x < 0 || y < 0 || x >= m_width || y >= m_height )
				{
					continue;
				}

				if ( m_state[ y * m_width + x ] != CELL_OFF_MESH )
				{
					return y * m_width + x;
				}
			}
		}
	}

	return -1;
}


//----------------------------------------------------------------------------------------------------------------
// Walking from one cell to its neighbor stays on the mesh: the same area or a connected one, no higher than a step
// (two areas either side of a wall are both on the mesh, but not connected)
bool CPropDetourGrid::CanStep( int from, int to ) const
{
	if ( m_state[ to ] == CELL_OFF_MESH )
	{
		return false;
	}

	if ( m_floor[ to ] - m_floor[ from ] > m_bot->GetLocomotionInterface()->GetStepHeight() )
	{
		return false;
	}

	return m_area[ from ] == m_area[ to ] || m_area[ from ]->IsConnected( m_area[ to ], NUM_DIRECTIONS );
}


//----------------------------------------------------------------------------------------------------------------
// A* over the cells, 8-connected. Occupied cells cost more than free ones rather than being walls,
// so a bot already under or against a prop finds its way out
bool CPropDetourGrid::FindRoute( const Vector &from, const Vector &to, CUtlVector< int > *route ) const
{
	const int start = GetNearestOnMesh( GetCell( from ) );
	const int goal = GetNearestOnMesh( GetCell( to ) );
	if ( start < 0 || goal < 0 )
	{
		return false;
	}

	const int cellCount = m_state.Count();
	CUtlVector< float > costSoFar;
	CUtlVector< int > cameFrom;
	CUtlVector< bool > isClosed;
	costSoFar.SetCount( cellCount );
	cameFrom.SetCount( cellCount );
	isClosed.SetCount( cellCount );
	for ( int cell = 0; cell < cellCount; ++cell )
	{
		costSoFar[ cell ] = FLT_MAX;
		cameFrom[ cell ] = -1;
		isClosed[ cell ] = false;
	}

	const int gx = goal % m_width;
	const int gy = goal / m_width;

	CUtlPriorityQueue< OpenCell_t > open( 0, 0, IsFartherFromGoal );
	costSoFar[ start ] = 0.0f;
	open.Insert( { 0.0f, start } );

	int expanded = 0;
	while ( open.Count() && expanded < SEARCH_MAX_EXPANSIONS )
	{
		const int cell = open.ElementAtHead().cell;
		open.RemoveAtHead();
		if ( isClosed[ cell ] )
		{
			continue;
		}

		isClosed[ cell ] = true;
		++expanded;

		if ( cell == goal )
		{
			for ( int at = goal; at >= 0; at = cameFrom[ at ] )
			{
				route->AddToHead( at );
			}

			return true;
		}

		const int cx = cell % m_width;
		const int cy = cell / m_width;
		for ( int dy = -1; dy <= 1; ++dy )
		{
			for ( int dx = -1; dx <= 1; ++dx )
			{
				const int x = cx + dx;
				const int y = cy + dy;
				if ( ( dx == 0 && dy == 0 ) || x < 0 || y < 0 || x >= m_width || y >= m_height )
				{
					continue;
				}

				const int next = y * m_width + x;
				if ( isClosed[ next ] || !CanStep( cell, next ) )
				{
					continue;
				}

				// no cutting a corner past a cell off the mesh
				const bool isDiagonal = ( dx != 0 && dy != 0 );
				if ( isDiagonal && ( m_state[ cy * m_width + x ] == CELL_OFF_MESH || m_state[ y * m_width + cx ] == CELL_OFF_MESH ) )
				{
					continue;
				}

				const float stepLength = isDiagonal ? DIAGONAL_STEP_LENGTH : 1.0f;
				const float stepCost = stepLength * ( IsOccupied( next ) ? OCCUPIED_CELL_COST : 1.0f );
				const float cost = costSoFar[ cell ] + stepCost;
				if ( cost >= costSoFar[ next ] )
				{
					continue;
				}

				costSoFar[ next ] = cost;
				cameFrom[ next ] = cell;
				const float remaining = FastSqrt( (float)( ( x - gx ) * ( x - gx ) + ( y - gy ) * ( y - gy ) ) );
				open.Insert( { cost + remaining, next } );
			}
		}
	}

	return false;
}


//----------------------------------------------------------------------------------------------------------------
// The straight grid line between two cells crosses only free cells the bot can step between
bool CPropDetourGrid::IsLineClear( int from, int to ) const
{
	const int ax = from % m_width;
	const int ay = from / m_width;
	const int bx = to % m_width;
	const int by = to / m_width;
	const int steps = 2 * MAX( abs( bx - ax ), abs( by - ay ) );

	int previous = from;
	for ( int k = 1; k <= steps; ++k )
	{
		const float t = (float)k / steps;
		const int x = (int)( ax + ( bx - ax ) * t + 0.5f );
		const int y = (int)( ay + ( by - ay ) * t + 0.5f );
		const int cell = y * m_width + x;
		if ( cell == previous )
		{
			continue;
		}

		if ( m_state[ cell ] != CELL_FREE || !CanStep( previous, cell ) )
		{
			return false;
		}

		previous = cell;
	}

	return true;
}


//----------------------------------------------------------------------------------------------------------------
// Keep only the cells of the route a straight line cannot skip
void CPropDetourGrid::Straighten( const CUtlVector< int > &route, CUtlVector< Vector > *waypoints ) const
{
	int anchor = 0;
	for ( int k = 1; k < route.Count(); ++k )
	{
		const bool isLast = ( k == route.Count() - 1 );
		if ( isLast || IsOccupied( route[ anchor ] ) || !IsLineClear( route[ anchor ], route[ k + 1 ] ) )
		{
			waypoints->AddToTail( GetCellFloor( route[ k ] ) );
			anchor = k;
		}
	}
}


//----------------------------------------------------------------------------------------------------------------
// A route from the bot to the rejoin point as waypoints:
// past the cells the bot starts in, a route through a prop is no way around it
static bool FindDetour( const CPropDetourGrid &grid, const Vector &from, const Vector &rejoin, CUtlVector< Vector > *waypoints )
{
	CUtlVector< int > route;
	if ( !grid.FindRoute( from, rejoin, &route ) )
	{
		return false;
	}

	int k = 0;
	while ( k < route.Count() && grid.IsOccupied( route[ k ] ) )
	{
		++k;
	}

	for ( ; k < route.Count(); ++k )
	{
		if ( grid.IsOccupied( route[ k ] ) )
		{
			return false;
		}
	}

	grid.Straighten( route, waypoints );
	return waypoints->Count() > 0;
}


//----------------------------------------------------------------------------------------------------------------
CNEOBotPropDetour::CNEOBotPropDetour()
{
	Reset();
}


//----------------------------------------------------------------------------------------------------------------
void CNEOBotPropDetour::Reset()
{
	m_replanTimer.Invalidate();
	m_waypoints.RemoveAll();
	m_pathGoal = NULL;
	m_rejoinGoal = NULL;
	m_resumeGoal = NULL;
}


//----------------------------------------------------------------------------------------------------------------
void CNEOBotPropDetour::Update( INextBot *bot, const PathFollower &path )
{
	m_pathGoal = NULL;

	if ( m_replanTimer.IsElapsed() )
	{
		m_replanTimer.Start( REPLAN_INTERVAL );

		if ( !IsDetouring() )
		{
			Plan( bot, path );
		}
		else if ( !Replan( bot ) )
		{
			// no way around from where the bot is now: take up the path again where the detour left it
			m_waypoints.RemoveAll();
			m_pathGoal = m_resumeGoal;
		}
	}

	const Vector &feet = bot->GetLocomotionInterface()->GetFeet();
	while ( m_waypoints.Count() && ( m_waypoints[ 0 ].AsVector2D() - feet.AsVector2D() ).IsLengthLessThan( WAYPOINT_REACHED_RANGE ) )
	{
		m_waypoints.Remove( 0 );
	}

	if ( IsDetouring() )
	{
		m_pathGoal = m_rejoinGoal;
	}
}


//----------------------------------------------------------------------------------------------------------------
// Look for props in the way on the path ahead, and when there are, a way past them on the mesh
void CNEOBotPropDetour::Plan( INextBot *bot, const PathFollower &path )
{
	ILocomotion *mover = bot->GetLocomotionInterface();
	const Path::Segment *goal = path.GetCurrentGoal();
	if ( !goal || goal->ladder || goal->type != Path::ON_GROUND || !mover->IsOnGround() )
	{
		return;
	}

	CUtlVector< Vector > line;
	CUtlVector< const Path::Segment * > segments;
	GetPathAhead( path, mover->GetFeet(), LOOK_AHEAD_RANGE + REJOIN_EXTRA_RANGE, &line, &segments );
	if ( line.Count() < 2 )
	{
		return;
	}

	Vector lineLo = line[ 0 ];
	Vector lineHi = line[ 0 ];
	for ( int i = 1; i < line.Count(); ++i )
	{
		VectorMin( lineLo, line[ i ], lineLo );
		VectorMax( lineHi, line[ i ], lineHi );
	}

	CUtlVector< PropObstacle_t > obstacles;
	CollectProps( bot, lineLo, lineHi, &obstacles );
	if ( obstacles.Count() == 0 )
	{
		return;
	}

	// which props the body would meet along the path, and where it would be past all of them
	const BodyBox_t body = GetBodyBox( bot );
	float rejoinDistance = -1.0f;
	Vector2D regionLo = line[ 0 ].AsVector2D();
	Vector2D regionHi = line[ 0 ].AsVector2D();
	FOR_EACH_VEC( obstacles, i )
	{
		float legStart = 0.0f;
		for ( int leg = 0; leg + 1 < line.Count() && legStart < LOOK_AHEAD_RANGE; ++leg )
		{
			const float legLength = ( line[ leg + 1 ] - line[ leg ] ).Length2D();
			float fraction, enter, exit;
			if ( BodyMeetsProp( body, line[ leg ], line[ leg + 1 ], obstacles[ i ], &fraction )
				&& legStart + fraction * legLength < LOOK_AHEAD_RANGE
				&& SegmentCrossesBox( line[ leg ].AsVector2D(), line[ leg + 1 ].AsVector2D(), obstacles[ i ].lo, obstacles[ i ].hi, &enter, &exit ) )
			{
				rejoinDistance = MAX( rejoinDistance, legStart + exit * legLength + REJOIN_PAST_PROP );
				regionLo = regionLo.Min( obstacles[ i ].lo );
				regionHi = regionHi.Max( obstacles[ i ].hi );
			}

			legStart += legLength;
		}
	}

	if ( rejoinDistance < 0.0f )
	{
		return;
	}

	int rejoinLegEnd;
	const Vector rejoin = GetPointAlong( line, rejoinDistance, &rejoinLegEnd );
	float floorLo = rejoin.z;
	float floorHi = rejoin.z;
	float distance = 0.0f;
	for ( int i = 0; i < line.Count() && distance <= rejoinDistance; ++i )
	{
		regionLo = regionLo.Min( line[ i ].AsVector2D() );
		regionHi = regionHi.Max( line[ i ].AsVector2D() );
		floorLo = MIN( floorLo, line[ i ].z );
		floorHi = MAX( floorHi, line[ i ].z );
		distance += ( i + 1 < line.Count() ) ? ( line[ i + 1 ] - line[ i ] ).Length2D() : 0.0f;
	}

	regionLo = regionLo.Min( rejoin.AsVector2D() ) - Vector2D( GRID_MARGIN, GRID_MARGIN );
	regionHi = regionHi.Max( rejoin.AsVector2D() ) + Vector2D( GRID_MARGIN, GRID_MARGIN );

	CPropDetourGrid grid( bot, regionLo, regionHi, floorLo, floorHi );
	grid.MarkProps( obstacles );

	if ( !FindDetour( grid, mover->GetFeet(), rejoin, &m_waypoints ) )
	{
		return;
	}

	// the detour keeps this target while the props and the bot move
	m_rejoin = rejoin;
	m_rejoinGoal = segments[ rejoinLegEnd ];
	m_resumeGoal = goal;
	m_regionLo = regionLo;
	m_regionHi = regionHi;
	m_floorLo = floorLo;
	m_floorHi = floorHi;
}


//----------------------------------------------------------------------------------------------------------------
// Search again for a way to the same rejoin point, from where the bot is and around the props where they are now
bool CNEOBotPropDetour::Replan( INextBot *bot )
{
	// in the air, the bot keeps its detour until it lands
	ILocomotion *mover = bot->GetLocomotionInterface();
	if ( !mover->IsOnGround() )
	{
		return true;
	}

	const Vector &feet = mover->GetFeet();
	const Vector2D margin( GRID_MARGIN, GRID_MARGIN );
	const Vector2D regionLo = m_regionLo.Min( feet.AsVector2D() - margin );
	const Vector2D regionHi = m_regionHi.Max( feet.AsVector2D() + margin );
	const float floorLo = MIN( m_floorLo, feet.z );
	const float floorHi = MAX( m_floorHi, feet.z );

	CUtlVector< PropObstacle_t > obstacles;
	CollectProps( bot, Vector( regionLo.x, regionLo.y, floorLo ), Vector( regionHi.x, regionHi.y, floorHi ), &obstacles );

	CPropDetourGrid grid( bot, regionLo, regionHi, floorLo, floorHi );
	grid.MarkProps( obstacles );

	m_waypoints.RemoveAll();
	return FindDetour( grid, feet, m_rejoin, &m_waypoints );
}
