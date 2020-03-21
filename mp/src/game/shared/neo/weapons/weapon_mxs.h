#ifndef NEO_WEAPON_MX_S_H
#define NEO_WEAPON_MX_S_H
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

#define	MX_S_FASTEST_REFIRE_TIME 0.1f
#define MX_S_FASTEST_DRY_REFIRE_TIME	0.2f

#define	MX_S_ACCURACY_SHOT_PENALTY_TIME		0.2f
#define	MX_S_ACCURACY_MAXIMUM_PENALTY_TIME	0.5f	// Maximum penalty to deal out

#ifdef CLIENT_DLL
#define CWeaponMX_S C_WeaponMX_S
#endif

class CWeaponMX_S : public CNEOBaseCombatWeapon
{
public:
	DECLARE_CLASS(CWeaponMX_S, CNEOBaseCombatWeapon);

	CWeaponMX_S();

	DECLARE_NETWORKCLASS();
	DECLARE_PREDICTABLE();

	void	ItemPostFrame(void);
	void	ItemPreFrame(void);
	void	ItemBusyFrame(void);
	void	PrimaryAttack(void);
	void	AddViewKick(void);
	void	DryFire(void);

	virtual void Spawn(void);
	virtual bool Deploy(void);

	virtual int GetNeoWepBits(void) const { return NEO_WEP_MX_S; }
	virtual int GetNeoWepXPCost(const int neoClass) const { return 0; }

	virtual float GetSpeedScale(void) const { return 145.0 / 170.0; }

	void	UpdatePenaltyTime(void);

	Activity	GetPrimaryAttackActivity(void);

	virtual const Vector& GetBulletSpread(void)
	{
		static Vector cone;

		float ramp = RemapValClamped(m_flAccuracyPenalty,
			0.0f,
			MX_S_ACCURACY_MAXIMUM_PENALTY_TIME,
			0.0f,
			1.0f);

		// We lerp from very accurate to inaccurate over time
		VectorLerp(VECTOR_CONE_1DEGREES, VECTOR_CONE_6DEGREES, ramp, cone);

		return cone;
	}

	virtual float GetFireRate(void);

#ifdef GAME_DLL
	DECLARE_ACTTABLE();
#endif

private:
	CNetworkVar(float, m_flSoonestAttack);
	CNetworkVar(float, m_flLastAttackTime);
	CNetworkVar(float, m_flAccuracyPenalty);

	CNetworkVar(int, m_nNumShotsFired);

private:
	CWeaponMX_S(const CWeaponMX_S &other);
};

#endif // NEO_WEAPON_MX_S_H