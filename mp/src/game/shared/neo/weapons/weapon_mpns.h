#ifndef NEO_WEAPON_MPN_S_H
#define NEO_WEAPON_MPN_S_H
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
#define CWeaponMPN_S C_WeaponMPN_S
#endif

class CWeaponMPN_S : public CNEOBaseCombatWeapon
{
	DECLARE_CLASS(CWeaponMPN_S, CNEOBaseCombatWeapon);
public:
	DECLARE_NETWORKCLASS();
	DECLARE_PREDICTABLE();

#ifdef GAME_DLL
	DECLARE_ACTTABLE();
#endif

	CWeaponMPN_S();

	void	AddViewKick(void) override;

	virtual NEO_WEP_BITS_UNDERLYING_TYPE GetNeoWepBits(void) const override { return NEO_WEP_MPN_S | NEO_WEP_SUPPRESSED; }
	virtual int GetNeoWepXPCost(const int neoClass) const override { return 0; }

	virtual float GetSpeedScale(void) const override { return 1.0; }

protected:
	virtual float GetFastestDryRefireTime() const OVERRIDE { return 0.2f; }

private:
	CWeaponMPN_S(const CWeaponMPN_S &other);
};

#endif // NEO_WEAPON_MPN_S_H
