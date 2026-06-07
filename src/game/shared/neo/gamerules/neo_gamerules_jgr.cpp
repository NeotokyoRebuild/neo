#include "cbase.h"
#include "neo_gamerules_jgr.h"

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

REGISTER_GAMERULES_CLASS( CNEORulesJGR );

BEGIN_NETWORK_TABLE_NOBASE( CNEORulesJGR, DT_NEORulesJGR )
END_NETWORK_TABLE()

LINK_ENTITY_TO_CLASS( neo_gamerules_jgr, CNEOGameRulesJGRProxy );
IMPLEMENT_NETWORKCLASS_ALIASED( NEOGameRulesJGRProxy, DT_NEOGameRulesJGRProxy );

#ifdef CLIENT_DLL
	void RecvProxy_NEORulesJGR( const RecvProp *pProp, void **pOut,
		void *pData, int objectID )
	{
		CNEORulesJGR *pRules = NEORulesJGR();
		Assert( pRules );
		*pOut = pRules;
	}

	BEGIN_RECV_TABLE( CNEOGameRulesJGRProxy, DT_NEOGameRulesJGRProxy )
		RecvPropDataTable( "neo_gamerules_jgr_data", 0, 0,	
			&REFERENCE_RECV_TABLE( DT_NEORulesJGR ), 
			RecvProxy_NEORulesJGR )
	END_RECV_TABLE()
#else
	void *SendProxy_NEORulesJGR( const SendProp *pProp,
		const void *pStructBase, const void *pData,
		CSendProxyRecipients *pRecipients, int objectID )
	{
		CNEORulesJGR *pRules = NEORulesJGR();
		Assert( pRules );
		return pRules;
	}

	BEGIN_SEND_TABLE(CNEOGameRulesJGRProxy, DT_NEOGameRulesJGRProxy)
		SendPropDataTable("neo_gamerules_jgr_data", 0,
			&REFERENCE_SEND_TABLE(DT_NEORulesJGR),
			SendProxy_NEORulesJGR)
		END_SEND_TABLE()
#endif
		
ConVar sv_neo_jgr_round_limit("sv_neo_jgr_round_limit", "5", FCVAR_REPLICATED, "JGR max amount of rounds, 0 for no limit.", true, 0.0f, false, 0.0f);
ConVar sv_neo_jgr_round_timelimit("sv_neo_jgr_round_timelimit", "4.25", FCVAR_REPLICATED, "JGR round timelimit, in minutes.", true, 0.0f, false, 0.0f);
ConVar sv_neo_jgr_score_limit("sv_neo_jgr_score_limit", "0", FCVAR_REPLICATED, "JGR score limit", true, 0.0f, true, 99.0f);
ConVar sv_neo_jgr_max_points("sv_neo_jgr_max_points", "20", FCVAR_GAMEDLL, "Maximum points required for a team to win in JGR", true, 1, false, 0);

extern bool RespawnWithRet(CBaseEntity *pEdict, bool fCopyCorpse);

//CNEORulesJGR::CNEORulesJGR()
//{
//
//}
//
//CNEORulesJGR::~CNEORulesJGR()
//{
//}

void CNEORulesJGR::FireGameEvent(IGameEvent* event)
{
	BaseClass::FireGameEvent(event);
}

#ifdef GAME_DLL
void CNEORulesJGR::PlayerRespawnThink()
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

float CNEORulesJGR::GetRoundRemainingTime() const
{
	return BaseClass::GetRoundRemainingTime(sv_neo_jgr_round_timelimit.GetFloat());
}

#ifdef GAME_DLL
bool CNEORulesJGR::FPlayerCanRespawn(CBasePlayer* pPlayer)
{
	CNEO_Player* pNeoPlayer = ToNEOPlayer(pPlayer);
	if (!pNeoPlayer)
	{
		Assert(false);
		return false;
	}
	
	// Special case for spectator player takeover
	if (pNeoPlayer->GetSpectatorTakeoverPlayerPending())
		return true;

	if (pPlayer->GetTeamNumber() == m_iLastJuggernautTeam &&
		(m_pJuggernautPlayer || (gpGlobals->curtime - m_flJuggernautDeathTime) <= 8.0f))
	{
		return false;
	}
	
	if (NeoRoundStatus::PostRound == m_nRoundStatus)
		return false;

	return true;
}

extern ConVar sv_neo_dm_max_class_dur;
bool CNEORulesJGR::PlayerCanChangeLoadout(CNEO_Player* pPlayer)
{
	if (!pPlayer->m_bIneligibleForLoadoutPick && pPlayer->GetAliveDuration() < sv_neo_dm_max_class_dur.GetFloat())
		return true;

	return BaseClass::PlayerCanChangeLoadout(pPlayer);
}

