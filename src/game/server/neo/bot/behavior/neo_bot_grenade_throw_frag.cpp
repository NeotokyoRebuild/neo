#include "cbase.h"
#include "bot/neo_bot.h"
#include "bot/neo_bot_path_compute.h"
#include "bot/behavior/neo_bot_grenade_throw_frag.h"
#include "neo_gamerules.h"
#include "neo_player.h"
#include "weapon_neobasecombatweapon.h"

#include "nav_pathfind.h"

extern ConVar sv_neo_grenade_blast_radius;

ConVar sv_neo_bot_grenade_frag_safety_range_multiplier("sv_neo_bot_grenade_frag_safety_range_multiplier", "1.2",
	FCVAR_NONE, "Multiplier for frag grenade blast radius safety check", true, 0.1, false, 0);

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

	m_PathFollower.SetMinLookAheadDistance( me->GetDesiredPathLookAheadRange() );
	if ( !CNEOBotPathCompute( me, m_PathFollower, m_vecThreatLastKnownPos, FASTEST_ROUTE ) )
	{
		return Done( "Path to threat last known position unavailable" );
	}

	return Continue();
}

CNEOBotGrenadeThrow::ThrowTargetResult CNEOBotGrenadeThrowFrag::UpdateGrenadeTargeting( CNEOBot *me, CNEOBaseCombatWeapon *pWeapon )
{
	// Should be checked by CNEOBotGrenadeThrow::Update
	Assert( m_hThreatGrenadeTarget.Get() && m_vecThreatLastKnownPos != vec3_invalid );

	// Check if there is a more immediate threat interrupting my grenade throw
	const CKnownEntity* pPrimaryThreat = me->GetVisionInterface()->GetPrimaryKnownThreat();
	// Not using LINE_OF_FIRE_FLAGS_SHOTGUN because we want to abort grenade throw if threat could shoot us from behind glass
	if (pPrimaryThreat && pPrimaryThreat->GetEntity() && me->IsLineOfFireClear(pPrimaryThreat->GetEntity(), CNEOBot::LINE_OF_FIRE_FLAGS_DEFAULT))
	{
		// consider panic throwing the grenade at the immediate threat
		m_vecTarget = pPrimaryThreat->GetLastKnownPosition();
	}
	else if (m_vecTarget == vec3_invalid)
	{
		if (me->IsLineOfSightClear(m_vecThreatLastKnownPos))
		{
			// See the last known location, but don't see threat
			// Infer where the threat could have gone
			// CHEAT: calculate a path from the last known position to the actual threat position
			// and throw at the furthest visible point along that path
			if (m_hThreatGrenadeTarget.Get())
			{
				if ( m_scanTimer.IsElapsed() )
				{
					Vector vecThrowTarget = FindEmergencePointAlongPath(me, m_vecThreatLastKnownPos, m_hThreatGrenadeTarget->GetAbsOrigin());

					if (vecThrowTarget != vec3_invalid)
					{
						m_vecTarget = vecThrowTarget;
					}
					else
					{
						m_scanTimer.Start( 0.2f );
					}
				}
			}
		}
		
		if ( m_vecTarget == vec3_invalid )
		{
			// continue investigating last known position
			m_PathFollower.Update(me);
			return THROW_TARGET_WAIT;
		}
	}
	Assert( m_vecTarget != vec3_invalid );

	// Safety checks
	if ( !me->IsLineOfFireClear( m_vecTarget, CNEOBot::LINE_OF_FIRE_FLAGS_SHOTGUN ) )
	{
		return THROW_TARGET_CANCEL; // risk of grenade bouncing back at me
	}

	if ( NEORules()->IsTeamplay() )
	{

		const float flSafeRadius = GetFragSafetyRadius();
		const float flSafeRadiusSqr = flSafeRadius * flSafeRadius;
		const CNEO_Player *pMePlayer = ToNEOPlayer( me->GetEntity() );

		for ( int i = 1; i <= gpGlobals->maxClients; i++ )
		{
			CNEO_Player *pPlayer = ToNEOPlayer( UTIL_PlayerByIndex( i ) );
			// Also happen to check my distance, as I am on the same team
			if ( pPlayer && pPlayer->IsAlive() && pPlayer->InSameTeam( pMePlayer ) )
			{
				if ( pPlayer->GetAbsOrigin().DistToSqr( m_vecTarget ) < flSafeRadiusSqr )
				{
					return THROW_TARGET_CANCEL; // risk of friendly fire
				}
			}
		}
	}

	return THROW_TARGET_READY;
}

float CNEOBotGrenadeThrowFrag::GetFragSafetyRadius()
{
	return sv_neo_grenade_blast_radius.GetFloat() * sv_neo_bot_grenade_frag_safety_range_multiplier.GetFloat();
}

