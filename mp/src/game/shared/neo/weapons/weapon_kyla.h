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
	CWeaponKyla(void);

	void PrimaryAttack(void);

	virtual int GetNeoWepBits(void) const { return NEO_WEP_KYLA; }
	virtual int GetNeoWepXPCost(const int neoClass) const { return 0; }

	DECLARE_NETWORKCLASS();
	DECLARE_PREDICTABLE();

#ifndef CLIENT_DLL
	DECLARE_ACTTABLE();
#endif

private:
	CWeaponKyla(const CWeaponKyla &other);
};

#endif // NEO_WEAPON_KYLA_H