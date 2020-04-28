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

	virtual void	ItemPostFrame(void) OVERRIDE;
	virtual void	ItemPreFrame(void) OVERRIDE;
	virtual void	ItemBusyFrame(void) OVERRIDE;
	virtual void	PrimaryAttack(void) OVERRIDE;
	virtual void	AddViewKick(void) OVERRIDE;
	void	DryFire(void);

	virtual int GetNeoWepBits(void) const OVERRIDE { return NEO_WEP_M41_L; }
	virtual int GetNeoWepXPCost(const int neoClass) const OVERRIDE { return 0; }

	virtual float GetSpeedScale(void) const OVERRIDE { return 145.0 / 170.0; }

	void	UpdatePenaltyTime(void);

	virtual Activity	GetPrimaryAttackActivity(void) OVERRIDE;

	virtual const Vector& GetBulletSpread(void) OVERRIDE
	{
		static Vector cone;

		float ramp = RemapValClamped(m_flAccuracyPenalty,
			0.0f,
			GetMaxAccuracyPenalty(),
			0.0f,
			1.0f);

		// We lerp from very accurate to inaccurate over time
		VectorLerp(VECTOR_CONE_1DEGREES, VECTOR_CONE_6DEGREES, ramp, cone);

		return cone;
	}

	virtual float GetFireRate(void) OVERRIDE { return 0.1f; }
protected:
	virtual float GetFastestDryRefireTime() const OVERRIDE { return 0.2f; }
	virtual float GetAccuracyPenalty() const OVERRIDE { return 0.2f; }
	virtual float GetMaxAccuracyPenalty() const OVERRIDE { return 1.5f; }

private:
	CWeaponM41L(const CWeaponM41L& other);
};

#endif // NEO_WEAPON_M41_L_H