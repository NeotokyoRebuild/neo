#ifndef NEO_WEAPON_DETPACK_H
#define NEO_WEAPON_DETPACK_H
#ifdef _WIN32
#pragma once
#endif

#include "cbase.h"

#ifdef CLIENT_DLL
#include "c_neo_player.h"
#else
#include "neo_player.h"
#endif

#include "weapon_neobasecombatweapon.h"

#define DETPACK_DEPLOY_PAUSED_NO			0
#define DETPACK_DEPLOY_PAUSED_PRIMARY		1
//#define DETPACK_DEPLOY_PAUSED_SECONDARY	2

#ifdef CLIENT_DLL
#define CWeaponDetpack C_WeaponDetpack
#endif

class CWeaponDetpack : public CNEOBaseCombatWeapon
{
	DECLARE_CLASS(CWeaponDetpack, CNEOBaseCombatWeapon);
public:
	DECLARE_NETWORKCLASS();
	DECLARE_PREDICTABLE();

	CWeaponDetpack();

	virtual int GetNeoWepBits(void) const { return NEO_WEP_DETPACK | NEO_WEP_THROWABLE; }

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

	void	TossDetpack(CBasePlayer* pPlayer);

private:
	// Check a throw from vecSrc.  If not valid, move the position back along the line to vecEye
	void	CheckTossPosition(CBasePlayer* pPlayer, const Vector& vecEye, Vector& vecSrc);

	CNetworkVar(bool, m_bRedraw);	//Draw the weapon again after deploying det
	CNetworkVar(bool, m_fDrawbackFinished);

	CNetworkVar(int, m_AttackPaused);

	CWeaponDetpack(const CWeaponDetpack &other);

#ifndef CLIENT_DLL
	DECLARE_ACTTABLE();
#endif
};

#endif // NEO_WEAPON_DETPACK_H