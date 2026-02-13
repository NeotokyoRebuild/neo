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

	void	ItemPreFrame(void) override;
	virtual bool	Deploy() override;
	virtual void	PrimaryAttack() override;
	virtual bool	Reload(void) OVERRIDE;

	virtual NEO_WEP_BITS_UNDERLYING_TYPE GetNeoWepBits(void) const override { return NEO_WEP_SRS | NEO_WEP_SCOPEDWEAPON | NEO_WEP_FIREARM; }
	virtual int GetNeoWepXPCost(const int neoClass) const override { return 20; }

	virtual float GetSpeedScale(void) const OVERRIDE { return 0.725f; }
	bool CanBePickedUpByClass(int classId) OVERRIDE;

	inline bool GetNeedsBolting() const { return m_bNeedsBolting; }

protected:
	virtual float GetFastestDryRefireTime() const OVERRIDE { return 0.2f; }

private:
	CWeaponSRS(const CWeaponSRS &other);

	CNetworkVar(bool, m_bNeedsBolting);
};

#endif // NEO_WEAPON_SRS_H
