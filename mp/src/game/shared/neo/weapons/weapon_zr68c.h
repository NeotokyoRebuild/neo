#ifndef NEO_WEAPON_ZR68_C_H
#define NEO_WEAPON_ZR68_C_H
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
#define CWeaponZR68C C_WeaponZR68C
#endif

class CWeaponZR68C : public CNEOBaseCombatWeapon
{
	DECLARE_CLASS(CWeaponZR68C, CNEOBaseCombatWeapon);
public:
	DECLARE_NETWORKCLASS();
	DECLARE_PREDICTABLE();

#ifdef GAME_DLL
	DECLARE_ACTTABLE();
#endif

	CWeaponZR68C();

	void	ItemPostFrame(void);
	void	ItemPreFrame(void);
	void	ItemBusyFrame(void);
	virtual void	PrimaryAttack(void) OVERRIDE { if (!ShootingIsPrevented()) { BaseClass::PrimaryAttack(); } }
	virtual void	SecondaryAttack(void) OVERRIDE { if (!ShootingIsPrevented()) { BaseClass::SecondaryAttack(); } }
	void	AddViewKick(void);
	void	DryFire(void);

	virtual void Spawn(void);
	virtual bool Deploy(void);

	virtual NEO_WEP_BITS_UNDERLYING_TYPE GetNeoWepBits(void) const { return NEO_WEP_ZR68_C; }
	virtual int GetNeoWepXPCost(const int neoClass) const { return 0; }

	virtual float GetSpeedScale(void) const { return 160.0 / 170.0; }

	void	UpdatePenaltyTime(void);

	Activity	GetPrimaryAttackActivity(void);

	virtual float GetFireRate(void) OVERRIDE { return 0.1f; }
protected:
	virtual float GetFastestDryRefireTime() const OVERRIDE { return 0.2f; }
	virtual float GetAccuracyPenalty() const OVERRIDE { return 0.2f; }
	virtual float GetMaxAccuracyPenalty() const OVERRIDE { return 1.5f; }

private:
	CWeaponZR68C(const CWeaponZR68C &other);
};

#endif // NEO_WEAPON_ZR68_C_H