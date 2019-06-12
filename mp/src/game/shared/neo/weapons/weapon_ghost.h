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
#endif

class CWeaponGhost : public CNEOBaseCombatWeapon
{
public:
	DECLARE_CLASS(CWeaponGhost, CNEOBaseCombatWeapon);

	CWeaponGhost(void);

	DECLARE_NETWORKCLASS();
	//DECLARE_PREDICTABLE();

	void ItemPreFrame(void);
	void PrimaryAttack(void);
	
	void SetShowEnemies(bool enabled);

	virtual void ItemHolsterFrame(void);



#ifdef GAME_DLL
	DECLARE_ACTTABLE();
#endif

private:
	CNetworkVar(bool, m_bShouldShowEnemies);

	void ShowEnemies(void);

private:
	CWeaponGhost(const CWeaponGhost &);
};

#endif // NEO_WEAPON_GHOST_H