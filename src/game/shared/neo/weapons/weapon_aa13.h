#ifndef NEO_WEAPON_AA13_H
#define NEO_WEAPON_AA13_H
#ifdef _WIN32
#pragma once
#endif

#include "weapon_neobasecombatweapon.h"

#ifdef CLIENT_DLL
#define CWeaponAA13 C_WeaponAA13
#endif

class CWeaponAA13 : public CNEOBaseCombatWeapon
{
	DECLARE_CLASS(CWeaponAA13, CNEOBaseCombatWeapon);
public:
	DECLARE_NETWORKCLASS();
	DECLARE_PREDICTABLE();

#ifdef GAME_DLL
	DECLARE_ACTTABLE();
#endif

	CWeaponAA13(void);

	virtual void	PrimaryAttack(void) OVERRIDE;

	virtual NEO_WEP_BITS_UNDERLYING_TYPE GetNeoWepBits(void) const OVERRIDE { return NEO_WEP_AA13 | NEO_WEP_FIREARM; }
	virtual int GetNeoWepXPCost(const int neoClass) const OVERRIDE { return 20; }

	virtual float GetSpeedScale(void) const OVERRIDE { return 0.725f; }

	bool CanBePickedUpByClass(int classId) OVERRIDE;
protected:
	virtual float GetFastestDryRefireTime() const OVERRIDE { return 0.2f; }

private:
	CWeaponAA13(const CWeaponAA13 &other);
};

#endif // NEO_WEAPON_AA13_H
