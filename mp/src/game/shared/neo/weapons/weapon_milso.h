#ifndef NEO_WEAPON_MILSO_H
#define NEO_WEAPON_MILSO_H
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

#define	MILSO_FASTEST_REFIRE_TIME 0.2f

#define	MILSO_FASTEST_DRY_REFIRE_TIME 0.2f

#define	MILSO_ACCURACY_SHOT_PENALTY_TIME 0.2f	
#define	MILSO_ACCURACY_MAXIMUM_PENALTY_TIME 1.5f	// Maximum penalty to deal out

#ifdef CLIENT_DLL
#define CWeaponMilso C_WeaponMilso
#endif

class CWeaponMilso : public CNEOBaseCombatWeapon
{
public:
	DECLARE_CLASS(CWeaponMilso, CNEOBaseCombatWeapon);

	CWeaponMilso(void);

	DECLARE_NETWORKCLASS();
	DECLARE_PREDICTABLE();

	void	Precache(void);
	void	ItemPostFrame(void);
	void	ItemPreFrame(void);
	void	ItemBusyFrame(void);
	void	PrimaryAttack(void);
	void	AddViewKick(void);
	void	DryFire(void);

	void	UpdatePenaltyTime(void);

	Activity	GetPrimaryAttackActivity(void);

	virtual bool Reload(void);

	virtual const Vector& GetBulletSpread(void)
	{
		static Vector cone;

		float ramp = RemapValClamped(m_flAccuracyPenalty,
			0.0f,
			MILSO_ACCURACY_MAXIMUM_PENALTY_TIME,
			0.0f,
			1.0f);

		// We lerp from very accurate to inaccurate over time
		VectorLerp(VECTOR_CONE_1DEGREES, VECTOR_CONE_6DEGREES, ramp, cone);

		return cone;
	}

	virtual float GetFireRate(void);

#ifndef CLIENT_DLL
	DECLARE_ACTTABLE();
#endif

private:
	CNetworkVar(float, m_flSoonestPrimaryAttack);
	CNetworkVar(float, m_flLastAttackTime);
	CNetworkVar(float, m_flAccuracyPenalty);

	CNetworkVar(int, m_nNumShotsFired);

private:
	CWeaponMilso(const CWeaponMilso &other);
};

#endif // NEO_WEAPON_MILSO_H