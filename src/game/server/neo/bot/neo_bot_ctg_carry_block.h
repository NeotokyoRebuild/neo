#pragma once

#include "mathlib/vector.h"

class CNEOBot;
class CNEO_Player;
class CBasePlayer;

// Whether a bot ghost carrier's route to its capture zone is blocked: a known enemy can stand on
// that route no later than the carrier. The carrier's verdict is shared with its team, as a human
// carrier would say "holding, enemy ahead". The team holds the ghost while it is blocked and breaks
// for the zone when it clears.
namespace NEOBotCarryBlock
{
	bool IsNamedDead( CBasePlayer *pPlayer );

	// Re-evaluate the carrier team's verdict, at most once per evaluation interval.
	// bCapInReach: the carrier sees its capture zone or has a clean run to it.
	void UpdateCarryBlock( CNEOBot *pCarrier, bool bCapInReach, const Vector &vecGoal );

	// Every tick: drop a verdict whose carrier no longer carries the ghost
	void Update();

	// Forget every verdict, e.g. on a map change where curtime restarts
	void Reset();

	// The team has a capture zone and its carrier's route to it is not blocked
	bool HasClearPathToGoal( int iTeam );

	// A ghost team without a clear path to a zone holds the ghost and hunts around it instead of escorting;
	// the one test for every behavior that switches between the two, so they cannot disagree
	bool IsTeamHoldingGhost( int iTeam );

	// The known enemy that blocks the team's carrier, or null
	CNEO_Player *GetCarryBlocker( int iTeam );
}
