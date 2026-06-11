#include "neo_gamerules_restore.h"

#include "cbase.h"
#include "convar.h"
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

extern ConVar sv_neo_readyup_lobby;
extern ConVar sv_neo_comp;

static inline bool InReadyUpState()
{
	return (sv_neo_readyup_lobby.GetBool() && NEORules()->GetRoundStatus() == NeoRoundStatus::Idle);
}

static inline bool InLiveState()
{
	return (NEORules()->GetRoundStatus() == RoundLive || NEORules()->GetRoundStatus() == PostRound);
}

static void RestoreSetRoundNumber(const int iRoundNumber, const char *pszFuncName)
{
	if (iRoundNumber < 0)
	{
		Msg("%s: error: Cannot have negative round number\n", pszFuncName);
		return;
	}

	NEORules()->SetRoundNumber(iRoundNumber);
	if (InReadyUpState() || InLiveState())
	{
		NEORules()->m_iNextRestore.iRoundNumber = iRoundNumber;
		NEORules()->m_iNextRestore.flags |= NEXT_ROUND_GAMERULE_RESTORE_FLAG_ROUND_NUMBER;
	}
	else
	{
		NEORules()->m_iNextRestore.flags &= ~(NEXT_ROUND_GAMERULE_RESTORE_FLAG_ROUND_NUMBER);
	}

	Msg("%s: Round number set %d\n", pszFuncName, NEORules()->roundNumber());
}

static void RestoreSetRoundsWon(const int iRoundsWonJinrai, const int iRoundsWonNSF, const char *pszFuncName)
{
	if (iRoundsWonJinrai < 0 || iRoundsWonNSF < 0)
	{
		Msg("%s: error: Cannot have negative rounds won\n", pszFuncName);
		return;
	}

	GetGlobalTeam(TEAM_JINRAI)->SetRoundsWon(iRoundsWonJinrai);
	GetGlobalTeam(TEAM_NSF)->SetRoundsWon(iRoundsWonNSF);
	if (InReadyUpState() || InLiveState())
	{
		NEORules()->m_iNextRestore.iRoundsWonJinrai = iRoundsWonJinrai;
		NEORules()->m_iNextRestore.iRoundsWonNSF = iRoundsWonNSF;
		NEORules()->m_iNextRestore.flags |= NEXT_ROUND_GAMERULE_RESTORE_FLAG_ROUNDSWONS;
	}
	else
	{
		NEORules()->m_iNextRestore.flags &= ~(NEXT_ROUND_GAMERULE_RESTORE_FLAG_ROUNDSWONS);
	}

	Msg("%s: Rounds won set: Jinrai %d, NSF %d\n",
			pszFuncName,
			GetGlobalTeam(TEAM_JINRAI)->GetRoundsWon(),
			GetGlobalTeam(TEAM_NSF)->GetRoundsWon());
}

static void RestoreSetScore(const int iScoreJinrai, const int iScoreNSF, const char *pszFuncName)
{
	GetGlobalTeam(TEAM_JINRAI)->SetScore(iScoreJinrai);
	GetGlobalTeam(TEAM_NSF)->SetScore(iScoreNSF);
	if (InReadyUpState() || InLiveState())
	{
		NEORules()->m_iNextRestore.iScoreJinrai = iScoreJinrai;
		NEORules()->m_iNextRestore.iScoreNSF = iScoreNSF;
		NEORules()->m_iNextRestore.flags |= NEXT_ROUND_GAMERULE_RESTORE_FLAG_SCORES;
	}
	else
	{
		NEORules()->m_iNextRestore.flags &= ~(NEXT_ROUND_GAMERULE_RESTORE_FLAG_SCORES);
	}

	Msg("%s: Score set Jinrai %d, NSF %d\n",
			pszFuncName,
			GetGlobalTeam(TEAM_JINRAI)->GetScore(), GetGlobalTeam(TEAM_NSF)->GetScore());
}

// NEO NOTE (nullsystem): If iDeaths < 0, it won't be set
static void RestoreSetXPDeath(CNEO_Player *pNeoPlayer, const int iXP, const int iDeaths,
		const char *pszFuncName)
{
	if (false == InLiveState())
	{
		pNeoPlayer->m_iXP.Set(iXP);
		if (iDeaths >= 0)
		{
			pNeoPlayer->ResetDeathCount();
			pNeoPlayer->IncrementDeathCount(iDeaths);
		}
	}

	if (InReadyUpState() || InLiveState())
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
		Msg("%s: Given %d XP and %d deaths to %s%s\n", pszFuncName, iXP, iDeaths,
				pNeoPlayer->GetNeoPlayerName(), InLiveState() ? " next round" : "");
	}
	else
	{
		Msg("%s: Given %d XP to %s%s\n", pszFuncName, iXP,
				pNeoPlayer->GetNeoPlayerName(), InLiveState() ? " next round" : "");
	}
}

