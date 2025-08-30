#ifndef NEO_WEAPON_BALC_H
#define NEO_WEAPON_BALC_H
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
#define CWeaponBALC C_WeaponBALC
#endif

class CWeaponBALC : public CNEOBaseCombatWeapon
{
	DECLARE_CLASS(CWeaponBALC, CNEOBaseCombatWeapon);
public:
	DECLARE_NETWORKCLASS();
	DECLARE_PREDICTABLE();

#ifdef GAME_DLL
	DECLARE_ACTTABLE();
#endif

	CWeaponBALC();

	virtual void Spawn() override;
	virtual void PrimaryAttack() override;
	virtual NEO_WEP_BITS_UNDERLYING_TYPE GetNeoWepBits(void) const override { return NEO_WEP_BALC; }
	virtual int GetNeoWepXPCost(const int neoClass) const override { return 20; }

	virtual float GetSpeedScale(void) const OVERRIDE { return 1.0f; }

	bool CanBePickedUpByClass(int classId) OVERRIDE;
	virtual bool CanDrop() final { return false; }
protected:
	virtual float GetFastestDryRefireTime() const OVERRIDE { return 0.2f; }

private:
	CWeaponBALC(const CWeaponBALC &other);
	virtual void Think() override;

	bool	m_bOverheated = false;
	float	m_flOverheatStartTime = 0.0;
};

#endif // NEO_WEAPON_BALC_H
