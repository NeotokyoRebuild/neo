#ifndef WEAPON_NEO_BASECOMBATWEAPON_SHARED_H
#define WEAPON_NEO_BASECOMBATWEAPON_SHARED_H
#ifdef _WIN32
#pragma once
#endif

#ifdef CLIENT_DLL
	#include "c_neo_player.h"
#else
	#include "neo_player.h"
#endif

#include "neo_player_shared.h"

#include "weapon_hl2mpbasehlmpcombatweapon.h"

#ifdef CLIENT_DLL
	#define CNEOBaseCombatWeapon C_NEOBaseCombatWeapon
#endif

// Weapon bit flags
enum NeoWepBits {
	NEO_WEP_INVALID =			0x0,

	NEO_WEP_AA13 =				(1 << 0),
	NEO_WEP_DETPACK =			(1 << 1),
	NEO_WEP_GHOST =				(1 << 2),
	NEO_WEP_FRAG_GRENADE =		(1 << 3),
	NEO_WEP_JITTE =				(1 << 4),
	NEO_WEP_JITTE_S =			(1 << 5),
	NEO_WEP_KNIFE =				(1 << 6),
	NEO_WEP_KYLA =				(1 << 7),
	NEO_WEP_M41 =				(1 << 8),
	NEO_WEP_M41_L =				(1 << 9),
	NEO_WEP_M41_S =				(1 << 10),
	NEO_WEP_MILSO =				(1 << 11),
	NEO_WEP_MPN =				(1 << 12),
	NEO_WEP_MPN_S =				(1 << 13),
	NEO_WEP_MX =				(1 << 14),
	NEO_WEP_MX_S =				(1 << 15),
	NEO_WEP_PROX_MINE =			(1 << 16),
	NEO_WEP_PZ =				(1 << 17),
	NEO_WEP_SMAC =				(1 << 18),
	NEO_WEP_SMOKE_GRENADE =		(1 << 19),
	NEO_WEP_SRM =				(1 << 20),
	NEO_WEP_SRM_S =				(1 << 21),
	NEO_WEP_SRS =				(1 << 22),
	NEO_WEP_SUPA7 =				(1 << 23),
	NEO_WEP_TACHI =				(1 << 24),
	NEO_WEP_ZR68_C =			(1 << 25),
	NEO_WEP_ZR68_L =			(1 << 26),
	NEO_WEP_ZR68_S =			(1 << 27),
	NEO_WEP_SCOPEDWEAPON =		(1 << 28), // Scoped weapons should OR this in their flags.
	NEO_WEP_THROWABLE =			(1 << 29), // Generic for grenades
};

// These are the .res file id numbers for
// NEO weapon loadout choices used by the
// client cvar "loadout <int>"
enum {
	NEO_WEP_LOADOUT_ID_MPN = 0,
	NEO_WEP_LOADOUT_ID_SRM,
	NEO_WEP_LOADOUT_ID_SRM_S,
	NEO_WEP_LOADOUT_ID_JITTE,
	NEO_WEP_LOADOUT_ID_JITTE_S,
	NEO_WEP_LOADOUT_ID_ZR68C,
	NEO_WEP_LOADOUT_ID_ZR68S,
	NEO_WEP_LOADOUT_ID_ZR68L,
	NEO_WEP_LOADOUT_ID_MX,
	NEO_WEP_LOADOUT_ID_PZ,
	NEO_WEP_LOADOUT_ID_SUPA7,
	NEO_WEP_LOADOUT_ID_M41,
	NEO_WEP_LOADOUT_ID_M41L,

	NEO_WEP_LOADOUT_ID_COUNT
};

const char *GetWeaponByLoadoutId(int id);

class CNEOBaseCombatWeapon : public CBaseHL2MPCombatWeapon
{
#ifndef CLIENT_DLL
	DECLARE_DATADESC();
#endif

	DECLARE_CLASS(CNEOBaseCombatWeapon, CBaseHL2MPCombatWeapon);

public:
	DECLARE_NETWORKCLASS();
	DECLARE_PREDICTABLE();

	CNEOBaseCombatWeapon();

	virtual void Spawn();

	virtual bool Reload( void );

	virtual bool CanBeSelected(void);

	virtual int GetNeoWepBits(void) const { return NEO_WEP_INVALID; }
	virtual int GetNeoWepXPCost(const int neoClass) const { return 0; }

	bool IsGhost(void) const { return (GetNeoWepBits() & NEO_WEP_GHOST) ? true : false; }

	virtual bool Holster(CBaseCombatWeapon* pSwitchingTo);

	// NEO HACK/FIXME (Rain):
	// We override with empty implementation to avoid getting removed by
	// some game logic somewhere. There's probably some flag we could set
	// somewhere to achieve the same without having to do this.
	virtual void SUB_Remove(void) { }

private:
	CNEOBaseCombatWeapon(const CNEOBaseCombatWeapon &other);
};

#endif // WEAPON_NEO_BASECOMBATWEAPON_SHARED_H