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
		
extern bool RespawnWithRet(CBaseEntity *pEdict, bool fCopyCorpse);

CNEORulesTDM::CNEORulesTDM()
{

}

CNEORulesTDM::~CNEORulesTDM()
{
}

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

void CNEORulesTDM::Think()
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
#endif // GAME_DLL
}