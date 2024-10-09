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
SET_SCHEDULE_TASKS( CCallBackupSchedule )
{
    ADD_TASK( BTASK_SAVE_POSITION, NULL );
    ADD_TASK( BTASK_RUN, NULL );
    ADD_TASK( BTASK_SAVE_FAR_COVER_SPOT, 1000.0f );
    ADD_TASK( BTASK_CROUCH, NULL );
    ADD_TASK( BTASK_HEAL, NULL );
    ADD_TASK( BTASK_RELOAD, NULL );
    ADD_TASK( BTASK_WAIT, RandomFloat( 2.0f, 6.0f ) );
    ADD_TASK( BTASK_CALL_FOR_BACKUP, NULL );
    ADD_TASK( BTASK_WAIT, RandomFloat( 2.0f, 6.0f ) );
    ADD_TASK( BTASK_RESTORE_POSITION, NULL );
}

SET_SCHEDULE_INTERRUPTS( CCallBackupSchedule )
{
    ADD_INTERRUPT( BCOND_DEJECTED );
    ADD_INTERRUPT( BCOND_GOAL_UNREACHABLE );
}