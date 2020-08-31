#ifndef NEO_WEAPON_PBK56S_H
#define NEO_WEAPON_PBK56S_H
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
#define CWeaponPBK56S C_WeaponPBK56S
#endif

class CWeaponPBK56S : public CNEOBaseCombatWeapon
{
	DECLARE_CLASS(CWeaponPBK56S, CNEOBaseCombatWeapon);
public:
	DECLARE_NETWORKCLASS();
	DECLARE_PREDICTABLE();

#ifdef GAME_DLL
	DECLARE_ACTTABLE();
#endif

	CWeaponPBK56S();

	virtual NEO_WEP_BITS_UNDERLYING_TYPE GetNeoWepBits(void) const OVERRIDE { return NEO_WEP_PBK56S; }
	virtual int GetNeoWepXPCost(const int neoClass) const OVERRIDE { return 20; }

	virtual float GetSpeedScale(void) const OVERRIDE { return 1.0; }

	virtual float GetFireRate(void) OVERRIDE { return 0.08f; }

protected:
	virtual float GetAccuracyPenalty() const { return 0.2f; }
	virtual float GetMaxAccuracyPenalty() const { return 0.2f; }
	virtual float GetFastestDryRefireTime() const { return 1.5f; }

private:
	CWeaponPBK56S(const CWeaponPBK56S& other);
};

#endif // NEO_WEAPON_PBK56S_H