#ifndef NEO_WEAPON_GHOST_H
#define NEO_WEAPON_GHOST_H
#ifdef _WIN32
#pragma once
#endif

#ifdef CLIENT_DLL
#include "c_neo_player.h"
#include <vgui_controls/Panel.h>
#include <vgui_controls/ImagePanel.h>
#else
#include "neo_player.h"
#endif

#include "weapon_neobasecombatweapon.h"

#ifdef CLIENT_DLL
#define CWeaponGhost C_WeaponGhost
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
	virtual void Spawn(void);

#ifdef GAME_DLL
	DECLARE_ACTTABLE();
#endif

private:
	inline void ZeroGhostedPlayerLocArray(void);
	inline void ShowBeacon(int panelIndex, const Vector &pos);
	inline void HideBeacon(int panelIndex);

#ifdef CLIENT_DLL
	void ShowEnemies(void);
	void Debug_ShowPos(const Vector &pos);
#else
	void SetShowEnemies(bool enabled);
	void UpdateNetworkedEnemyLocations(void);
#endif

private:

#ifdef CLIENT_DLL
	bool m_bShouldShowEnemies;

	Vector m_rvPlayerPositions[MAX_PLAYERS];

	vgui::Panel *rootGhostPanel;
#else
	CNetworkVar(bool, m_bShouldShowEnemies);
	CNetworkArray(Vector, m_rvPlayerPositions, MAX_PLAYERS);
#endif

	CWeaponGhost(const CWeaponGhost &);
};

#endif // NEO_WEAPON_GHOST_H