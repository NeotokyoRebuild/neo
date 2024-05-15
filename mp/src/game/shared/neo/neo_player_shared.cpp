#include "cbase.h"
#include "neo_player_shared.h"

#include "in_buttons.h"
#include "neo_gamerules.h"

#ifdef CLIENT_DLL
#include "c_neo_player.h"
#ifndef CNEO_Player
#define CNEO_Player C_NEO_Player
#endif
#else
#include "neo_player.h"
#endif

#include "neo_playeranimstate.h"

#include "weapon_neobasecombatweapon.h"

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

ConVar cl_autoreload_when_empty("cl_autoreload_when_empty", "1", FCVAR_USERINFO,
	"Automatically start reloading when the active weapon becomes empty.",
	true, 0.0f, true, 1.0f);

ConVar neo_aim_hold("neo_aim_hold", "0", FCVAR_USERINFO, "Hold to aim as opposed to toggle aim.", true, 0.0f, true, 1.0f);

ConVar neo_recon_superjump_intensity("neo_recon_superjump_intensity", "250", FCVAR_REPLICATED | FCVAR_CHEAT,
	"Recon superjump intensity multiplier.", true, 1.0, false, 0);

bool IsAllowedToZoom(CNEOBaseCombatWeapon *pWep)
{
	if (!pWep || pWep->m_bInReload)
	{
		return false;
	}

	// These weapons are not allowed to be zoomed in with.
	const auto forbiddenZooms =
		NEO_WEP_DETPACK |
		NEO_WEP_FRAG_GRENADE |
		NEO_WEP_GHOST |
		NEO_WEP_KNIFE |
		NEO_WEP_PROX_MINE |
		NEO_WEP_SMOKE_GRENADE;

	return !(pWep->GetNeoWepBits() & forbiddenZooms);
}

CBaseCombatWeapon* GetNeoWepWithBits(const CNEO_Player* player, const NEO_WEP_BITS_UNDERLYING_TYPE& neoWepBits)
{
	int nonNeoWepFoundAtIndex = -1;
	for (int i = 0; i < MAX_WEAPONS; i++)
	{
		auto pWep = player->GetWeapon(i);
		if (!pWep)
		{
			continue;
		}

		auto pNeoWep = dynamic_cast<CNEOBaseCombatWeapon*>(pWep);
		if (!pNeoWep)
		{
			nonNeoWepFoundAtIndex = i;
			continue;
		}

		if (pNeoWep->GetNeoWepBits() & neoWepBits)
		{
			return pWep;
		}
	}

	// NEO HACK (Rain): If wanted a knife, assume it's the non-CNEOBaseCombatWeapon one we found.
	// Should clean this up once all guns inherit from CNEOBaseCombatWeapon.
	if ((neoWepBits & NEO_WEP_KNIFE) && nonNeoWepFoundAtIndex != -1)
	{
		return player->GetWeapon(nonNeoWepFoundAtIndex);
	}

	return NULL;
}

bool PlayerAnimToPlayerAnimEvent(const PLAYER_ANIM playerAnim, PlayerAnimEvent_t& outAnimEvent)
{
	bool success = true;
	if (playerAnim == PLAYER_ANIM::PLAYER_JUMP) { outAnimEvent = PlayerAnimEvent_t::PLAYERANIMEVENT_JUMP; }
	else if (playerAnim == PLAYER_ANIM::PLAYER_RELOAD) { outAnimEvent = PlayerAnimEvent_t::PLAYERANIMEVENT_RELOAD; }
	else if (playerAnim == PLAYER_ANIM::PLAYER_ATTACK1) { outAnimEvent = PlayerAnimEvent_t::PLAYERANIMEVENT_FIRE_GUN_PRIMARY; }
	else { success = false; }
	return success;
}

bool ClientWantsAimHold(const CNEO_Player* player)
{
#ifdef CLIENT_DLL
	return neo_aim_hold.GetBool();
#else
	if (!player)
	{
		return false;
	}
	else if (player->GetFlags() & FL_FAKECLIENT)
	{
		return true;
	}

	return 1 == atoi(engine->GetClientConVarValue(engine->IndexOfEdict(player->edict()), "neo_aim_hold"));
#endif
}