CON_COMMAND(sv_neo_restore_session, "Restore the previous session")
{
	if (false == sv_neo_restore_xp_death_any_round.GetBool() && false == InReadyUpState())
	{
		Msg("%s: error: Cannot set XPs if not idle and in a ready up lobby\n", __func__);
		return;
	}

	KeyValues *kv = new KeyValues(GIVEXP_SESSION_RESTORE_KV_ROOT);
	if (false == kv->LoadFromFile(g_pFullFileSystem, "scripts/" GIVEXP_SESSION_RESTORE_FNAME))
	{
		Msg("%s: error: No restore session file found\n", __func__);
		kv->deleteThis();
		return;
	}

	if (KeyValues *kvInfo = kv->FindKey("info"))
	{
		const char *pszInfileMap = kvInfo->GetString("map");
		const char *pszCurMap = gpGlobals->mapname.ToCStr();
		if (0 != V_strcmp(pszInfileMap, pszCurMap))
		{
			Msg("%s: Will not restore since map session differs: in file %s vs current %s\n",
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
				Msg("%s: Player steamID: %s, name: %s skipped: not found\n",
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

	{
		KeyValues *kvScore = new KeyValues("score");
		kvScore->SetInt("jinrai", GetGlobalTeam(TEAM_JINRAI)->GetScore());
		kvScore->SetInt("nsf", GetGlobalTeam(TEAM_NSF)->GetScore());
		kv->AddSubKey(kvScore);
	}

	{
		KeyValues *kvRounds = new KeyValues("rounds");
		kvRounds->SetInt("number", NEORules()->roundNumber());
		kvRounds->SetInt("jinrai", GetGlobalTeam(TEAM_JINRAI)->GetRoundsWon());
		kvRounds->SetInt("nsf", GetGlobalTeam(TEAM_NSF)->GetRoundsWon());
		kv->AddSubKey(kvRounds);
	}

	{
		KeyValues *kvPlayersList = new KeyValues("players_list");
		for (int i = 1; i <= gpGlobals->maxClients; i++)
		{
			auto pNeoPlayer = static_cast<CNEO_Player *>(UTIL_PlayerByIndex(i));
			if (pNeoPlayer
					&& (pNeoPlayer->GetTeamNumber() == TEAM_JINRAI
						|| pNeoPlayer->GetTeamNumber() == TEAM_NSF))
			{
				KeyValues *kvPlayer = new KeyValues("player");
				kvPlayer->SetInt("xp", pNeoPlayer->m_iXP.Get());
				kvPlayer->SetInt("deaths", pNeoPlayer->DeathCount());
				// team - Unused on de-serialization as steamid3 is enough, but have descriptive purpose
				kvPlayer->SetString("team", (pNeoPlayer->GetTeamNumber() == TEAM_JINRAI) ? "j" : "n");
				// name - Always set regardless of sv_neo_restore_session_name_match, have a descriptive
				//        purpose when looking inside the file
				kvPlayer->SetString("name", pNeoPlayer->GetNeoPlayerName());
				{
					const CSteamID playerSteamID = GetSteamIDForPlayerIndex(pNeoPlayer->entindex());
					if (playerSteamID.IsValid())
					{
						kvPlayer->SetString("steamid3", playerSteamID.Render());
					}
				}
				kvPlayersList->AddSubKey(kvPlayer);
			}
		}
		kv->AddSubKey(kvPlayersList);
	}

	if (kv->SaveToFile(g_pFullFileSystem, "scripts/" GIVEXP_SESSION_RESTORE_FNAME))
	{
		Msg("Session backed up at %s\n", szDateTime);
	}
	else
	{
		Msg("Session backup failed to save at %s!\n", szDateTime);
	}

	kv->deleteThis();
}

CON_COMMAND(sv_neo_restore_round_number, "Set the next round number")
{
	if (2 != args.ArgC())
	{
		Msg("Usage: %s <round number>\n", __func__);
		return;
	}

	const int iRoundNumber = V_atoi(args[1]);
	RestoreSetRoundNumber(iRoundNumber, __func__);
}

CON_COMMAND(sv_neo_restore_rounds_won, "Set the rounds won")
{
	if (3 != args.ArgC())
	{
		Msg("Usage: %s <jinrai won> <nsf won>\n", __func__);
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
		Msg("Usage: %s <jinrai score> <nsf score>\n", __func__);
		return;
	}

	const int iJinraiScore = V_atoi(args[1]);
	const int iNSFScore = V_atoi(args[2]);
	RestoreSetScore(iJinraiScore, iNSFScore, __func__);
}

CON_COMMAND(sv_neo_restore_xp, "Give a player XP (and death) count")
{
	static constexpr const char SZ_COMMON_USAGE_PF[] = "Usage: %s <player argument> <xp> <deaths (optional)>\n";

	if (!IN_BETWEEN_EQ(3, args.ArgC(), 4))
	{
		Msg(SZ_COMMON_USAGE_PF, __func__);
		return;
	}

	const char *pszNameFind = args[1];
	const int iXP = V_atoi(args[2]);
	if (false == IN_BETWEEN_EQ(-99, iXP, 1000))
	{
		Msg("%s: error: Unreasonable XP\n", __func__);
		return;
	}

	const int iDeaths = (args.ArgC() == 4) ? V_atoi(args[3]) : -1;
	if (4 == args.ArgC() && false == IN_BETWEEN_EQ(0, iDeaths, 1000))
	{
		Msg((iDeaths < 0)
				? "%s: error: Death count must be positive\n"
				: "%s: error: Unresonable death count\n"
				, __func__);
		Msg(SZ_COMMON_USAGE_PF, __func__);
		return;
	}

	if (false == sv_neo_restore_xp_death_any_round.GetBool() && false == InReadyUpState())
	{
		Msg("%s: error: Cannot set XPs if not idle and in a ready up lobby\n", __func__);
		return;
	}

	for (int i = 1; i <= gpGlobals->maxClients; ++i)
	{
		auto pNeoPlayer = static_cast<CNEO_Player*>(UTIL_PlayerByIndex(i));
		if (pNeoPlayer)
		{
			const char *pszNameCmp = pNeoPlayer->GetNeoPlayerName();
			if (0 == V_strcmp(pszNameCmp, pszNameFind))
			{
				RestoreSetXPDeath(pNeoPlayer, iXP, iDeaths, __func__);
				return;
			}
		}
	}

	Msg("%s: error: Cannot find player \"%s\"\n", __func__, pszNameFind);
}

