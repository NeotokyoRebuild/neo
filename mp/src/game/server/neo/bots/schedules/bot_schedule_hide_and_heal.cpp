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
SET_SCHEDULE_TASKS( CHideAndHealSchedule )
{
    ADD_TASK( BTASK_RUN, NULL );
    ADD_TASK( BTASK_SAVE_POSITION, NULL );
    ADD_TASK( BTASK_SAVE_COVER_SPOT, 1500.0f );
    ADD_TASK( BTASK_MOVE_DESTINATION, NULL );
    ADD_TASK( BTASK_CROUCH, NULL );
    ADD_TASK( BTASK_HEAL, NULL );
    ADD_TASK( BTASK_RELOAD_SAFE, NULL );
    ADD_TASK( BTASK_WAIT, RandomFloat( 1.0f, 3.0f ) );
    ADD_TASK( BTASK_RESTORE_POSITION, NULL );
}

SET_SCHEDULE_INTERRUPTS( CHideAndHealSchedule )
{
    ADD_INTERRUPT( BCOND_DEJECTED );
    ADD_INTERRUPT( BCOND_MOBBED_BY_ENEMIES );
    ADD_INTERRUPT( BCOND_GOAL_UNREACHABLE );
}

//================================================================================
//================================================================================
float CHideAndHealSchedule::GetDesire() const
{
    if ( !GetDecision()->CanMove() )
        return BOT_DESIRE_NONE;

    if ( !HasCondition( BCOND_LOW_HEALTH ) )
        return BOT_DESIRE_NONE;

    if ( !GetDecision()->ShouldCover() )
        return BOT_DESIRE_NONE;

    return 0.95f;
}