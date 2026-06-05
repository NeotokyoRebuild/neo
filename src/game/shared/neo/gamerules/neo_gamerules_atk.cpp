#include "cbase.h"
#include "neo_gamerules_atk.h"

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

REGISTER_GAMERULES_CLASS( CNEORulesATK );

BEGIN_NETWORK_TABLE_NOBASE( CNEORulesATK, DT_NEORulesATK )
END_NETWORK_TABLE()

LINK_ENTITY_TO_CLASS( neo_gamerules_atk, CNEOGameRulesATKProxy );
IMPLEMENT_NETWORKCLASS_ALIASED( NEOGameRulesATKProxy, DT_NEOGameRulesATKProxy );

#ifdef CLIENT_DLL
	void RecvProxy_NEORulesATK( const RecvProp *pProp, void **pOut,
		void *pData, int objectID )
	{
		CNEORulesATK *pRules = NEORulesATK();
		Assert( pRules );
		*pOut = pRules;
	}

	BEGIN_RECV_TABLE( CNEOGameRulesATKProxy, DT_NEOGameRulesATKProxy )
		RecvPropDataTable( "neo_gamerules_atk_data", 0, 0,	
			&REFERENCE_RECV_TABLE( DT_NEORulesATK ), 
			RecvProxy_NEORulesATK )
	END_RECV_TABLE()
#else
	void *SendProxy_NEORulesATK( const SendProp *pProp,
		const void *pStructBase, const void *pData,
		CSendProxyRecipients *pRecipients, int objectID )
	{
		CNEORulesATK *pRules = NEORulesATK();
		Assert( pRules );
		return pRules;
	}

	BEGIN_SEND_TABLE(CNEOGameRulesATKProxy, DT_NEOGameRulesATKProxy)
		SendPropDataTable("neo_gamerules_atk_data", 0,
			&REFERENCE_SEND_TABLE(DT_NEORulesATK),
			SendProxy_NEORulesATK)
		END_SEND_TABLE()
#endif

ConVar sv_neo_atk_round_limit("sv_neo_atk_round_limit", "0", FCVAR_REPLICATED, "ATK max amount of rounds, 0 for no limit.", true, 0.0f, false, 0.0f);
ConVar sv_neo_atk_round_timelimit("sv_neo_atk_round_timelimit", "3.25", FCVAR_REPLICATED, "ATK round timelimit, in minutes.", true, 0.0f, false, 600.0f);
ConVar sv_neo_atk_score_limit("sv_neo_atk_score_limit", "7", FCVAR_REPLICATED, "ATK score limit", true, 0.0f, true, 99.0f);

ConVar sv_neo_atk_ghost_overtime_enabled("sv_neo_atk_ghost_overtime_enabled", "0", FCVAR_REPLICATED, "Enable ghost overtime in the ATK mode.", true, 0, true, 1);
ConVar sv_neo_atk_ghost_overtime("sv_neo_atk_ghost_overtime", "45", FCVAR_REPLICATED, "Adds up to this many seconds to the round while the ghost is held.", true, 0, true, 120);
ConVar sv_neo_atk_ghost_overtime_grace("sv_neo_atk_ghost_overtime_grace", "10", FCVAR_REPLICATED, "Number of seconds left in the round when the ghost is dropped in overtime.", true, 0, true, 30);
ConVar sv_neo_atk_ghost_overtime_grace_decay("sv_neo_atk_ghost_overtime_grace_decay", "0", FCVAR_REPLICATED, "Slowly reduce the grace time as overtime goes on.", true, 0, true, 1);

//CNEORulesATK::CNEORulesATK()
//{
//}
//
//CNEORulesATK::~CNEORulesATK()
//{
//}

void CNEORulesATK::FireGameEvent(IGameEvent* event)
{
	BaseClass::FireGameEvent(event);
}

void CNEORulesATK::CheckOvertime()
{
	if (!sv_neo_atk_ghost_overtime_enabled.GetBool())
		return;

	float overtimeGrace = sv_neo_atk_round_timelimit.GetFloat() * 60;
	float roundTimeLimit = sv_neo_atk_ghost_overtime_grace.GetFloat();

	if (NeoRoundStatus::RoundLive == m_nRoundStatus && m_iGhosterPlayer && GetAttackingTeam() == m_iGhosterTeam &&
		(m_flNeoRoundStartTime + roundTimeLimit - overtimeGrace) < gpGlobals->curtime)
	{
		m_nRoundStatus = NeoRoundStatus::Overtime;
	}

	if (NeoRoundStatus::Overtime == m_nRoundStatus && m_iGhosterPlayer)
	{
		m_flGhostLastHeld = gpGlobals->curtime;
	}
}

float CNEORulesATK::GetRoundRemainingTime() const
{
	if (NeoRoundStatus::Overtime == m_nRoundStatus)
	{
		return GetOverTime(sv_neo_atk_round_timelimit.GetFloat() * 60.f, 
						sv_neo_atk_ghost_overtime.GetFloat(), 
						sv_neo_atk_ghost_overtime_grace.GetFloat(), 
						sv_neo_atk_ghost_overtime_grace_decay.GetBool());
	}
	return BaseClass::GetRoundRemainingTime(sv_neo_atk_round_timelimit.GetFloat());
}

#ifdef GAME_DLL
void CNEORulesATK::SetGameRelatedVars()
{
	ResetGhost();
	SpawnTheGhost();
}

const int CNEORulesATK::GetScoreLimit() const
{
	return sv_neo_atk_score_limit.GetInt();
}

const int CNEORulesATK::GetRoundLimit() const
{
	return sv_neo_atk_round_limit.GetInt();
}

void CNEORulesATK::RoundTimeout()
{
	SetWinningTeam(GetDefendingTeam(), NEO_VICTORY_ATK_TIMEOUT, false, true, false, false);
}
#endif // GAME_DLL

void CNEORulesATK::Think()
{
#ifdef GAME_DLL
	CGameRules::Think();

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

	CheckWinByElimination();
#endif // GAME_DLL
}