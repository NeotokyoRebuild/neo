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

#ifdef INSOURCE_DLL

//================================================================================
//================================================================================
SET_SCHEDULE_TASKS( CHelpDejectedFriendSchedule )
{
    CDataMemory *memory = GetMemory()->GetDataMemory( "DejectedFriend" );
    Assert( memory );

    ADD_TASK( BTASK_SAVE_POSITION, NULL );
    ADD_TASK( BTASK_RUN, NULL );
    ADD_TASK( BTASK_MOVE_DESTINATION, memory->GetEntity() );
    ADD_TASK( BTASK_AIM, memory->GetEntity() );
    ADD_TASK( BTASK_HELP, NULL );
    ADD_TASK( BTASK_RESTORE_POSITION, NULL );
}

SET_SCHEDULE_INTERRUPTS( CHelpDejectedFriendSchedule )
{
    ADD_INTERRUPT( BCOND_REPEATED_DAMAGE );
    ADD_INTERRUPT( BCOND_HEAVY_DAMAGE );
    ADD_INTERRUPT( BCOND_LOW_HEALTH );
    ADD_INTERRUPT( BCOND_DEJECTED );
}

//================================================================================
//================================================================================
bool CHelpDejectedFriendSchedule::ShouldHelp()
{
    if ( !GetMemory() )
        return false;

    CDataMemory *memory = GetMemory()->GetDataMemory( "DejectedFriend" );

    if ( !memory )
        return false;

    CPlayer *pFriend = ToInPlayer( memory->GetEntity() );

    if ( !pFriend )
        return false;

    // Estamos en modo defensivo, si estamos en combate solo ayudarlo si esta cerca
    if ( GetBot()->GetTacticalMode() == TACTICAL_MODE_DEFENSIVE ) {
        float distance = GetAbsOrigin().DistTo( pFriend->GetAbsOrigin() );
        const float tolerance = 400.0f;

        if ( !IsIdle() && distance >= tolerance )
            return false;
    }

    return true;
}

//================================================================================
//================================================================================
float CHelpDejectedFriendSchedule::GetDesire() const
{
    if ( !GetMemory() )
        return BOT_DESIRE_NONE;

    CDataMemory *memory = GetMemory()->GetDataMemory( "DejectedFriend" );

    if ( !memory )
        return BOT_DESIRE_NONE;
    
	if ( !GetDecision()->CanMove() )
		return BOT_DESIRE_NONE;

	if ( !HasCondition(BCOND_SEE_DEJECTED_FRIEND) )
		return BOT_DESIRE_NONE;

	if ( GetBot()->IsCombating() )
		return BOT_DESIRE_NONE;

	if ( !GetDecision()->ShouldHelpFriends() )
		return BOT_DESIRE_NONE;

	return 0.51f;
}

//================================================================================
//================================================================================
void CHelpDejectedFriendSchedule::TaskRun()
{
    CDataMemory *memory = GetMemory()->GetDataMemory( "DejectedFriend" );
    Assert( memory );

    CPlayer *pFriend = ToInPlayer( memory->GetEntity() );

    BotTaskInfo_t *pTask = GetActiveTask();

    switch ( pTask->task ) {
        // Recargamos
        case BTASK_HELP:
        {
            // Miramos a nuestro amigo
            if ( GetBot()->GetVision() ) {
                GetBot()->GetVision()->LookAt( "Dejected Friend", pFriend->GetAbsOrigin(), PRIORITY_CRITICAL, 1.0f );

                // Empezamos a ayudarlo
                if ( GetBot()->GetVision()->IsAimReady() ) {
                    InjectButton( IN_USE );
                }
            }

            // Ya no esta vivo o se ha levantado
            if ( !pFriend || !pFriend->IsAlive() || pFriend->IsBeingHelped() || !pFriend->IsDejected() ) {
                TaskComplete();
            }

            break;
        }

        default:
        {
            // Nuestro amigo ha muerto o 
            // ya esta siendo ayudado
            if ( !pFriend || !pFriend->IsAlive() || pFriend->IsBeingHelped() || !pFriend->IsDejected() ) {
                GetMemory()->ForgetData( "Dejected Friend" );
                Fail( "The friend." );
                return;
            }

            BaseClass::TaskRun();
            break;
        }
    }
}

#endif