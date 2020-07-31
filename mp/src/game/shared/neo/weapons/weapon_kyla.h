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
	virtual void	SecondaryAttack(void) OVERRIDE { if (!ShootingIsPrevented()) { BaseClass::SecondaryAttack(); } }
	
	virtual int GetNeoWepBits(void) const { return NEO_WEP_KYLA; }
	virtual int GetNeoWepXPCost(const int neoClass) const { return 0; }

	virtual float GetSpeedScale(void) const { return 1.0; }

	virtual float GetFireRate(void) OVERRIDE { return 0.35f; }
protected:
	virtual float GetFastestDryRefireTime() const OVERRIDE { return 0.2f; }
	virtual float GetAccuracyPenalty() const OVERRIDE { return 0.2f; }
	virtual float GetMaxAccuracyPenalty() const OVERRIDE { return 0.5f; }

private:
	CWeaponKyla(const CWeaponKyla &other);
};

#endif // NEO_WEAPON_KYLA_H