#include "cbase.h"
#include "neo_bot_suppressive_fire.h"
#include "neo/bot/neo_bot.h"
#include "neo_gamerules.h"
#include "weapon_neobasecombatweapon.h"
#include "neo/neo_smokelineofsightblocker.h"

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

ConVar neo_bot_suppressive_fire( "neo_bot_suppressive_fire", "1", FCVAR_NONE,
	"Bots fire at the believed position of a threat they cannot see.",
	true, 0, true, 1 );

namespace
{
// Randomised so a team caught by the same burst does not stop shooting in unison
constexpr float SUPPRESS_WINDOW_MIN = 2.0f;
constexpr float SUPPRESS_WINDOW_MAX = 3.0f;
// A threat not seen for longer than this is too stale to guess a position from
constexpr float SUPPRESS_MAX_THREAT_AGE = 3.0f;
constexpr float SUPPRESS_TORSO_HEIGHT = 36.0f;
// Short, so the line of fire is checked again before a teammate can run into a held burst
constexpr float SUPPRESS_BURST_TIME = 0.1f;
constexpr float SUPPRESS_AIM_TOLERANCE_DEG = 3.6f;
// Being hit or hearing a shot only tells roughly which way it came from, and how far
constexpr float SUPPRESS_BEARING_ERROR_DEG = 10.0f;
constexpr float SUPPRESS_RANGE_ERROR = 0.2f; // fraction of the range, either way
// Hose the aim side to side across the believed position, about two hulls each way,
// so a magazine covers where the threat could have moved instead of a single point
constexpr float SUPPRESS_SWEEP_HALF_WIDTH = 64.0f;
constexpr float SUPPRESS_SWEEP_MIN_DEG = 3.0f;
constexpr float SUPPRESS_SWEEP_MAX_DEG = 12.0f;
// Slow enough to stay under nb_head_aim_steady_max_rate, so the aim keeps up with the sweep
constexpr float SUPPRESS_SWEEP_PERIOD = 1.5f;
// Keep sweeping through a stretch of the arc that is not worth firing down
constexpr float SUPRESS_HOLD_AIM_TIME = 0.5f;
}

//---------------------------------------------------------------------------------------------
// Swing 'to' around 'from' by the given yaw and scale its range, keeping its height
Vector CNEOBotSuppressiveFire::RotateBearing( const Vector &from, const Vector &to, float degrees, float rangeScale )
{
	const Vector along = to - from;
	const float range = along.Length2D() * rangeScale;
	const float yaw = atan2f( along.y, along.x ) + DEG2RAD( degrees );
	return Vector( from.x + cosf( yaw ) * range, from.y + sinf( yaw ) * range, to.z );
}


//---------------------------------------------------------------------------------------------
// Used to reduce amount of wasted ammo
// Sanity check that we are on target with a smoke cloud
// NEO Jank: For enemy hit case, we're looking for a cloaked enemy and don't want to waste excessive ammo
bool CNEOBotSuppressiveFire::IsSmokeOrEnemyOnLine( CNEOBot *me, const Vector &from, const Vector &to )
{
	trace_t result;
	CTraceFilterSimple filter( me, COLLISION_GROUP_NONE );

	{
		ScopedSmokeLOS smokeGuard( false ); // smoke only stops a trace taken inside this guard
		UTIL_TraceLine( from, to, MASK_SHOT | CONTENTS_BLOCKLOS, &filter, &result );
	}

	if ( !result.DidHit() || !result.m_pEnt )
	{
		return false;
	}

	// Support thermal vision sees through smoke, so a cloud obscures nothing from them
	if ( me->GetClass() != NEO_CLASS_SUPPORT && FStrEq( result.m_pEnt->GetClassname(), SMOKELINEOFSIGHTBLOCKER_ENTITYNAME ) )
	{
		return true;
	}

	return result.m_pEnt->IsPlayer() && me->IsEnemy( result.m_pEnt );
}


//---------------------------------------------------------------------------------------------
bool CNEOBotSuppressiveFire::IsLineWorthFiring( CNEOBot *me, const Vector &from, const Vector &to )
{
	return !me->IsFriendlyNearLineOfFire( from, to )
		&& IsSmokeOrEnemyOnLine( me, from, to )
		&& me->IsLineOfFireClear( from, to, CNEOBot::LINE_OF_FIRE_FLAGS_DEFAULT );
}


