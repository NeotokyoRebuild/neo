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

#include "in_buttons.h"

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

//================================================================================
//================================================================================
SET_SCHEDULE_TASKS( CDefendSpawnSchedule )
{
    ADD_TASK( BTASK_SET_FAIL_SCHEDULE, SCHEDULE_COVER );
    ADD_TASK( BTASK_SAVE_SPAWN_POSITION, NULL );
    ADD_TASK( BTASK_MOVE_DESTINATION, NULL );
}

SET_SCHEDULE_INTERRUPTS( CDefendSpawnSchedule )
{
    ADD_INTERRUPT( BCOND_EMPTY_PRIMARY_AMMO );
    ADD_INTERRUPT( BCOND_EMPTY_CLIP1_AMMO );
    ADD_INTERRUPT( BCOND_HELPLESS );
    ADD_INTERRUPT( BCOND_SEE_HATE );
    ADD_INTERRUPT( BCOND_SEE_ENEMY );
    ADD_INTERRUPT( BCOND_LIGHT_DAMAGE );
    ADD_INTERRUPT( BCOND_HEAVY_DAMAGE );
    ADD_INTERRUPT( BCOND_REPEATED_DAMAGE );
    ADD_INTERRUPT( BCOND_LOW_HEALTH );
    ADD_INTERRUPT( BCOND_DEJECTED );
    ADD_INTERRUPT( BCOND_CAN_RANGE_ATTACK1 );
    ADD_INTERRUPT( BCOND_CAN_RANGE_ATTACK2 );
    ADD_INTERRUPT( BCOND_CAN_MELEE_ATTACK1 );
    ADD_INTERRUPT( BCOND_CAN_MELEE_ATTACK2 );
    ADD_INTERRUPT( BCOND_BETTER_WEAPON_AVAILABLE );
    ADD_INTERRUPT( BCOND_MOBBED_BY_ENEMIES );
    ADD_INTERRUPT( BCOND_GOAL_UNREACHABLE );

    if ( GetDecision()->ShouldHelpFriends() ) {
        ADD_INTERRUPT( BCOND_SEE_DEJECTED_FRIEND );
    }
}

//================================================================================
//================================================================================
float CDefendSpawnSchedule::GetDesire() const
{
    if ( !GetMemory() )
        return BOT_DESIRE_NONE;

    CDataMemory *memory = GetMemory()->GetDataMemory( "SpawnPosition" );

    if ( memory == NULL )
        return BOT_DESIRE_NONE;

    if ( !GetDecision()->CanMove() )
        return BOT_DESIRE_NONE;

    if ( !GetBot()->IsIdle() )
        return BOT_DESIRE_NONE;

    if ( GetBot()->GetTacticalMode() != TACTICAL_MODE_DEFENSIVE )
        return BOT_DESIRE_NONE;

    if ( GetBot()->GetFollow() && GetBot()->GetFollow()->IsFollowingActive() )
        return BOT_DESIRE_NONE;

    float distance = GetHost()->GetAbsOrigin().DistTo( memory->GetVector() );

    if ( distance < 130.0f )
        return BOT_DESIRE_NONE;

    return 0.05f;
}