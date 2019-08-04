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

#include "weapon_hl2mpbasehlmpcombatweapon.h"

#ifdef CLIENT_DLL
	#define CNEOBaseCombatWeapon C_NEOBaseCombatWeapon
#endif

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

	// NEO HACK/FIXME (Rain):
	// We override with empty implementation to avoid getting removed by
	// some game logic somewhere. There's probably some flag we could set
	// somewhere to achieve the same without having to do this.
	virtual void SUB_Remove(void) { }

private:
	CNEOBaseCombatWeapon(const CNEOBaseCombatWeapon &);
};

#endif // WEAPON_NEO_BASECOMBATWEAPON_SHARED_H