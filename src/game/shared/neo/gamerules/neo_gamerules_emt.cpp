#include "cbase.h"
#include "neo_gamerules_emt.h"

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

REGISTER_GAMERULES_CLASS( CNEORulesEMT );

BEGIN_NETWORK_TABLE_NOBASE( CNEORulesEMT, DT_NEORulesEMT )
END_NETWORK_TABLE()

LINK_ENTITY_TO_CLASS( neo_gamerules_emt, CNEOGameRulesEMTProxy );
IMPLEMENT_NETWORKCLASS_ALIASED( NEOGameRulesEMTProxy, DT_NEOGameRulesEMTProxy );

#ifdef CLIENT_DLL
	void RecvProxy_NEORulesEMT( const RecvProp *pProp, void **pOut,
		void *pData, int objectID )
	{
		CNEORulesEMT *pRules = NEORulesEMT();
		Assert( pRules );
		*pOut = pRules;
	}

	BEGIN_RECV_TABLE( CNEOGameRulesEMTProxy, DT_NEOGameRulesEMTProxy )
		RecvPropDataTable( "neo_gamerules_emt_data", 0, 0,	
			&REFERENCE_RECV_TABLE( DT_NEORulesEMT ), 
			RecvProxy_NEORulesEMT )
	END_RECV_TABLE()
#else
	void *SendProxy_NEORulesEMT( const SendProp *pProp,
		const void *pStructBase, const void *pData,
		CSendProxyRecipients *pRecipients, int objectID )
	{
		CNEORulesEMT *pRules = NEORulesEMT();
		Assert( pRules );
		return pRules;
	}

	BEGIN_SEND_TABLE(CNEOGameRulesEMTProxy, DT_NEOGameRulesEMTProxy)
		SendPropDataTable("neo_gamerules_emt_data", 0,
			&REFERENCE_SEND_TABLE(DT_NEORulesEMT),
			SendProxy_NEORulesEMT)
		END_SEND_TABLE()
#endif

//CNEORulesEMT::CNEORulesEMT()
//{
//}
//
//CNEORulesEMT::~CNEORulesEMT()
//{
//}

void CNEORulesEMT::FireGameEvent(IGameEvent* event)
{
	BaseClass::FireGameEvent(event);
}

float CNEORulesEMT::GetRoundRemainingTime() const
{
#ifdef GAME_DLL
	Assert(false); // Shouldn't be calling this server side
#endif // GAME_DLL
	return 0.f;
}

#ifdef GAME_DLL
bool CNEORulesEMT::FPlayerCanRespawn(CBasePlayer* pPlayer)
{
	return true;
}

const int CNEORulesEMT::GetScoreLimit() const
{
	Assert(false);
	return 1;
}

const int CNEORulesEMT::GetRoundLimit() const
{
	Assert(false);
	return 1;
}

void CNEORulesEMT::RoundTimeout()
{
	Assert(false);
}
#endif // GAME_DLL

void CNEORulesEMT::Think()
{
#ifdef GAME_DLL
	UpdateFromGameConfig();
#endif // GAME_DLL
}