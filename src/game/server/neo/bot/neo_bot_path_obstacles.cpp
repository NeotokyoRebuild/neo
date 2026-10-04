#include "cbase.h"
#include "collisionutils.h"
#include "NextBot.h"
#include "NextBot/Path/NextBotPathFollow.h"
#include "NextBotLocomotionInterface.h"
#include "NextBotBodyInterface.h"
#include "neo_bot_obstacle_props.h"
#include "neo_bot_path_obstacles.h"

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

namespace
{
// How often the path ahead is checked, so a prop that is pushed or animates is followed
// and a breakable in the way is seen before the bot walks into it
constexpr float PATH_OBSTACLE_LOOK_INTERVAL = 0.3f;

// How far along the path props are looked for, and how much further the detour may rejoin it
constexpr float PATH_OBSTACLE_LOOK_AHEAD_RANGE = 256.0f;
constexpr float PATH_OBSTACLE_REJOIN_EXTRA_RANGE = 128.0f;

// A breakable is in the way when the body meets it this far along the path:
// beyond a look's walk at a run, so it is found before the bot reaches it, and near enough to melee it on arrival
constexpr float PATH_OBSTACLE_BREAKABLE_RANGE = 128.0f;

// The detour rejoins the path this far past the last prop in the way
constexpr float PATH_OBSTACLE_REJOIN_PAST_PROP = 32.0f;
}

//----------------------------------------------------------------------------------------------------------------
// Return true if the line from a to b crosses the box, with the entry and exit fractions
static bool SegmentCrossesBox( const Vector2D &a, const Vector2D &b, const Vector2D &lo, const Vector2D &hi, float *enter, float *exit )
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


//----------------------------------------------------------------------------------------------------------------
// The box around the line's points, over its first 'range' along it
static void GetLineBounds( const CUtlVector< Vector > &line, float range, Vector *lo, Vector *hi )
{
	*lo = line[ 0 ];
	*hi = line[ 0 ];
	float length = 0.0f;
	for ( int i = 1; i < line.Count() && length < range; ++i )
	{
		length += ( line[ i ] - line[ i - 1 ] ).Length2D();
		VectorMin( *lo, line[ i ], *lo );
		VectorMax( *hi, line[ i ], *hi );
	}
}


//----------------------------------------------------------------------------------------------------------------
// The breakable the body meets first walking the line, within PATH_OBSTACLE_BREAKABLE_RANGE,
// when nothing else stands in front of it
static CBaseEntity *FindBreakableInWay( INextBot *bot, const BodyBox_t &body, const CUtlVector< Vector > &line, const CUtlVector< CBaseEntity * > &breakables )
{
	// a breakable away from the body's way over the range cannot be met, so it costs no sweep
	Vector reachLo, reachHi;
	GetLineBounds( line, PATH_OBSTACLE_BREAKABLE_RANGE, &reachLo, &reachHi );
	reachLo += body.mins;
	reachHi += body.maxs;

	CBaseEntity *nearest = NULL;
	int nearestLeg = -1;
	float nearestDistance = PATH_OBSTACLE_BREAKABLE_RANGE;
	FOR_EACH_VEC( breakables, i )
	{
		Vector entityLo, entityHi;
		breakables[ i ]->CollisionProp()->WorldSpaceAABB( &entityLo, &entityHi );
		if ( !IsBoxIntersectingBox( reachLo, reachHi, entityLo, entityHi ) )
		{
			continue;
		}

		float legStart = 0.0f;
		for ( int leg = 0; leg + 1 < line.Count() && legStart < nearestDistance; ++leg )
		{
			const float legLength = ( line[ leg + 1 ] - line[ leg ] ).Length2D();
			float fraction;
			if ( BodyMeetsEntity( body, line[ leg ], line[ leg + 1 ], breakables[ i ], &fraction ) && legStart + fraction * legLength < nearestDistance )
			{
				nearest = breakables[ i ];
				nearestLeg = leg;
				nearestDistance = legStart + fraction * legLength;
				break;
			}

			legStart += legLength;
		}
	}

	if ( !nearest )
	{
		return NULL;
	}

	// what the body meets first on the way may be a crate, a door or a wall in front of the breakable:
	// keep the breakable only if the body reaches it first, passing through players, who move
	CTraceFilterNoNPCsOrPlayer filter( bot->GetEntity(), COLLISION_GROUP_NONE );
	for ( int leg = 0; leg <= nearestLeg; ++leg )
	{
		trace_t result;
		UTIL_TraceHull( line[ leg ], line[ leg + 1 ], body.mins, body.maxs, MASK_PLAYERSOLID, &filter, &result );
		if ( result.DidHit() )
		{
			return ( result.m_pEnt == nearest ) ? nearest : NULL;
		}
	}

	return NULL;
}

