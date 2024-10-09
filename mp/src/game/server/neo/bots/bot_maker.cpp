//========= Copyright © 1996-2005, Valve Corporation, All rights reserved. ============//
// Authors: 
// Iván Bravo Bravo (linkedin.com/in/ivanbravobravo), 2017

#include "cbase.h"
#include "bots\bot_maker.h"
#include "bots\bot.h"

#ifdef INSOURCE_DLL
#include "in_player.h"
#include "in_utils.h"
#include "players_system.h"
#include "in_gamerules.h"
#else
#include "bots\in_utils.h"
#endif

#include "eventqueue.h"

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

extern ConVar ai_inhibit_spawners;

//================================================================================
// Entity and network information
//================================================================================

LINK_ENTITY_TO_CLASS( info_bot_spawn, CBotSpawn );

BEGIN_DATADESC( CBotSpawn )
    DEFINE_KEYFIELD( m_nBotTargetname, FIELD_STRING, "BotTargetname" ),
    DEFINE_KEYFIELD( m_nBotPlayername, FIELD_STRING, "Playername" ),
    DEFINE_KEYFIELD( m_nBotSquadname, FIELD_STRING, "BotSquad" ),
    DEFINE_KEYFIELD( m_nAdditionalEquipment, FIELD_STRING, "AdditionalEquipment" ),
    DEFINE_KEYFIELD( m_iBotTeam, FIELD_INTEGER, "PlayerTeam" ),
    DEFINE_KEYFIELD( m_iBotClass, FIELD_INTEGER, "PlayerClass" ),
    DEFINE_KEYFIELD( m_iBotSkill, FIELD_INTEGER, "BotSkill" ),
    DEFINE_KEYFIELD( m_iBotTacticalMode, FIELD_INTEGER, "BotTacticalMode" ),
	DEFINE_KEYFIELD( m_iBlockLookAround, FIELD_INTEGER, "BlockLookAround" ),
    DEFINE_KEYFIELD( m_iPerformance, FIELD_INTEGER, "Performance" ),
	DEFINE_KEYFIELD( m_nFollowEntity, FIELD_STRING, "FollowEntity" ),
    DEFINE_KEYFIELD( m_bDisabledMovement, FIELD_BOOLEAN, "DisableMovement" ),
    DEFINE_KEYFIELD( m_bIsLeader, FIELD_BOOLEAN, "IsLeader" ),

    // Inputs
    DEFINE_INPUTFUNC( FIELD_VOID, "Spawn", InputSpawn ),
	DEFINE_INPUTFUNC( FIELD_VOID, "Respawn", InputRespawn ),
    DEFINE_INPUTFUNC( FIELD_VOID, "Enable", InputEnable ),
    DEFINE_INPUTFUNC( FIELD_VOID, "Disable", InputDisable ),
    DEFINE_INPUTFUNC( FIELD_VOID, "Toggle", InputToggle ),

    DEFINE_INPUTFUNC( FIELD_INTEGER, "SetSkill", InputSetSkill ),
    DEFINE_INPUTFUNC( FIELD_INTEGER, "SetTacticalMode", InputSetTacticalMode ),
    DEFINE_INPUTFUNC( FIELD_INTEGER, "BlockLook", InputBlockLook ),
    DEFINE_INPUTFUNC( FIELD_STRING, "SetSquad", InputSetSquad ),
    DEFINE_INPUTFUNC( FIELD_VOID, "DisableMovement", InputDisableMovement ),
    DEFINE_INPUTFUNC( FIELD_VOID, "EnableMovement", InputEnableMovement ),
    DEFINE_INPUTFUNC( FIELD_VOID, "StartPeaceful", InputStartPeaceful ),
    DEFINE_INPUTFUNC( FIELD_VOID, "StopPeaceful", InputStopPeaceful ),

    DEFINE_INPUTFUNC( FIELD_EHANDLE, "DriveTo", InputDriveTo ),

    // Outputs
    DEFINE_OUTPUT( m_OnSpawn, "OnSpawn" ),
    DEFINE_OUTPUT( m_OnBotDead, "OnBotDead" ),
END_DATADESC()

//================================================================================
//================================================================================
void CBotSpawn::Spawn()
{
    m_bDisabled = false;
    m_pPlayer = NULL;

    SetSolid( SOLID_NONE );

    // Create the bot immediately
    if ( HasSpawnFlags( SF_SPAWN_IMMEDIATELY ) ) {
        g_EventQueue.AddEvent( this, "Spawn", 1.0f, this, this );
    }
}

