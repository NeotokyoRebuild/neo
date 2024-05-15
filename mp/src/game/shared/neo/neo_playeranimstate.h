// This animstate code is based off the SDK (CS:S) sdk_playeranimstate code.

#ifndef NEO_PLAYERANIMSTATE_H
#define NEO_PLAYERANIMSTATE_H
#ifdef _WIN32
#pragma once
#endif

#include "convar.h"
#include "iplayeranimstate.h"
#include "base_playeranimstate.h"

#define FIRESEQUENCE_LAYER		(AIMSEQUENCE_LAYER+NUM_AIMSEQUENCE_LAYERS)
#define RELOADSEQUENCE_LAYER	(FIRESEQUENCE_LAYER + 1)
#define GRENADESEQUENCE_LAYER	(RELOADSEQUENCE_LAYER + 1)
#define NUM_LAYERS_WANTED		(GRENADESEQUENCE_LAYER + 1)

class CBaseAnimatingOverlay;
class CBaseCombatWeapon;
class CNEO_Player;

enum PlayerAnimEvent_t : uint
{
	PLAYERANIMEVENT_FIRE_GUN_PRIMARY = 0,
	PLAYERANIMEVENT_FIRE_GUN_SECONDARY,
	PLAYERANIMEVENT_THROW_GRENADE,
	PLAYERANIMEVENT_JUMP,
	PLAYERANIMEVENT_RELOAD,

	PLAYERANIMEVENT_COUNT
};

class INEOPlayerAnimState : virtual public IPlayerAnimState
{
public:
	// This is called by both the client and the server in the same way to trigger events for
	// players firing, jumping, throwing grenades, etc.
	virtual void DoAnimationEvent(PlayerAnimEvent_t event, int nData = 0) = 0;

	// Returns true if we're playing the grenade prime or throw animation.
	virtual bool IsThrowingGrenade() = 0;
};

// This abstracts the differences between SDK players and hostages.
// NEO TODO (Rain): porting this over from the SDK code for now,
// in case it becomes useful for VIPs or something. Should be cleaned up later.
class INEOPlayerAnimStateHelpers
{
public:
	virtual CBaseCombatWeapon* NEOAnim_GetActiveWeapon() = 0;
	virtual bool NEOAnim_CanMove() = 0;
	virtual ~INEOPlayerAnimStateHelpers() {}
};

INEOPlayerAnimState *CreatePlayerAnimState(CBaseAnimatingOverlay *pEntity,
	INEOPlayerAnimStateHelpers *pHelpers, LegAnimType_t legAnimType, bool bUseAimSequences);

INEOPlayerAnimStateHelpers* CreateAnimStateHelpers(CNEO_Player* pPlayer);

// If this is set, then the game code needs to make sure to send player animation events
// to the local player if he's the one being watched.
extern ConVar cl_showanimstate;

#endif // NEO_PLAYERANIMSTATE_H
