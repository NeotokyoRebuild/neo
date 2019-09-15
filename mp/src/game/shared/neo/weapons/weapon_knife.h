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
#endif

	CWeaponKnife();

	void PrimaryAttack(void);
	void SecondaryAttack(void) { return; }
	void Drop(const Vector &vecVelocity);

	float GetRange(void) const { return KNIFE_RANGE; }
	float GetDamageForActivity(Activity activity) const { return 25.0f; }

	virtual float GetFireRate(void) const { return 0.534f; }

private:
	CWeaponKnife(const CWeaponKnife &other);
};

#endif // NEO_WEAPON_KNIFE_H