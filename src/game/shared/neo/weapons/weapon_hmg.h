#ifndef NEO_WEAPON_HMG_H
#define NEO_WEAPON_HMG_H
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
#define CWeaponHMG C_WeaponHMG
#endif

class CWeaponHMG : public CNEOBaseCombatWeapon
{
	DECLARE_CLASS(CWeaponHMG, CNEOBaseCombatWeapon);
public:
	DECLARE_NETWORKCLASS();
	DECLARE_PREDICTABLE();

#ifdef GAME_DLL
	DECLARE_ACTTABLE();
#endif

	CWeaponHMG();

	virtual void Spawn() override;
	virtual void PrimaryAttack() override;
	virtual NEO_WEP_BITS_UNDERLYING_TYPE GetNeoWepBits(void) const override { return NEO_WEP_HMG; }
	virtual int GetNeoWepXPCost(const int neoClass) const override { return 20; }

	virtual float GetSpeedScale(void) const OVERRIDE { return 1.0f; }

	bool CanBePickedUpByClass(int classId) OVERRIDE;
	virtual bool CanDrop() final { return false; }
protected:
	virtual float GetFastestDryRefireTime() const OVERRIDE { return 0.2f; }

private:
	CWeaponHMG(const CWeaponHMG &other);
	virtual void Think() override;

	bool	m_bOverheated = false;
	float	m_flOverheatStartTime = 0.0;
};

#endif // NEO_WEAPON_HMG_H
