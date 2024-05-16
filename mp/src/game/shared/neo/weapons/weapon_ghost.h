#ifndef NEO_WEAPON_GHOST_H
#define NEO_WEAPON_GHOST_H
#ifdef _WIN32
#pragma once
#endif

#include "weapon_neobasecombatweapon.h"

#ifdef CLIENT_DLL
#define CWeaponGhost C_WeaponGhost
#define CBaseCombatCharacter C_BaseCombatCharacter
#define CBasePlayer C_BasePlayer
#define CNEO_Player C_NEO_Player
#endif

class CNEO_Player;

class CWeaponGhost : public CNEOBaseCombatWeapon
{
	DECLARE_CLASS(CWeaponGhost, CNEOBaseCombatWeapon);
public:
	DECLARE_NETWORKCLASS();
	DECLARE_PREDICTABLE();

#ifdef GAME_DLL
	DECLARE_ACTTABLE();
#endif

	CWeaponGhost(void);
	
	virtual void PrimaryAttack(void) OVERRIDE { }
	virtual void SecondaryAttack(void) OVERRIDE { }

	virtual void ItemHolsterFrame(void);
	virtual void OnPickedUp(CBaseCombatCharacter *pNewOwner);
	void HandleGhostUnequip(void);

	virtual NEO_WEP_BITS_UNDERLYING_TYPE GetNeoWepBits(void) const { return NEO_WEP_GHOST; }
	virtual int GetNeoWepXPCost(const int neoClass) const { return 0; }

	virtual float GetSpeedScale(void) const { return 1.0; }
	float GetGhostRangeInHammerUnits() const;
	bool IsPosWithinViewDistance(const Vector &otherPlayerPos);
	bool IsPosWithinViewDistance(const Vector &otherPlayerPos, float &dist);
	float DistanceToPos(const Vector& otherPlayerPos);

#ifdef CLIENT_DLL
	void PlayGhostSound(float volume = 1.0f);
	void StopGhostSound(void);
	void HandleGhostEquip(void);

	void TryGhostPing(float closestEnemy);
#endif

private:

#ifdef CLIENT_DLL
	bool m_bHavePlayedGhostEquipSound;
	bool m_bHaveHolsteredTheGhost;

	float m_flLastGhostBeepTime;
#endif

	CWeaponGhost(const CWeaponGhost &other);
};

#endif // NEO_WEAPON_GHOST_H