#ifndef NEO_WEAPON_ZR68_L_H
#define NEO_WEAPON_ZR68_L_H
#ifdef _WIN32
#pragma once
#endif

#include "cbase.h"
#include "npcevent.h"
#include "in_buttons.h"

#ifdef CLIENT_DLL
#include "c_neo_player.h"
#else
#include "neo_player.h"
#endif

#include "weapon_neobasecombatweapon.h"

#ifdef CLIENT_DLL
#define CWeaponZR68L C_WeaponZR68L
#endif

class CWeaponZR68L : public CNEOBaseCombatWeapon
{
	DECLARE_CLASS(CWeaponZR68L, CNEOBaseCombatWeapon);
public:
	DECLARE_NETWORKCLASS();
	DECLARE_PREDICTABLE();

#ifdef GAME_DLL
	DECLARE_ACTTABLE();
#endif

	CWeaponZR68L();

	virtual NEO_WEP_BITS_UNDERLYING_TYPE GetNeoWepBits(void) const override { return NEO_WEP_ZR68_L | NEO_WEP_SCOPEDWEAPON; }
	virtual int GetNeoWepXPCost(const int neoClass) const override { return 0; }

	virtual float GetSpeedScale(void) const override { return 145.0 / 170.0; }

protected:
	virtual float GetFastestDryRefireTime() const OVERRIDE { return 0.2f; }

private:
	CWeaponZR68L(const CWeaponZR68L &other);
};

#endif // NEO_WEAPON_ZR68_L_H
