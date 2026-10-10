#include "cbase.h"
#include "nav_mesh.h"
#include "NextBot.h"
#include "NextBotLocomotionInterface.h"
#include "NextBotBodyInterface.h"
#include "neo_bot_obstacle_props.h"
#include "neo_bot_prop_detour.h"
#include "tier1/utlpriorityqueue.h"

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

namespace
{
// While every prop in a wide detour's region rests where the last search saw it, the detour is searched again only this often:
// the bot may since have come to see props that were out of its sight
constexpr float PROP_DETOUR_AT_REST_REPLAN_INTERVAL = 1.0f;

// A waypoint this close is reached,
constexpr float PROP_DETOUR_WAYPOINT_REACHED_RANGE = 12.0f;
// and a bot this far off the straight line to its next waypoint has left the route the search checked
constexpr float PROP_DETOUR_OFF_ROUTE_RANGE = 16.0f;

// The grid laid over the floor a detour may use reaches as far past the path and the props as the prop query does,
constexpr float PROP_DETOUR_GRID_MARGIN = OBSTACLE_PROP_QUERY_MARGIN;
// and as far up or down from them
constexpr float PROP_DETOUR_FLOOR_HEIGHT_TOLERANCE = OBSTACLE_PROP_FLOOR_TOLERANCE;
constexpr float PROP_DETOUR_GRID_CELL_SIZE = 16.0f;
constexpr int PROP_DETOUR_GRID_MAX_CELLS_PER_SIDE = 96;

// With no way around near the path, as past a row of props that runs on beyond the ones the path crosses,
// the search region grows over the props it touches, as far as cells of the finest size reach,
constexpr float PROP_DETOUR_WIDE_SEARCH_MAX_SIZE = PROP_DETOUR_GRID_MAX_CELLS_PER_SIDE * PROP_DETOUR_GRID_CELL_SIZE;
// and when even that finds no way around, the bot looks again after this long, not at every replan
constexpr float PROP_DETOUR_WIDE_SEARCH_RETRY_INTERVAL = 1.0f;

// Stepping through a cell a prop occupies costs as much as this many free cells:
// the search crosses a prop only to get out from under one, and a route that has to cross one is no detour
constexpr float PROP_DETOUR_OCCUPIED_CELL_COST = 1000.0f;
constexpr float PROP_DETOUR_FREE_CELL_COST = 1.0f;

// A light prop's cells cost this many free cells, so a bot walks around one chair where there is room,
constexpr float PROP_DETOUR_PUSHABLE_CELL_COST = 4.0f;
// and when every prop in the way on the path is light, a route through them past its first few cells is no detour:
// the bot keeps its path, shoves through or climbs over them as a player does, and looks again after this long
constexpr int PROP_DETOUR_ROUTE_START_CELLS = 3;
constexpr float PROP_DETOUR_PUSH_THROUGH_RETRY_INTERVAL = 1.0f;

constexpr float PROP_DETOUR_DIAGONAL_STEP_LENGTH = 1.41421356f;
}

//----------------------------------------------------------------------------------------------------------------
namespace
{
	// What a search for a way past the props found
	enum DetourResult
	{
		DETOUR_FOUND,
		DETOUR_NONE,				// no way past that keeps clear of the props
		DETOUR_THROUGH_PUSHABLE,	// the way past is through props the bot can shove aside
	};

	// Whether a route through light props, which the bot can shove aside, is a way around the props in the way
	enum PushableRoute
	{
		PUSHABLE_ROUTE_IS_DETOUR,		// a heavy prop is in the way: going around it through light ones is fine
		PUSHABLE_ROUTE_IS_NO_DETOUR,	// only light props are in the way: the bot keeps its path through them
	};
}


//----------------------------------------------------------------------------------------------------------------
// Cells over the nav mesh around the path ahead, each off the mesh, free, or occupied by a prop, searched for a way past the props;
// a cell's nav area is looked up only when the search or a prop first reaches it
class CPropDetourGrid
{
public:
	CPropDetourGrid( INextBot *bot, const Vector2D &lo, const Vector2D &hi, float floorLo, float floorHi );

	void MarkProps( const CUtlVector< PropObstacle_t > &obstacles );
	bool FindRoute( const Vector &from, const Vector &to, CUtlVector< int > *route ) const;
	void Straighten( const CUtlVector< int > &route, CUtlVector< Vector > *waypoints ) const;

