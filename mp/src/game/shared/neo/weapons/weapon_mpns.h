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

#define	MPN_S_FASTEST_REFIRE_TIME			0.065f
#define MPN_S_FASTEST_DRY_REFIRE_TIME		0.2f

#define	MPN_S_ACCURACY_SHOT_PENALTY_TIME	0.025f
#define	MPN_S_ACCURACY_MAXIMUM_PENALTY_TIME	0.5f	// Maximum penalty to deal out

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

	void	ItemPostFrame(void);
	void	ItemPreFrame(void);
	void	ItemBusyFrame(void);
	void	PrimaryAttack(void);
	void	AddViewKick(void);
	void	DryFire(void);

	virtual void Spawn(void);
	virtual bool Deploy(void);

	virtual int GetNeoWepBits(void) const { return NEO_WEP_MPN_S; }
	virtual int GetNeoWepXPCost(const int neoClass) const { return 0; }

	virtual float GetSpeedScale(void) const { return 1.0; }

	void	UpdatePenaltyTime(void);

	Activity	GetPrimaryAttackActivity(void);

	virtual const Vector& GetBulletSpread(void)
	{
		static Vector cone;

		float ramp = RemapValClamped(m_flAccuracyPenalty,
			0.0f,
			MPN_S_ACCURACY_MAXIMUM_PENALTY_TIME,
			0.0f,
			1.0f);

		// We lerp from very accurate to inaccurate over time
		VectorLerp(VECTOR_CONE_1DEGREES, VECTOR_CONE_20DEGREES, ramp, cone);

		return cone;
	}

	virtual float GetFireRate(void);

private:
	CWeaponMPN_S(const CWeaponMPN_S &other);
};

#endif // NEO_WEAPON_MPN_S_H