//---------------------------------------------------------------------------------------------
// The head trails a moving aim spot. Rather than hold fire until it catches up,
// fire down wherever the barrel points while that is still inside the sweep.
bool CNEOBotSuppressiveFire::IsBarrelOnWorthwhileLine( CNEOBot *me, const Vector &aimSpot, const Vector &believedSpot, float sweepHalfAngle )
{
	const Vector myEyes = me->EyePosition();
	Vector forward;
	me->EyeVectors( &forward );
	Vector toAimSpot = aimSpot - myEyes;
	const float aimRange = toAimSpot.NormalizeInPlace();
	if ( DotProduct( forward, toAimSpot ) >= cosf( DEG2RAD( SUPPRESS_AIM_TOLERANCE_DEG ) ) )
	{
		return true; // on the aim line, which the caller has already checked
	}

	Vector toBelievedSpot = believedSpot - myEyes;
	toBelievedSpot.NormalizeInPlace();
	if ( DotProduct( forward, toBelievedSpot ) < cosf( DEG2RAD( sweepHalfAngle + SUPPRESS_AIM_TOLERANCE_DEG ) ) )
	{
		return false;
	}

	return IsLineWorthFiring( me, myEyes, myEyes + forward * aimRange );
}


//---------------------------------------------------------------------------------------------
CNEOBaseCombatWeapon *CNEOBotSuppressiveFire::GetLoadedFirearm( CNEOBot *me )
{
	auto *myWeapon = static_cast<CNEOBaseCombatWeapon *>( me->GetActiveWeapon() );
	if ( !myWeapon || !( myWeapon->GetNeoWepBits() & NEO_WEP_FIREARM ) || myWeapon->m_bInReload )
	{
		return nullptr;
	}

	if ( myWeapon->Clip1() <= 0 )
	{
		me->ReloadIfLowClip( true );
		return nullptr;
	}

	return myWeapon;
}


//---------------------------------------------------------------------------------------------
void CNEOBotSuppressiveFire::Reset()
{
	m_hTarget = nullptr;
	m_vecBelievedPos = vec3_origin;
	m_fireWindowTimer.Invalidate();
	m_holdAimTimer.Invalidate();
	m_sweepPhase = 0.0f;
	m_bearingErrorDeg = 0.0f;
	m_rangeScale = 1.0f;
}


//---------------------------------------------------------------------------------------------
// A hit or a heard shot gives only a rough bearing and range, and a less skilled bot places it worse.
// The error is kept for the whole window, so following a moving sound does not jump the aim.
void CNEOBotSuppressiveFire::OpenFireWindow( CNEOBot *me, CBaseEntity *target, const Vector &pos, bool bSeen )
{
	static constexpr float errorScaleByDifficulty[ CNEOBot::NUM_DIFFICULTY_LEVELS ] = { 2.5f, 2.0f, 1.5f, 1.0f };
	const float errorScale = bSeen ? 0.0f : errorScaleByDifficulty[ clamp( me->GetDifficulty(), CNEOBot::EASY, CNEOBot::EXPERT ) ];

	m_hTarget = target;
	m_bearingErrorDeg = RandomFloat( -SUPPRESS_BEARING_ERROR_DEG, SUPPRESS_BEARING_ERROR_DEG ) * errorScale;
	m_rangeScale = 1.0f + RandomFloat( -SUPPRESS_RANGE_ERROR, SUPPRESS_RANGE_ERROR ) * errorScale;
	m_vecBelievedPos = RotateBearing( me->GetAbsOrigin(), pos, m_bearingErrorDeg, m_rangeScale );
	m_fireWindowTimer.Start( RandomFloat( SUPPRESS_WINDOW_MIN, SUPPRESS_WINDOW_MAX ) );
	m_holdAimTimer.Invalidate();
	m_sweepPhase = RandomFloat( 0.0f, 1.0f ); // where in the sweep, and which way, the hose starts
}


//---------------------------------------------------------------------------------------------
Vector CNEOBotSuppressiveFire::GetSweptAimSpot( const Vector &from, const Vector &center, float *halfAngle ) const
{
	*halfAngle = 0.0f;
	const float range = ( center - from ).Length2D();
	if ( range < 1.0f )
	{
		return center;
	}

	*halfAngle = clamp( RAD2DEG( atanf( SUPPRESS_SWEEP_HALF_WIDTH / range ) ),
		SUPPRESS_SWEEP_MIN_DEG, SUPPRESS_SWEEP_MAX_DEG );

	// Triangle wave from -1 to 1, so the sweep moves at a constant rate between the turns
	const float cycle = gpGlobals->curtime / SUPPRESS_SWEEP_PERIOD + m_sweepPhase;
	const float wave = 4.0f * fabsf( ( cycle - floorf( cycle ) ) - 0.5f ) - 1.0f;

	return RotateBearing( from, center, *halfAngle * wave );
}


//---------------------------------------------------------------------------------------------
// A hit refreshes the believed position even when the attacker's known-entity record is stale
// NEO Jank: Suppression just looks really bad if the default threat update doesn't update the position
void CNEOBotSuppressiveFire::OnInjured( CNEOBot *me, const CTakeDamageInfo &info )
{
	CBaseEntity *attacker = info.GetAttacker();
	if ( !neo_bot_suppressive_fire.GetBool() || !attacker || !attacker->IsPlayer() || !me->IsEnemy( attacker ) )
	{
		return;
	}

	if ( !( info.GetDamageType() & ( DMG_BULLET | DMG_BUCKSHOT ) ) )
	{
		return; // grenade damage says nothing about where the thrower is now
	}

	const CKnownEntity *known = me->GetVisionInterface()->GetKnown( attacker );
	if ( known && known->IsVisibleInFOVNow() )
	{
		return; // FireWeaponAtEnemy owns threats I can see
	}

	OpenFireWindow( me, attacker, attacker->GetAbsOrigin(), false );
}


