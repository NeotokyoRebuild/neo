#include "cbase.h"
#include "neo_gamerules_vip.h"

#ifdef GAME_DLL
	#include "neo_ghost_cap_point.h"
#else
#endif // GAME_DLL

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

REGISTER_GAMERULES_CLASS( CNEORulesVIP );

BEGIN_NETWORK_TABLE_NOBASE( CNEORulesVIP, DT_NEORulesVIP )
END_NETWORK_TABLE()

LINK_ENTITY_TO_CLASS( neo_gamerules_vip, CNEOGameRulesVIPProxy );
IMPLEMENT_NETWORKCLASS_ALIASED( NEOGameRulesVIPProxy, DT_NEOGameRulesVIPProxy );

#ifdef CLIENT_DLL
	void RecvProxy_NEORulesVIP( const RecvProp *pProp, void **pOut,
		void *pData, int objectID )
	{
		CNEORulesVIP *pRules = NEORulesVIP();
		Assert( pRules );
		*pOut = pRules;
	}

	BEGIN_RECV_TABLE( CNEOGameRulesVIPProxy, DT_NEOGameRulesVIPProxy )
		RecvPropDataTable( "neo_gamerules_vip_data", 0, 0,	
			&REFERENCE_RECV_TABLE( DT_NEORulesVIP ), 
			RecvProxy_NEORulesVIP )
	END_RECV_TABLE()
#else
	void *SendProxy_NEORulesVIP( const SendProp *pProp,
		const void *pStructBase, const void *pData,
		CSendProxyRecipients *pRecipients, int objectID )
	{
		CNEORulesVIP *pRules = NEORulesVIP();
		Assert( pRules );
		return pRules;
	}

	BEGIN_SEND_TABLE(CNEOGameRulesVIPProxy, DT_NEOGameRulesVIPProxy)
		SendPropDataTable("neo_gamerules_vip_data", 0,
			&REFERENCE_SEND_TABLE(DT_NEORulesVIP),
			SendProxy_NEORulesVIP)
		END_SEND_TABLE()
#endif
		
ConVar sv_neo_vip_round_limit("sv_neo_vip_round_limit", "0", FCVAR_REPLICATED, "VIP max amount of rounds, 0 for no limit.", true, 0.0f, false, 0.0f);
ConVar sv_neo_vip_round_timelimit("sv_neo_vip_round_timelimit", "3.25", FCVAR_REPLICATED, "VIP round timelimit, in minutes.", true, 0.0f, false, 0.0f);
ConVar sv_neo_vip_score_limit("sv_neo_vip_score_limit", "7", FCVAR_REPLICATED, "VIP score limit", true, 0.0f, true, 99.0f);

ConVar sv_neo_vip_ctg_on_death("sv_neo_vip_ctg_on_death", "0", FCVAR_ARCHIVE, "Spawn Ghost when VIP dies, continue the game", true, 0, true, 1);

void CNEORulesVIP::FireGameEvent(IGameEvent* event)
{
	BaseClass::FireGameEvent(event);
}

float CNEORulesVIP::GetRoundRemainingTime() const
{
	return BaseClass::GetRoundRemainingTime(sv_neo_vip_round_timelimit.GetFloat());
}

#ifdef GAME_DLL
void CNEORulesVIP::SetGameRelatedVars()
{
	ResetVIP();
	
	if (!m_iEscortingTeam)
	{
		m_iEscortingTeam.Set(RandomInt(TEAM_JINRAI, TEAM_NSF));
	}
	else
	{
		m_iEscortingTeam.Set(m_iEscortingTeam.Get() == TEAM_JINRAI ? TEAM_NSF : TEAM_JINRAI);
	}

	SelectTheVIP();
}

const int CNEORulesVIP::GetScoreLimit() const
{
	return sv_neo_vip_score_limit.GetInt();
}

const int CNEORulesVIP::GetRoundLimit() const
{
	return sv_neo_vip_round_limit.GetInt();
}
#endif // GAME_DLL

void CNEORulesVIP::Think()
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

	// Check win condition
	if (!m_pGhost)
	{
		if (!m_pVIP)
		{
			if (sv_neo_vip_ctg_on_death.GetBool())
			{
				UTIL_CenterPrintAll("- HVT DOWN - RECOVER THE GHOST -\n");
				SpawnTheGhost();
			}
			else
			{
				// Assume vip player disconnected, forfeit round
				SetWinningTeam(GetOpposingTeam(m_iEscortingTeam), NEO_VICTORY_FORFEIT, false, true, false, false);
			}

			if (IGameEvent* event = gameeventmanager->CreateEvent("vip_death"))
			{
				gameeventmanager->FireEvent(event);
			}
		}
		else if (!m_pVIP->IsAlive())
		{
			if (sv_neo_vip_ctg_on_death.GetBool())
			{
				UTIL_CenterPrintAll("- HVT DOWN - RECOVER THE GHOST -\n");
				SpawnTheGhost(&m_pVIP->GetAbsOrigin());
			}
			else
			{
				// VIP was killed, end round
				SetWinningTeam(GetOpposingTeam(m_iEscortingTeam), NEO_VICTORY_VIP_ELIMINATION, false, true, false, false);
			}
			
			if (IGameEvent* event = gameeventmanager->CreateEvent("vip_death"))
			{
				event->SetInt("userid", m_pVIP->GetUserID());
				gameeventmanager->FireEvent(event);
			}
		}

		// Check if the vip was escorted during this Think
		int captorTeam, captorClient;
		for (int i = 0; i < m_pGhostCaps.Count(); i++)
		{
			auto pGhostCap = dynamic_cast<CNEOGhostCapturePoint*>(UTIL_EntityByIndex(m_pGhostCaps[i]));
			if (!pGhostCap)
			{
				Assert(false);
				continue;
			}

			// If vip was escorted
			if (pGhostCap->IsGhostCaptured(captorTeam, captorClient))
			{
				// Turn off all capzones
				for (int i = 0; i < m_pGhostCaps.Count(); i++)
				{
					auto pGhostCap = dynamic_cast<CNEOGhostCapturePoint*>(UTIL_EntityByIndex(m_pGhostCaps[i]));
					if (!pGhostCap)
					{
						Assert(false);
						continue;
					}
					pGhostCap->SetActive(false);
				}

				// And then announce team victory
				SetWinningTeam(captorTeam, NEO_VICTORY_VIP_ESCORT, false, true, false, false);

				if (IGameEvent* event = gameeventmanager->CreateEvent("vip_extract"))
				{
					CBasePlayer* pCaptorClient = UTIL_PlayerByIndex(captorClient);
					event->SetInt("userid", pCaptorClient ? pCaptorClient->GetUserID() : INVALID_USER_ID);
					gameeventmanager->FireEvent(event);
				}

				break;
			}
		}
	}

	CheckWinByElimination();
#endif // GAME_DLL
}