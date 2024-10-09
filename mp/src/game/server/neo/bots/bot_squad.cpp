//========= Copyright © 1996-2005, Valve Corporation, All rights reserved. ============//
// Authors: 
// Iván Bravo Bravo (linkedin.com/in/ivanbravobravo), 2017

#include "cbase.h"
#include "bots\bot_squad.h"
#include "bots\bot.h"

#ifdef INSOURCE_DLL
#include "in_player.h"
#include "in_utils.h"
#include "in_gamerules.h"
#else
#include "bots\in_utils.h"
#endif

#include "squad.h"
#include "squad_manager.h"

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

//================================================================================
// Información y Red
//================================================================================

LINK_ENTITY_TO_CLASS( bot_squad, CBotSquad );

BEGIN_DATADESC( CBotSquad )
    DEFINE_KEYFIELD( m_nSquadName, FIELD_STRING, "SquadName" ),
    DEFINE_KEYFIELD( m_iSquadLimit, FIELD_INTEGER, "SquadLimit" ),
    DEFINE_KEYFIELD( m_iSquadTacticalMode, FIELD_INTEGER, "SquadTacticalMode" ),
    DEFINE_KEYFIELD( m_iSquadStrategie, FIELD_INTEGER, "SquadStrategie" ),
    DEFINE_KEYFIELD( m_iSquadSkill, FIELD_INTEGER, "SquadSkill" ),
    DEFINE_KEYFIELD( m_bFollowLeader, FIELD_BOOLEAN, "FollowLeader" ),

    // Inputs
    DEFINE_INPUTFUNC( FIELD_VOID, "Destroy", InputDestroy ),

    // Outputs
    DEFINE_OUTPUT( m_OnMemberDead, "OnMemberDead" ),
    DEFINE_OUTPUT( m_OnReportEnemy, "OnReportEnemy" ),
    DEFINE_OUTPUT( m_OnReportPlayerEnemy, "OnReportPlayerEnemy" ),
    DEFINE_OUTPUT( m_OnReportHumanEnemy, "OnReportHumanEnemy" ),
    DEFINE_OUTPUT( m_OnReportDamage, "ReportDamage" ),
    DEFINE_OUTPUT( m_OnNewLeader, "OnNewLeader" ),
END_DATADESC()

//================================================================================
// Creación en el mapa
//================================================================================
void CBotSquad::Spawn()
{
    SetSolid( SOLID_NONE );

    if ( m_nSquadName == NULL_STRING ) {
        Assert( !"m_nSquadName == NULL_STRING" );
        UTIL_Remove( this );
        return;
    }

    CSquad *pSquad = TheSquads->GetOrCreateSquad( STRING( m_nSquadName ) );
    Assert( pSquad );

    pSquad->SetController( this );
    pSquad->SetMemberLimit( m_iSquadLimit );
    pSquad->SetTacticalMode( m_iSquadTacticalMode );
    pSquad->SetStrategie( (BotStrategie)m_iSquadStrategie );
    pSquad->SetSkill( m_iSquadSkill );
    pSquad->SetFollowLeader( m_bFollowLeader );
}

void CBotSquad::InputDestroy( inputdata_t &inputdata ) 
{
}