//================================================================================
//================================================================================
void CBotSpawn::DeathNotice( CBaseEntity *pVictim )
{
    CPlayer *pPlayer = ToInPlayer( pVictim );
    AssertMsg( pPlayer && pPlayer->IsBot(), "Death Notice from an entity that is not a Bot." );

    m_OnBotDead.FireOutput( this, this );

    if ( HasSpawnFlags( SF_KICK_ON_DEAD ) ) {
#ifdef INSOURCE_DLL
        pPlayer->Kick();
#else
        engine->ServerCommand( UTIL_VarArgs( "kickid %i\n", pPlayer->GetPlayerInfo()->GetUserID() ) );
#endif

        if ( pPlayer == GetPlayer() ) {
            m_pPlayer = NULL;
        }
    }
    else {
        g_EventQueue.AddEvent( this, "Respawn", 1.0f, this, this );
    }
}

//================================================================================
// Returns if we can create a Bot.
//================================================================================
bool CBotSpawn::CanMake( bool bRespawn )
{
    if ( ai_inhibit_spawners.GetBool() )
        return false;

    if ( m_bDisabled )
        return false;

    if ( !bRespawn ) {
        if ( HasSpawnFlags( SF_ONLY_ONE_ACTIVE_BOT ) && GetPlayer() ) {
            return false;
        }
    }

    if ( HasSpawnFlags( SF_HIDE_FROM_PLAYERS ) ) {
#ifdef INSOURCE_DLL
        CUsersRecipientFilter filter;
        if ( ThePlayersSystem->IsAbleToSee( this, filter ) )
            return false;
#else
        for ( int it = 0; it <= gpGlobals->maxClients; ++it ) {
            CPlayer *pPlayer = ToInPlayer( UTIL_PlayerByIndex( it ) );

            if ( !pPlayer )
                continue;

            if ( !pPlayer->IsAlive() )
                continue;

            if ( pPlayer->IsAbleToSee(this, CBaseCombatCharacter::USE_FOV ) )
                return false;
        }
#endif
    }

    return true;
}

//================================================================================
// Create a Bot
//================================================================================
void CBotSpawn::SpawnBot()
{
    if ( !CanMake() )
        return;

    const char *pPlayername = ( m_nBotPlayername != NULL_STRING ) ? STRING(m_nBotPlayername) : NULL;

    m_pPlayer = CreateBot( pPlayername, NULL, NULL );
    Assert( m_pPlayer );

    if ( !m_pPlayer )
        return;

    SetUpBot();
}

//================================================================================
// Prepare the Bot with the configuration provided
//================================================================================
void CBotSpawn::SetUpBot()
{
    IBot *pBot = GetPlayer()->GetBotController();
    Assert( pBot );

    pBot->SetSkill( m_iBotSkill );
    pBot->SetTacticalMode( m_iBotTacticalMode );
    pBot->SetPerformance( (BotPerformance)m_iPerformance );

    if ( m_bDisabledMovement ) {
        if ( pBot->GetLocomotion() ) {
            pBot->GetLocomotion()->SetDisabled( m_bDisabledMovement );
        }
    }

    if ( m_iBlockLookAround > 0 ) {
        if ( pBot->GetMemory() ) {
            pBot->GetMemory()->UpdateDataMemory( "BlockLookAround", m_iBlockLookAround, m_iBlockLookAround );
        }
    }

    if ( m_nFollowEntity != NULL_STRING ) {
        if ( pBot->GetFollow() ) {
            pBot->GetFollow()->Start( STRING( m_nFollowEntity ) );
        }
    }

    if ( HasSpawnFlags( SF_PEACEFUL ) ) {
        pBot->SetPeaceful( true );
    }

    if ( m_nBotTargetname != NULL_STRING ) {
        GetPlayer()->SetName( m_nBotTargetname );
    }

    GetPlayer()->ChangeTeam( m_iBotTeam );

#ifdef INSOURCE_DLL
    GetPlayer()->SetPlayerClass( m_iBotClass );
#endif

    GetPlayer()->SetOwnerEntity( this );

    if ( m_nBotSquadname != NULL_STRING ) {
        GetPlayer()->SetSquad( STRING( m_nBotSquadname ) );

        if ( m_bIsLeader ) {
            GetPlayer()->GetSquad()->SetLeader( GetPlayer() );
        }
    }

    if ( HasSpawnFlags( SF_USE_SPAWNER_POSITION ) ) {
        if ( pBot->GetMemory() ) {
            pBot->GetMemory()->UpdateDataMemory( "SpawnPosition", GetAbsOrigin(), -1.0f );
        }

        GetPlayer()->Teleport( &GetAbsOrigin(), &GetAbsAngles(), NULL );
    }

    if ( m_nAdditionalEquipment != NULL_STRING ) {
        GetPlayer()->GiveNamedItem( STRING( m_nAdditionalEquipment ) );

#ifdef INSOURCE_DLL
#ifdef DEBUG
        GetPlayer()->GiveAmmo( 60, "AMMO_TYPE_SNIPERRIFLE" );
        GetPlayer()->GiveAmmo( 300, "AMMO_TYPE_ASSAULTRIFLE" );
        GetPlayer()->GiveAmmo( 999, "AMMO_TYPE_PISTOL" );
        GetPlayer()->GiveAmmo( 300, "AMMO_TYPE_SHOTGUN" );
#endif
#endif
    }

    m_OnSpawn.Set( GetPlayer(), GetPlayer(), this );
}

