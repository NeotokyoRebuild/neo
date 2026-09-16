#include "neo_gamerules_restore.h"

#include "cbase.h"
#include "convar.h"
#include <annotations.h>
#include "KeyValues.h"
#include "filesystem.h"
#include "neo_gamerules.h"

#include <ctime>

#define GIVEXP_SESSION_RESTORE_FNAME "match_session_restore_point.txt"
#define GIVEXP_SESSION_RESTORE_KV_ROOT "neo_match_session"

static ConVar sv_neo_restore_xp_death_any_round("sv_neo_restore_xp_death_any_round", "0",
		FCVAR_REPLICATED | FCVAR_CHEAT | FCVAR_DONTRECORD,
		"Permit giving XP at any round rather than restricting to only readyup stage",
		true, 0.0f, true, 1.0f);

static ConVar sv_neo_restore_session_allow_name_match("sv_neo_restore_session_allow_name_match", "0",
		FCVAR_REPLICATED | FCVAR_DONTRECORD,
		"Permit restoring session with matching players name",
		true, 0.0f, true, 1.0f);

extern ConVar sv_neo_comp;

// NOTE: Spawn restore is snapshot only, session restore cannot
// from a crashed session cannot reliably restore spawn from
// handle.

struct MatchSnapshotPlayer
{
	CSteamID steamID;
	int iXP;
	int iDeaths;
	int iSpawnHdlEntryIndex;
	int iSpawnHdlSerialNumber;
};

struct MatchSnapshot
{
	int iScoreJinrai;
	int iScoreNSF;
	int iRoundsWonJinrai;
	int iRoundsWonNSF;
	int iGhostSpawnIdx;
	MatchSnapshotPlayer players[MAX_PLAYERS_ARRAY_SAFE];
	int iPlayersSize;
};

static constexpr const int SNAPSHOTS_TOTAL = 64;
static MatchSnapshot gSnapshots[SNAPSHOTS_TOTAL];
static int giSnapshotsMax = 0;

void ClearSnapshots()
{
	V_memset(gSnapshots, 0, sizeof(gSnapshots));
	giSnapshotsMax = 0;
}

static void PrintToMsgAndTalk(PRINTF_FORMAT_STRING const char *pFormat, ...)
{
	static const constexpr int MAX_LEN_IN_CHARS = 128;
	char szDest[MAX_LEN_IN_CHARS] = {};

	va_list params;
	va_start(params, pFormat);
	V_vsnprintf(szDest, MAX_LEN_IN_CHARS, pFormat, params);
	va_end(params);

	Msg("%s\n", szDest);
	UTIL_ClientPrintAll(HUD_PRINTTALK, szDest);
}

static void ErrorToWarningAndTalk(PRINTF_FORMAT_STRING const char *pFormat, ...)
{
	static const constexpr int MAX_LEN_IN_CHARS = 128;
	char szDest[MAX_LEN_IN_CHARS] = {};
	char szDestAll[MAX_LEN_IN_CHARS + 16] = {};

	va_list params;
	va_start(params, pFormat);
	V_vsnprintf(szDest, MAX_LEN_IN_CHARS, pFormat, params);
	va_end(params);

	V_sprintf_safe(szDestAll, "[ERROR]: %s", szDest);

	Warning("%s\n", szDestAll);
	UTIL_ClientPrintAll(HUD_PRINTTALK, szDestAll);
}

static void RestoreSetRoundNumber(const int iRoundNumber, const char *pszFuncName)
{
	if (iRoundNumber < 0)
	{
		ErrorToWarningAndTalk("%s: error: Cannot have negative round number", pszFuncName);
		return;
	}

	NEORules()->SetRoundNumber(iRoundNumber);
	if (NEORules()->InReadyUpState() || NEORules()->IsRoundOn())
	{
		NEORules()->m_iNextRestore.iRoundNumber = iRoundNumber;
		NEORules()->m_iNextRestore.flags |= NEXT_ROUND_GAMERULE_RESTORE_FLAG_ROUND_NUMBER;
	}
	else
	{
		NEORules()->m_iNextRestore.flags &= ~(NEXT_ROUND_GAMERULE_RESTORE_FLAG_ROUND_NUMBER);
	}

	PrintToMsgAndTalk("%s: Round number set %d", pszFuncName, iRoundNumber);
}

