#ifndef NEO_WEAPON_SMAC_H
#define NEO_WEAPON_SMAC_H
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
#define CWeaponSMAC C_WeaponSMAC
#endif

class CWeaponSMAC : public CNEOBaseCombatWeapon
{
	DECLARE_CLASS(CWeaponSMAC, CNEOBaseCombatWeapon);
public:
	DECLARE_NETWORKCLASS();
	DECLARE_PREDICTABLE();

#ifdef GAME_DLL
	DECLARE_ACTTABLE();
#endif

	CWeaponSMAC();

	virtual NEO_WEP_BITS_UNDERLYING_TYPE GetNeoWepBits(void) const override { return NEO_WEP_SMAC | NEO_WEP_FIREARM; }
	virtual int GetNeoWepXPCost(const int neoClass) const override { return 0; }

	virtual float GetSpeedScale(void) const OVERRIDE { return 0.8f; }

	bool CanBePickedUpByClass(int classId) OVERRIDE;
protected:
	virtual float GetFastestDryRefireTime() const OVERRIDE { return 0.2f; }

private:
	CWeaponSMAC(const CWeaponSMAC& other);
};

#endif // NEO_WEAPON_SMAC_H