void CNEORulesJGR::EnemyPlayerKilled(CNEO_Player* pVictim, CNEO_Player* pAttacker, const CTakeDamageInfo& info)
{
	if (!IsRoundLive())
		return;

	if (pAttacker->GetClass() == NEO_CLASS_JUGGERNAUT)
	{
		auto jgrTeam = pAttacker->GetTeam();
		jgrTeam->SetScore(Min(jgrTeam->GetScore() + 2, sv_neo_jgr_max_points.GetInt()));
	}
	else if (m_pJuggernautPlayer)
	{
		const int attackerTeam = pAttacker->GetTeamNumber();
		const int jgrTeam = m_pJuggernautPlayer->GetTeamNumber();

		if (attackerTeam == jgrTeam)
		{
			pAttacker->GetTeam()->AddScore(1);
		}
	}
}

void CNEORulesJGR::SetGameRelatedVars()
{
	ResetTeamScores();
	ResetJGR();
	SpawnTheJuggernaut();
}

const int CNEORulesJGR::GetScoreLimit() const
{
	return sv_neo_jgr_score_limit.GetInt();
}

const int CNEORulesJGR::GetRoundLimit() const
{
	return sv_neo_jgr_round_limit.GetInt();
}

void CNEORulesJGR::RoundTimeout()
{
	if ((!m_pJuggernautPlayer && m_pJuggernautItem && !m_pJuggernautItem->IsBeingActivatedByLosingTeam()) ||
		(!m_pJuggernautPlayer && !m_pJuggernautItem)) // Juggernaut is absent entirely
	{
		if (GetGlobalTeam(TEAM_JINRAI)->GetScore() > GetGlobalTeam(TEAM_NSF)->GetScore())
		{
			SetWinningTeam(TEAM_JINRAI, NEO_VICTORY_POINTS, false, true, false, false);
			return;
		}

		if (GetGlobalTeam(TEAM_NSF)->GetScore() > GetGlobalTeam(TEAM_JINRAI)->GetScore())
		{
			SetWinningTeam(TEAM_NSF, NEO_VICTORY_POINTS, false, true, false, false);
			return;
		}
	}
	else
	{
		if (m_nRoundStatus == NeoRoundStatus::RoundLive)
		{
			m_nRoundStatus = NeoRoundStatus::Overtime;
		}

		if (m_pJuggernautPlayer)
		{
			const int jgrTeam = m_pJuggernautPlayer->GetTeamNumber();
			const int oppositeTeam = (m_pJuggernautPlayer->GetTeamNumber() == TEAM_JINRAI ? TEAM_NSF : TEAM_JINRAI);
			if (GetGlobalTeam(jgrTeam)->GetScore() > GetGlobalTeam(oppositeTeam)->GetScore())
			{
				SetWinningTeam(jgrTeam, NEO_VICTORY_POINTS, false, true, false, false);
				return;
			}
		}

		return;
	}

	SetWinningTeam(TEAM_SPECTATOR, NEO_VICTORY_STALEMATE, false, false, true, false);
}
#endif // GAME_DLL

extern ConVar sv_neo_preround_freeze_time;
void CNEORulesJGR::Think()
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
	
	if (CHL2MPRulesThink())
		return;

	TeamDamageThink();

	if (RoundOverThink())
		return;
	
	RoundStatusThink();

	if (IsRoundLive())
	{
		// Unlock the Juggernaut
		if (m_pJuggernautItem && (gpGlobals->curtime > (m_flNeoRoundStartTime + sv_neo_preround_freeze_time.GetFloat()) + 20.0f) && IsJuggernautLocked())
		{
			UTIL_CenterPrintAll("- JUGGERNAUT ENABLED -\n");

			EmitSound_t soundParams;
			soundParams.m_pSoundName = "HUD.GhostPickUp";
			soundParams.m_nChannel = CHAN_USER_BASE;
			soundParams.m_bWarnOnDirectWaveReference = false;
			soundParams.m_bEmitCloseCaption = false;
			soundParams.m_SoundLevel = ATTN_TO_SNDLVL(ATTN_NONE);

			CRecipientFilter soundFilter;
			soundFilter.AddAllPlayers();
			soundFilter.MakeReliable();
			m_pJuggernautItem->EmitSound(soundFilter, m_pJuggernautItem->entindex(), soundParams);

			m_pJuggernautItem->m_bLocked = false;
		}

		// Check win condition
		{
			const int maxPoints = sv_neo_jgr_max_points.GetInt();
			if (GetGlobalTeam(TEAM_JINRAI)->GetScore() >= maxPoints)
			{
				SetWinningTeam(TEAM_JINRAI, NEO_VICTORY_POINTS, false, true, false, false);
				return;
			}

			if (GetGlobalTeam(TEAM_NSF)->GetScore() >= maxPoints)
			{
				SetWinningTeam(TEAM_NSF, NEO_VICTORY_POINTS, false, true, false, false);
				return;
			}
		}
	}

#endif // GAME_DLL
}