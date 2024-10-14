//========= Copyright © 1996-2005, Valve Corporation, All rights reserved. ============//
// Authors: 
// Iván Bravo Bravo (linkedin.com/in/ivanbravobravo), 2017

#include "cbase.h"
#include "bots\bot.h"

#ifdef INSOURCE_DLL
#include "in_utils.h"
#include "in_gamerules.h"
#else
#include "bots\in_utils.h"
#endif

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

//================================================================================
// Sets the level of difficulty
//================================================================================
void CBot::SetSkill( int level )
{
    // Random
    if ( level == 0 ) {
        level = RandomInt( SKILL_EASY, SKILL_HARDEST );
    }

    // Same game difficulty
    if ( level == 99 ) {
        level = TheGameRules->GetSkillLevel();
    }

    GetProfile()->SetSkill( level );
}

//================================================================================
// Sets the current Bot status
//================================================================================
void CBot::SetState( BotState state, float duration )
{
    // We avoid being panicked forever
    if ( IsPanicked() && state == STATE_PANIC )
        return;

    if ( state != m_iState && state != STATE_PANIC && state != STATE_COMBAT ) {
        if ( m_iStateTimer.HasStarted() && !m_iStateTimer.IsElapsed() )
            return;
    }

    m_iState = state;
    m_iStateTimer.Invalidate();

    if ( duration > 0 ) {
        m_iStateTimer.Start( duration );
    }
}

//================================================================================
// Clears the current status.
// If we were in combat/panic we put ourselves on alert.
// If we were on alert, we put ourselves in idle.
//================================================================================
void CBot::CleanState() 
{
    if ( IsPanicked() || IsCombating() ) {
        Alert();
    }
    else {
        Idle();
    }
}

//================================================================================
// Put the bot in a state of panic where you can not do anything
//================================================================================
void CBot::Panic( float duration )
{
    // Skill duration
    if ( duration < 0 ) {
        duration = GetProfile()->GetReactionDelay();
    }

    SetState( STATE_PANIC, duration );
}

//================================================================================
// Put the bot on a state of alert
//================================================================================
void CBot::Alert( float duration )
{
    // Skill duration
    if ( duration < 0 ) {
        duration = GetProfile()->GetAlertDuration();
    }

    SetState( STATE_ALERT, duration );
}

//================================================================================
// Put the bot on a state of idle
//================================================================================
void CBot::Idle()
{
    SetState( STATE_IDLE, -1 );

    if ( GetLocomotion() ) {
        GetLocomotion()->StandUp();
    }
}

//================================================================================
// Put the bot on a state combat
//================================================================================
void CBot::Combat()
{
    SetState( STATE_COMBAT, 2.0f );
}