static void RestoreSetRoundsWon(const int iRoundsWonJinrai, const int iRoundsWonNSF, const char *pszFuncName)
{
	if (iRoundsWonJinrai < 0 || iRoundsWonNSF < 0)
	{
		ErrorToWarningAndTalk("%s: error: Cannot have negative rounds won", pszFuncName);
		return;
	}

	GetGlobalTeam(TEAM_JINRAI)->SetRoundsWon(iRoundsWonJinrai);
	GetGlobalTeam(TEAM_NSF)->SetRoundsWon(iRoundsWonNSF);
	if (NEORules()->InReadyUpState() || NEORules()->IsRoundOn())
	{
		NEORules()->m_iNextRestore.iRoundsWonJinrai = iRoundsWonJinrai;
		NEORules()->m_iNextRestore.iRoundsWonNSF = iRoundsWonNSF;
		NEORules()->m_iNextRestore.flags |= NEXT_ROUND_GAMERULE_RESTORE_FLAG_ROUNDSWONS;
	}
	else
	{
		NEORules()->m_iNextRestore.flags &= ~(NEXT_ROUND_GAMERULE_RESTORE_FLAG_ROUNDSWONS);
	}

	PrintToMsgAndTalk("%s: Rounds won set: Jinrai %d, NSF %d", pszFuncName, iRoundsWonJinrai, iRoundsWonNSF);
}

static void RestoreSetGhostSpawnIdx(const int iGhostSpawnIdx, const char *pszFuncName)
{
	if (iGhostSpawnIdx < 0)
	{
		ErrorToWarningAndTalk("%s: error: Cannot have negative spawn index", pszFuncName);
		return;
	}

	if (NEORules()->InReadyUpState() || NEORules()->IsRoundOn())
	{
		NEORules()->m_iNextRestore.iGhostSpawnIdx = iGhostSpawnIdx;
		NEORules()->m_iNextRestore.flags |= NEXT_ROUND_GAMERULE_RESTORE_FLAG_GHOST;
	}
	else
	{
		NEORules()->m_iNextRestore.flags &= ~(NEXT_ROUND_GAMERULE_RESTORE_FLAG_GHOST);
	}

	PrintToMsgAndTalk("%s: Ghost spawn index set: %d", pszFuncName, iGhostSpawnIdx);
}

static void RestoreSetScore(const int iScoreJinrai, const int iScoreNSF, const char *pszFuncName)
{
	GetGlobalTeam(TEAM_JINRAI)->SetScore(iScoreJinrai);
	GetGlobalTeam(TEAM_NSF)->SetScore(iScoreNSF);
	if (NEORules()->InReadyUpState() || NEORules()->IsRoundOn())
	{
		NEORules()->m_iNextRestore.iScoreJinrai = iScoreJinrai;
		NEORules()->m_iNextRestore.iScoreNSF = iScoreNSF;
		NEORules()->m_iNextRestore.flags |= NEXT_ROUND_GAMERULE_RESTORE_FLAG_SCORES;
	}
	else
	{
		NEORules()->m_iNextRestore.flags &= ~(NEXT_ROUND_GAMERULE_RESTORE_FLAG_SCORES);
	}

	PrintToMsgAndTalk("%s: Score set Jinrai %d, NSF %d", pszFuncName, iScoreJinrai, iScoreNSF);
}

