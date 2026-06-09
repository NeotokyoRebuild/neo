#include "cbase.h"
#include "neo_gamerules_dm.h"
#ifdef GAME_DLL
#include "neo_game_config.h"
#else
#include "c_playerresource.h"
#endif // GAME_DLL

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

REGISTER_GAMERULES_CLASS( CNEORulesDM );

BEGIN_NETWORK_TABLE_NOBASE( CNEORulesDM, DT_NEORulesDM )
END_NETWORK_TABLE()

LINK_ENTITY_TO_CLASS( neo_gamerules_dm, CNEOGameRulesDMProxy );
IMPLEMENT_NETWORKCLASS_ALIASED( NEOGameRulesDMProxy, DT_NEOGameRulesDMProxy );

#ifdef CLIENT_DLL
	void RecvProxy_NEORulesDM( const RecvProp *pProp, void **pOut,
		void *pData, int objectID )
	{
		CNEORulesDM *pRules = NEORulesDM();
		Assert( pRules );
		*pOut = pRules;
	}

	BEGIN_RECV_TABLE( CNEOGameRulesDMProxy, DT_NEOGameRulesDMProxy )
		RecvPropDataTable( "neo_gamerules_dm_data", 0, 0,	
			&REFERENCE_RECV_TABLE( DT_NEORulesDM ), 
			RecvProxy_NEORulesDM )
	END_RECV_TABLE()
#else
	void *SendProxy_NEORulesDM( const SendProp *pProp,
		const void *pStructBase, const void *pData,
		CSendProxyRecipients *pRecipients, int objectID )
	{
		CNEORulesDM *pRules = NEORulesDM();
		Assert( pRules );
		return pRules;
	}

	BEGIN_SEND_TABLE(CNEOGameRulesDMProxy, DT_NEOGameRulesDMProxy)
		SendPropDataTable("neo_gamerules_dm_data", 0,
			&REFERENCE_SEND_TABLE(DT_NEORulesDM),
			SendProxy_NEORulesDM)
		END_SEND_TABLE()
#endif
		
ConVar sv_neo_dm_round_limit("sv_neo_dm_round_limit", "0", FCVAR_REPLICATED, "DM max amount of rounds, 0 for no limit.", true, 0.0f, false, 0.0f);
ConVar sv_neo_dm_round_timelimit("sv_neo_dm_round_timelimit", "10.25", FCVAR_REPLICATED, "DM round timelimit, in minutes.", true, 0.0f, false, 0.0f);
ConVar sv_neo_dm_score_limit("sv_neo_dm_score_limit", "7", FCVAR_REPLICATED, "DM score limit", true, 0.0f, true, 99.0f);

ConVar sv_neo_dm_win_xp("sv_neo_dm_win_xp", "50", FCVAR_REPLICATED, "The XP limit to win the match.", true, 0.0f, true, 1000.0f);

extern bool RespawnWithRet(CBaseEntity *pEdict, bool fCopyCorpse);

void CNEORulesDM::FireGameEvent(IGameEvent* event)
{
	BaseClass::FireGameEvent(event);
}

#ifdef GAME_DLL
void CNEORulesDM::PlayerRespawnThink()
{
	for (int i = 1; i <= gpGlobals->maxClients; i++)
	{
		auto player = static_cast<CNEO_Player *>(UTIL_PlayerByIndex(i));
		if (player && player->IsDead() && (IsRoundPaused() || player->DeathCount() > 0))
		{
			const int playerTeam = player->GetTeamNumber();
			if ((playerTeam == TEAM_JINRAI || playerTeam == TEAM_NSF) && RespawnWithRet(player, false))
			{
				player->m_bInAim = false;
				player->m_bCarryingGhost = false;
				player->m_bInThermOpticCamo = false;
				player->m_bInVision = false;
				player->m_bIneligibleForLoadoutPick = false;
				player->SetTestMessageVisible(false);

				engine->ClientCommand(player->edict(), "loadoutmenu");
			}
		}
	}
}
#endif // GAME_DLL

float CNEORulesDM::GetRoundRemainingTime() const
{
	return BaseClass::GetRoundRemainingTime(sv_neo_dm_round_timelimit.GetFloat());
}

#ifdef GAME_DLL
bool CNEORulesDM::FPlayerCanRespawn(CBasePlayer* pPlayer)
{
	if (NeoRoundStatus::PostRound == m_nRoundStatus)
		return false;

	return true;
}

extern ConVar sv_neo_dm_max_class_dur;
bool CNEORulesDM::PlayerCanChangeLoadout(CNEO_Player* pPlayer)
{
	if (!pPlayer->m_bIneligibleForLoadoutPick && pPlayer->GetAliveDuration() < sv_neo_dm_max_class_dur.GetFloat())
		return true;

	return BaseClass::PlayerCanChangeLoadout(pPlayer);
}

void CNEORulesDM::SetGameRelatedVars()
{
	for (int i = 1; i <= gpGlobals->maxClients; ++i)
	{
		if (auto pPlayer = static_cast<CNEO_Player *>(UTIL_PlayerByIndex(i)))
		{
			pPlayer->m_iXP.GetForModify() = 0;
		}
	}
}

const int CNEORulesDM::GetScoreLimit() const
{
	return sv_neo_dm_score_limit.GetInt();
}

const int CNEORulesDM::GetRoundLimit() const
{
	return sv_neo_dm_round_limit.GetInt();
}

void CNEORulesDM::RoundTimeout()
{
	// Winning player
	CNEO_Player* pWinners[MAX_PLAYERS + 1] = {};
	int iWinnersTotal = 0;
	int iWinnerXP = 0;
	GetDMHighestScorers(&pWinners, &iWinnersTotal, &iWinnerXP);
	if (iWinnersTotal == 1)
	{
		SetWinningDMPlayer(pWinners[0]);
	}

	// Otherwise go into overtime
}
#endif // GAME_DLL

