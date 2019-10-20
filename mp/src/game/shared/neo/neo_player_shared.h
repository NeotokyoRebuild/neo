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

#define SUPER_JMP_COST 45.0f

// Original NT allows chaining superjumps up ramps,
// so leaving this zeroed for enabling movement tricks.
#define SUPER_JMP_DELAY_BETWEEN_JUMPS 0

// NEO Activities
#define ACT_NEO_ATTACK ACT_RANGE_ATTACK1
#define ACT_NEO_RELOAD ACT_RELOAD
#define ACT_NEO_IDLE_STAND ACT_IDLE
#define ACT_NEO_IDLE_CROUCH ACT_CROUCHIDLE
#define ACT_NEO_MOVE_STAND ACT_RUN
#define ACT_NEO_MOVE_CROUCH ACT_CROUCH
#define ACT_NEO_DIE ACT_DIESIMPLE
#define ACT_NEO_HOVER ACT_HOVER
#define ACT_NEO_JUMP ACT_HOP
#define ACT_NEO_SWIM ACT_SWIM

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

#if defined(CLIENT_DLL) && !defined(CNEOBaseCombatWeapon)
#define CNEOBaseCombatWeapon C_NEOBaseCombatWeapon
#endif

class CNEO_Player;
class CNEOBaseCombatWeapon;

extern bool IsThereRoomForLeanSlide(CNEO_Player *player,
	const Vector &targetViewOffset, bool &outStartInSolid);

// Is the player allowed to aim zoom with a weapon of this type?
bool IsAllowedToZoom(CNEOBaseCombatWeapon *pWep);

extern ConVar cl_autoreload_when_empty;
extern ConVar neo_recon_superjump_intensity;

inline const char *GetRankName(int xp)
{
	if (xp < 0)
	{
		return "Rankless Dog";
	}
	else if (xp < 4)
	{
		return "Private";
	}
	else if (xp < 10)
	{
		return "Corporal";
	}
	else if (xp < 20)
	{
		return "Sergeant";
	}
	else if (xp < 75)
	{
		return "Lieutenant";
	}
	else // :-o
	{
		return "Puppet Master";
	}
}

#endif // NEO_PLAYER_SHARED_H