// NEO NOTE (nullsystem): If iDeaths < 0, it won't be set
static void RestoreSetXPDeath(CNEO_Player *pNeoPlayer, const int iXP, const int iDeaths,
		const char *pszFuncName)
{
	if (false == NEORules()->IsRoundOn())
	{
		pNeoPlayer->m_iXP.Set(iXP);
		if (iDeaths >= 0)
		{
			pNeoPlayer->ResetDeathCount();
			pNeoPlayer->IncrementDeathCount(iDeaths);
		}
	}

	if (NEORules()->InReadyUpState() || NEORules()->IsRoundOn())
	{
		pNeoPlayer->m_iNextRestore.iXP = iXP;
		pNeoPlayer->m_iNextRestore.flags |= NEXT_ROUND_PLAYER_RESTORE_FLAG_XP;
		if (iDeaths >= 0)
		{
			pNeoPlayer->m_iNextRestore.iDeaths = iDeaths;
			pNeoPlayer->m_iNextRestore.flags |= NEXT_ROUND_PLAYER_RESTORE_FLAG_DEATH;
		}
		else
		{
			pNeoPlayer->m_iNextRestore.flags &= ~(NEXT_ROUND_PLAYER_RESTORE_FLAG_DEATH);
		}
	}
	else
	{
		pNeoPlayer->m_iNextRestore.flags &=
				~(NEXT_ROUND_PLAYER_RESTORE_FLAG_XP | NEXT_ROUND_PLAYER_RESTORE_FLAG_DEATH);
	}

	if (iDeaths >= 0)
	{
		PrintToMsgAndTalk("%s: Given %d XP and %d deaths to %s%s", pszFuncName, iXP, iDeaths,
				pNeoPlayer->GetNeoPlayerName(), NEORules()->IsRoundOn() ? " next round" : "");
	}
	else
	{
		PrintToMsgAndTalk("%s: Given %d XP to %s%s", pszFuncName, iXP,
				pNeoPlayer->GetNeoPlayerName(), NEORules()->IsRoundOn() ? " next round" : "");
	}
}

static void RestoreSetSpawn(CNEO_Player *pNeoPlayer,
		const int iSpawnHdlEntryIndex,
		const int iSpawnHdlSerialNumber,
		const char *pszFuncName)
{
	if (NEORules()->InReadyUpState() || NEORules()->IsRoundOn())
	{
		pNeoPlayer->m_iNextRestore.iSpawnHdlEntryIndex = iSpawnHdlEntryIndex;
		pNeoPlayer->m_iNextRestore.iSpawnHdlSerialNumber = iSpawnHdlSerialNumber;
		pNeoPlayer->m_iNextRestore.flags |= NEXT_ROUND_PLAYER_RESTORE_FLAG_SPAWN;
		PrintToMsgAndTalk("%s: Set spawn %d %d for %s", pszFuncName,
				iSpawnHdlEntryIndex, iSpawnHdlSerialNumber,
				pNeoPlayer->GetNeoPlayerName());
	}
	else
	{
		// Never going to make sense for this scenario
		pNeoPlayer->m_iNextRestore.flags &= ~(NEXT_ROUND_PLAYER_RESTORE_FLAG_SPAWN);
		ErrorToWarningAndTalk("%s: Cannot set spawn for %s", pszFuncName,
				pNeoPlayer->GetNeoPlayerName());
	}
}

