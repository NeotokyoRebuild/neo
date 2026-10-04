#include "cbase.h"
#include "NextBot.h"
#include "NextBotBodyInterface.h"
#include "NextBotLocomotionInterface.h"
#include "neo_bot_obstacle_props.h"
#include "vphysics_interface.h"

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

namespace
{
// A moving prop is kept clear of where it will be within this time, at its current velocity
constexpr float PROP_DETOUR_MOTION_PREDICT_TIME = 1.0f;
constexpr float PROP_DETOUR_MOVING_PROP_MIN_SPEED = 10.0f;

// A prop this light that physics moves is shoved out of the way, as a player does
constexpr float PROP_DETOUR_PUSHABLE_PROP_MAX_MASS = 200.0f;

// Props are looked for up to this high above the path, so one coming down is seen in time
constexpr float PATH_OBSTACLE_QUERY_HEADROOM = 512.0f;

// A prop smaller than this in every direction (a can, a bottle) is pushed aside, not walked around
const Vector PROP_DETOUR_SMALL_PROP_SIZE( 16.0f, 16.0f, 40.0f );

// The body box is this much narrower than the hull, so a prop the bot only brushes is not in its way
constexpr float PATH_OBSTACLE_BODY_CLEARANCE = 1.0f;
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
namespace
{
	// The movable props solid to players a spatial partition query meets, however much else lies there:
	// a box query into a fixed-size list fills up with everything it finds, cans, weapons and triggers alike.
	// Given a list for them, it also keeps the solid entities the bot can break, such as glass
	class CMovablePropEnum : public IPartitionEnumerator
	{
	public:
		CMovablePropEnum( CUtlVector< CBaseEntity * > *props, INextBot *bot, CUtlVector< CBaseEntity * > *breakables )
			: m_props( props ), m_bot( bot ), m_breakables( breakables ) {}

		virtual IterationRetval_t EnumElement( IHandleEntity *handleEntity )
		{
			CBaseEntity *entity = gEntList.GetBaseEntity( handleEntity->GetRefEHandle() );
			if ( !entity || !IsSolidToPlayers( entity ) )
			{
				return ITERATION_CONTINUE;
			}

			if ( IsMovableProp( entity ) )
			{
				m_props->AddToTail( entity );
			}

			if ( m_breakables && m_bot->IsAbleToBreak( entity ) && entity->GetHealth() > 0 )
			{
				m_breakables->AddToTail( entity );
			}

			return ITERATION_CONTINUE;
		}

	private:
		CUtlVector< CBaseEntity * > *m_props;
		INextBot *m_bot;
		CUtlVector< CBaseEntity * > *m_breakables;
	};
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
	return velocity.IsLengthGreaterThan( PROP_DETOUR_MOVING_PROP_MIN_SPEED ) ? velocity : vec3_origin;
}


//----------------------------------------------------------------------------------------------------------------
// A light prop physics moves freely: walking into it shoves it aside.
// One hung on a rope or a hinge swings back, and an animated prop's bone followers move with their prop
static bool IsPushable( CBaseEntity *entity )
{
	IPhysicsObject *physics = entity->VPhysicsGetObject();
	return physics && physics->IsMoveable() && physics->GetMass() <= PROP_DETOUR_PUSHABLE_PROP_MAX_MASS
		&& !physics->IsAttachedToConstraint( false ) && !FClassnameIs( entity, "phys_bone_follower" );
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
BodyBox_t GetBodyBox( INextBot *bot )
{
	const float halfWidth = 0.5f * bot->GetBodyInterface()->GetHullWidth() - PATH_OBSTACLE_BODY_CLEARANCE;

	BodyBox_t box;
	box.mins.Init( -halfWidth, -halfWidth, bot->GetLocomotionInterface()->GetStepHeight() );
	box.maxs.Init( halfWidth, halfWidth, bot->GetBodyInterface()->GetStandHullHeight() );
	return box;
}

//----------------------------------------------------------------------------------------------------------------
bool BodyMeetsProp( const BodyBox_t &body, const Vector &from, const Vector &to, const PropObstacle_t &obstacle, float *fraction )
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
void FindMovableProps( const Vector &floorLo, const Vector &floorHi, CUtlVector< CBaseEntity * > *props,
	INextBot *bot, CUtlVector< CBaseEntity * > *breakables )
{
	const Vector queryLo = floorLo - Vector( PROP_DETOUR_GRID_MARGIN, PROP_DETOUR_GRID_MARGIN, PROP_DETOUR_FLOOR_HEIGHT_TOLERANCE );
	const Vector queryHi = floorHi + Vector( PROP_DETOUR_GRID_MARGIN, PROP_DETOUR_GRID_MARGIN, PATH_OBSTACLE_QUERY_HEADROOM );

	CMovablePropEnum propEnum( props, bot, breakables );
	partition->EnumerateElementsInBox( PARTITION_ENGINE_NON_STATIC_EDICTS, queryLo, queryHi, false, &propEnum );
}


//----------------------------------------------------------------------------------------------------------------
void CollectProps( INextBot *bot, const Vector &floorLo, const Vector &floorHi, CUtlVector< PropObstacle_t > *obstacles,
	CUtlVector< CBaseEntity * > *breakables )
{
	const BodyBox_t body = GetBodyBox( bot );
	const float halfWidth = 0.5f * bot->GetBodyInterface()->GetHullWidth();

	CUtlVector< CBaseEntity * > props;
	FindMovableProps( floorLo, floorHi, &props, bot, breakables );
	FOR_EACH_VEC( props, i )
	{
		CBaseEntity *entity = props[ i ];
		Vector propLo, propHi;
		entity->CollisionProp()->WorldSpaceAABB( &propLo, &propHi );

		const Vector size = propHi - propLo;
		if ( size.x < PROP_DETOUR_SMALL_PROP_SIZE.x && size.y < PROP_DETOUR_SMALL_PROP_SIZE.y && size.z < PROP_DETOUR_SMALL_PROP_SIZE.z )
		{
			continue;
		}

		const Vector sweep = GetPropVelocity( entity ) * PROP_DETOUR_MOTION_PREDICT_TIME;
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
		obstacle.isPushable = IsPushable( entity );
		obstacle.lo.Init( sweptLo.x - halfWidth, sweptLo.y - halfWidth );
		obstacle.hi.Init( sweptHi.x + halfWidth, sweptHi.y + halfWidth );
		obstacles->AddToTail( obstacle );
	}
}


//----------------------------------------------------------------------------------------------------------------
int CountRestingProps( const Vector &floorLo, const Vector &floorHi )
{
	CUtlVector< CBaseEntity * > props;
	FindMovableProps( floorLo, floorHi, &props );
	FOR_EACH_VEC( props, i )
	{
		IPhysicsObject *physics = props[ i ]->VPhysicsGetObject();
		if ( physics && !physics->IsAsleep() )
		{
			return PROP_DETOUR_PROPS_MOVING;
		}
	}

	return props.Count();
}
