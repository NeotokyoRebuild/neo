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
	DECLARE_CLASS(CWeaponGhost, CNEOBaseCombatWeapon);
public:
	DECLARE_NETWORKCLASS();
	DECLARE_PREDICTABLE();

#ifdef GAME_DLL
	DECLARE_ACTTABLE();
	DECLARE_DATADESC();
#endif

	CWeaponGhost(void);
	virtual ~CWeaponGhost(void);

	void ItemPreFrame(void);
	void PrimaryAttack(void);

	virtual void ItemHolsterFrame(void);
	virtual void OnPickedUp(CBaseCombatCharacter *pNewOwner);
	void HandleGhostUnequip(void);

	virtual int GetNeoWepBits(void) const { return NEO_WEP_GHOST; }
	virtual int GetNeoWepXPCost(const int neoClass) const { return 0; }

	virtual float GetSpeedScale(void) const { return 1.0; }

private:
	void ZeroGhostedPlayerLocArray(void);
	void ShowBeacon(int clientIndex, const Vector &pos);
	void HideBeacon(int clientIndex);
	void SetShowEnemies(bool enabled);

#ifdef CLIENT_DLL
	float ShowEnemies(void);
	void HideEnemies(void);
	void Debug_ShowPos(const Vector &pos, bool pvs);
	void PlayGhostSound(float volume = 1.0f);
	void StopGhostSound(void);
	void HandleGhostEquip(void);

	void TryGhostPing(float closestEnemy);
#else
	void UpdateNetworkedEnemyLocations(void);
#endif

private:

#ifdef CLIENT_DLL
	bool m_bHavePlayedGhostEquipSound;
	bool m_bHaveHolsteredTheGhost;

	float m_flLastGhostBeepTime;

	// NEO TODO (Rain): It's probably better to just have one beacon,
	// and call it numplayers times on each update between hud buffer swaps
	CNEOHud_GhostBeacon *m_pGhostBeacons[MAX_PLAYERS];
#else
	
#endif
	CNetworkVar(bool, m_bShouldShowEnemies);
	CNetworkArray(Vector, m_rvPlayerPositions, MAX_PLAYERS);

	CWeaponGhost(const CWeaponGhost &other);
};

#endif // NEO_WEAPON_GHOST_H