CON_COMMAND(sv_neo_restore_round_snapshot, "Restore the current match's recorded round snapshot")
{
	if (2 != args.ArgC())
	{
		ErrorToWarningAndTalk("Usage: %s <round number>", __func__);
		return;
	}

	const int iRoundNumber = V_atoi(args[1]);
	if (iRoundNumber < 1 || iRoundNumber > giSnapshotsMax)
	{
		if (giSnapshotsMax == 0)
		{
			ErrorToWarningAndTalk("%s: There's no snapshots", __func__);
		}
		else
		{
			ErrorToWarningAndTalk("%s: Round number must be within 1 to %d", __func__, giSnapshotsMax);
		}
		return;
	}

	const MatchSnapshot *pSnapshot = &gSnapshots[iRoundNumber];
	RestoreSetScore(pSnapshot->iScoreJinrai, pSnapshot->iScoreNSF, __func__);
	RestoreSetRoundNumber(iRoundNumber, __func__);
	RestoreSetRoundsWon(pSnapshot->iRoundsWonJinrai, pSnapshot->iRoundsWonNSF, __func__);
	RestoreSetGhostSpawnIdx(pSnapshot->iGhostSpawnIdx, __func__);
	for (int idxClient = 1; idxClient <= gpGlobals->maxClients; ++idxClient)
	{
		auto pNeoPlayer = static_cast<CNEO_Player *>(UTIL_PlayerByIndex(idxClient));
		if (!pNeoPlayer)
		{
			continue;
		}

		const CSteamID playerSteamID = GetSteamIDForPlayerIndex(pNeoPlayer->entindex());
		if (!playerSteamID.IsValid())
		{
			continue;
		}

		for (int idxSnPlayer = 0; idxSnPlayer < pSnapshot->iPlayersSize; ++idxSnPlayer)
		{
			const MatchSnapshotPlayer *pSnPlayer = &pSnapshot->players[idxSnPlayer];
			if (playerSteamID == pSnPlayer->steamID)
			{
				RestoreSetXPDeath(pNeoPlayer, pSnPlayer->iXP, pSnPlayer->iDeaths, __func__);
				RestoreSetSpawn(pNeoPlayer, pSnPlayer->iSpawnHdlEntryIndex, pSnPlayer->iSpawnHdlSerialNumber, __func__);
				break;
			}
		}
	}

	// Unlike sv_neo_restore_session, this immediately resets to the restoring round
	NEORules()->StartNextRound();
	char szCenterPrint[64];
	V_sprintf_safe(szCenterPrint, "- MATCH RESTORED TO ROUND %d SNAPSHOT -\n", iRoundNumber);
	UTIL_CenterPrintAll(szCenterPrint);
}

