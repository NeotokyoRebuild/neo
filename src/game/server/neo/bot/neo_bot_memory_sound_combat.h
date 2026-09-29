#pragma once

#include "mathlib/vector.h"

// Short-term memory of combat sounds, so bots can listen for fights they heard a moment ago
namespace NEOMemorySoundCombat
{
	// Combat sounds stay in the engine's sound list for only ~0.2 s,
	// so sample them every server tick instead of on each bot's 0.25 s search throttle
	void Update();

	// Forget every source, e.g. on a map change where curtime restarts
	void Reset();

	// Find the nearest recently heard fight: enemy combat sounds around the nearest source.
	// Only counts sources within their own audible radius of vEar, as CSoundEnt::Listen does.
	bool FindNearestFight( const Vector &vEar, int iMyTeam, int iMyEnt, Vector &vFight );
}