	bool IsOccupied( int cell ) const { return GetState( cell ) == CELL_OCCUPIED; }
	bool IsPushableCell( int cell ) const { return GetState( cell ) == CELL_PUSHABLE; }
	bool IsNearFeet( int cell, const Vector &feet ) const;

private:
	enum CellState
	{
		CELL_UNKNOWN,
		CELL_OFF_MESH,
		CELL_FREE,
		CELL_PUSHABLE,		// the body there would touch only props it can shove aside
		CELL_OCCUPIED,
	};

	CellState GetState( int cell ) const;
	bool IsWalled( int cell ) const;
	float GetStepCost( int cell ) const;

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
	int m_team;
	Vector2D m_origin;
	float m_cellSize;
	int m_width;
	int m_height;
	float m_floorTop;
	float m_floorRange;

	// filled in as cells are first looked at
	mutable CUtlVector< unsigned char > m_state;
	mutable CUtlVector< float > m_floor;
	mutable CUtlVector< const CNavArea * > m_area;
};


//----------------------------------------------------------------------------------------------------------------
CPropDetourGrid::CPropDetourGrid( INextBot *bot, const Vector2D &lo, const Vector2D &hi, float floorLo, float floorHi )
{
	m_bot = bot;
	m_origin = lo;

	// coarser cells for a big region, so the search stays cheap
	m_cellSize = PROP_DETOUR_GRID_CELL_SIZE;
	while ( ( hi.x - lo.x ) / m_cellSize > PROP_DETOUR_GRID_MAX_CELLS_PER_SIDE || ( hi.y - lo.y ) / m_cellSize > PROP_DETOUR_GRID_MAX_CELLS_PER_SIDE )
	{
		m_cellSize *= 1.5f;
	}

	m_width = (int)ceilf( ( hi.x - lo.x ) / m_cellSize );
	m_height = (int)ceilf( ( hi.y - lo.y ) / m_cellSize );

	const int cellCount = m_width * m_height;
	m_state.SetCount( cellCount );
	m_floor.SetCount( cellCount );
	m_area.SetCount( cellCount );
	for ( int cell = 0; cell < cellCount; ++cell )
	{
		m_state[ cell ] = CELL_UNKNOWN;
	}

	m_team = bot->GetEntity()->GetTeamNumber();
	m_floorTop = floorHi + PROP_DETOUR_FLOOR_HEIGHT_TOLERANCE;
	m_floorRange = m_floorTop - ( floorLo - PROP_DETOUR_FLOOR_HEIGHT_TOLERANCE );
}


//----------------------------------------------------------------------------------------------------------------
// The cell's state, looking up its nav area the first time: free on an area not blocked for the bot's team
CPropDetourGrid::CellState CPropDetourGrid::GetState( int cell ) const
{
	if ( m_state[ cell ] == CELL_UNKNOWN )
	{
		const Vector2D center = GetCellCenter( cell );
		const CNavArea *area = TheNavMesh->GetNavArea( Vector( center.x, center.y, m_floorTop ), m_floorRange );

		m_area[ cell ] = area;
		m_state[ cell ] = ( area && !area->IsBlocked( m_team ) ) ? CELL_FREE : CELL_OFF_MESH;
		m_floor[ cell ] = area ? area->GetZ( center.x, center.y ) : 0.0f;
	}

	return (CellState)m_state[ cell ];
}


//----------------------------------------------------------------------------------------------------------------
// The cell is off the mesh or under a prop the bot cannot shove: nothing gets past it
bool CPropDetourGrid::IsWalled( int cell ) const
{
	const CellState state = GetState( cell );
	return state == CELL_OFF_MESH || state == CELL_OCCUPIED;
}


//----------------------------------------------------------------------------------------------------------------
// What stepping into the cell costs: a light prop's cell costs a little more than a free one, a heavy prop's a lot
float CPropDetourGrid::GetStepCost( int cell ) const
{
	switch ( GetState( cell ) )
	{
	case CELL_OCCUPIED:
		return PROP_DETOUR_OCCUPIED_CELL_COST;
	case CELL_PUSHABLE:
		return PROP_DETOUR_PUSHABLE_CELL_COST;
	default:
		return PROP_DETOUR_FREE_CELL_COST;
	}
}


