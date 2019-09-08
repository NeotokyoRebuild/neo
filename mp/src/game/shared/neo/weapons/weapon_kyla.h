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

	DECLARE_NETWORKCLASS();
	DECLARE_PREDICTABLE();

#ifndef CLIENT_DLL
	DECLARE_ACTTABLE();
#endif

private:
	CWeaponKyla(const CWeaponKyla &other);
};

#endif // NEO_WEAPON_KYLA_H