CON_COMMAND(sv_neo_restore_session, "Restore the previous session")
{
	if (false == sv_neo_restore_xp_death_any_round.GetBool() && false == NEORules()->InReadyUpState())
	{
		ErrorToWarningAndTalk("%s: error: Cannot set restore session if not idle and in a ready up lobby", __func__);
		ErrorToWarningAndTalk("Set \"sv_neo_restore_xp_death_any_round 1\" if need to set anytime.", __func__);
		return;
	}

	KeyValues *kv = new KeyValues(GIVEXP_SESSION_RESTORE_KV_ROOT);
	if (false == kv->LoadFromFile(g_pFullFileSystem, "scripts/" GIVEXP_SESSION_RESTORE_FNAME))
	{
		ErrorToWarningAndTalk("%s: error: No restore session file found", __func__);
		kv->deleteThis();
		return;
	}

	if (KeyValues *kvInfo = kv->FindKey("info"))
	{
		const char *pszInfileMap = kvInfo->GetString("map");
		const char *pszCurMap = gpGlobals->mapname.ToCStr();
		if (0 != V_strcmp(pszInfileMap, pszCurMap))
		{
			ErrorToWarningAndTalk("%s: Will not restore since map session differs: in file %s vs current %s",
					__func__, pszInfileMap, pszCurMap);
			kv->deleteThis();
			return;
		}
	}

	if (KeyValues *kvScore = kv->FindKey("score"))
	{
		RestoreSetScore(kvScore->GetInt("jinrai"), kvScore->GetInt("nsf"), __func__);
	}

	if (KeyValues *kvRounds = kv->FindKey("rounds"))
	{
		RestoreSetRoundNumber(kvRounds->GetInt("number"), __func__);
		RestoreSetRoundsWon(kvRounds->GetInt("jinrai"), kvRounds->GetInt("nsf"), __func__);
		RestoreSetGhostSpawnIdx(kvRounds->GetInt("ghost"), __func__);
	}

	if (KeyValues *kvPlayersList = kv->FindKey("players_list"))
	{
		for (KeyValues *kvPlayer = kvPlayersList->GetFirstTrueSubKey();
				kvPlayer;
				kvPlayer = kvPlayer->GetNextTrueSubKey())
		{
			if (0 != V_strcmp(kvPlayer->GetName(), "player"))
			{
				continue;
			}

			CNEO_Player *pNeoPlayerUpdate = nullptr;

			const char *pszSteamID3 = kvPlayer->GetString("steamid3");
			const char *pszName = kvPlayer->GetString("name");

			CSteamID fileSteamID;
			if (fileSteamID.SetFromStringStrict(pszSteamID3, k_EUniversePublic)
					&& fileSteamID.IsValid())
			{
				for (int i = 1; i <= gpGlobals->maxClients; i++)
				{
					auto pNeoPlayer = static_cast<CNEO_Player *>(UTIL_PlayerByIndex(i));
					if (pNeoPlayer && false == pNeoPlayer->IsBot())
					{
						const CSteamID playerSteamID = GetSteamIDForPlayerIndex(pNeoPlayer->entindex());
						if (playerSteamID.IsValid() && playerSteamID == fileSteamID)
						{
							pNeoPlayerUpdate = pNeoPlayer;
							break;
						}
					}
				}
			}

			if (sv_neo_restore_session_allow_name_match.GetBool() && nullptr == pNeoPlayerUpdate)
			{
				for (int i = 1; i <= gpGlobals->maxClients; i++)
				{
					auto pNeoPlayer = static_cast<CNEO_Player *>(UTIL_PlayerByIndex(i));
					if (pNeoPlayer && 0 == V_strcmp(pNeoPlayer->GetNeoPlayerName(), pszName))
					{
						pNeoPlayerUpdate = pNeoPlayer;
						break;
					}
				}
			}

			if (pNeoPlayerUpdate)
			{
				// Only readyup state can set full session restore
				const int iXP = kvPlayer->GetInt("xp");
				const int iDeaths = kvPlayer->GetInt("deaths");
				RestoreSetXPDeath(pNeoPlayerUpdate, iXP, iDeaths, __func__);
			}
			else
			{
				ErrorToWarningAndTalk("%s: Player steamID: %s, name: %s skipped: not found",
						__func__, pszSteamID3, pszName);
			}
		}
	}

	kv->deleteThis();
}

