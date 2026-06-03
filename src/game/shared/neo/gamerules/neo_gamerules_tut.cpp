#include "cbase.h"
#include "neo_gamerules_tut.h"

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

REGISTER_GAMERULES_CLASS( CNEORulesTUT );

BEGIN_NETWORK_TABLE_NOBASE( CNEORulesTUT, DT_NEORulesTUT )
END_NETWORK_TABLE()

LINK_ENTITY_TO_CLASS( neo_gamerules_tut, CNEOGameRulesTUTProxy );
IMPLEMENT_NETWORKCLASS_ALIASED( NEOGameRulesTUTProxy, DT_NEOGameRulesTUTProxy );

#ifdef CLIENT_DLL
	void RecvProxy_NEORulesTUT( const RecvProp *pProp, void **pOut,
		void *pData, int objectID )
	{
		CNEORulesTUT *pRules = NEORulesTUT();
		Assert( pRules );
		*pOut = pRules;
	}

	BEGIN_RECV_TABLE( CNEOGameRulesTUTProxy, DT_NEOGameRulesTUTProxy )
		RecvPropDataTable( "neo_gamerules_tut_data", 0, 0,	
			&REFERENCE_RECV_TABLE( DT_NEORulesTUT ), 
			RecvProxy_NEORulesTUT )
	END_RECV_TABLE()
#else
	void *SendProxy_NEORulesTUT( const SendProp *pProp,
		const void *pStructBase, const void *pData,
		CSendProxyRecipients *pRecipients, int objectID )
	{
		CNEORulesTUT *pRules = NEORulesTUT();
		Assert( pRules );
		return pRules;
	}

	BEGIN_SEND_TABLE(CNEOGameRulesTUTProxy, DT_NEOGameRulesTUTProxy)
		SendPropDataTable("neo_gamerules_tut_data", 0,
			&REFERENCE_SEND_TABLE(DT_NEORulesTUT),
			SendProxy_NEORulesTUT)
		END_SEND_TABLE()
#endif

//CNEORulesTUT::CNEORulesTUT()
//{
//}
//
//CNEORulesTUT::~CNEORulesTUT()
//{
//}

void CNEORulesTUT::FireGameEvent(IGameEvent* event)
{
	BaseClass::FireGameEvent(event);
}

float CNEORulesTUT::GetRoundRemainingTime() const
{
#ifdef GAME_DLL
	Assert(false); // Shouldn't be calling this server side
#endif // GAME_DLL
	return 0.f;
}

#ifdef GAME_DLL
bool CNEORulesTUT::FPlayerCanRespawn(CBasePlayer* pPlayer)
{
	if (pPlayer->IsAlive())
		return false;

	return true;
}

const int CNEORulesTUT::GetScoreLimit() const
{
	Assert(false);
	return 1;
}

const int CNEORulesTUT::GetRoundLimit() const
{
	Assert(false);
	return 1;
}

void CNEORulesTUT::RoundTimeout()
{
	Assert(false);
}
#endif // GAME_DLL

void CNEORulesTUT::Think()
{
#ifdef GAME_DLL
	UpdateFromGameConfig();

	{
		// This is kind of wonky, but we only need it for the tutorial, in order to play the dummy beacon sounds...
		if (!m_pGhost)
		{
			auto pEnt = gEntList.FirstEnt();
			while (pEnt)
			{
				if (dynamic_cast<CWeaponGhost*>(pEnt))
				{
					m_pGhost = static_cast<CWeaponGhost*>(pEnt);
					m_hGhost = m_pGhost;
					return;
				}
				pEnt = gEntList.NextEnt(pEnt);
			}
		}
		if (m_pGhost) m_pGhost->UpdateNearestGhostBeaconDist();
	}
#endif // GAME_DLL
}