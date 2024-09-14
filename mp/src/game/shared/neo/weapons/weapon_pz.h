#ifndef NEO_WEAPON_PZ_H
#define NEO_WEAPON_PZ_H
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
#define CWeaponPZ C_WeaponPZ
#endif

class CWeaponPZ : public CNEOBaseCombatWeapon
{
	DECLARE_CLASS(CWeaponPZ, CNEOBaseCombatWeapon);
public:
	DECLARE_NETWORKCLASS();
	DECLARE_PREDICTABLE();

#ifdef GAME_DLL
	DECLARE_ACTTABLE();
#endif

	CWeaponPZ();

	virtual NEO_WEP_BITS_UNDERLYING_TYPE GetNeoWepBits(void) const override { return NEO_WEP_PZ; }
	virtual int GetNeoWepXPCost(const int neoClass) const override { return 20; }

	virtual float GetSpeedScale(void) const override { return 108.0 / 136.0; }

	bool CanBePickedUpByClass(int classId) OVERRIDE;
protected:
	virtual float GetFastestDryRefireTime() const OVERRIDE { return 0.2f; }

private:
	CWeaponPZ(const CWeaponPZ &other);
};

#endif // NEO_WEAPON_PZ_H