void MatchSessionBackup()
{
	// Match session backup only used for competitive mode
	if (false == sv_neo_comp.GetBool())
	{
		return;
	}

	char szDateTime[32] = {};
	KeyValues *kv = new KeyValues(GIVEXP_SESSION_RESTORE_KV_ROOT);

	{
		KeyValues *kvInfo = new KeyValues("info");
		kvInfo->SetString("map", gpGlobals->mapname.ToCStr());
		{
			// Time is not de-serialized on read, mainly descriptive
			const time_t timeVal = time(nullptr);
			std::tm tmd{};
			Plat_localtime(&timeVal, &tmd);
			V_sprintf_safe(szDateTime, "%04d-%02d-%02d %02d:%02d:%02d",
					tmd.tm_year + 1900, tmd.tm_mon + 1, tmd.tm_mday,
					tmd.tm_hour, tmd.tm_min, tmd.tm_sec);
			kvInfo->SetString("datetime", szDateTime);
		}
		kv->AddSubKey(kvInfo);
	}

	// NEO NOTE (nullsystem): Just fall back to unused index 0 if rounds somewhat
	// manages to get to 64+ or some invalid index for some reason.
	// From then on it won't crash the snapshotter as it just starts writing to
	// snapshot index-0, but round numbers are indexed-1 so they won't get used anyway.
	const int iRoundNumber = NEORules()->roundNumber();
	MatchSnapshot *pSnapshot =
			(iRoundNumber < 0 || iRoundNumber >= SNAPSHOTS_TOTAL) ?
					&gSnapshots[0] : &gSnapshots[iRoundNumber];
	giSnapshotsMax = Max(giSnapshotsMax, iRoundNumber);

	{
		pSnapshot->iScoreJinrai = GetGlobalTeam(TEAM_JINRAI)->GetScore();
		pSnapshot->iScoreNSF = GetGlobalTeam(TEAM_NSF)->GetScore();

		KeyValues *kvScore = new KeyValues("score");
		kvScore->SetInt("jinrai", pSnapshot->iScoreJinrai);
		kvScore->SetInt("nsf", pSnapshot->iScoreNSF);
		kv->AddSubKey(kvScore);
	}

	{
		pSnapshot->iRoundsWonJinrai = GetGlobalTeam(TEAM_JINRAI)->GetRoundsWon();
		pSnapshot->iRoundsWonNSF = GetGlobalTeam(TEAM_NSF)->GetRoundsWon();
		pSnapshot->iGhostSpawnIdx = NEORules()->m_iGhostSpawnIdx;

		KeyValues *kvRounds = new KeyValues("rounds");
		kvRounds->SetInt("number", iRoundNumber);
		kvRounds->SetInt("jinrai", pSnapshot->iRoundsWonJinrai);
		kvRounds->SetInt("nsf", pSnapshot->iRoundsWonNSF);
		kvRounds->SetInt("ghost", pSnapshot->iGhostSpawnIdx);
		kv->AddSubKey(kvRounds);
	}

	{
		pSnapshot->iPlayersSize = 0;

		KeyValues *kvPlayersList = new KeyValues("players_list");
		for (int i = 1; i <= gpGlobals->maxClients && pSnapshot->iPlayersSize < MAX_PLAYERS_ARRAY_SAFE; i++)
		{
			auto pNeoPlayer = static_cast<CNEO_Player *>(UTIL_PlayerByIndex(i));
			if (pNeoPlayer
					&& (pNeoPlayer->GetTeamNumber() == TEAM_JINRAI
						|| pNeoPlayer->GetTeamNumber() == TEAM_NSF))
			{
				MatchSnapshotPlayer *pSnPlayer = &pSnapshot->players[pSnapshot->iPlayersSize++];
				pSnPlayer->steamID = GetSteamIDForPlayerIndex(pNeoPlayer->entindex());
				pSnPlayer->iXP = pNeoPlayer->m_iXP.Get();
				pSnPlayer->iDeaths = pNeoPlayer->DeathCount();
				pSnPlayer->iSpawnHdlEntryIndex = pNeoPlayer->m_iSpawnHdlEntryIndex;
				pSnPlayer->iSpawnHdlSerialNumber = pNeoPlayer->m_iSpawnHdlSerialNumber;

				KeyValues *kvPlayer = new KeyValues("player");
				kvPlayer->SetInt("xp", pSnPlayer->iXP);
				kvPlayer->SetInt("deaths", pSnPlayer->iDeaths);
				// team - Unused on de-serialization as steamid3 is enough, but have descriptive purpose
				kvPlayer->SetString("team", (pNeoPlayer->GetTeamNumber() == TEAM_JINRAI) ? "j" : "n");
				// name - Always set regardless of sv_neo_restore_session_name_match, have a descriptive
				//        purpose when looking inside the file
				kvPlayer->SetString("name", pNeoPlayer->GetNeoPlayerName());
				if (pSnPlayer->steamID.IsValid())
				{
					kvPlayer->SetString("steamid3", pSnPlayer->steamID.Render());
				}
				kvPlayersList->AddSubKey(kvPlayer);
			}
		}
		kv->AddSubKey(kvPlayersList);
	}

	if (kv->SaveToFile(g_pFullFileSystem, "scripts/" GIVEXP_SESSION_RESTORE_FNAME))
	{
		// Would be spammy in text chat if going as usual, so kept as Msg here
		Msg("Session backed up at %s\n", szDateTime);
	}
	else
	{
		ErrorToWarningAndTalk("Session backup failed to save at %s!", szDateTime);
	}

	kv->deleteThis();
}

