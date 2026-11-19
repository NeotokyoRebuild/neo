#ifndef NEO_SMOKE_VISUAL_STATE_H
#define NEO_SMOKE_VISUAL_STATE_H
#ifdef _WIN32
#pragma once
#endif

#include "mathlib/vector.h"

class C_BaseEntity;

// Snapshot of how big a smoke grenade's cloud currently looks, so other code
// (e.g. the cap zone ring's depth proxies) can follow it as it expands and
// dissipates. C_ParticleSmokeGrenade has no header of its own.
struct NeoSmokeGrenadeVisualState_t
{
	Vector center;
	// Radius of the expanding smoke sphere right now (0 -> ~330 over the first
	// SMOKESPHERE_EXPAND_TIME seconds, then constant).
	float expandRadius;
	// Pure time fade: 1 until the fade window starts, 0 once it has ended.
	float fade;
};

// Returns false if pEnt isn't a smoke grenade or its volume isn't formed yet.
bool NeoGetSmokeGrenadeVisualState(C_BaseEntity *pEnt, NeoSmokeGrenadeVisualState_t &outState);

#endif // NEO_SMOKE_VISUAL_STATE_H
