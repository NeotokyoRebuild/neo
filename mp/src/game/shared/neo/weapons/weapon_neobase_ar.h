#ifndef WEAPON_NEO_BASE_AR_H
#define WEAPON_NEO_BASE_AR_H
#ifdef _WIN32
#pragma once
#endif

#include "weapon_neobasecombatweapon.h"

#ifdef CLIENT_DLL
#define CNEOAssaultRifle C_NEOAssaultRifle
#endif

// Purpose: Base class for assault rifle type Neo weapons
// to derive from. Current implementation is based on
// weapon_hl2mpbase_machinegun.h
class CNEOAssaultRifle : public CNEOBaseCombatWeapon
{
public:
	DECLARE_CLASS(CNEOAssaultRifle, CNEOBaseCombatWeapon);
	DECLARE_DATADESC();

	CNEOAssaultRifle();

	DECLARE_NETWORKCLASS();
	DECLARE_PREDICTABLE();

	void PrimaryAttack();

	virtual void	ItemPostFrame(void);
	virtual void	FireBullets(const FireBulletsInfo_t &info);
	virtual bool	Deploy(void);

	virtual const Vector &GetBulletSpread(void);

	int WeaponSoundRealtime(WeaponSound_t shoot_type);

	// utility function
	static void DoAssaultRifleGunKick(CBasePlayer *pPlayer, float dampEasy,
		float maxVerticleKickAngle, float fireDurationTime, float slideLimitTime);

protected:
	int m_nShotsFired; // Number of consecutive shots fired

	float m_flNextSoundTime; // real-time clock of when to make next sound

private:
	CNEOAssaultRifle(const CNEOAssaultRifle &other);
};

#endif // WEAPON_NEO_BASE_AR_H