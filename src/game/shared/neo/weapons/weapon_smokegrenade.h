#ifndef NEO_WEAPON_SMOKEGRENADE_H
#define NEO_WEAPON_SMOKEGRENADE_H
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

#define GRENADE_PAUSED_NO			0
#define GRENADE_PAUSED_PRIMARY		1
#define GRENADE_PAUSED_SECONDARY	2

#define GRENADE_RADIUS	4.0f // inches

#ifdef CLIENT_DLL
#define CWeaponSmokeGrenade C_WeaponSmokeGrenade
#endif

class CWeaponSmokeGrenade : public CNEOBaseCombatWeapon
{
	DECLARE_CLASS(CWeaponSmokeGrenade, CNEOBaseCombatWeapon);
public:
	DECLARE_NETWORKCLASS();
	DECLARE_PREDICTABLE();

#ifdef GAME_DLL
	DECLARE_ACTTABLE();
#endif

	CWeaponSmokeGrenade();

	virtual NEO_WEP_BITS_UNDERLYING_TYPE GetNeoWepBits(void) const { return NEO_WEP_SMOKE_GRENADE | NEO_WEP_THROWABLE; }

	virtual float GetSpeedScale(void) const OVERRIDE { return 0.85f; }

	void	Precache(void);
	void	PrimaryAttack(void);
	void	DecrementAmmo(CBaseCombatCharacter* pOwner);
	void	ItemPostFrame(void);

	bool	Deploy(void);
	bool	Holster(CBaseCombatWeapon* pSwitchingTo = NULL);

	virtual float GetFastestDryRefireTime() const { return 1.f; } // is called if attack button spammed, doesn't really mean much for grenades

	bool	Reload(void);
	void	Drop(const Vector& vecVelocity) OVERRIDE;
	bool	CanDrop() OVERRIDE;
	virtual bool CanPerformSecondaryAttack() const override final { return false; }

#ifndef CLIENT_DLL
	void Operator_HandleAnimEvent(animevent_t* pEvent, CBaseCombatCharacter* pOperator);
#endif

	void	ThrowGrenade(CNEO_Player* pPlayer, bool isAlive = true, CBaseEntity *pAttacker = NULL);
	bool	IsPrimed() const { return (m_AttackPaused != 0); }

	bool CanBePickedUpByClass(int classId) OVERRIDE;
private:
	// Check a throw from vecSrc.  If not valid, move the position back along the line to vecEye
	void	CheckThrowPosition(CBasePlayer* pPlayer, const Vector& vecEye, Vector& vecSrc);

	CNetworkVar(bool, m_bRedraw);	//Draw the weapon again after throwing a grenade

	CNetworkVar(int, m_AttackPaused);
	CNetworkVar(bool, m_bDrawbackFinished);

	CWeaponSmokeGrenade(const CWeaponSmokeGrenade& other);
};

#endif // NEO_WEAPON_SMOKEGRENADE_H
