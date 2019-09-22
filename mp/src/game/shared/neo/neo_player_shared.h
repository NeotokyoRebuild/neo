#ifndef NEO_PLAYER_SHARED_H
#define NEO_PLAYER_SHARED_H
#ifdef _WIN32
#pragma once
#endif

#include "neo_predicted_viewmodel.h"

#define NEO_WEAPON_MELEE_SLOT 0
#define NEO_WEAPON_SECONDARY_SLOT 1
#define NEO_WEAPON_PRIMARY_SLOT 2
#define NEO_WEAPON_EXPLOSIVE_SLOT 3

// All of these should be able to stack create even slower speeds (at least in original NT)
#define NEO_SPRINT_MODIFIER 1.6
#define NEO_SLOW_MODIFIER 0.75

#define NEO_BASE_NORM_SPEED 136
#define NEO_BASE_SPRINT_SPEED (NEO_BASE_NORM_SPEED * NEO_SPRINT_MODIFIER)
#define NEO_BASE_WALK_SPEED (NEO_BASE_NORM_SPEED * NEO_SLOW_MODIFIER)
#define NEO_BASE_CROUCH_SPEED (NEO_BASE_NORM_SPEED * NEO_SLOW_MODIFIER)

#define NEO_RECON_SPEED_MODIFIER 1.25
#define NEO_ASSAULT_SPEED_MODIFIER 1.0
#define NEO_SUPPORT_SPEED_MODIFIER 0.75

#define NEO_RECON_NORM_SPEED (NEO_BASE_NORM_SPEED * NEO_RECON_SPEED_MODIFIER)
#define NEO_RECON_SPRINT_SPEED (NEO_BASE_SPRINT_SPEED * NEO_RECON_SPEED_MODIFIER)
#define NEO_RECON_WALK_SPEED (NEO_BASE_WALK_SPEED * NEO_RECON_SPEED_MODIFIER)
#define NEO_RECON_CROUCH_SPEED (NEO_BASE_CROUCH_SPEED * NEO_RECON_SPEED_MODIFIER)

#define NEO_ASSAULT_NORM_SPEED (NEO_BASE_NORM_SPEED * NEO_ASSAULT_SPEED_MODIFIER)
#define NEO_ASSAULT_SPRINT_SPEED (NEO_BASE_SPRINT_SPEED * NEO_ASSAULT_SPEED_MODIFIER)
#define NEO_ASSAULT_WALK_SPEED (NEO_BASE_WALK_SPEED * NEO_ASSAULT_SPEED_MODIFIER)
#define NEO_ASSAULT_CROUCH_SPEED (NEO_BASE_CROUCH_SPEED * NEO_ASSAULT_SPEED_MODIFIER)

#define NEO_SUPPORT_NORM_SPEED (NEO_BASE_NORM_SPEED * NEO_SUPPORT_SPEED_MODIFIER)
#define NEO_SUPPORT_SPRINT_SPEED (NEO_BASE_SPRINT_SPEED * NEO_SUPPORT_SPEED_MODIFIER)
#define NEO_SUPPORT_WALK_SPEED (NEO_BASE_WALK_SPEED * NEO_SUPPORT_SPEED_MODIFIER)
#define NEO_SUPPORT_CROUCH_SPEED (NEO_BASE_CROUCH_SPEED * NEO_SUPPORT_SPEED_MODIFIER)

enum NeoSkin {
	NEO_SKIN_FIRST = 0,
	NEO_SKIN_SECOND,
	NEO_SKIN_THIRD,

	NEO_SKIN_ENUM_COUNT
};

enum NeoClass {
	NEO_CLASS_RECON = 0,
	NEO_CLASS_ASSAULT,
	NEO_CLASS_SUPPORT,

	// NOTENOTE: VIP *must* be last, because we are
	// using array offsets for recon/assault/support
	NEO_CLASS_VIP,

	NEO_CLASS_ENUM_COUNT
};

class CNEO_Player;

extern bool IsThereRoomForLeanSlide(CNEO_Player *player,
	const Vector &targetViewOffset, bool &outStartInSolid);

// Is the player allowed to aim zoom with a weapon of this type?
inline bool IsAllowedToZoom(CBasePlayer *player, CBaseCombatWeapon *pWep)
{
	if (!pWep)
	{
		return false;
	}

	// NEO TODO (Rain): this list will probably eventually become longer
	// than forbidden list; swap logic?
	const char *allowedAimZoom[] = {
		"weapon_aa13",
		"weapon_kyla",
		"weapon_milso",
		"weapon_tachi",
		"weapon_zr68s",
	};

	CBaseCombatWeapon *pTest = NULL;
	for (int i = 0; i < ARRAYSIZE(allowedAimZoom); i++)
	{
		pTest = player->Weapon_OwnsThisType(allowedAimZoom[i]);
		if (pWep == pTest)
		{
			return true;
		}
	}

	return false;
}

extern ConVar cl_autoreload_when_empty;
extern ConVar neo_recon_superjump_intensity;

#endif // NEO_PLAYER_SHARED_H