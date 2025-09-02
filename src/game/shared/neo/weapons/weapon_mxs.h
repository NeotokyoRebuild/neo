#ifndef NEO_WEAPON_MX_S_H
#define NEO_WEAPON_MX_S_H
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
#define CWeaponMX_S C_WeaponMX_S
#endif

class CWeaponMX_S : public CNEOBaseCombatWeapon
{
	DECLARE_CLASS(CWeaponMX_S, CNEOBaseCombatWeapon);
public:
	DECLARE_NETWORKCLASS();
	DECLARE_PREDICTABLE();

#ifdef GAME_DLL
	DECLARE_ACTTABLE();
#endif

	CWeaponMX_S();

	virtual NEO_WEP_BITS_UNDERLYING_TYPE GetNeoWepBits(void) const override { return NEO_WEP_MX_S | NEO_WEP_SUPPRESSED | NEO_WEP_FIREARM; }
	virtual int GetNeoWepXPCost(const int neoClass) const override { return 0; }

	virtual float GetSpeedScale(void) const OVERRIDE { return 0.725f; }

	bool CanBePickedUpByClass(int classId) OVERRIDE;
protected:
	virtual float GetFastestDryRefireTime() const OVERRIDE { return 0.2f; }

private:
	CWeaponMX_S(const CWeaponMX_S &other);
};

#endif // NEO_WEAPON_MX_S_H
