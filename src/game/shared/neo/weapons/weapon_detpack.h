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

#ifdef GAME_DLL
	DECLARE_ACTTABLE();
	DECLARE_DATADESC();
#endif

	CWeaponDetpack();

	virtual NEO_WEP_BITS_UNDERLYING_TYPE GetNeoWepBits(void) const { return NEO_WEP_DETPACK | NEO_WEP_THROWABLE | NEO_WEP_EXPLOSIVE; }
	virtual int GetNeoWepXPCost(const int neoClass) const OVERRIDE;

	void	Precache(void);
	void	PrimaryAttack(void);
	void	DecrementAmmo(CBaseCombatCharacter* pOwner);
	void	ItemPostFrame(void);

	bool	Deploy(void);
	virtual bool	Holster(CBaseCombatWeapon* pSwitchingTo = NULL) OVERRIDE;

	virtual float GetFastestDryRefireTime() const { return 1.f; } // is called if attack button spammed, doesn't really mean much for grenades
	const char* GetWorldModel(void) const override { return	m_bThisDetpackHasBeenThrown ? "models/weapons/w_detremote.mdl" : "models/weapons/w_detpack.mdl"; };
	bool	Reload(void) { return false; }

	virtual float GetSpeedScale(void) const OVERRIDE { return 0.85f; }

	bool	CanDrop(void) OVERRIDE;
	virtual bool CanPerformSecondaryAttack() const override final { return false; }

	bool CanBePickedUpByClass(int classId) OVERRIDE;

#ifndef CLIENT_DLL
	void Operator_HandleAnimEvent(animevent_t* pEvent, CBaseCombatCharacter* pOperator);

#endif
	void WeaponIdle(void)
	{
		if (HasWeaponIdleTimeElapsed())
		{
			m_bThisDetpackHasBeenThrown ? SendWeaponAnim(ACT_VM_IDLE_DEPLOYED) : SendWeaponAnim(ACT_VM_IDLE);
			SetWeaponIdleTime(gpGlobals->curtime + GetViewModelSequenceDuration());
		}
	}

	void	TossDetpack(CBasePlayer* pPlayer);

	CNetworkVar(bool, m_fDrawbackFinished);
	CNetworkVar(bool, m_bWantsToThrowThisDetpack);
	CNetworkVar(bool, m_bThisDetpackHasBeenThrown);
	CNetworkVar(bool, m_bRemoteHasBeenTriggered);

private:
	// Check a throw from vecSrc.  If not valid, move the position back along the line to vecEye
	void	CheckTossPosition(CBasePlayer* pPlayer, const Vector& vecEye, Vector& vecSrc);

	CWeaponDetpack(const CWeaponDetpack &other);

#ifdef GAME_DLL
	CNEODeployedDetpack* m_pDetpack;
#endif
};

#endif // NEO_WEAPON_DETPACK_H
