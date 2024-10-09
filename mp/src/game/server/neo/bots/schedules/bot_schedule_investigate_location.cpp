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
//================================================================================
SET_SCHEDULE_TASKS( CInvestigateLocationSchedule )
{
    CDataMemory *memory = GetMemory()->GetDataMemory( "InvestigateLocation" );
    Assert( memory );

    ADD_TASK( BTASK_SAVE_POSITION, NULL );
    ADD_TASK( BTASK_MOVE_DESTINATION, memory->GetVector() );
    ADD_TASK( BTASK_WAIT, RandomFloat( 3.0f, 6.0f ) ); // TODO
    ADD_TASK( BTASK_RESTORE_POSITION, NULL );
}

SET_SCHEDULE_INTERRUPTS( CInvestigateLocationSchedule )
{
    ADD_INTERRUPT( BCOND_HELPLESS );
    ADD_INTERRUPT( BCOND_SEE_HATE );
    ADD_INTERRUPT( BCOND_SEE_FEAR );
    ADD_INTERRUPT( BCOND_LIGHT_DAMAGE );
    ADD_INTERRUPT( BCOND_HEAVY_DAMAGE );
    ADD_INTERRUPT( BCOND_REPEATED_DAMAGE );
    ADD_INTERRUPT( BCOND_LOW_HEALTH );
    ADD_INTERRUPT( BCOND_DEJECTED );
    ADD_INTERRUPT( BCOND_MOBBED_BY_ENEMIES );
    ADD_INTERRUPT( BCOND_GOAL_UNREACHABLE );
}

//================================================================================
//================================================================================
float CInvestigateLocationSchedule::GetDesire() const
{
	return BOT_DESIRE_NONE;
}