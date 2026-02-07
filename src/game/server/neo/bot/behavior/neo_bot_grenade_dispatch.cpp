#include "cbase.h"
#include "neo_gamerules.h"
#include "neo_player.h"
#include "bot/neo_bot.h"
#include "bot/behavior/neo_bot_grenade_dispatch.h"
#include "bot/behavior/neo_bot_grenade_throw_frag.h"
#include "bot/behavior/neo_bot_grenade_throw_smoke.h"
#include "weapon_neobasecombatweapon.h"
#include "weapon_grenade.h"
#include "weapon_smokegrenade.h"

extern ConVar sv_neo_grenade_blast_radius;
extern ConVar sv_neo_grenade_fuse_timer;
extern ConVar sv_neo_grenade_throw_intensity;

ConVar sv_neo_bot_grenade_use_frag("sv_neo_bot_grenade_use_frag", "1", FCVAR_NONE, "Allow bots to use frag grenades", true, 0, true, 1);
ConVar sv_neo_bot_grenade_use_smoke("sv_neo_bot_grenade_use_smoke", "1", FCVAR_NONE, "Allow bots to use smoke grenades", true, 0, true, 1);

//---------------------------------------------------------------------------------------------
Action< CNEOBot > *CNEOBotGrenadeDispatch::ChooseGrenadeThrowBehavior( CNEOBot *me, const CKnownEntity *threat )
{
	if ( !threat || !threat->GetEntity() || !threat->GetEntity()->IsPlayer() || !threat->GetEntity()->IsAlive() )
	{
		return nullptr;
	}

	CNEO_Player *pNEOPlayer = ToNEOPlayer( me->GetEntity() );
	if ( !pNEOPlayer )
	{
		return nullptr;
	}

	if (!sv_neo_bot_grenade_use_frag.GetBool() && !sv_neo_bot_grenade_use_smoke.GetBool())
	{
		return nullptr;
	}

	CWeaponGrenade *pFragGrenade = nullptr;
	CWeaponSmokeGrenade *pSmokeGrenade = nullptr;

	for ( int i=0; i<pNEOPlayer->WeaponCount(); ++i )
	{
		CBaseCombatWeapon *pWep = pNEOPlayer->GetWeapon( i );
		if ( !pWep )
		{
			continue;
		}

		if ( sv_neo_bot_grenade_use_frag.GetBool() && ( pFragGrenade = dynamic_cast< CWeaponGrenade * >( pWep ) ) )
		{
			if ( pSmokeGrenade )
			{
				break; // found both
			}
		}
		else if ( sv_neo_bot_grenade_use_smoke.GetBool() && ( pSmokeGrenade = dynamic_cast< CWeaponSmokeGrenade * >( pWep ) ) )
		{
			if ( pFragGrenade )
			{
				break; // found both
			}
		}
	}

	if ( !pFragGrenade && !pSmokeGrenade )
	{
		return nullptr;
	}

	Vector vecThreatPos = threat->GetLastKnownPosition();

	// Should I toss a frag grenade? 
	if ( pFragGrenade )
	{
		float flDistToThreatSqr = me->GetAbsOrigin().DistToSqr( vecThreatPos );
		float flSafeRadius = sv_neo_grenade_blast_radius.GetFloat();
		float flSafeRadiusSqr = flSafeRadius * flSafeRadius;

		if ( flDistToThreatSqr < flSafeRadiusSqr )
		{
			return nullptr;
		}

		float flMaxThrowDist = sv_neo_grenade_throw_intensity.GetFloat() * sv_neo_grenade_fuse_timer.GetFloat();
		if ( flDistToThreatSqr > ( flMaxThrowDist * flMaxThrowDist ) )
		{
			return nullptr;
		}

		if ( NEORules()->IsTeamplay() )
		{
			bool bTeammateTooClose = false;
			for ( int i = 1; i <= gpGlobals->maxClients; i++ )
			{
				CNEO_Player *pPlayer = ToNEOPlayer( UTIL_PlayerByIndex( i ) );
				if ( pPlayer && pPlayer->IsAlive() && pPlayer != pNEOPlayer && pPlayer->InSameTeam(me) )
				{
					if ( pPlayer->GetAbsOrigin().DistToSqr( vecThreatPos ) < flSafeRadiusSqr )
					{
						bTeammateTooClose = true;
						break;
					}
				}
			}

			if ( bTeammateTooClose )
			{
				return nullptr;
			}
		}

		return new CNEOBotGrenadeThrowFrag( pFragGrenade, threat );
	}

	// Should I toss a smoke grenade?
	if ( pSmokeGrenade )
	{
		CNEO_Player *pThreatPlayer = ToNEOPlayer( threat->GetEntity() );
		// Only smoke if threat is NOT support (supports have thermal vision)
		if ( pThreatPlayer && pThreatPlayer->GetClass() != NEO_CLASS_SUPPORT )
		{
			return new CNEOBotGrenadeThrowSmoke( pSmokeGrenade, threat );
		}
	}

	return nullptr;
}
