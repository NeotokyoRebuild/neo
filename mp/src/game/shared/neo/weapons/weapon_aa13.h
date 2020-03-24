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

	virtual void	ItemPostFrame(void);
	virtual void	ItemPreFrame(void);
	virtual void	ItemBusyFrame(void);
	virtual void	PrimaryAttack(void);
	virtual void	AddViewKick(void);
	void	DryFire(void);

	virtual int GetNeoWepBits(void) const { return NEO_WEP_AA13; }
	virtual int GetNeoWepXPCost(const int neoClass) const { return 20; }

	virtual float GetSpeedScale(void) const { return 145.0 / 170.0; }

	Activity GetPrimaryAttackActivity(void);

	const Vector& GetBulletSpread(void);

private:
	CNetworkVar(float, m_flSoonestPrimaryAttack);
	CNetworkVar(float, m_flLastAttackTime);
	CNetworkVar(float, m_flAccuracyPenalty);

	CNetworkVar(int, m_nNumShotsFired);

private:
	CWeaponAA13(const CWeaponAA13 &other);
};

#endif // NEO_WEAPON_AA13_H