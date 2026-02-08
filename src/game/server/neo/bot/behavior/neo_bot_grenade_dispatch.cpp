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

ConVar sv_neo_bot_grenade_use_frag("sv_neo_bot_grenade_use_frag", "1", FCVAR_NONE, "Allow bots to use frag grenades", true, 0, true, 1);
ConVar sv_neo_bot_grenade_use_smoke("sv_neo_bot_grenade_use_smoke", "1", FCVAR_NONE, "Allow bots to use smoke grenades", true, 0, true, 1);
ConVar sv_neo_bot_grenade_throw_cooldown("sv_neo_bot_grenade_throw_cooldown", "10", FCVAR_NONE, "Cooldown in seconds between grenade throws for bots");

//---------------------------------------------------------------------------------------------
Action< CNEOBot > *CNEOBotGrenadeDispatch::ChooseGrenadeThrowBehavior( CNEOBot *me, const CKnownEntity *threat )
{
	if (!sv_neo_bot_grenade_use_frag.GetBool() && !sv_neo_bot_grenade_use_smoke.GetBool())
	{
		return nullptr;
	}

	if ( !threat || !threat->GetEntity() || !threat->GetEntity()->IsPlayer() || !threat->GetEntity()->IsAlive() )
	{
		return nullptr;
	}

	CNEO_Player *pNEOPlayer = ToNEOPlayer( me->GetEntity() );
	if ( !pNEOPlayer )
	{
		return nullptr;
	}

	CWeaponGrenade *pFragGrenade = nullptr;
	CWeaponSmokeGrenade *pSmokeGrenade = nullptr;

	int iWeaponCount = pNEOPlayer->WeaponCount();
	for ( int i=0; i<iWeaponCount; ++i )
	{
		CBaseCombatWeapon *pWep = pNEOPlayer->GetWeapon( i );
		if ( !pWep )
		{
			continue;
		}

		CNEOBaseCombatWeapon *pNeoWep = static_cast< CNEOBaseCombatWeapon * >( pWep );
		if ( pNeoWep ) 
		{
			auto bits = pNeoWep->GetNeoWepBits();
			if ( sv_neo_bot_grenade_use_frag.GetBool() && (bits & NEO_WEP_FRAG_GRENADE) )
			{
				pFragGrenade = static_cast< CWeaponGrenade * >( pNeoWep );
				if ( pSmokeGrenade )
				{
					break; // found both
				}
			}
			else if ( sv_neo_bot_grenade_use_smoke.GetBool() && (bits & NEO_WEP_SMOKE_GRENADE) )
			{
				pSmokeGrenade = static_cast< CWeaponSmokeGrenade * >( pNeoWep );
				if ( pFragGrenade )
				{
					break; // found both
				}
			}
		}
	}

	if ( !pFragGrenade && !pSmokeGrenade )
	{
		return nullptr;
	}

	// Should I toss a smoke grenade?
	if ( pSmokeGrenade )
	{
		if ( pNEOPlayer->GetClass() == NEO_CLASS_SUPPORT )
		{
			// I can see through smoke
			return new CNEOBotGrenadeThrowSmoke( pSmokeGrenade, threat );
		}

		bool bEnemySupportInField = false;
		for ( int i = 1; i <= gpGlobals->maxClients; i++ )
		{
			CNEO_Player *pPlayer = ToNEOPlayer( UTIL_PlayerByIndex( i ) );
			if ( pPlayer && pPlayer->IsAlive() && pPlayer->GetClass() == NEO_CLASS_SUPPORT )
			{
			    if ( !pPlayer->InSameTeam(pNEOPlayer) )
				{
					bEnemySupportInField = true; // Enemy support could see through smoke
					break;
				}
			}
		}

		// Enemy team does not have Support players in the field
		if (!bEnemySupportInField)
		{
			return new CNEOBotGrenadeThrowSmoke( pSmokeGrenade, threat );
		}
	}

	// Should I toss a frag grenade? 
	if ( pFragGrenade )
	{
		if ( !CNEOBotGrenadeThrowFrag::IsFragSafe( me, threat->GetLastKnownPosition() ) )
		{
			return nullptr;
		}

		return new CNEOBotGrenadeThrowFrag( pFragGrenade, threat );
	}

	return nullptr;
}
