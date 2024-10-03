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
SET_SCHEDULE_TASKS( CCoverSchedule )
{
    ADD_TASK( BTASK_RUN, NULL );
    ADD_TASK( BTASK_SAVE_POSITION, NULL );
    ADD_TASK( BTASK_SAVE_COVER_SPOT, NULL );
    ADD_TASK( BTASK_MOVE_DESTINATION, NULL );
    ADD_TASK( BTASK_CROUCH, NULL );
    ADD_TASK( BTASK_RELOAD_SAFE, NULL );
    ADD_TASK( BTASK_WAIT, RandomFloat( 1.0f, 3.0f ) );
    ADD_TASK( BTASK_RESTORE_POSITION, NULL );
}

SET_SCHEDULE_INTERRUPTS( CCoverSchedule )
{
    ADD_INTERRUPT( BCOND_HELPLESS );
    ADD_INTERRUPT( BCOND_WITHOUT_ENEMY );
    ADD_INTERRUPT( BCOND_LOW_HEALTH );
    ADD_INTERRUPT( BCOND_DEJECTED );
    ADD_INTERRUPT( BCOND_MOBBED_BY_ENEMIES );
    ADD_INTERRUPT( BCOND_GOAL_UNREACHABLE );
}

//================================================================================
//================================================================================
float CCoverSchedule::GetDesire() const
{
    if ( !GetDecision()->CanMove() )
        return BOT_DESIRE_NONE;

    if ( !GetDecision()->ShouldCover() )
        return BOT_DESIRE_NONE;

    if ( GetBot()->IsCombating() || GetBot()->IsAlerted() ) {
        if ( GetProfile()->GetSkill() >= SKILL_HARD && GetDecision()->IsDangerousEnemy() ) {
            if ( HasCondition( BCOND_LIGHT_DAMAGE ) )
                return 0.82f;
        }

        if ( HasCondition( BCOND_REPEATED_DAMAGE ) )
            return 0.83f;

        if ( HasCondition( BCOND_HEAVY_DAMAGE ) )
            return 0.9f;

        //if ( HasCondition(BCOND_HEAR_BULLET_IMPACT_SNIPER) )
            //return 0.93f;
    }

    return BOT_DESIRE_NONE;
}

//================================================================================
//================================================================================
SET_SCHEDULE_TASKS( CHideSchedule )
{
    ADD_TASK( BTASK_RUN, NULL );
    ADD_TASK( BTASK_SAVE_FAR_COVER_SPOT, NULL );
    ADD_TASK( BTASK_MOVE_DESTINATION, NULL );
    ADD_TASK( BTASK_RELOAD, NULL );
    ADD_TASK( BTASK_WAIT, RandomFloat( 1.0f, 3.0f ) );
}


SET_SCHEDULE_INTERRUPTS( CHideSchedule )
{
    ADD_INTERRUPT( BCOND_WITHOUT_ENEMY );
    ADD_INTERRUPT( BCOND_LOW_HEALTH );
    ADD_INTERRUPT( BCOND_DEJECTED );
    ADD_INTERRUPT( BCOND_BETTER_WEAPON_AVAILABLE );
    ADD_INTERRUPT( BCOND_GOAL_UNREACHABLE );
}

//================================================================================
//================================================================================
float CHideSchedule::GetDesire() const
{
    if ( !GetDecision()->CanMove() )
        return BOT_DESIRE_NONE;

    if ( GetBot()->IsCombating() || GetBot()->IsAlerted() ) {
        if ( HasCondition( BCOND_HELPLESS ) )
            return 0.94f;
    }

    return BOT_DESIRE_NONE;
}