CON_COMMAND(sv_neo_restore_round_number, "Set the next round number")
{
	if (2 != args.ArgC())
	{
		ErrorToWarningAndTalk("Usage: %s <round number>", __func__);
		return;
	}

	const int iRoundNumber = V_atoi(args[1]);
	RestoreSetRoundNumber(iRoundNumber, __func__);
}

CON_COMMAND(sv_neo_restore_rounds_won, "Set the rounds won")
{
	if (3 != args.ArgC())
	{
		ErrorToWarningAndTalk("Usage: %s <jinrai won> <nsf won>", __func__);
		return;
	}

	const int iRoundsWonJinrai = V_atoi(args[1]);
	const int iRoundsWonNSF = V_atoi(args[2]);
	RestoreSetRoundsWon(iRoundsWonJinrai, iRoundsWonNSF, __func__);
}

CON_COMMAND(sv_neo_restore_team_scores, "Set the scores for each team (not used in CTG, use sv_neo_restore_rounds_won)")
{
	if (3 != args.ArgC())
	{
		ErrorToWarningAndTalk("Usage: %s <jinrai score> <nsf score>", __func__);
		return;
	}

	const int iJinraiScore = V_atoi(args[1]);
	const int iNSFScore = V_atoi(args[2]);
	RestoreSetScore(iJinraiScore, iNSFScore, __func__);
}

CON_COMMAND(sv_neo_restore_xp, "Give a player XP (and death) count")
{
	static constexpr const char SZ_COMMON_USAGE_PF[] =
			"Usage: %s <player name|steamid3> <xp> <deaths (optional)>\n"
			"steamid3 = uniqueid in \"status\" command, must be surrounded with quotes";

	if (!IN_BETWEEN_EQ(3, args.ArgC(), 4))
	{
		ErrorToWarningAndTalk(SZ_COMMON_USAGE_PF, __func__);
		return;
	}

	const char *pszNameFind = args[1];
	const int iXP = V_atoi(args[2]);
	const int iDeaths = (args.ArgC() == 4) ? V_atoi(args[3]) : -1;
	if (4 == args.ArgC() && (iDeaths < 0))
	{
		ErrorToWarningAndTalk("%s: error: Death count must be positive", __func__);
		ErrorToWarningAndTalk(SZ_COMMON_USAGE_PF, __func__);
		return;
	}

	if (false == sv_neo_restore_xp_death_any_round.GetBool() && false == NEORules()->InReadyUpState())
	{
		ErrorToWarningAndTalk("%s: error: Cannot set XPs if not idle and in a ready up lobby", __func__);
		ErrorToWarningAndTalk("Set \"sv_neo_restore_xp_death_any_round 1\" if need to set anytime.", __func__);
		return;
	}

	CSteamID steamIDFind;
	steamIDFind.SetFromStringStrict(pszNameFind, k_EUniversePublic);

	for (int i = 1; i <= gpGlobals->maxClients; ++i)
	{
		auto pNeoPlayer = static_cast<CNEO_Player*>(UTIL_PlayerByIndex(i));
		if (pNeoPlayer)
		{
			bool bFoundPlayer = false;
			if (steamIDFind.IsValid())
			{
				const CSteamID playerSteamID = GetSteamIDForPlayerIndex(pNeoPlayer->entindex());
				bFoundPlayer = (playerSteamID.IsValid() && playerSteamID == steamIDFind);
			}
			else
			{
				const char *pszNameCmp = pNeoPlayer->GetNeoPlayerName();
				bFoundPlayer = (0 == V_strcmp(pszNameCmp, pszNameFind));
			}

			if (bFoundPlayer)
			{
				RestoreSetXPDeath(pNeoPlayer, iXP, iDeaths, __func__);
				return;
			}
		}
	}

	ErrorToWarningAndTalk("%s: error: Cannot find player \"%s\"", __func__, pszNameFind);
}

