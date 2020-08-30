#ifndef NEO_WEAPON_KNIFE_H
#define NEO_WEAPON_KNIFE_H
#ifdef _WIN32
#pragma once
#endif

#include "weapon_hl2mpbasebasebludgeon.h"

#ifdef CLIENT_DLL
#include "c_neo_player.h"
#else
#include "neo_player.h"
#endif

#ifdef CLIENT_DLL
#define CWeaponKnife C_WeaponKnife
#endif

#define KNIFE_RANGE 51.0f

class CWeaponKnife : public CBaseHL2MPBludgeonWeapon
{
	DECLARE_CLASS(CWeaponKnife, CBaseHL2MPBludgeonWeapon);
public:
	DECLARE_NETWORKCLASS();
	DECLARE_PREDICTABLE();

#ifdef GAME_DLL
	DECLARE_ACTTABLE();
	DECLARE_DATADESC();
#endif

	CWeaponKnife();

	void PrimaryAttack(void);
	void SecondaryAttack(void) { }
	void Drop(const Vector &vecVelocity) { /* knives shouldn't drop */ }

	float GetRange(void) const { return KNIFE_RANGE; }
	
	virtual	float GetDamageForActivity(Activity hitActivity) OVERRIDE
	{
		return 25.0f;
	}

	virtual float GetFireRate(void) const { return 0.534f; }

	virtual float GetSpeedScale(void) const { return 1.0; }

	virtual bool Deploy(void);

	// FIXME: we should inherit CNEOMelee -> CNEOBaseWep etc...
	//virtual NEO_WEP_BITS_UNDERLYING_TYPE GetNeoWepBits(void) const { return NEO_WEP_KNIFE; }

protected:
	virtual void		Swing(int bIsSecondary) OVERRIDE;
	virtual Activity	ChooseIntersectionPointAndActivity(trace_t& hitTrace, const Vector& mins,
		const Vector& maxs, CBasePlayer* pOwner) OVERRIDE;
	virtual void		Hit(trace_t& traceHit, Activity nHitActivity) OVERRIDE;
private:
	CWeaponKnife(const CWeaponKnife &other);
};

#endif // NEO_WEAPON_KNIFE_H