#ifndef NEO_WEAPON_SRS_H
#define NEO_WEAPON_SRS_H
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
#define CWeaponSRS C_WeaponSRS
#endif

class CWeaponSRS : public CNEOBaseCombatWeapon
{
	DECLARE_CLASS(CWeaponSRS, CNEOBaseCombatWeapon);
public:
	DECLARE_NETWORKCLASS();
	DECLARE_PREDICTABLE();

#ifdef GAME_DLL
	DECLARE_ACTTABLE();
#endif

	CWeaponSRS();

	void	ItemPostFrame(void);
	void	ItemPreFrame(void);
	void	ItemBusyFrame(void);
	void	PrimaryAttack(void);
	void	AddViewKick(void);
	void	DryFire(void);

	virtual void Spawn(void);
	virtual bool Deploy(void);

	virtual int GetNeoWepBits(void) const { return NEO_WEP_SRS | NEO_WEP_SCOPEDWEAPON; }
	virtual int GetNeoWepXPCost(const int neoClass) const { return 20; }

	virtual float GetSpeedScale(void) const { return 116.0 / 136.0; }

	void	UpdatePenaltyTime(void);

	Activity	GetPrimaryAttackActivity(void);

	virtual float GetFireRate(void) OVERRIDE { return 1.0f; }
protected:
	virtual float GetFastestDryRefireTime() const OVERRIDE { return 0.2f; }
	virtual float GetAccuracyPenalty() const OVERRIDE { return 0.2f; }
	virtual float GetMaxAccuracyPenalty() const OVERRIDE { return 1.5f; }

private:
	CWeaponSRS(const CWeaponSRS &other);
};

#endif // NEO_WEAPON_SRS_H