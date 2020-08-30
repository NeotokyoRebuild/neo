#ifndef NEO_WEAPON_SUPA7_H
#define NEO_WEAPON_SUPA7_H
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
#define CWeaponSupa7 C_WeaponSupa7
#endif

class CWeaponSupa7 : public CNEOBaseCombatWeapon
{
	DECLARE_CLASS(CWeaponSupa7, CNEOBaseCombatWeapon);
public:
	DECLARE_NETWORKCLASS();
	DECLARE_PREDICTABLE();

#ifdef GAME_DLL
	DECLARE_ACTTABLE();
	DECLARE_DATADESC();
#endif

	CWeaponSupa7();

private:
	CNetworkVar(bool, m_bNeedPump); // When emptied completely
	CNetworkVar(bool, m_bDelayedFire1); // Fire primary when finished reloading
	CNetworkVar(bool, m_bDelayedFire2); // Fire secondary when finished reloading
	CNetworkVar(bool, m_bDelayedReload); // Reload when finished pump;

	inline void ProposeNextAttack(const float flNextAttackProposal);

public:
	virtual NEO_WEP_BITS_UNDERLYING_TYPE GetNeoWepBits(void) const { return NEO_WEP_SUPA7; }
	virtual int GetNeoWepXPCost(const int neoClass) const { return 0; }

	virtual float GetSpeedScale(void) const { return 140.0 / 170.0; }

	virtual int GetMinBurst() { return 1; }
	virtual int GetMaxBurst() { return 3; }

	bool StartReload(void);
	bool Reload(void);

	void FillClip(void);
	void FinishReload(void);
	void CheckHolsterReload(void);
	void Pump(void);
	void ItemHolsterFrame(void);
	void ItemPostFrame(void);
	void PrimaryAttack(void);
	void SecondaryAttack(void);
	void DryFire(void);

	virtual float GetFireRate(void) OVERRIDE { return 0.7f; }
protected:
	virtual float GetFastestDryRefireTime() const OVERRIDE { return 0.2f; }
	virtual float GetAccuracyPenalty() const OVERRIDE { return 0; }
	virtual float GetMaxAccuracyPenalty() const OVERRIDE { return 0; }

#if(0)
	void WeaponIdle( void );
#endif

private:
	CWeaponSupa7(const CWeaponSupa7 &other);
};

#endif // NEO_WEAPON_SUPA7_H