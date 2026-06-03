#include "cbase.h"
#include "neo_gamerules_tdm.h"

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

REGISTER_GAMERULES_CLASS( CNEORulesTDM );

BEGIN_NETWORK_TABLE_NOBASE( CNEORulesTDM, DT_NEORulesTDM )
END_NETWORK_TABLE()

LINK_ENTITY_TO_CLASS( neo_gamerules_tdm, CNEOGameRulesTDMProxy );
IMPLEMENT_NETWORKCLASS_ALIASED( NEOGameRulesTDMProxy, DT_NEOGameRulesTDMProxy );

#ifdef CLIENT_DLL
	void RecvProxy_NEORulesTDM( const RecvProp *pProp, void **pOut,
		void *pData, int objectID )
	{
		CNEORulesTDM *pRules = NEORulesTDM();
		Assert( pRules );
		*pOut = pRules;
	}

	BEGIN_RECV_TABLE( CNEOGameRulesTDMProxy, DT_NEOGameRulesTDMProxy )
		RecvPropDataTable( "neo_gamerules_tdm_data", 0, 0,	
			&REFERENCE_RECV_TABLE( DT_NEORulesTDM ), 
			RecvProxy_NEORulesTDM )
	END_RECV_TABLE()
#else
	void *SendProxy_NEORulesTDM( const SendProp *pProp,
		const void *pStructBase, const void *pData,
		CSendProxyRecipients *pRecipients, int objectID )
	{
		CNEORulesTDM *pRules = NEORulesTDM();
		Assert( pRules );
		return pRules;
	}

	BEGIN_SEND_TABLE(CNEOGameRulesTDMProxy, DT_NEOGameRulesTDMProxy)
		SendPropDataTable("neo_gamerules_tdm_data", 0,
			&REFERENCE_SEND_TABLE(DT_NEORulesTDM),
			SendProxy_NEORulesTDM)
		END_SEND_TABLE()
#endif
		

ConVar sv_neo_tdm_score_limit("sv_neo_tdm_score_limit", "1", FCVAR_REPLICATED, "TDM score limit", true, 0.0f, true, 99.0f);
ConVar sv_neo_tdm_round_limit("sv_neo_tdm_round_limit", "0", FCVAR_REPLICATED, "TDM max amount of rounds, 0 for no limit.", true, 0.0f, false, 0.0f);
ConVar sv_neo_tdm_round_timelimit("sv_neo_tdm_round_timelimit", "10.25", FCVAR_REPLICATED, "TDM round timelimit, in minutes.",	true, 0.0f, false, 0.0f);
		
extern bool RespawnWithRet(CBaseEntity *pEdict, bool fCopyCorpse);

//CNEORulesTDM::CNEORulesTDM()
//{
//}
//
//CNEORulesTDM::~CNEORulesTDM()
//{
//}

void CNEORulesTDM::FireGameEvent(IGameEvent* event)
{
	BaseClass::FireGameEvent(event);
}

#ifdef GAME_DLL
void CNEORulesTDM::PlayerRespawnThink()
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

float CNEORulesTDM::GetRoundRemainingTime() const
{
	return BaseClass::GetRoundRemainingTime(sv_neo_tdm_round_timelimit.GetFloat());
}

#ifdef GAME_DLL
bool CNEORulesTDM::FPlayerCanRespawn(CBasePlayer* pPlayer)
{
	if (NeoRoundStatus::PostRound == m_nRoundStatus)
		return false;

	return true;
}

void CNEORulesTDM::SetGameRelatedVars()
{
	ResetTeamScores();
}

const int CNEORulesTDM::GetScoreLimit() const
{
	return sv_neo_tdm_score_limit.GetInt();
}

const int CNEORulesTDM::GetRoundLimit() const
{
	return sv_neo_tdm_round_limit.GetInt();
}

void CNEORulesTDM::RoundTimeout()
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

	SetWinningTeam(TEAM_SPECTATOR, NEO_VICTORY_STALEMATE, false, false, true, false);
}
#endif // GAME_DLL

void CNEORulesTDM::Think()
{
#ifdef GAME_DLL
	if (RoundStartFromIdleOrPausedThink())
		return;

	PlayerRespawnThink();

	CheckClantagsThink();

	if (IsRoundPaused())
		return;

	GameOverThink();
	
	CheckOvertime();

	CHL2MPRules::Think();

	TeamDamageThink();

	if (RoundOverThink())
		return;

	RoundStatusThink();
#endif // GAME_DLL
}