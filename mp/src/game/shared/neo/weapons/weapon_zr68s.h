#ifndef NEO_WEAPON_ZR68_S_H
#define NEO_WEAPON_ZR68_S_H
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

#define	ZR68S_FASTEST_REFIRE_TIME 0.1f
#define	ZR68S_FASTEST_DRY_REFIRE_TIME	0.2f

#define	ZR68S_ACCURACY_SHOT_PENALTY_TIME		0.2f
#define	ZR68S_ACCURACY_MAXIMUM_PENALTY_TIME	1.5f	// Maximum penalty to deal out

#ifdef CLIENT_DLL
#define CWeaponZR68S C_WeaponZR68S
#endif

class CWeaponZR68S : public CNEOBaseCombatWeapon
{
public:
	DECLARE_CLASS(CWeaponZR68S, CNEOBaseCombatWeapon);

	CWeaponZR68S();

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

	virtual int GetNeoWepBits(void) const { return NEO_WEP_ZR68_S; }
	virtual int GetNeoWepXPCost(const int neoClass) const { return 0; }

	void	UpdatePenaltyTime(void);

	Activity	GetPrimaryAttackActivity(void);

	virtual const Vector& GetBulletSpread(void)
	{
		static Vector cone;

		float ramp = RemapValClamped(m_flAccuracyPenalty,
			0.0f,
			ZR68S_ACCURACY_MAXIMUM_PENALTY_TIME,
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
	CWeaponZR68S(const CWeaponZR68S &other);
};

#endif // NEO_WEAPON_ZR68_S_H