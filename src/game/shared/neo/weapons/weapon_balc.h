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
	DECLARE_DATADESC();
#endif

	CWeaponBALC();

	virtual void Precache() override;
	virtual void Spawn() override;
	virtual void PrimaryAttack() override;
	virtual void SecondaryAttack() override;
	virtual NEO_WEP_BITS_UNDERLYING_TYPE GetNeoWepBits(void) const override { return NEO_WEP_BALC | NEO_WEP_FIREARM; }
	virtual int GetNeoWepXPCost(const int neoClass) const override { return 20; }
	virtual void ItemPostFrame() override;

	virtual float GetSpeedScale(void) const OVERRIDE { return 1.0f; }

	bool CanBePickedUpByClass(int classId) OVERRIDE;
	virtual bool UsesTracers() override final { return true; }
	virtual const char *GetTracerType() override final { return "AirboatGunTracer"; }
	virtual bool CanDrop() final { return false; }
	virtual bool CanAim() final { return true; }
	inline virtual bool IsAutomatic(void) const override final
	{
		return m_bIsPrimaryFireMode;
	}

	CNetworkVar(bool, m_bOverheated);
	CNetworkVar(bool, m_bCharging);
	CNetworkVar(bool, m_bCharged);
	CNetworkVar(bool, m_bIsPrimaryFireMode);

protected:
	virtual float GetFastestDryRefireTime() const OVERRIDE { return 0.2f; }

private:
	CWeaponBALC(const CWeaponBALC &other);
#ifdef GAME_DLL
	virtual void Think() override;
	float GetCoolingRate();
#endif

	CountdownTimer m_chargeTimer;

	CNetworkVar(float, m_flOverheatStartTime);
	CNetworkVar(float, m_flChargeStartTime);
};

#endif // NEO_WEAPON_BALC_H
