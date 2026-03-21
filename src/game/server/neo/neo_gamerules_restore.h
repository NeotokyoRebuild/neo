#pragma once

// Both Jinrai and NSF scores and rounds won are set together
enum NextRoundGameruleRestoreFlag_
{
	NEXT_ROUND_GAMERULE_RESTORE_FLAG_NIL = 0,
	NEXT_ROUND_GAMERULE_RESTORE_FLAG_SCORES = 1 << 0,
	NEXT_ROUND_GAMERULE_RESTORE_FLAG_ROUND_NUMBER = 1 << 1,
	NEXT_ROUND_GAMERULE_RESTORE_FLAG_ROUNDSWONS = 1 << 2,
};
typedef int NextRoundGameruleRestoreFlags;

enum NextRoundPlayerRestoreFlag_
{
	NEXT_ROUND_PLAYER_RESTORE_FLAG_NIL = 0,
	NEXT_ROUND_PLAYER_RESTORE_FLAG_XP = 1 << 0,
	NEXT_ROUND_PLAYER_RESTORE_FLAG_DEATH = 1 << 1,
};
typedef int NextRoundPlayerRestoreFlags;

// Backup current match state to disk
void MatchSessionBackup();