//================================================================================
//================================================================================
void CBotSpawn::InputSpawn( inputdata_t &inputdata )
{
    SpawnBot();
}

//================================================================================
//================================================================================
void CBotSpawn::InputRespawn( inputdata_t & inputdata ) 
{
    if ( !GetPlayer() ) {
        SpawnBot();
        return;
    }

    /*if ( !CanMake(true) ) {
        g_EventQueue.AddEvent( this, "Respawn", 1.0f, this, this );
        return;
    }*/

	SetUpBot();
}

//================================================================================
//================================================================================
void CBotSpawn::InputEnable( inputdata_t &inputdata )
{
    m_bDisabled = false;
}

//================================================================================
//================================================================================
void CBotSpawn::InputDisable( inputdata_t &inputdata )
{
    m_bDisabled = true;
}

//================================================================================
//================================================================================
void CBotSpawn::InputToggle( inputdata_t &inputdata )
{
    m_bDisabled = !m_bDisabled;
}

//================================================================================
//================================================================================
void CBotSpawn::InputSetSkill( inputdata_t &inputdata ) 
{
    m_iBotSkill = inputdata.value.Int();

    if ( GetPlayer() && GetPlayer()->GetBotController() )
    {
        GetPlayer()->GetBotController()->SetSkill( m_iBotSkill );
    }
}

//================================================================================
//================================================================================
void CBotSpawn::InputSetTacticalMode( inputdata_t & inputdata )
{
    m_iBotTacticalMode = inputdata.value.Int();

    if ( GetPlayer() && GetPlayer()->GetBotController() )
        GetPlayer()->GetBotController()->SetTacticalMode( m_iBotTacticalMode );
}

//================================================================================
//================================================================================
void CBotSpawn::InputBlockLook( inputdata_t &inputdata ) 
{
    m_iBlockLookAround = inputdata.value.Int();

    if ( GetPlayer() && GetPlayer()->GetBotController() && GetPlayer()->GetBotController()->GetMemory() ) {
        GetPlayer()->GetBotController()->GetMemory()->UpdateDataMemory( "BlockLookAround", m_iBlockLookAround, -1.0f );
    }
}

//================================================================================
//================================================================================
void CBotSpawn::InputSetSquad( inputdata_t &inputdata ) 
{
    m_nBotSquadname = MAKE_STRING( inputdata.value.String() );

    if ( GetPlayer() && GetPlayer()->GetBotController() ) {
        GetPlayer()->GetBotController()->SetSquad( inputdata.value.String() );
        GetPlayer()->GetBotController()->SetSkill( m_iBotSkill );
    }
}

//================================================================================
//================================================================================
void CBotSpawn::InputDisableMovement( inputdata_t &inputdata ) 
{
    m_bDisabledMovement = true;

    if ( GetPlayer() && GetPlayer()->GetBotController() && GetPlayer()->GetBotController()->GetLocomotion() ) {
        GetPlayer()->GetBotController()->GetLocomotion()->SetDisabled( m_bDisabledMovement );
    }
}

//================================================================================
//================================================================================
void CBotSpawn::InputEnableMovement( inputdata_t &inputdata ) 
{
    m_bDisabledMovement = false;

    if ( GetPlayer() && GetPlayer()->GetBotController() && GetPlayer()->GetBotController()->GetLocomotion() ) {
        GetPlayer()->GetBotController()->GetLocomotion()->SetDisabled( m_bDisabledMovement );
    }
}

void CBotSpawn::InputStartPeaceful( inputdata_t & inputdata )
{
    if ( !GetPlayer() )
        return;

    GetPlayer()->GetBotController()->SetPeaceful( true );
}

void CBotSpawn::InputStopPeaceful( inputdata_t & inputdata )
{
    if ( !GetPlayer() )
        return;

    GetPlayer()->GetBotController()->SetPeaceful( false );
}

void CBotSpawn::InputDriveTo( inputdata_t & inputdata )
{
    if( !GetPlayer() )
        return;

    if ( !GetPlayer()->GetBotController()->GetLocomotion() )
        return;

    CBaseEntity *pTarget = inputdata.value.Entity().Get();

    GetPlayer()->GetBotController()->GetLocomotion()->DriveTo( "Input DriveTo", pTarget, PRIORITY_CRITICAL );
}
