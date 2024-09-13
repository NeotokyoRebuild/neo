#ifndef NEO_WEAPON_KYLA_H
#define NEO_WEAPON_KYLA_H
#ifdef _WIN32
#pragma once
#endif

#include "weapon_neobasecombatweapon.h"

#ifdef CLIENT_DLL
#define CWeaponKyla C_WeaponKyla
#endif

class CWeaponKyla : public CNEOBaseCombatWeapon
{
	DECLARE_CLASS(CWeaponKyla, CNEOBaseCombatWeapon);
public:
	DECLARE_NETWORKCLASS();
	DECLARE_PREDICTABLE();

#ifdef GAME_DLL
	DECLARE_ACTTABLE();
#endif

	CWeaponKyla(void);

	virtual void	PrimaryAttack(void) OVERRIDE;

	virtual NEO_WEP_BITS_UNDERLYING_TYPE GetNeoWepBits(void) const override { return NEO_WEP_KYLA; }
	virtual int GetNeoWepXPCost(const int neoClass) const override { return 0; }

	virtual float GetSpeedScale(void) const override { return 1.0; }

protected:
	virtual float GetFastestDryRefireTime() const OVERRIDE { return 0.2f; }

private:
	CWeaponKyla(const CWeaponKyla &other);
};

#endif // NEO_WEAPON_KYLA_H
