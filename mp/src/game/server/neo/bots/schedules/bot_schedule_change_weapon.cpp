//========= Copyright © 1996-2005, Valve Corporation, All rights reserved. ============//
// Authors: 
// Iván Bravo Bravo (linkedin.com/in/ivanbravobravo), 2017

#include "cbase.h"
#include "bots\bot.h"

#ifdef INSOURCE_DLL
#include "in_utils.h"
#include "weapon_base.h"
#else
#include "bots\in_utils.h"
#endif

#include "in_buttons.h"

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

//================================================================================
//================================================================================
SET_SCHEDULE_TASKS( CChangeWeaponSchedule )
{
    CDataMemory *memory = GetMemory()->GetDataMemory( "BestWeapon" );
    Assert( memory );

    ADD_TASK( BTASK_SAVE_POSITION, NULL );
    ADD_TASK( BTASK_RUN, NULL );
    ADD_TASK( BTASK_MOVE_DESTINATION, memory->GetEntity() );
    ADD_TASK( BTASK_AIM, memory->GetEntity() );
    ADD_TASK( BTASK_USE, NULL );
    ADD_TASK( BTASK_RESTORE_POSITION, NULL );
}

SET_SCHEDULE_INTERRUPTS( CChangeWeaponSchedule )
{
    if ( GetProfile()->GetSkill() < SKILL_ULTRA_HARD ) {
        ADD_INTERRUPT( BCOND_SEE_HATE );
        ADD_INTERRUPT( BCOND_SEE_FEAR );
    }

    ADD_INTERRUPT( BCOND_HEAVY_DAMAGE );
    ADD_INTERRUPT( BCOND_REPEATED_DAMAGE );
    ADD_INTERRUPT( BCOND_LOW_HEALTH );
    ADD_INTERRUPT( BCOND_DEJECTED );
    ADD_INTERRUPT( BCOND_MOBBED_BY_ENEMIES );
    ADD_INTERRUPT( BCOND_GOAL_UNREACHABLE );
    ADD_INTERRUPT( BCOND_HEAR_MOVE_AWAY );
}

//================================================================================
//================================================================================
float CChangeWeaponSchedule::GetDesire() const
{
    if ( !GetMemory() )
        return BOT_DESIRE_NONE;

    CDataMemory *memory = GetMemory()->GetDataMemory( "BestWeapon" );

    if ( memory == NULL )
        return BOT_DESIRE_NONE;

	if ( !GetDecision()->CanMove() )
		return BOT_DESIRE_NONE;

	if ( HasCondition(BCOND_BETTER_WEAPON_AVAILABLE) )
		return 0.41f;

	return BOT_DESIRE_NONE;
}

//================================================================================
//================================================================================
void CChangeWeaponSchedule::TaskStart()
{
	BotTaskInfo_t *pTask = GetActiveTask();

	switch( pTask->task )
	{
        case BTASK_USE:
        {
            InjectButton( IN_USE );
            break;
        }

        default:
        {
            BaseClass::TaskStart();
            break;
        }
    }
}

//================================================================================
//================================================================================
void CChangeWeaponSchedule::TaskRun()
{
    CDataMemory *memory = GetMemory()->GetDataMemory( "BestWeapon" );

    if ( !memory ) {
        Fail( "Best weapon not available" );
        return;
    }

    CBaseWeapon *pWeapon = dynamic_cast<CBaseWeapon *>( memory->GetEntity() );

    // El arma ha dejado de existir
    if ( !pWeapon ) {
        Fail( "The weapon dont exists." );
        return;
    }

    // Ya tiene un dueño
    if ( pWeapon->GetOwner() ) {
        if ( pWeapon->GetOwner() != GetHost() ) {
            Fail( "The weapon has been taken" );
            return;
        }
    }

    BotTaskInfo_t *pTask = GetActiveTask();

    switch ( pTask->task ) {
        case BTASK_USE:
        {
            if ( pWeapon->GetOwner() && pWeapon->GetOwner() == GetHost() ) {
                TaskComplete();
            }

            break;
        }

        default:
        {
            BaseClass::TaskRun();
            break;
        }
    }
}