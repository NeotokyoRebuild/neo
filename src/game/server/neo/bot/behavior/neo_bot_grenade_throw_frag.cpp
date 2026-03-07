#include "cbase.h"
#include "bot/neo_bot.h"
#include "bot/neo_bot_path_compute.h"
#include "bot/behavior/neo_bot_grenade_throw_frag.h"
#include "neo_gamerules.h"
#include "neo_player.h"
#include "weapon_neobasecombatweapon.h"

#include "nav_pathfind.h"

extern ConVar sv_neo_grenade_blast_radius;
extern ConVar sv_neo_grenade_fuse_timer;
extern ConVar sv_neo_grenade_throw_intensity;

ConVar sv_neo_bot_grenade_frag_safety_range_multiplier("sv_neo_bot_grenade_frag_safety_range_multiplier", "1.2",
	FCVAR_NONE, "Multiplier for frag grenade blast radius safety check", true, 0.1, false, 0);

ConVar sv_neo_bot_grenade_frag_max_range_ratio("sv_neo_bot_grenade_frag_max_range_ratio", "0.7",
	FCVAR_NONE, "Ratio of max frag grenade throw distance to account for slowing factors", true, 0.1, true, 1);

CNEOBotGrenadeThrowFrag::CNEOBotGrenadeThrowFrag( CNEOBaseCombatWeapon *pWeapon, const CKnownEntity *threat )
	: CNEOBotGrenadeThrow( pWeapon, threat )
{
}

ActionResult< CNEOBot >	CNEOBotGrenadeThrowFrag::OnStart( CNEOBot *me, Action< CNEOBot > *priorAction )
{
	ActionResult< CNEOBot > result = CNEOBotGrenadeThrow::OnStart( me, priorAction );
	if ( result.IsDone() )
	{
		return result;
	}

	return Continue();
}

CNEOBotGrenadeThrow::ThrowTargetResult CNEOBotGrenadeThrowFrag::UpdateGrenadeTargeting( CNEOBot *me, CNEOBaseCombatWeapon *pWeapon )
{
	// Should be checked by CNEOBotGrenadeThrow::Update by this point
	Assert( m_hThreatGrenadeTarget.Get() && m_vecThreatLastKnownPos != vec3_invalid );

	const float flSafeRadius = GetFragSafetyRadius();
	if ( me->GetAbsOrigin().DistToSqr( m_vecThreatLastKnownPos ) < ( flSafeRadius * flSafeRadius ) )
	{
		return THROW_TARGET_CANCEL; // Too close to investigation location
	}

	// Check if there is a more immediate threat interrupting my grenade throw
	bool bImmediateThreatPresent = false;
	const CKnownEntity* pPrimaryThreat = me->GetVisionInterface()->GetPrimaryKnownThreat();
	// Not using LINE_OF_FIRE_FLAGS_SHOTGUN because we want to abort grenade throw if threat could shoot us from behind glass
	if (pPrimaryThreat && pPrimaryThreat->GetEntity() && me->IsLineOfFireClear(pPrimaryThreat->GetEntity(), CNEOBot::LINE_OF_FIRE_FLAGS_DEFAULT))
	{
		// consider panic throwing the grenade at the immediate threat
		bImmediateThreatPresent = true;
		if ( m_scanTimer.IsElapsed() || m_vecTarget == vec3_invalid )
		{
			// Update target to immediate threat
			m_hThreatGrenadeTarget = pPrimaryThreat->GetEntity();
			m_vecTarget = pPrimaryThreat->GetLastKnownPosition();
			m_vecThreatLastKnownPos = m_vecTarget;
			m_scanTimer.Start( 0.2f );
		}
	}
	else if (m_vecTarget == vec3_invalid)
	{
		// Last known location in view, but don't see threat
		// Infer where the threat could have gone
		// CHEAT: calculate a path from the last known position to the actual threat position
		// and throw at the furthest visible point along that path
		if ( me->IsLineOfSightClear( m_vecThreatLastKnownPos ) && m_scanTimer.IsElapsed() )
		{
			const Vector& vecThrowTarget = FindEmergencePointAlongPath( me, m_vecThreatLastKnownPos, m_hThreatGrenadeTarget->GetAbsOrigin() );

			if ( vecThrowTarget != vec3_invalid )
			{
				m_vecTarget = vecThrowTarget;
			}
			else
			{
				m_scanTimer.Start( 0.2f );
			}
		}
		
		if ( m_vecTarget == vec3_invalid )
		{
			// continue investigating last known position
			return THROW_TARGET_WAIT;
		}
	}
	Assert( m_vecTarget != vec3_invalid );

	// Safety checks
	if ( !me->IsLineOfFireClear( m_vecTarget, CNEOBot::LINE_OF_FIRE_FLAGS_SHOTGUN ) )
	{
		if (bImmediateThreatPresent)
		{
			return THROW_TARGET_CANCEL;
		}
		return THROW_TARGET_WAIT; // attempt to reroute if blocked
	}

	if ( !IsFragSafe( me, m_vecTarget ) )
	{
		return THROW_TARGET_CANCEL; // risk of friendly fire
	}

	return THROW_TARGET_READY;
}

