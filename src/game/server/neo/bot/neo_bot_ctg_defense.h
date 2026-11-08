#pragma once

class CNEOBot;
class CNEO_Player;
class CNavArea;
class CKnownEntity;

// A covered area where a defender waits for an enemy on its way to our carrier
struct WatchArea_t
{
	CNavArea *pWatch = nullptr;
	CNavArea *pWatched = nullptr;
	float flMyTravel = 0.0f;
	float flEnemyEta = 0.0f;
	float flReportAge = 0.0f;
};

// Route watch for the teammates of a ghost carrier: predict the known enemy's route to the carrier
// from its last known area and a naive run speed, and wait beside that route where we arrive first
namespace NEOBotDefense
{
	// A watch area I reach before the enemy reaches the route area it watches.
	// pHeld: the watch I hold or walk to, kept under a looser margin; the same call decides entering and keeping it.
	// ppszFailReason: why no watch was found, for logging.
	bool FindWatchArea( CNEOBot *me, const CKnownEntity &known, CNEO_Player *pCarrier,
		const WatchArea_t *pHeld, WatchArea_t &out, const char **ppszFailReason = nullptr );

	// Team report union and sticky crossfire plan; false keeps the caller's ordinary hunt behavior.
	bool GetAssignment( CNEOBot *me, CNEO_Player *pCarrier, CNEO_Player *&pEnemy, WatchArea_t &out );

	// Forget the cached routes, e.g. on a map change where nav areas are rebuilt
	void Reset();
}