//---------------------------------------------------------------------------------------------
// Called for gunfire CNEOBot::OnWeaponFired
// AddKnownEntity does not refresh the position of a threat I already know about, so without this
// a threat only heard would be suppressed where it was last seen, however far it has moved since.
// The rough position stays here rather than in the shared record (IVision::UpdateKnownEntityPosition),
// which every other behaviour reads as exact.
void CNEOBotSuppressiveFire::OnHeardGunfire( CNEOBot *me, CBaseEntity *shooter )
{
	if ( !neo_bot_suppressive_fire.GetBool() || !shooter || !shooter->IsPlayer() || !me->IsEnemy( shooter ) )
	{
		return;
	}

	const CKnownEntity *known = me->GetVisionInterface()->GetKnown( shooter );
	if ( known && known->IsVisibleInFOVNow() )
	{
		return; // FireWeaponAtEnemy owns threats I can see
	}

	if ( m_fireWindowTimer.IsElapsed() )
	{
		OpenFireWindow( me, shooter, shooter->GetAbsOrigin(), false );
	}
	else if ( m_hTarget.Get() == shooter )
	{
		m_vecBelievedPos = RotateBearing( me->GetAbsOrigin(), shooter->GetAbsOrigin(), m_bearingErrorDeg, m_rangeScale );
	}
}


//---------------------------------------------------------------------------------------------
// FireWeaponAtEnemy calls this with its primary threat, once its own checks say I may fire at all.
// Returns true while this is driving aiming behavior, so FireWeaponAtEnemy leaves the aim and trigger alone.
bool CNEOBotSuppressiveFire::Update( CNEOBot *me, const CKnownEntity *threat )
{
	if ( !neo_bot_suppressive_fire.GetBool() )
	{
		return false;
	}

	if ( threat && threat->IsVisibleInFOVNow() )
	{
		m_fireWindowTimer.Invalidate(); // FireWeaponAtEnemy owns threats I can see
		return false;
	}

	if ( m_fireWindowTimer.IsElapsed() )
	{
		if ( !threat || threat->GetTimeSinceLastSeen() > SUPPRESS_MAX_THREAT_AGE )
		{
			return false;
		}

		OpenFireWindow( me, threat->GetEntity(), threat->GetLastKnownPosition(), true );
	}

	CBaseEntity *target = m_hTarget.Get();
	if ( !target || !target->IsAlive() )
	{
		m_fireWindowTimer.Invalidate();
		return false;
	}

	// UpdateLookingAroundForEnemies turns me toward a hidden threat.
	// Only take the aim over once the believed position is in view and the line to it is worth shooting down.
	const Vector believedSpot = m_vecBelievedPos + Vector( 0, 0, SUPPRESS_TORSO_HEIGHT );
	const IVision *vision = me->GetVisionInterface(); // CNEOBotVision hides the position overload
	if ( !vision->IsInFieldOfView( believedSpot ) )
	{
		return false;
	}

	CNEOBaseCombatWeapon *myWeapon = GetLoadedFirearm( me );
	if ( !myWeapon )
	{
		return false;
	}

	const Vector myEyes = me->EyePosition();
	float sweepHalfAngle;
	const Vector aimSpot = GetSweptAimSpot( myEyes, believedSpot, &sweepHalfAngle );
	const bool bLineWorthFiring = IsLineWorthFiring( me, myEyes, aimSpot );

	if ( !bLineWorthFiring && m_holdAimTimer.IsElapsed() )
	{
		return false; // the aim is held longer than a burst, so a trigger still held here is not mine
	}

	me->GetBodyInterface()->AimHeadTowards( aimSpot, IBody::CRITICAL, 0.5f, nullptr, "Suppressing an unseen threat" );

	if ( bLineWorthFiring )
	{
		m_holdAimTimer.Start( SUPRESS_HOLD_AIM_TIME );
	}

	if ( !bLineWorthFiring || !IsBarrelOnWorthwhileLine( me, aimSpot, believedSpot, sweepHalfAngle ) )
	{
		// Keep the hose moving across the gap, but stop a held burst before it reaches a teammate
		me->ReleaseFireButton();
		return true;
	}

	if ( me->IsContinuousFireWeapon( myWeapon ) )
	{
		me->PressFireButton( SUPPRESS_BURST_TIME );
	}
	else if ( me->m_nButtons & IN_ATTACK )
	{
		me->ReleaseFireButton(); // semi-auto needs the trigger released between shots
	}
	else
	{
		me->PressFireButton();
	}

	return true;
}
