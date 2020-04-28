#ifndef NEO_WEAPON_M41_L_H
#define NEO_WEAPON_M41_L_H
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

#define	M41_L_FASTEST_REFIRE_TIME 0.1f
#define	M41_L_FASTEST_DRY_REFIRE_TIME	0.2f

#define	M41_L_ACCURACY_SHOT_PENALTY_TIME		0.2f
#define	M41_L_ACCURACY_MAXIMUM_PENALTY_TIME	1.5f	// Maximum penalty to deal out

#ifdef CLIENT_DLL
#define CWeaponM41L C_WeaponM41L
#endif

class CWeaponM41L : public CNEOBaseCombatWeapon
{
	DECLARE_CLASS(CWeaponM41L, CNEOBaseCombatWeapon);
public:
	DECLARE_NETWORKCLASS();
	DECLARE_PREDICTABLE();

#ifdef GAME_DLL
	DECLARE_ACTTABLE();
#endif

	CWeaponM41L();

	void	ItemPostFrame(void);
	void	ItemPreFrame(void);
	void	ItemBusyFrame(void);
	void	PrimaryAttack(void);
	void	AddViewKick(void);
	void	DryFire(void);

	virtual int GetNeoWepBits(void) const { return NEO_WEP_M41_L; }
	virtual int GetNeoWepXPCost(const int neoClass) const { return 0; }

	virtual float GetSpeedScale(void) const { return 145.0 / 170.0; }

	void	UpdatePenaltyTime(void);

	Activity	GetPrimaryAttackActivity(void);

	virtual const Vector& GetBulletSpread(void)
	{
		static Vector cone;

		float ramp = RemapValClamped(m_flAccuracyPenalty,
			0.0f,
			M41_L_ACCURACY_MAXIMUM_PENALTY_TIME,
			0.0f,
			1.0f);

		// We lerp from very accurate to inaccurate over time
		VectorLerp(VECTOR_CONE_1DEGREES, VECTOR_CONE_6DEGREES, ramp, cone);

		return cone;
	}

	virtual float GetFireRate(void) { return M41_L_FASTEST_REFIRE_TIME; }

private:
	CWeaponM41L(const CWeaponM41L& other);
};

#endif // NEO_WEAPON_M41_L_H