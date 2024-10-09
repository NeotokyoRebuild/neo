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
SET_SCHEDULE_TASKS( CHuntEnemySchedule )
{
    bool carefulApproach = GetDecision()->ShouldMustBeCareful();

    ADD_TASK( BTASK_SET_FAIL_SCHEDULE, SCHEDULE_COVER );
    ADD_TASK( BTASK_RUN, NULL );

#ifndef HL2MP
    // We must be careful!
    if ( carefulApproach ) {
        // We run towards the target until we reach a distance of between 700 and 900 units
        ADD_TASK( BTASK_HUNT_ENEMY, RandomFloat( 700.0, 900.0f ) );

        // We walked slowly until reaching a short distance and 
        // we waited a little in case the target leaves its coverage.
        ADD_TASK( BTASK_SNEAK, NULL );
        ADD_TASK( BTASK_HUNT_ENEMY, RandomFloat( 400.0f, 500.0f ) );
        ADD_TASK( BTASK_WAIT, RandomFloat( 0.5f, 3.5f ) );
    }
#endif

    ADD_TASK( BTASK_HUNT_ENEMY, NULL );
}


SET_SCHEDULE_INTERRUPTS( CHuntEnemySchedule )
{
    ADD_INTERRUPT( BCOND_EMPTY_PRIMARY_AMMO );
    ADD_INTERRUPT( BCOND_EMPTY_CLIP1_AMMO );
    ADD_INTERRUPT( BCOND_HELPLESS );
    ADD_INTERRUPT( BCOND_WITHOUT_ENEMY );
    ADD_INTERRUPT( BCOND_ENEMY_DEAD );
    ADD_INTERRUPT( BCOND_HEAVY_DAMAGE );
    ADD_INTERRUPT( BCOND_REPEATED_DAMAGE );
    ADD_INTERRUPT( BCOND_LOW_HEALTH );
    ADD_INTERRUPT( BCOND_DEJECTED );
    ADD_INTERRUPT( BCOND_NEW_ENEMY );
    ADD_INTERRUPT( BCOND_BETTER_WEAPON_AVAILABLE );
    ADD_INTERRUPT( BCOND_MOBBED_BY_ENEMIES );
    ADD_INTERRUPT( BCOND_GOAL_UNREACHABLE );
    ADD_INTERRUPT( BCOND_HEAR_MOVE_AWAY );
}

//================================================================================
//================================================================================
float CHuntEnemySchedule::GetDesire() const
{
    if ( !GetMemory() || !GetLocomotion() )
        return BOT_DESIRE_NONE;

    if ( !GetDecision()->CanHuntThreat() )
        return BOT_DESIRE_NONE;

    CEntityMemory *memory = GetBot()->GetPrimaryThreat();

    if ( memory == NULL )
        return BOT_DESIRE_NONE;

    // We have no vision of the enemy
    if ( HasCondition( BCOND_ENEMY_LOST ) ) {
        // But we have a vision of his last position and we are close
        if ( HasCondition( BCOND_ENEMY_LAST_POSITION_VISIBLE ) && (HasCondition( BCOND_ENEMY_TOO_NEAR ) || HasCondition( BCOND_ENEMY_NEAR )) )
            return BOT_DESIRE_NONE;
        
        return 0.65;
    }

    // We do not have direct vision to the enemy (a person, window, etc.)
    if ( HasCondition( BCOND_ENEMY_OCCLUDED ) )
        return 0.65;

    // We do not have range of attack
    if ( HasCondition( BCOND_TOO_FAR_TO_ATTACK ) )
        return 0.38f;

    return BOT_DESIRE_NONE;
}