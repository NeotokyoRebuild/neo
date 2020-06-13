#ifndef NEO_WEAPON_JITTE_H
#define NEO_WEAPON_JITTE_H
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
#define CWeaponJitte C_WeaponJitte
#endif

class CWeaponJitte : public CNEOBaseCombatWeapon
{
	DECLARE_CLASS(CWeaponJitte, CNEOBaseCombatWeapon);
public:
	DECLARE_NETWORKCLASS();
	DECLARE_PREDICTABLE();

#ifdef GAME_DLL
	DECLARE_ACTTABLE();
#endif

	CWeaponJitte();

	virtual void	ItemPostFrame(void) OVERRIDE;
	virtual void	ItemPreFrame(void) OVERRIDE;
	virtual void	ItemBusyFrame(void) OVERRIDE;
	virtual void	PrimaryAttack(void) OVERRIDE;
	virtual void	AddViewKick(void) OVERRIDE;
	void	DryFire(void);

	virtual void Spawn(void) OVERRIDE;
	virtual bool Deploy(void) OVERRIDE;

	virtual int GetNeoWepBits(void) const OVERRIDE { return NEO_WEP_JITTE; }
	virtual int GetNeoWepXPCost(const int neoClass) const OVERRIDE { return 0; }

	virtual float GetSpeedScale(void) const OVERRIDE { return 160.0 / 170.0; }

	void	UpdatePenaltyTime(void);

	Activity	GetPrimaryAttackActivity(void) OVERRIDE;

	virtual float GetFireRate(void) OVERRIDE { return 0.085f; }

protected:
	virtual float GetAccuracyPenalty() const OVERRIDE { return 0.2f; }
	virtual float GetMaxAccuracyPenalty() const OVERRIDE { return 0.5f; }
	virtual float GetFastestDryRefireTime() const OVERRIDE { return 0.2f; }

private:
	CWeaponJitte(const CWeaponJitte &other);
};

#endif // NEO_WEAPON_JITTE_H