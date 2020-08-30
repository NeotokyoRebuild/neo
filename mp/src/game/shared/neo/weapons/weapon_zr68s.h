#ifndef NEO_WEAPON_ZR68_S_H
#define NEO_WEAPON_ZR68_S_H
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
#define CWeaponZR68S C_WeaponZR68S
#endif

class CWeaponZR68S : public CNEOBaseCombatWeapon
{
	DECLARE_CLASS(CWeaponZR68S, CNEOBaseCombatWeapon);
public:
	DECLARE_NETWORKCLASS();
	DECLARE_PREDICTABLE();

#ifdef GAME_DLL
	DECLARE_ACTTABLE();
#endif

	CWeaponZR68S();

	virtual void	ItemPostFrame(void) OVERRIDE;
	virtual void	ItemPreFrame(void) OVERRIDE;
	virtual void	ItemBusyFrame(void) OVERRIDE;
	virtual void	PrimaryAttack(void) OVERRIDE { if (!ShootingIsPrevented()) { BaseClass::PrimaryAttack(); } }
	virtual void	SecondaryAttack(void) OVERRIDE { if (!ShootingIsPrevented()) { BaseClass::SecondaryAttack(); } }
	virtual void	AddViewKick(void) OVERRIDE;

	void	DryFire(void);

	virtual void Spawn(void) OVERRIDE;
	virtual bool Deploy(void) OVERRIDE;

	virtual NEO_WEP_BITS_UNDERLYING_TYPE GetNeoWepBits(void) const OVERRIDE { return NEO_WEP_ZR68_S; }
	virtual int GetNeoWepXPCost(const int neoClass) const OVERRIDE { return 0; }

	virtual float GetSpeedScale(void) const OVERRIDE { return 155.0 / 170.0; }

	void	UpdatePenaltyTime(void);

	virtual Activity	GetPrimaryAttackActivity(void) OVERRIDE;

	virtual float GetFireRate(void) OVERRIDE { return 0.1f; }
protected:
	virtual float GetFastestDryRefireTime() const OVERRIDE { return 0.2f; }
	virtual float GetAccuracyPenalty() const OVERRIDE { return 0.2f; }
	virtual float GetMaxAccuracyPenalty() const OVERRIDE { return 1.5f; }

private:
	CWeaponZR68S(const CWeaponZR68S &other);
};

#endif // NEO_WEAPON_ZR68_S_H