//----------------------------------------------------------------------------------------------------------------
// A cell is occupied when the bot's body standing there would touch a prop, now or soon,
// and pushable when every prop it would touch is light enough to shove
void CPropDetourGrid::MarkProps( const CUtlVector< PropObstacle_t > &obstacles )
{
	const BodyBox_t body = GetBodyBox( m_bot );

	// only a cell whose middle lies in a prop's footprint can touch the prop: visit those, not every cell
	FOR_EACH_VEC( obstacles, i )
	{
		const PropObstacle_t &obstacle = obstacles[ i ];
		const int x0 = MAX( 0, (int)floorf( ( obstacle.lo.x - m_origin.x ) / m_cellSize ) );
		const int x1 = MIN( m_width - 1, (int)floorf( ( obstacle.hi.x - m_origin.x ) / m_cellSize ) );
		const int y0 = MAX( 0, (int)floorf( ( obstacle.lo.y - m_origin.y ) / m_cellSize ) );
		const int y1 = MIN( m_height - 1, (int)floorf( ( obstacle.hi.y - m_origin.y ) / m_cellSize ) );
		for ( int y = y0; y <= y1; ++y )
		{
			for ( int x = x0; x <= x1; ++x )
			{
				const int cell = y * m_width + x;
				const CellState state = GetState( cell );
				if ( state == CELL_OFF_MESH || state == CELL_OCCUPIED || ( state == CELL_PUSHABLE && obstacle.isPushable ) )
				{
					continue;
				}

				const Vector at = GetCellFloor( cell );
				if ( at.x < obstacle.lo.x || at.x > obstacle.hi.x || at.y < obstacle.lo.y || at.y > obstacle.hi.y )
				{
					continue;
				}

				float fraction;
				if ( BodyMeetsProp( body, at, at, obstacle, &fraction ) )
				{
					m_state[ cell ] = obstacle.isPushable ? CELL_PUSHABLE : CELL_OCCUPIED;
				}
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
// The cell's middle is within a hull width of the bot's feet
bool CPropDetourGrid::IsNearFeet( int cell, const Vector &feet ) const
{
	const float hullWidth = m_bot->GetBodyInterface()->GetHullWidth();
	return ( GetCellCenter( cell ) - feet.AsVector2D() ).IsLengthLessThan( hullWidth );
}


//----------------------------------------------------------------------------------------------------------------
// The floor at the middle of the cell
Vector CPropDetourGrid::GetCellFloor( int cell ) const
{
	GetState( cell );	// looks the floor up with the state, the first time the cell is reached
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

				if ( GetState( y * m_width + x ) != CELL_OFF_MESH )
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
	if ( GetState( to ) == CELL_OFF_MESH || GetState( from ) == CELL_OFF_MESH )
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

	while ( open.Count() )
	{
		const int cell = open.ElementAtHead().cell;
		open.RemoveAtHead();
		if ( isClosed[ cell ] )
		{
			continue;
		}

		isClosed[ cell ] = true;

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

				// no cutting a corner past a cell off the mesh or a prop the bot cannot shove:
				// two props that touch at a corner leave no gap, however the cells fall
				const bool isDiagonal = ( dx != 0 && dy != 0 );
				if ( isDiagonal && ( IsWalled( cy * m_width + x ) || IsWalled( y * m_width + cx ) ) )
				{
					continue;
				}

				const float stepLength = isDiagonal ? PROP_DETOUR_DIAGONAL_STEP_LENGTH : 1.0f;
				const float stepCost = stepLength * GetStepCost( next );
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

		if ( GetState( cell ) != CELL_FREE || !CanStep( previous, cell ) )
		{
			return false;
		}

		// a diagonal step cuts no corner past a cell off the mesh or a prop the bot cannot shove, as in the search
		const int px = previous % m_width;
		const int py = previous / m_width;
		if ( px != x && py != y && ( IsWalled( py * m_width + x ) || IsWalled( y * m_width + px ) ) )
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
	// a route of one cell: the bot stands in the rejoin cell, which is then the one waypoint
	if ( route.Count() == 1 )
	{
		waypoints->AddToTail( GetCellFloor( route[ 0 ] ) );
		return;
	}

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
// past the prop cells by the bot's feet, a route through a prop is no way around it
static DetourResult FindDetour( const CPropDetourGrid &grid, const Vector &from, const Vector &rejoin, PushableRoute pushableRoute,
	CUtlVector< Vector > *waypoints )
{
	CUtlVector< int > route;
	if ( !grid.FindRoute( from, rejoin, &route ) )
	{
		return DETOUR_NONE;
	}

	// the route may start through the prop cells by the bot's feet, so a bot under or against a prop gets out,
	// but no further: a bot against a wall of props would otherwise route through the whole wall
	int k = 0;
	while ( k < route.Count() && grid.IsOccupied( route[ k ] ) && grid.IsNearFeet( route[ k ], from ) )
	{
		++k;
	}

	for ( ; k < route.Count(); ++k )
	{
		if ( grid.IsOccupied( route[ k ] ) )
		{
			return DETOUR_NONE;
		}
	}

	// with only light props in the way, a route through them is no detour,
	// and as a long run of their cells costs little, only the route's first cells count as where the bot starts
	if ( pushableRoute == PUSHABLE_ROUTE_IS_NO_DETOUR )
	{
		int start = 0;
		while ( start < route.Count() && start < PROP_DETOUR_ROUTE_START_CELLS && ( grid.IsOccupied( route[ start ] ) || grid.IsPushableCell( route[ start ] ) ) )
		{
			++start;
		}

		for ( k = start; k < route.Count(); ++k )
		{
			if ( grid.IsPushableCell( route[ k ] ) )
			{
				return DETOUR_THROUGH_PUSHABLE;
			}
		}
	}

	grid.Straighten( route, waypoints );
	return ( waypoints->Count() > 0 ) ? DETOUR_FOUND : DETOUR_NONE;
}


//----------------------------------------------------------------------------------------------------------------
// Search the region for a way from the bot's feet to the rejoin point, around every prop on it
static DetourResult SearchRegion( INextBot *bot, const Vector2D &lo, const Vector2D &hi, float floorLo, float floorHi,
	const Vector &rejoin, PushableRoute pushableRoute, CUtlVector< Vector > *waypoints )
{
	CUtlVector< PropObstacle_t > obstacles;
	CollectProps( bot, Vector( lo.x, lo.y, floorLo ), Vector( hi.x, hi.y, floorHi ), &obstacles );

	CPropDetourGrid grid( bot, lo, hi, floorLo, floorHi );
	grid.MarkProps( obstacles );
	return FindDetour( grid, bot->GetLocomotionInterface()->GetFeet(), rejoin, pushableRoute, waypoints );
}


//----------------------------------------------------------------------------------------------------------------
// Grow the region over every prop it touches, with a margin around each so the way around the prop is in it too,
// as long as the region stays within the wide search's size
static void GrowOverProps( const CUtlVector< PropObstacle_t > &obstacles, Vector2D *regionLo, Vector2D *regionHi )
{
	const Vector2D margin( PROP_DETOUR_GRID_MARGIN, PROP_DETOUR_GRID_MARGIN );
	for ( ;; )
	{
		// the prop that grows the region least goes in first,
		// so the near end of a long row is in before its far end uses up the size
		Vector2D bestLo = *regionLo;
		Vector2D bestHi = *regionHi;
		float bestArea = FLT_MAX;
		FOR_EACH_VEC( obstacles, i )
		{
			const PropObstacle_t &obstacle = obstacles[ i ];
			const bool isTouching = obstacle.hi.x > regionLo->x && obstacle.lo.x < regionHi->x
				&& obstacle.hi.y > regionLo->y && obstacle.lo.y < regionHi->y;
			if ( !isTouching )
			{
				continue;
			}

			const Vector2D lo = regionLo->Min( obstacle.lo - margin );
			const Vector2D hi = regionHi->Max( obstacle.hi + margin );
			const Vector2D size = hi - lo;
			if ( ( lo == *regionLo && hi == *regionHi ) || size.x > PROP_DETOUR_WIDE_SEARCH_MAX_SIZE || size.y > PROP_DETOUR_WIDE_SEARCH_MAX_SIZE )
			{
				continue;
			}

			if ( size.x * size.y < bestArea )
			{
				bestArea = size.x * size.y;
				bestLo = lo;
				bestHi = hi;
			}
		}

		if ( bestArea == FLT_MAX )
		{
			return;
		}

		*regionLo = bestLo;
		*regionHi = bestHi;
	}
}


//----------------------------------------------------------------------------------------------------------------
// One wide search a server tick between all bots, so a crowd of blocked bots cannot stall a frame:
// return true if this tick's is still free, and take it
static int s_wideSearchTick = -1;
static bool ClaimWideSearch()
{
	if ( s_wideSearchTick == gpGlobals->tickcount )
	{
		return false;
	}

	s_wideSearchTick = gpGlobals->tickcount;
	return true;
}


//----------------------------------------------------------------------------------------------------------------
// With no way around near the path, as past a wall of props that runs on beyond the ones the path crosses,
// search again over a region grown over the props it touches, and on success hand back that region for the replans
static bool FindWideDetour( INextBot *bot, float floorLo, float floorHi, const Vector &rejoin, PushableRoute pushableRoute,
	Vector2D *regionLo, Vector2D *regionHi, CUtlVector< Vector > *waypoints )
{
	// the region grows to at most the wide search's size either way from the near one,
	// so every prop it could reach lies in this box: twice that size less the near region a side, and at least the near region
	const Vector2D reach( PROP_DETOUR_WIDE_SEARCH_MAX_SIZE, PROP_DETOUR_WIDE_SEARCH_MAX_SIZE );
	const Vector2D reachLo = ( *regionHi - reach ).Min( *regionLo );
	const Vector2D reachHi = ( *regionLo + reach ).Max( *regionHi );

	// every prop in the box within the floor's height band costs a sight trace, before the grown region is known
	CUtlVector< PropObstacle_t > obstacles;
	CollectProps( bot, Vector( reachLo.x, reachLo.y, floorLo ), Vector( reachHi.x, reachHi.y, floorHi ), &obstacles );

	Vector2D lo = *regionLo;
	Vector2D hi = *regionHi;
	GrowOverProps( obstacles, &lo, &hi );
	if ( lo == *regionLo && hi == *regionHi )
	{
		return false;
	}

	CPropDetourGrid grid( bot, lo, hi, floorLo, floorHi );
	grid.MarkProps( obstacles );
	if ( FindDetour( grid, bot->GetLocomotionInterface()->GetFeet(), rejoin, pushableRoute, waypoints ) != DETOUR_FOUND )
	{
		return false;
	}

	*regionLo = lo;
	*regionHi = hi;
	return true;
}


//----------------------------------------------------------------------------------------------------------------
CNEOBotPropDetour::CNEOBotPropDetour()
{
	Reset();
}


//----------------------------------------------------------------------------------------------------------------
void CNEOBotPropDetour::Reset()
{
	m_waypoints.RemoveAll();
	m_pathGoal = NULL;
	m_rejoinGoal = NULL;
	m_resumeGoal = NULL;
	m_isWide = false;
	m_isPathPushable = false;
	m_legStart = vec3_origin;
	m_restingPropCount = OBSTACLE_PROPS_MOVING;
	m_searchAgeTimer.Invalidate();
}


//----------------------------------------------------------------------------------------------------------------
float CNEOBotPropDetour::Plan( INextBot *bot, const PropDetourRequest_t &request, float lookInterval )
{
	const Vector2D margin( PROP_DETOUR_GRID_MARGIN, PROP_DETOUR_GRID_MARGIN );
	Vector2D regionLo = request.regionLo - margin;
	Vector2D regionHi = request.regionHi + margin;

	// the region reaches past the props looked for around the path, by the footprints of those it crosses,
	// so the search looks for every prop on it again, as a replan does
	bool isWide = false;
	const PushableRoute pushableRoute = request.isPathPushable ? PUSHABLE_ROUTE_IS_NO_DETOUR : PUSHABLE_ROUTE_IS_DETOUR;
	const DetourResult result = SearchRegion( bot, regionLo, regionHi, request.floorLo, request.floorHi, request.rejoin, pushableRoute, &m_waypoints );
	if ( result == DETOUR_THROUGH_PUSHABLE )
	{
		// the bot keeps its path through props it can shove, so no wide search for a way around them
		return PROP_DETOUR_PUSH_THROUGH_RETRY_INTERVAL;
	}

	if ( result == DETOUR_NONE )
	{
		// another bot had this tick's wide search: plan again within a look interval,
		// at a random tick so blocked bots take turns instead of all planning again at the next one
		if ( !ClaimWideSearch() )
		{
			return RandomFloat( gpGlobals->interval_per_tick, lookInterval );
		}

		if ( !FindWideDetour( bot, request.floorLo, request.floorHi, request.rejoin, pushableRoute, &regionLo, &regionHi, &m_waypoints ) )
		{
			// a bot pushing a prop with no way around looks again less often: nothing changes quickly
			return PROP_DETOUR_WIDE_SEARCH_RETRY_INTERVAL;
		}

		isWide = true;
	}

	// the detour keeps this target while the props and the bot move
	m_rejoin = request.rejoin;
	m_rejoinGoal = request.rejoinGoal;
	m_resumeGoal = request.resumeGoal;
	m_regionLo = regionLo;
	m_regionHi = regionHi;
	m_floorLo = request.floorLo;
	m_floorHi = request.floorHi;
	m_isWide = isWide;
	m_isPathPushable = request.isPathPushable;
	NoteSearch( bot->GetLocomotionInterface()->GetFeet() );
	return 0.0f;
}


//----------------------------------------------------------------------------------------------------------------
bool CNEOBotPropDetour::Update( INextBot *bot, bool isSearchDue )
{
	m_pathGoal = NULL;

	bool isWaitingForTick = false;
	if ( isSearchDue && IsDetouring() )
	{
		if ( m_isWide && IsLastSearchValid( bot ) )
		{
			// nothing in the wide detour's region has moved and the bot keeps to its route: the detour holds
		}
		else if ( m_isWide && !ClaimWideSearch() )
		{
			// a detour the wide search found searches its whole region again: wait for a free tick
			isWaitingForTick = true;
		}
		else if ( !Replan( bot ) )
		{
			// no way around from where the bot is now: take up the path again where the detour left it
			m_waypoints.RemoveAll();
			m_pathGoal = m_resumeGoal;
		}
	}

	const Vector &feet = bot->GetLocomotionInterface()->GetFeet();
	while ( m_waypoints.Count() && ( m_waypoints[ 0 ].AsVector2D() - feet.AsVector2D() ).IsLengthLessThan( PROP_DETOUR_WAYPOINT_REACHED_RANGE ) )
	{
		m_legStart = m_waypoints[ 0 ];
		m_waypoints.Remove( 0 );
	}

	if ( IsDetouring() )
	{
		m_pathGoal = m_rejoinGoal;
	}

	return isWaitingForTick;
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
	const Vector2D margin( PROP_DETOUR_GRID_MARGIN, PROP_DETOUR_GRID_MARGIN );
	const Vector2D regionLo = m_regionLo.Min( feet.AsVector2D() - margin );
	const Vector2D regionHi = m_regionHi.Max( feet.AsVector2D() + margin );
	const float floorLo = MIN( m_floorLo, feet.z );
	const float floorHi = MAX( m_floorHi, feet.z );

	// the same question the plan answered: whether only light props were in the way
	const PushableRoute pushableRoute = m_isPathPushable ? PUSHABLE_ROUTE_IS_NO_DETOUR : PUSHABLE_ROUTE_IS_DETOUR;
	m_waypoints.RemoveAll();
	if ( SearchRegion( bot, regionLo, regionHi, floorLo, floorHi, m_rejoin, pushableRoute, &m_waypoints ) != DETOUR_FOUND )
	{
		return false;
	}

	NoteSearch( feet );
	return true;
}


//----------------------------------------------------------------------------------------------------------------
// After a search that found a detour: note where the bot sets out from, and whether the props in the region rest
void CNEOBotPropDetour::NoteSearch( const Vector &feet )
{
	// only a wide detour's replans are skipped while its props rest
	if ( !m_isWide )
	{
		return;
	}

	m_legStart = feet;
	m_restingPropCount = CountRestingProps( Vector( m_regionLo.x, m_regionLo.y, m_floorLo ), Vector( m_regionHi.x, m_regionHi.y, m_floorHi ) );
	m_searchAgeTimer.Start( PROP_DETOUR_AT_REST_REPLAN_INTERVAL );
}


//----------------------------------------------------------------------------------------------------------------
// The last search still holds: the props in the region rest where it saw them, and the bot keeps to its route
bool CNEOBotPropDetour::IsLastSearchValid( INextBot *bot ) const
{
	if ( m_restingPropCount == OBSTACLE_PROPS_MOVING || m_searchAgeTimer.IsElapsed() )
	{
		return false;
	}

	const Vector &feet = bot->GetLocomotionInterface()->GetFeet();
	if ( CalcDistanceToLineSegment2D( feet.AsVector2D(), m_legStart.AsVector2D(), m_waypoints[ 0 ].AsVector2D() ) > PROP_DETOUR_OFF_ROUTE_RANGE )
	{
		return false;
	}

	return CountRestingProps( Vector( m_regionLo.x, m_regionLo.y, m_floorLo ), Vector( m_regionHi.x, m_regionHi.y, m_floorHi ) ) == m_restingPropCount;
}
