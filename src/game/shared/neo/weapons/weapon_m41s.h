#ifndef NEO_WEAPON_M41_S_H
#define NEO_WEAPON_M41_S_H
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
#define CWeaponM41S C_WeaponM41S
#endif

class CWeaponM41S : public CNEOBaseCombatWeapon
{
	DECLARE_CLASS(CWeaponM41S, CNEOBaseCombatWeapon);
public:
	DECLARE_NETWORKCLASS();
	DECLARE_PREDICTABLE();

#ifdef GAME_DLL
	DECLARE_ACTTABLE();
#endif

	CWeaponM41S();

	virtual NEO_WEP_BITS_UNDERLYING_TYPE GetNeoWepBits(void) const OVERRIDE { return NEO_WEP_M41_S | NEO_WEP_SUPPRESSED | NEO_WEP_FIREARM; }
	virtual int GetNeoWepXPCost(const int neoClass) const OVERRIDE { return 0; }

	virtual float GetSpeedScale(void) const OVERRIDE { return 0.725f; }

	bool CanBePickedUpByClass(int classId) OVERRIDE;
protected:
	virtual float GetFastestDryRefireTime() const OVERRIDE { return 0.2f; }

private:
	CWeaponM41S(const CWeaponM41S& other);
};

#endif // NEO_WEAPON_M41_S_H
