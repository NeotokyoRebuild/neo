#ifndef NEO_WEAPON_MPN_H
#define NEO_WEAPON_MPN_H
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
#define CWeaponMPN C_WeaponMPN
#endif

class CWeaponMPN : public CNEOBaseCombatWeapon
{
	DECLARE_CLASS(CWeaponMPN, CNEOBaseCombatWeapon);
public:
	DECLARE_NETWORKCLASS();
	DECLARE_PREDICTABLE();

#ifdef GAME_DLL
	DECLARE_ACTTABLE();
#endif

	CWeaponMPN();

	virtual void	ItemPostFrame(void) OVERRIDE;
	virtual void	ItemPreFrame(void) OVERRIDE;
	virtual void	ItemBusyFrame(void) OVERRIDE;
	virtual void	PrimaryAttack(void) OVERRIDE { if (!ShootingIsPrevented()) { BaseClass::PrimaryAttack(); } }
	virtual void	SecondaryAttack(void) OVERRIDE { if (!ShootingIsPrevented()) { BaseClass::SecondaryAttack(); } }
	virtual void	AddViewKick(void) OVERRIDE;
	void	DryFire(void);

	virtual void Spawn(void) OVERRIDE;
	virtual bool Deploy(void) OVERRIDE;

	virtual NEO_WEP_BITS_UNDERLYING_TYPE GetNeoWepBits(void) const OVERRIDE { return NEO_WEP_MPN; }
	virtual int GetNeoWepXPCost(const int neoClass) const OVERRIDE { return 0; }

	virtual float GetSpeedScale(void) const OVERRIDE { return 1.0; }

	void	UpdatePenaltyTime(void);

	virtual Activity GetPrimaryAttackActivity(void) OVERRIDE;

	virtual float GetFireRate(void) OVERRIDE { return 0.065f; }
protected:
	virtual float GetFastestDryRefireTime() const OVERRIDE { return 0.2f; }
	virtual float GetAccuracyPenalty() const OVERRIDE { return 0.025f; }
	virtual float GetMaxAccuracyPenalty() const OVERRIDE { return 0.5f; }

private:
	CWeaponMPN(const CWeaponMPN &other);
};

#endif // NEO_WEAPON_MPN_H