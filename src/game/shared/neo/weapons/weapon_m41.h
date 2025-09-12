#ifndef NEO_WEAPON_M41_H
#define NEO_WEAPON_M41_H
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
#define CWeaponM41 C_WeaponM41
#endif

class CWeaponM41 : public CNEOBaseCombatWeapon
{
	DECLARE_CLASS(CWeaponM41, CNEOBaseCombatWeapon);
public:
	DECLARE_NETWORKCLASS();
	DECLARE_PREDICTABLE();

#ifdef GAME_DLL
	DECLARE_ACTTABLE();
#endif

	CWeaponM41();

	virtual NEO_WEP_BITS_UNDERLYING_TYPE GetNeoWepBits(void) const override { return NEO_WEP_M41 | NEO_WEP_FIREARM; }
	virtual int GetNeoWepXPCost(const int neoClass) const override { return 0; }

	virtual float GetSpeedScale(void) const OVERRIDE { return 0.725f; }

	bool CanBePickedUpByClass(int classId) OVERRIDE;
protected:
	virtual float GetFastestDryRefireTime() const OVERRIDE { return 0.2f; }

private:
	CWeaponM41(const CWeaponM41& other);
};

#endif // NEO_WEAPON_M41_H
