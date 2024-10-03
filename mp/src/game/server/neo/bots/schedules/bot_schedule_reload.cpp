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
SET_SCHEDULE_TASKS( CReloadSchedule )
{
    ADD_TASK( BTASK_RELOAD, NULL );
}

SET_SCHEDULE_INTERRUPTS( CReloadSchedule )
{
    ADD_INTERRUPT( BCOND_HEAVY_DAMAGE );
    ADD_INTERRUPT( BCOND_EMPTY_PRIMARY_AMMO );
}

//================================================================================
//================================================================================
float CReloadSchedule::GetDesire() const
{
	if ( GetDecision()->ShouldCover() )
		return BOT_DESIRE_NONE;

    if ( HasCondition(BCOND_EMPTY_CLIP1_AMMO) )
		return 0.81f;

    if ( HasCondition(BCOND_LOW_CLIP1_AMMO) && !IsCombating() )
        return 0.43f;

	return BOT_DESIRE_NONE;
}