void CNEORulesDM::Think()
{
#ifdef GAME_DLL
	CGameRules::Think();
	
	UpdateFromGameConfig();

	if (RoundStartFromIdleOrPausedThink())
		return;

	PlayerRespawnThink();

	CheckClantagsThink();

	if (IsRoundPaused())
		return;

	GameOverThink();
	
	CheckOvertime();
	
	if (CHL2MPRulesThink())
		return;

	TeamDamageThink();

	if (RoundOverThink())
		return;
	
	RoundStatusThink();

	// Win by XP
	if (IsRoundLive() && sv_neo_dm_win_xp.GetInt() > 0)
	{
		// End game early if there's already a player past the winning XP
		CNEO_Player *pHighestPlayers[MAX_PLAYERS + 1] = {};
		int iWinningTotal = 0;
		int iWinningXP = 0;
		GetDMHighestScorers(&pHighestPlayers, &iWinningTotal, &iWinningXP);
		if (iWinningXP >= sv_neo_dm_win_xp.GetInt() && iWinningTotal == 1)
		{
			SetWinningDMPlayer(pHighestPlayers[0]);
		}
	}
#endif // GAME_DLL
}


void CNEORulesDM::GetDMHighestScorers(
#ifdef GAME_DLL
		CNEO_Player *(*pHighestPlayers)[MAX_PLAYERS + 1],
#endif
		int *iHighestPlayersTotal,
		int *iHighestXP) const
{
	*iHighestPlayersTotal = 0;
	*iHighestXP = 0;
#ifdef GAME_DLL
	for (int i = 1; i <= gpGlobals->maxClients; ++i)
#else
	if (!g_PR)
	{
		return;
	}

	for (int i = 0; i < (MAX_PLAYERS + 1); ++i)
#endif
	{
		int iXP = 0;

#ifdef GAME_DLL
		auto pCmpPlayer = static_cast<CNEO_Player *>(UTIL_PlayerByIndex(i));
		if (!pCmpPlayer)
		{
			continue;
		}
		iXP = pCmpPlayer->m_iXP;
#else
		if (!g_PR->IsConnected(i))
		{
			continue;
		}
		iXP = g_PR->GetXP(i);
#endif

		if (iXP == *iHighestXP)
		{
#ifdef GAME_DLL
			(*pHighestPlayers)[(*iHighestPlayersTotal)++] = pCmpPlayer;
#else
			(*iHighestPlayersTotal)++;
#endif
		}
		else if (iXP > *iHighestXP)
		{
			*iHighestPlayersTotal = 0;
			*iHighestXP = iXP;
#ifdef GAME_DLL
			(*pHighestPlayers)[(*iHighestPlayersTotal)++] = pCmpPlayer;
#else
			(*iHighestPlayersTotal)++;
#endif
		}
	}
}

#ifdef GAME_DLL
void CNEORulesDM::SetWinningDMPlayer(CNEO_Player *pWinner)
{
	if (IsRoundOver())
	{
		return;
	}

	if (auto pEntGameCfg = NEOGameConfig())
	{
		pEntGameCfg->m_OnDMRoundEnd.FireOutput(pWinner, pEntGameCfg);
	}

	SetRoundStatus(NeoRoundStatus::PostRound);
	char victoryMsg[128];
	// TODO: Per client since client has neo_name settings
	V_sprintf_safe(victoryMsg, "%s is the winner of the deathmatch!\n", pWinner->GetNeoPlayerName());

	CRecipientFilter filter;
	filter.AddAllPlayers();
	UserMessageBegin(filter, "RoundResult");
	WRITE_STRING("tie");
	WRITE_FLOAT(gpGlobals->curtime);
	WRITE_STRING(victoryMsg);
	MessageEnd();

	EmitSound_t soundParams;
	soundParams.m_nChannel = CHAN_AUTO;
	soundParams.m_SoundLevel = SNDLVL_NONE;
	soundParams.m_flVolume = 0.33f;
	// Differing between Jinrai/NSF only as a sound cosmetic (no affect on DM)
	const int team = pWinner->GetTeamNumber();
	soundParams.m_pSoundName = (team == TEAM_JINRAI) ? "gameplay/jinrai.mp3" : (team == TEAM_NSF) ? "gameplay/nsf.mp3" : "gameplay/draw.mp3";
	soundParams.m_bWarnOnDirectWaveReference = false;
	soundParams.m_bEmitCloseCaption = false;

	for (int i = 1; i <= gpGlobals->maxClients; ++i)
	{
		CBasePlayer* basePlayer = UTIL_PlayerByIndex(i);
		auto player = static_cast<CNEO_Player*>(basePlayer);
		if (player)
		{
			if (!player->IsBot() || player->IsHLTV())
			{
				const char* volStr = engine->GetClientConVarValue(i, "snd_victory_volume");
				const float jingleVolume = volStr ? atof(volStr) : 0.33f;
				soundParams.m_flVolume = jingleVolume;

				CRecipientFilter soundFilter;
				soundFilter.AddRecipient(basePlayer);
				soundFilter.MakeReliable();
				player->EmitSound(soundFilter, i, soundParams);
			}
		}
	}

	GoToIntermission();

	IGameEvent *event = gameeventmanager->CreateEvent("game_end");
	if (event)
	{
		event->SetInt("winner", pWinner->GetUserID());
		gameeventmanager->FireEvent(event);
	}
}
#endif // GAME_DLL