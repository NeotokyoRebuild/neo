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

	CWeaponSmokeGrenade();

	virtual int GetNeoWepBits(void) const { return NEO_WEP_SMOKE_GRENADE | NEO_WEP_THROWABLE; }

	virtual float GetSpeedScale(void) const { return 1.0; }

	void	Precache(void);
	void	PrimaryAttack(void);
	void	SecondaryAttack(void);
	void	DecrementAmmo(CBaseCombatCharacter* pOwner);
	void	ItemPostFrame(void);

	bool	Deploy(void);
	bool	Holster(CBaseCombatWeapon* pSwitchingTo = NULL);

	bool	Reload(void);

#ifndef CLIENT_DLL
	void Operator_HandleAnimEvent(animevent_t* pEvent, CBaseCombatCharacter* pOperator);
#endif

	void	ThrowGrenade(CBasePlayer* pPlayer);
	void	LobGrenade(CBasePlayer* pPlayer);
	bool	IsPrimed(bool) { return (m_AttackPaused != 0); }

private:
	void	RollGrenade(CBasePlayer* pPlayer);
	// Check a throw from vecSrc.  If not valid, move the position back along the line to vecEye
	void	CheckThrowPosition(CBasePlayer* pPlayer, const Vector& vecEye, Vector& vecSrc);

	CNetworkVar(bool, m_bRedraw);	//Draw the weapon again after throwing a grenade

	CNetworkVar(int, m_AttackPaused);
	CNetworkVar(bool, m_fDrawbackFinished);

	CWeaponSmokeGrenade(const CWeaponSmokeGrenade& other);

#ifndef CLIENT_DLL
	DECLARE_ACTTABLE();
#endif
};

#endif // NEO_WEAPON_SMOKEGRENADE_H