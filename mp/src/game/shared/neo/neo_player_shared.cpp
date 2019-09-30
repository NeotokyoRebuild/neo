#include "cbase.h"
#include "neo_player_shared.h"

#include "in_buttons.h"
#include "neo_gamerules.h"

#include "weapon_neobasecombatweapon.h"

#ifdef CLIENT_DLL
#include "c_neo_player.h"
#ifndef CNEO_Player
#define CNEO_Player C_NEO_Player
#endif
#else
#include "neo_player.h"
#endif

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

ConVar cl_autoreload_when_empty("cl_autoreload_when_empty", "1", FCVAR_USERINFO,
	"Automatically start reloading when the active weapon becomes empty.",
	true, 0.0f, true, 1.0f);

ConVar neo_recon_superjump_intensity("neo_recon_superjump_intensity", "250", FCVAR_REPLICATED | FCVAR_CHEAT,
	"Recon superjump intensity multiplier.", true, 1.0, false, 0);

bool IsAllowedToZoom(CNEOBaseCombatWeapon *pWep)
{
	if (!pWep)
	{
		return false;
	}

	// These weapons are not allowed to be zoomed in with.
	const int forbiddenZooms = NEO_WEP_DETPACK | NEO_WEP_FRAG_GRENADE | NEO_WEP_GHOST |
		NEO_WEP_KNIFE | NEO_WEP_PROX_MINE | NEO_WEP_SMOKE_GRENADE;

	return !(pWep->GetNeoWepBits() & forbiddenZooms);
}
