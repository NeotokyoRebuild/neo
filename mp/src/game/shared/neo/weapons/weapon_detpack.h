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

#ifdef CLIENT_DLL
#define CWeaponDetpack C_WeaponDetpack
#else
class CNEODeployedDetpack;
#endif

class CWeaponDetpack : public CNEOBaseCombatWeapon
{
	DECLARE_CLASS(CWeaponDetpack, CNEOBaseCombatWeapon);
public:
	DECLARE_NETWORKCLASS();
	DECLARE_PREDICTABLE();

	CWeaponDetpack();

	virtual int GetNeoWepBits(void) const { return NEO_WEP_DETPACK | NEO_WEP_THROWABLE; }
	virtual int GetNeoWepXPCost(const int neoClass) const { return 4; }

	void	Precache(void);
	void	PrimaryAttack(void);
	void	SecondaryAttack(void) { }
	void	DecrementAmmo(CBaseCombatCharacter* pOwner);
	void	ItemPostFrame(void);

	bool	Deploy(void);
	bool	Holster(CBaseCombatWeapon* pSwitchingTo = NULL);

	bool	Reload(void) { return false; }

#ifndef CLIENT_DLL
	void Operator_HandleAnimEvent(animevent_t* pEvent, CBaseCombatCharacter* pOperator);
#endif

	void	TossDetpack(CBasePlayer* pPlayer);

private:
	// Check a throw from vecSrc.  If not valid, move the position back along the line to vecEye
	void	CheckTossPosition(CBasePlayer* pPlayer, const Vector& vecEye, Vector& vecSrc);

	CNetworkVar(bool, m_fDrawbackFinished);
	CNetworkVar(bool, m_bWantsToThrowThisDetpack);
	CNetworkVar(bool, m_bThisDetpackHasBeenThrown);
	CNetworkVar(bool, m_bRemoteHasBeenTriggered);

	CWeaponDetpack(const CWeaponDetpack &other);

#ifdef GAME_DLL
	CNEODeployedDetpack* m_pDetpack;

	DECLARE_ACTTABLE();
#endif
};

#endif // NEO_WEAPON_DETPACK_H