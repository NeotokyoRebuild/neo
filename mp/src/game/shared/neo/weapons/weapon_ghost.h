#ifndef NEO_WEAPON_GHOST_H
#define NEO_WEAPON_GHOST_H
#ifdef _WIN32
#pragma once
#endif

#ifdef CLIENT_DLL
#include "c_neo_player.h"
#else
#include "neo_player.h"
#endif

#include "weapon_neobasecombatweapon.h"

#ifdef CLIENT_DLL
#define CWeaponGhost C_WeaponGhost
#define CBaseCombatCharacter C_BaseCombatCharacter
#define CBasePlayer C_BasePlayer
#define CNEOPlayer C_NEO_Player
#endif

#ifdef CLIENT_DLL
class CNEOHud_GhostBeacon;
#endif

class CWeaponGhost : public CNEOBaseCombatWeapon
{
public:
	DECLARE_CLASS(CWeaponGhost, CNEOBaseCombatWeapon);

	CWeaponGhost(void);
#ifdef CLIENT_DLL
	virtual ~CWeaponGhost(void);
#endif

	DECLARE_NETWORKCLASS();
	DECLARE_PREDICTABLE();

	void ItemPreFrame(void);
	void PrimaryAttack(void);
	
	virtual void ItemHolsterFrame(void);
	virtual void Equip(CBaseCombatCharacter *pOwner);
	void HandleGhostUnequip(void);

#ifdef GAME_DLL
	DECLARE_ACTTABLE();
#endif

private:
	inline void ZeroGhostedPlayerLocArray(void);
	inline void ShowBeacon(int clientIndex, const Vector &pos);
	inline void HideBeacon(int clientIndex);
	inline void SetShowEnemies(bool enabled);

#ifdef CLIENT_DLL
	inline void ShowEnemies(void);
	inline void HideEnemies(void);
	inline void Debug_ShowPos(const Vector &pos);
	inline void PlayGhostSound(float volume = 1.0f);
	inline void StopGhostSound(void);
	inline void HandleGhostEquip(void);
#else
	void UpdateNetworkedEnemyLocations(void);
#endif

private:

#ifdef CLIENT_DLL
	bool m_bShouldShowEnemies;
	bool m_bHavePlayedGhostEquipSound;
	bool m_bHaveHolsteredTheGhost;

	Vector m_rvPlayerPositions[MAX_PLAYERS];

	// NEO TODO (Rain): It's probably better to just have one beacon,
	// and call it numplayers times on each update between hud buffer swaps
	CNEOHud_GhostBeacon *m_pGhostBeacons[MAX_PLAYERS];
#else
	CNetworkVar(bool, m_bShouldShowEnemies);
	CNetworkArray(Vector, m_rvPlayerPositions, MAX_PLAYERS);
#endif

	CWeaponGhost(const CWeaponGhost &);
};

#endif // NEO_WEAPON_GHOST_H