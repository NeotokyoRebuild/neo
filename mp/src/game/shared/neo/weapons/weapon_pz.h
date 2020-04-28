#ifndef NEO_WEAPON_PZ_H
#define NEO_WEAPON_PZ_H
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
#define CWeaponPZ C_WeaponPZ
#endif

class CWeaponPZ : public CNEOBaseCombatWeapon
{
	DECLARE_CLASS(CWeaponPZ, CNEOBaseCombatWeapon);
public:
	DECLARE_NETWORKCLASS();
	DECLARE_PREDICTABLE();

#ifdef GAME_DLL
	DECLARE_ACTTABLE();
#endif

	CWeaponPZ();

	void	ItemPostFrame(void);
	void	ItemPreFrame(void);
	void	ItemBusyFrame(void);
	void	PrimaryAttack(void);
	void	AddViewKick(void);
	void	DryFire(void);

	virtual void Spawn(void);
	virtual bool Deploy(void);

	virtual int GetNeoWepBits(void) const { return NEO_WEP_PZ; }
	virtual int GetNeoWepXPCost(const int neoClass) const { return 20; }

	virtual float GetSpeedScale(void) const { return 108.0 / 136.0; }

	void	UpdatePenaltyTime(void);

	Activity	GetPrimaryAttackActivity(void);

	virtual const Vector& GetBulletSpread(void) OVERRIDE
	{
		static Vector cone;

		const float ramp = RemapValClamped(m_flAccuracyPenalty,
			0.0f,
			GetMaxAccuracyPenalty(),
			0.0f,
			1.0f);

		// We lerp from very accurate to inaccurate over time
		VectorLerp(VECTOR_CONE_1DEGREES, VECTOR_CONE_6DEGREES, ramp, cone);

		return cone;
	}

	virtual float GetFireRate(void) OVERRIDE { return 0.085f; }
protected:
	virtual float GetFastestDryRefireTime() const OVERRIDE { return 0.2f; }
	virtual float GetAccuracyPenalty() const OVERRIDE { return 0.2f; }
	virtual float GetMaxAccuracyPenalty() const OVERRIDE { return 0.5f; }

private:
	CWeaponPZ(const CWeaponPZ &other);
};

#endif // NEO_WEAPON_PZ_H