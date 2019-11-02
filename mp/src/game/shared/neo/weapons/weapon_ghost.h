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

#ifdef CLIENT_DLL
class CNEOHud_GhostBeacon;
#endif

class CNEO_Player;

class CWeaponGhost : public CNEOBaseCombatWeapon
{
public:
	DECLARE_CLASS(CWeaponGhost, CNEOBaseCombatWeapon);

	CWeaponGhost(void);
	virtual ~CWeaponGhost(void);

	DECLARE_NETWORKCLASS();
	DECLARE_PREDICTABLE();

	void ItemPreFrame(void);
	void PrimaryAttack(void);

	virtual void ItemHolsterFrame(void);
	virtual void OnPickedUp(CBaseCombatCharacter *pNewOwner);
	void HandleGhostUnequip(void);

	virtual int GetNeoWepBits(void) const { return NEO_WEP_GHOST; }

#ifdef GAME_DLL
	DECLARE_ACTTABLE();
#endif

private:
	inline void ZeroGhostedPlayerLocArray(void);
	inline void ShowBeacon(int clientIndex, const Vector &pos);
	inline void HideBeacon(int clientIndex);
	inline void SetShowEnemies(bool enabled);

#ifdef CLIENT_DLL
	inline float ShowEnemies(void);
	inline void HideEnemies(void);
	inline void Debug_ShowPos(const Vector &pos, bool pvs);
	inline void PlayGhostSound(float volume = 1.0f);
	inline void StopGhostSound(void);
	inline void HandleGhostEquip(void);

	inline void TryGhostPing(float closestEnemy);
#else
	void UpdateNetworkedEnemyLocations(void);
#endif

private:

#ifdef CLIENT_DLL
	bool m_bHavePlayedGhostEquipSound;
	bool m_bHaveHolsteredTheGhost;

	// NEO TODO (Rain): It's probably better to just have one beacon,
	// and call it numplayers times on each update between hud buffer swaps
	CNEOHud_GhostBeacon *m_pGhostBeacons[MAX_PLAYERS];
#else
	
#endif
	CNetworkVar(bool, m_bShouldShowEnemies);
	CNetworkArray(Vector, m_rvPlayerPositions, MAX_PLAYERS);

	CWeaponGhost(const CWeaponGhost &);
};

#endif // NEO_WEAPON_GHOST_H