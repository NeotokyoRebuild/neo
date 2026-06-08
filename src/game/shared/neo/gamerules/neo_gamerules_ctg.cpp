#include "cbase.h"
#include "neo_gamerules_ctg.h"

#ifdef GAME_DLL
	#include "neo_ghost_spawn_point.h"
	#include "neo_ghost_cap_point.h"
#else
#endif // GAME_DLL

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

REGISTER_GAMERULES_CLASS( CNEORulesCTG );

BEGIN_NETWORK_TABLE_NOBASE( CNEORulesCTG, DT_NEORulesCTG )
END_NETWORK_TABLE()

LINK_ENTITY_TO_CLASS( neo_gamerules_ctg, CNEOGameRulesCTGProxy );
IMPLEMENT_NETWORKCLASS_ALIASED( NEOGameRulesCTGProxy, DT_NEOGameRulesCTGProxy );

#ifdef CLIENT_DLL
	void RecvProxy_NEORulesCTG( const RecvProp *pProp, void **pOut,
		void *pData, int objectID )
	{
		CNEORulesCTG *pRules = NEORulesCTG();
		Assert( pRules );
		*pOut = pRules;
	}

	BEGIN_RECV_TABLE( CNEOGameRulesCTGProxy, DT_NEOGameRulesCTGProxy )
		RecvPropDataTable( "neo_gamerules_ctg_data", 0, 0,	
			&REFERENCE_RECV_TABLE( DT_NEORulesCTG ), 
			RecvProxy_NEORulesCTG )
	END_RECV_TABLE()
#else
	void *SendProxy_NEORulesCTG( const SendProp *pProp,
		const void *pStructBase, const void *pData,
		CSendProxyRecipients *pRecipients, int objectID )
	{
		CNEORulesCTG *pRules = NEORulesCTG();
		Assert( pRules );
		return pRules;
	}

	BEGIN_SEND_TABLE(CNEOGameRulesCTGProxy, DT_NEOGameRulesCTGProxy)
		SendPropDataTable("neo_gamerules_ctg_data", 0,
			&REFERENCE_SEND_TABLE(DT_NEORulesCTG),
			SendProxy_NEORulesCTG)
		END_SEND_TABLE()
#endif
		
ConVar sv_neo_ctg_round_limit("sv_neo_ctg_round_limit", "0", FCVAR_REPLICATED, "CTG max amount of rounds, 0 for no limit.", true, 0.0f, false, 0.0f);
ConVar sv_neo_ctg_round_timelimit("sv_neo_ctg_round_timelimit", "3.25", FCVAR_REPLICATED, "CTG round timelimit, in minutes.",	true, 0.0f, false, 0.0f);
ConVar sv_neo_ctg_score_limit("sv_neo_ctg_score_limit", "7", FCVAR_REPLICATED, "CTG score limit", true, 0.0f, true, 99.0f);

ConVar sv_neo_ctg_ghost_overtime_enabled("sv_neo_ctg_ghost_overtime_enabled", "0", FCVAR_REPLICATED, "Enable ghost overtime.", true, 0, true, 1);
ConVar sv_neo_ctg_ghost_overtime("sv_neo_ctg_ghost_overtime", "45", FCVAR_REPLICATED, "Adds up to this many seconds to the round while the ghost is held.", true, 0, true, 120);
ConVar sv_neo_ctg_ghost_overtime_grace("sv_neo_ctg_ghost_overtime_grace", "10", FCVAR_REPLICATED, "Number of seconds left in the round when the ghost is dropped in overtime.", true, 0, true, 30);
ConVar sv_neo_ctg_ghost_overtime_grace_decay("sv_neo_ctg_ghost_overtime_grace_decay", "0", FCVAR_REPLICATED, "Slowly reduce the grace time as overtime goes on.", true, 0, true, 1);

//CNEORulesCTG::CNEORulesCTG()
//{
//}
//
//CNEORulesCTG::~CNEORulesCTG()
//{
//}

void CNEORulesCTG::FireGameEvent(IGameEvent* event)
{
	BaseClass::FireGameEvent(event);
}

float CNEORulesCTG::GetRoundRemainingTime() const
{
	if (NeoRoundStatus::Overtime == m_nRoundStatus)
	{
		return GetOverTime(sv_neo_ctg_round_timelimit.GetFloat() * 60.f, 
						sv_neo_ctg_ghost_overtime.GetFloat(), 
						sv_neo_ctg_ghost_overtime_grace.GetFloat(), 
						sv_neo_ctg_ghost_overtime_grace_decay.GetBool());
	}
	return BaseClass::GetRoundRemainingTime(sv_neo_ctg_round_timelimit.GetFloat());
}

#ifdef GAME_DLL
void CNEORulesCTG::SetGameRelatedVars()
{
	ResetGhost();
	SpawnTheGhost();
}

const int CNEORulesCTG::GetScoreLimit() const
{
	return sv_neo_ctg_score_limit.GetInt();
}

const int CNEORulesCTG::GetRoundLimit() const
{
	return sv_neo_ctg_round_limit.GetInt();
}

void CNEORulesCTG::CheckOvertime()
{
	if (!sv_neo_ctg_ghost_overtime_enabled.GetBool())
		return;

	float overtimeGrace = sv_neo_ctg_round_timelimit.GetFloat() * 60;
	float roundTimeLimit = sv_neo_ctg_ghost_overtime_grace.GetFloat();

	if (NeoRoundStatus::RoundLive == m_nRoundStatus && m_iGhosterPlayer &&
		(m_flNeoRoundStartTime + roundTimeLimit - overtimeGrace) < gpGlobals->curtime)
	{
		m_nRoundStatus = NeoRoundStatus::Overtime;
	}

	if (m_nRoundStatus == NeoRoundStatus::Overtime && m_iGhosterPlayer)
	{
		m_flGhostLastHeld = gpGlobals->curtime;
	}
}

void CNEORulesCTG::RoundTimeout()
{
	SetWinningTeam(TEAM_SPECTATOR, NEO_VICTORY_STALEMATE, false, false, true, false);
}
#endif // GAME_DLL

void CNEORulesCTG::Think()
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

	if (GhostTeamUpdateWinCondition())
		return;

	CheckWinByElimination();
#endif // GAME_DLL
}