float CNEOBotGrenadeThrowFrag::GetFragSafetyRadius()
{
	return sv_neo_grenade_blast_radius.GetFloat() * sv_neo_bot_grenade_frag_safety_range_multiplier.GetFloat();
}

bool CNEOBotGrenadeThrowFrag::IsFragSafe( const CNEOBot *me, const Vector &vecTarget )
{
	const float flDistToTargetSqr = me->GetAbsOrigin().DistToSqr( vecTarget );
	const float flSafeRadius = GetFragSafetyRadius();
	const float flSafeRadiusSqr = flSafeRadius * flSafeRadius;

	if ( flDistToTargetSqr < flSafeRadiusSqr )
	{
		return false; // Too close to self to safely throw
	}

	const float flMaxThrowDist = sv_neo_grenade_throw_intensity.GetFloat() * sv_neo_grenade_fuse_timer.GetFloat() * sv_neo_bot_grenade_frag_max_range_ratio.GetFloat();
	if ( flDistToTargetSqr > ( flMaxThrowDist * flMaxThrowDist ) )
	{
		return false; // Too far to throw
	}

	if ( !NEORules()->IsTeamplay() )
	{
		return true;
	}

	for ( int i = 1; i <= gpGlobals->maxClients; i++ )
	{
		CNEO_Player *pPlayer = ToNEOPlayer( UTIL_PlayerByIndex( i ) );
		if ( pPlayer == me->GetEntity() )
		{
			continue;
		}

		if ( pPlayer && pPlayer->IsAlive() && pPlayer->InSameTeam( me->GetEntity() ) )
		{
			// Check if teammate is within a cone in front of the throw trajectory
			Vector vecToTarget = vecTarget - me->GetBodyInterface()->GetEyePosition();
			vecToTarget.NormalizeInPlace();

			Vector vecToPlayer = pPlayer->WorldSpaceCenter() - me->GetBodyInterface()->GetEyePosition();
			vecToPlayer.NormalizeInPlace();

			if ( DotProduct( vecToTarget, vecToPlayer ) > 0.8f )
			{
				trace_t tr;
				UTIL_TraceLine( me->GetBodyInterface()->GetEyePosition(), pPlayer->WorldSpaceCenter(), MASK_SHOT_HULL, me->GetEntity(), COLLISION_GROUP_NONE, &tr );

				if ( tr.m_pEnt && tr.m_pEnt->IsPlayer() )
				{
					CNEO_Player *pHitPlayer = ToNEOPlayer( tr.m_pEnt );
					if ( pHitPlayer && pHitPlayer->InSameTeam( me->GetEntity() ) )
					{
						return false; // teammate in path of proposed grenade trajectory
					}
				}
			}

			// Check if teammate would be near blast radius
			if ( pPlayer->GetAbsOrigin().DistToSqr( vecTarget ) < flSafeRadiusSqr )
			{
				trace_t tr;
				UTIL_TraceLine( vecTarget, pPlayer->WorldSpaceCenter(), MASK_SHOT_HULL, nullptr, COLLISION_GROUP_NONE, &tr );

				if ( tr.m_pEnt && tr.m_pEnt->IsPlayer() )
				{
					CNEO_Player *pHitPlayer = ToNEOPlayer( tr.m_pEnt );
					if ( pHitPlayer && pHitPlayer->InSameTeam( me->GetEntity() ) )
					{
						return false; // teammate potentially in blast radius
					}
				}
			}
		}
	}

	return true;
}

