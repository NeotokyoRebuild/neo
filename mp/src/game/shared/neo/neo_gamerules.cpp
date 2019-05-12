#include "cbase.h"
#include "neo_gamerules.h"

#ifdef CLIENT_DLL
    #include "c_neo_player.h"
#else
    #include "neo_player.h"
    #include "team.h"
#endif

REGISTER_GAMERULES_CLASS( CNEORules );

BEGIN_NETWORK_TABLE_NOBASE( CNEORules, DT_NEORules )
// NEO TODO (Rain): NEO specific game modes var (CTG/TDM/...)
#ifdef CLIENT_DLL
    //RecvPropInt( RECVINFO( m_iGameMode ) ),
#else
    //SendPropInt( SENDINFO( m_iGameMode ) ),
#endif
END_NETWORK_TABLE()

LINK_ENTITY_TO_CLASS( neo_gamerules, CNEOGameRulesProxy );
IMPLEMENT_NETWORKCLASS_ALIASED( NEOGameRulesProxy, DT_NEOGameRulesProxy );

// NEO TODO (Rain): set accurately
static NEOViewVectors g_NEOViewVectors(
    Vector( 0, 0, 64 ),       //VEC_VIEW (m_vView) 
							  
	Vector(-16, -16, 0 ),	  //VEC_HULL_MIN (m_vHullMin)
	Vector( 16,  16,  72 ),	  //VEC_HULL_MAX (m_vHullMax)
							  					
	Vector(-16, -16, 0 ),	  //VEC_DUCK_HULL_MIN (m_vDuckHullMin)
	Vector( 16,  16,  36 ),	  //VEC_DUCK_HULL_MAX	(m_vDuckHullMax)
	Vector( 0, 0, 28 ),		  //VEC_DUCK_VIEW		(m_vDuckView)
							  					
	Vector(-10, -10, -10 ),	  //VEC_OBS_HULL_MIN	(m_vObsHullMin)
	Vector( 10,  10,  10 ),	  //VEC_OBS_HULL_MAX	(m_vObsHullMax)
							  					
	Vector( 0, 0, 14 ),		  //VEC_DEAD_VIEWHEIGHT (m_vDeadViewHeight)

	Vector(-16, -16, 0 ),	  //VEC_CROUCH_TRACE_MIN (m_vCrouchTraceMin)
	Vector( 16,  16,  60 ),	  //VEC_CROUCH_TRACE_MAX (m_vCrouchTraceMax)

    // NEO specific
    Vector(-24, 0, 50), // vViewLeanLeft
    Vector(24, 0, 50), // vViewLeanRight

    Vector(-33, 0, 0), // vViewAngLeanLeft
    Vector(33, 0, 0), // vViewAngLeanRight

    Vector(-24, -16, 0), // vHullLeanLeftMin
    Vector(16, 16, 58), // vHullLeanLeftMax
    Vector(-16, -16, 0), // vHullLeanRightMin
    Vector(24, 16, 58) // vHullLeanRightMax
);

#ifdef CLIENT_DLL
    void RecvProxy_NEORules( const RecvProp *pProp, void **pOut,
        void *pData, int objectID )
    {
        CNEORules *pRules = NEORules();
        Assert( pRules );
        *pOut = pRules;
    }

    BEGIN_RECV_TABLE( CNEOGameRulesProxy, DT_NEOGameRulesProxy )
        RecvPropDataTable( "neo_gamerules_data", 0, 0,
            &REFERENCE_RECV_TABLE( DT_NEORules ),
            RecvProxy_NEORules )
    END_RECV_TABLE()
#else
    void *SendProxy_NEORules( const SendProp *pProp,
        const void *pStructBase, const void *pData,
        CSendProxyRecipients *pRecipients, int objectID )
    {
        CNEORules *pRules = NEORules();
        Assert( pRules );
        return pRules;
    }

    BEGIN_SEND_TABLE( CNEOGameRulesProxy, DT_NEOGameRulesProxy )
        SendPropDataTable( "neo_gamerules_data", 0,
            &REFERENCE_SEND_TABLE( DT_NEORules ),
            SendProxy_NEORules )
    END_SEND_TABLE()
#endif

CNEORules::CNEORules()
{
#ifndef CLIENT_DLL
    DevMsg("CNEORules serverside ctor\n");
    
    Q_strncpy(g_Teams[TEAM_JINRAI]->m_szTeamname.GetForModify(),
        TEAM_STR_JINRAI, MAX_TEAM_NAME_LENGTH);
    
    Q_strncpy(g_Teams[TEAM_NSF]->m_szTeamname.GetForModify(),
        TEAM_STR_NSF, MAX_TEAM_NAME_LENGTH);
    
    Msg("Server teams %s & %s\n",
        g_Teams[TEAM_JINRAI]->GetName(), g_Teams[TEAM_NSF]->GetName());
    
    for (int i = 1; i <= gpGlobals->maxClients; i++)
    {
        CBasePlayer *player = UTIL_PlayerByIndex(i);
        if (player)
        {
            g_Teams[TEAM_JINRAI]->UpdateClientData(player);
            g_Teams[TEAM_NSF]->UpdateClientData(player);
        }
    }
#endif
}

CNEORules::~CNEORules()
{

}

bool CNEORules::ShouldCollide(int collisionGroup0, int collisionGroup1)
{
    return BaseClass::ShouldCollide(collisionGroup0, collisionGroup1);
}

void CNEORules::Think(void)
{
    BaseClass::Think();
}

void CNEORules::CreateStandardEntities(void)
{
    BaseClass::CreateStandardEntities();
}

int CNEORules::WeaponShouldRespawn(CBaseCombatWeapon *pWep)
{
    return BaseClass::WeaponShouldRespawn(pWep);
}

const char *CNEORules::GetGameDescription(void)
{
    //DevMsg("Querying CNEORules game description\n");
    return BaseClass::GetGameDescription();
}

const CViewVectors *CNEORules::GetViewVectors() const
{
    return &g_NEOViewVectors;
}

const NEOViewVectors* CNEORules::GetNEOViewVectors() const
{
    return &g_NEOViewVectors;
}

float CNEORules::GetMapRemainingTime()
{
    return BaseClass::GetMapRemainingTime();
}

#ifndef CLIENT_DLL
void CNEORules::CleanUpMap()
{
    BaseClass::CleanUpMap();
}

void CNEORules::CheckRestartGame()
{
    BaseClass::CheckRestartGame();
}

void CNEORules::RestartGame()
{
    BaseClass::RestartGame();
}
#endif