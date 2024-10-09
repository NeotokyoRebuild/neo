//========= Copyright © 1996-2005, Valve Corporation, All rights reserved. ============//
// Authors: 
// Iván Bravo Bravo (linkedin.com/in/ivanbravobravo), 2017

#include "cbase.h"
#include "bots\bot.h"

#ifdef INSOURCE_DLL
#include "in_utils.h"
#else
#include "bots\in_utils.h"
#endif

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

//================================================================================
// Commands
//================================================================================

DECLARE_CHEAT_COMMAND( bot_follow_tolerance, "130.0", "" )

//================================================================================
//================================================================================
void CBotFollow::Update()
{
    VPROF_BUDGET( "CBotFollow::Update", VPROF_BUDGETGROUP_BOTS );

    if ( !GetLocomotion() )
        return;

    if ( !GetDecision()->ShouldFollow() )
        return;

    CBaseEntity *pEntity = GetEntity();

    if ( !pEntity || !pEntity->IsAlive() ) {
        Stop();
        return;
    }

    GetLocomotion()->Approach( pEntity, GetTolerance(), PRIORITY_FOLLOWING );
}

//================================================================================
//================================================================================
void CBotFollow::Start( CBaseEntity *pEntity, bool enabled )
{
    if ( pEntity ) {
        if ( !pEntity->IsAlive() )
            return;

        // wut?
        if ( pEntity == GetHost() ) {
            Stop();
            return;
        }
    }

    m_Entity = pEntity;
    SetEnabled( enabled );
}

//================================================================================
//================================================================================
void CBotFollow::Start( const char *pEntityName, bool enabled )
{
    CBaseEntity *pEntity = gEntList.FindEntityByName( NULL, pEntityName );

    if ( !pEntity )
        return;

    Start( pEntity, enabled );
}

//================================================================================
//================================================================================
void CBotFollow::Stop()
{
    m_Entity = NULL;
    m_bEnabled = false;
}

//================================================================================
//================================================================================
float CBotFollow::GetTolerance()
{
    return bot_follow_tolerance.GetFloat();
}

//================================================================================
// Returns if we are following a player
//================================================================================
bool CBotFollow::IsFollowingPlayer()
{
    if ( !GetEntity() )
        return false;

    return GetEntity()->IsPlayer();
}

//================================================================================
// Returns if we are following a bot player
//================================================================================
bool CBotFollow::IsFollowingBot()
{
    if ( !IsFollowingPlayer() )
        return false;

    CPlayer *pPlayer = ToInPlayer( GetEntity() );
    Assert( pPlayer );

    return pPlayer->IsBot();
}

//================================================================================
// Returns if we are following a human player
//================================================================================
bool CBotFollow::IsFollowingHuman()
{
    if ( !IsFollowingPlayer() )
        return false;

    CPlayer *pPlayer = ToInPlayer( GetEntity() );
    Assert( pPlayer );

    return !(pPlayer->IsBot());
}