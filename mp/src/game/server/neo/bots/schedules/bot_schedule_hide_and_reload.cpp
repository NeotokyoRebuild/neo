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
SET_SCHEDULE_TASKS( CHideAndReloadSchedule )
{
    ADD_TASK( BTASK_SET_FAIL_SCHEDULE, SCHEDULE_RELOAD );
    ADD_TASK( BTASK_RUN, NULL );
    ADD_TASK( BTASK_SAVE_POSITION, NULL );
    ADD_TASK( BTASK_SAVE_COVER_SPOT, NULL );
    ADD_TASK( BTASK_RELOAD, true );
    ADD_TASK( BTASK_MOVE_DESTINATION, NULL );
    ADD_TASK( BTASK_CROUCH, NULL );
    ADD_TASK( BTASK_WAIT, RandomFloat( 1.0f, 2.5f ) );
    ADD_TASK( BTASK_RESTORE_POSITION, NULL );
}

SET_SCHEDULE_INTERRUPTS( CHideAndReloadSchedule )
{
    ADD_INTERRUPT( BCOND_EMPTY_PRIMARY_AMMO );
    ADD_INTERRUPT( BCOND_HELPLESS );
    ADD_INTERRUPT( BCOND_WITHOUT_ENEMY );
    ADD_INTERRUPT( BCOND_LOW_HEALTH );
    ADD_INTERRUPT( BCOND_DEJECTED );
    ADD_INTERRUPT( BCOND_MOBBED_BY_ENEMIES );
    ADD_INTERRUPT( BCOND_GOAL_UNREACHABLE );
    ADD_INTERRUPT( BCOND_HEAR_MOVE_AWAY );
}

//================================================================================
//================================================================================
float CHideAndReloadSchedule::GetDesire() const
{
    if ( !GetDecision()->CanMove() )
        return BOT_DESIRE_NONE;

    if ( !GetDecision()->ShouldCover() )
        return BOT_DESIRE_NONE;

    if ( HasCondition( BCOND_EMPTY_CLIP1_AMMO ) ) {
        if ( GetBot()->IsCombating() || GetBot()->IsAlerted() )
            return 0.92f;
    }

    return BOT_DESIRE_NONE;
}

//================================================================================
//================================================================================
void CHideAndReloadSchedule::TaskRun()
{
    BotTaskInfo_t *pTask = GetActiveTask();

    switch ( pTask->task ) {
        case BTASK_MOVE_DESTINATION:
        case BTASK_CROUCH:
        case BTASK_WAIT:
        {
            // Si no somos noob, entonces cancelamos estas tareas
            // en cuanto tengamos el arma recargada.
            if ( !GetProfile()->IsEasy() ) {
                CBaseWeapon *pWeapon = GetHost()->GetActiveBaseWeapon();

                // Sin arma
                if ( !pWeapon ) {
                    TaskComplete();
                    return;
                }

                // Ya hemos terminado de recargar
#ifdef INSOURCE_DLL
                if ( !pWeapon->IsReloading() || !HasCondition( BCOND_EMPTY_CLIP1_AMMO ) ) {
#else
                if ( !HasCondition( BCOND_EMPTY_CLIP1_AMMO ) ) {
#endif
                    TaskComplete();
                    return;
                }
            }

            BaseClass::TaskRun();
            break;
        }

        default:
        {
            BaseClass::TaskRun();
            break;
        }
    }
}