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

extern ConVar sv_neo_preround_freeze_time;

void CNEORulesJGR::Think()
{
#ifdef GAME_DLL
	if (RoundIdlePausedStartThink())
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