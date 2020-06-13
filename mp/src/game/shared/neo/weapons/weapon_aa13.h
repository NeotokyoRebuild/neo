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

	void UpdatePenaltyTime(void);

	virtual void	ItemPostFrame(void) OVERRIDE;
	virtual void	ItemPreFrame(void) OVERRIDE;
	virtual void	ItemBusyFrame(void) OVERRIDE;
	virtual void	PrimaryAttack(void) OVERRIDE;
	virtual void	AddViewKick(void) OVERRIDE;
	void	DryFire(void);

	virtual int GetNeoWepBits(void) const OVERRIDE { return NEO_WEP_AA13; }
	virtual int GetNeoWepXPCost(const int neoClass) const OVERRIDE { return 20; }

	virtual float GetSpeedScale(void) const OVERRIDE { return 145.0 / 170.0; }

	Activity GetPrimaryAttackActivity(void) OVERRIDE;

	virtual float GetFireRate() OVERRIDE { return 0.333f; }

protected:
	virtual float GetAccuracyPenalty() const OVERRIDE { return 0.2f; }
	virtual float GetMaxAccuracyPenalty() const OVERRIDE { return 1.5f; }
	virtual float GetFastestDryRefireTime() const OVERRIDE { return 0.2f; }

private:
	CWeaponAA13(const CWeaponAA13 &other);
};

#endif // NEO_WEAPON_AA13_H