//----------------------------------------------------------------------------------------------------------------
// The path ahead as a line from the bot's feet, over walked segments only, up to the range,
// and, given a list for them, the segment each point of the line belongs to
static void GetPathAhead( const PathFollower &path, const Vector &feet, float range, CUtlVector< Vector > *line, CUtlVector< const Path::Segment * > *segments = NULL )
{
	line->AddToTail( feet );
	if ( segments )
	{
		segments->AddToTail( path.GetCurrentGoal() );
	}

	float length = 0.0f;
	for ( const Path::Segment *seg = path.GetCurrentGoal(); seg && length < range; seg = path.NextSegment( seg ) )
	{
		if ( seg->ladder || seg->type != Path::ON_GROUND )
		{
			break;
		}

		const Vector leg = seg->pos - line->Tail();
		const float legLength = leg.Length2D();
		if ( segments )
		{
			segments->AddToTail( seg );
		}

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
CNEOBotPathObstacles::CNEOBotPathObstacles()
{
	Reset();
}


//----------------------------------------------------------------------------------------------------------------
void CNEOBotPathObstacles::Reset()
{
	m_lookTimer.Invalidate();
	m_breakableTimer.Invalidate();
	m_breakable = NULL;
	m_detour.Reset();
}


//----------------------------------------------------------------------------------------------------------------
void CNEOBotPathObstacles::Update( INextBot *bot, const PathFollower &path )
{
	bool isSearchDue = false;
	if ( m_lookTimer.IsElapsed() )
	{
		m_lookTimer.Start( PATH_OBSTACLE_LOOK_INTERVAL );

		if ( !m_detour.IsDetouring() )
		{
			Plan( bot, path );
		}
		else
		{
			isSearchDue = true;
		}
	}

	if ( m_detour.Update( bot, isSearchDue ) )
	{
		m_lookTimer.Invalidate();
	}

	// a plan looks for breakables with the props; while none does (detouring, or waiting to retry), they are looked for alone
	if ( m_breakableTimer.IsElapsed() )
	{
		LookForBreakable( bot, path );
	}
}


//----------------------------------------------------------------------------------------------------------------
// Look for props in the way on the path ahead, and when there are, a way past them on the mesh
void CNEOBotPathObstacles::Plan( INextBot *bot, const PathFollower &path )
{
	ILocomotion *mover = bot->GetLocomotionInterface();
	const Path::Segment *goal = path.GetCurrentGoal();
	if ( !goal || goal->ladder || goal->type != Path::ON_GROUND || !mover->IsOnGround() )
	{
		return;
	}

	CUtlVector< Vector > line;
	CUtlVector< const Path::Segment * > segments;
	GetPathAhead( path, mover->GetFeet(), PATH_OBSTACLE_LOOK_AHEAD_RANGE + PATH_OBSTACLE_REJOIN_EXTRA_RANGE, &line, &segments );
	if ( line.Count() < 2 )
	{
		return;
	}

	Vector lineLo, lineHi;
	GetLineBounds( line, FLT_MAX, &lineLo, &lineHi );

	CUtlVector< PropObstacle_t > obstacles;
	CUtlVector< CBaseEntity * > breakables;
	CollectProps( bot, lineLo, lineHi, &obstacles, &breakables );

	const BodyBox_t body = GetBodyBox( bot );
	m_breakable = FindBreakableInWay( bot, body, line, breakables );
	m_breakableTimer.Start( PATH_OBSTACLE_LOOK_INTERVAL );

	if ( obstacles.Count() == 0 )
	{
		return;
	}

	// which props the body would meet along the path, and where it would be past all of them
	float rejoinDistance = -1.0f;
	bool isPathPushable = true;
	Vector2D regionLo = line[ 0 ].AsVector2D();
	Vector2D regionHi = line[ 0 ].AsVector2D();
	FOR_EACH_VEC( obstacles, i )
	{
		float legStart = 0.0f;
		for ( int leg = 0; leg + 1 < line.Count() && legStart < PATH_OBSTACLE_LOOK_AHEAD_RANGE; ++leg )
		{
			const float legLength = ( line[ leg + 1 ] - line[ leg ] ).Length2D();
			float fraction, enter, exit;
			if ( BodyMeetsProp( body, line[ leg ], line[ leg + 1 ], obstacles[ i ], &fraction )
				&& legStart + fraction * legLength < PATH_OBSTACLE_LOOK_AHEAD_RANGE
				&& SegmentCrossesBox( line[ leg ].AsVector2D(), line[ leg + 1 ].AsVector2D(), obstacles[ i ].lo, obstacles[ i ].hi, &enter, &exit ) )
			{
				rejoinDistance = MAX( rejoinDistance, legStart + exit * legLength + PATH_OBSTACLE_REJOIN_PAST_PROP );
				isPathPushable = isPathPushable && obstacles[ i ].isPushable;
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

	PropDetourRequest_t request;
	request.rejoin = rejoin;
	request.rejoinGoal = segments[ rejoinLegEnd ];
	request.resumeGoal = goal;
	request.regionLo = regionLo.Min( rejoin.AsVector2D() );
	request.regionHi = regionHi.Max( rejoin.AsVector2D() );
	request.floorLo = floorLo;
	request.floorHi = floorHi;
	request.isPathPushable = isPathPushable;

	const float lookAgainIn = m_detour.Plan( bot, request, PATH_OBSTACLE_LOOK_INTERVAL );
	if ( lookAgainIn > 0.0f )
	{
		m_lookTimer.Start( lookAgainIn );
	}
}


//----------------------------------------------------------------------------------------------------------------
// Look along the path ahead for a breakable in the way, without the props
void CNEOBotPathObstacles::LookForBreakable( INextBot *bot, const PathFollower &path )
{
	m_breakableTimer.Start( PATH_OBSTACLE_LOOK_INTERVAL );
	m_breakable = NULL;

	CUtlVector< Vector > line;
	GetPathAhead( path, bot->GetLocomotionInterface()->GetFeet(), PATH_OBSTACLE_BREAKABLE_RANGE, &line );
	if ( line.Count() < 2 )
	{
		return;
	}

	Vector lineLo, lineHi;
	GetLineBounds( line, FLT_MAX, &lineLo, &lineHi );

	CUtlVector< CBaseEntity * > breakables;
	FindBreakables( bot, lineLo, lineHi, &breakables );
	m_breakable = FindBreakableInWay( bot, GetBodyBox( bot ), line, breakables );
}
