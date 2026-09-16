#pragma once

// Both Jinrai and NSF scores and rounds won are set together
enum NextRoundGameruleRestoreFlag_
{
	NEXT_ROUND_GAMERULE_RESTORE_FLAG_NIL = 0,
	NEXT_ROUND_GAMERULE_RESTORE_FLAG_SCORES = 1 << 0,
	NEXT_ROUND_GAMERULE_RESTORE_FLAG_ROUND_NUMBER = 1 << 1,
	NEXT_ROUND_GAMERULE_RESTORE_FLAG_ROUNDSWONS = 1 << 2,
	NEXT_ROUND_GAMERULE_RESTORE_FLAG_GHOST = 1 << 3,
};
typedef int NextRoundGameruleRestoreFlags;

enum NextRoundPlayerRestoreFlag_
{
	NEXT_ROUND_PLAYER_RESTORE_FLAG_NIL = 0,
	NEXT_ROUND_PLAYER_RESTORE_FLAG_XP = 1 << 0,
	NEXT_ROUND_PLAYER_RESTORE_FLAG_DEATH = 1 << 1,
	NEXT_ROUND_PLAYER_RESTORE_FLAG_SPAWN = 1 << 2,
	NEXT_ROUND_PLAYER_RESTORE_FLAG_WEAPON = 1 << 3,
};
typedef int NextRoundPlayerRestoreFlags;

// Backup current match state to disk (for sv_neo_restore_session)
// and in memory (for sv_neo_restore_round_snapshot)
void MatchSessionBackup();
void ClearSnapshots();

