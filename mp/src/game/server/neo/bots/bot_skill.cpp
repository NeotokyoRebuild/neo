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
//================================================================================
CBotProfile::CBotProfile()
{
    int minSkill = TheGameRules->GetSkillLevel();
    SetSkill( RandomInt( minSkill, minSkill+2 ) );
}

//================================================================================
//================================================================================
CBotProfile::CBotProfile( int skill )
{
    SetSkill( skill );
}

//================================================================================
// Sets the level of difficulty
//================================================================================
void CBotProfile::SetSkill( int skill )
{
    skill = clamp( skill, SKILL_EASIEST, SKILL_HARDEST );

    // TODO: Put all this in a script file

    switch ( skill ) {
#ifdef HL2MP
        case SKILL_EASY:
            SetMemoryDuration( 3.0f );
            SetReactionDelay( RandomFloat( 0.3f, 0.5f ) );
            SetAlertDuration( RandomFloat( 3.0f, 5.0f ) );
            SetAimSpeed( AIM_SPEED_LOW, AIM_SPEED_NORMAL );
            SetAttackDelay( RandomFloat( 0.01f, 0.05f ) );
            SetFavoriteHitbox( HITGROUP_STOMACH );
            SetAggression( 30.0f );
            break;

        case SKILL_MEDIUM:
        default:
            SetMemoryDuration( 5.0f );
            SetReactionDelay( RandomFloat( 0.1f, 0.3f ) );
            SetAlertDuration( RandomFloat( 3.0f, 6.0f ) );
            SetAimSpeed( AIM_SPEED_NORMAL, AIM_SPEED_FAST );
            SetAttackDelay( RandomFloat( 0.005f, 0.01f ) );
            SetFavoriteHitbox( RandomInt( HITGROUP_CHEST, HITGROUP_STOMACH ) );
            SetAggression( 60.0f );
            break;

        case SKILL_HARD:
            SetMemoryDuration( 10.0f );
            SetReactionDelay( RandomFloat( 0.0f, 0.01f ) );
            SetAlertDuration( RandomFloat( 4.0f, 7.0f ) );
            SetAimSpeed( AIM_SPEED_FAST, AIM_SPEED_VERYFAST );
            SetAttackDelay( RandomFloat( 0.0001f, 0.005f ) );
            SetFavoriteHitbox( RandomInt( HITGROUP_CHEST, HITGROUP_STOMACH ) );
            SetAggression( 100.0f );
            break;
#else
        case SKILL_EASY:
            SetMemoryDuration( 5.0f );
            SetReactionDelay( RandomFloat( 1.0f, 1.5f ) );
            SetAlertDuration( RandomFloat( 3.0f, 5.0f ) );
            SetAimSpeed( AIM_SPEED_VERYLOW, AIM_SPEED_LOW );
            SetAttackDelay( RandomFloat(0.5f, 0.8f) );
            SetFavoriteHitbox( HITGROUP_STOMACH );
            SetAggression( RandomInt(10.0f, 20.0f) );
            break;

        case SKILL_MEDIUM:
        default:
            SetMemoryDuration( 7.0f );
            SetReactionDelay( RandomFloat( 0.6f, 1.1f ) );
            SetAlertDuration( RandomFloat( 5.0f, 8.0f ) );
            SetAimSpeed( AIM_SPEED_VERYLOW, AIM_SPEED_NORMAL );
            SetAttackDelay( RandomFloat( 0.3f, 0.5f ) );
            SetFavoriteHitbox( RandomInt( HITGROUP_CHEST, HITGROUP_STOMACH ) );
            SetAggression( RandomInt( 30.0f, 40.0f ) );
            break;

        case SKILL_HARD:
            SetMemoryDuration( 9.0f );
            SetReactionDelay( RandomFloat( 0.3f, 0.6f ) );
            SetAlertDuration( RandomFloat( 5.0f, 10.0f ) );
            SetAimSpeed( AIM_SPEED_LOW, AIM_SPEED_FAST );
            SetAttackDelay( RandomFloat( 0.1f, 0.3f ) );
            SetFavoriteHitbox( RandomInt( HITGROUP_HEAD, HITGROUP_STOMACH ) );
            SetAggression( RandomInt( 50.0f, 60.0f ) );
            break;
#endif

#ifdef INSOURCE_DLL
        case SKILL_VERY_HARD:
            SetMemoryDuration( 11.0f );
            SetReactionDelay( RandomFloat( 0.1f, 0.2f ) );
            SetAlertDuration( RandomFloat( 8.0f, 10.0f ) );
            SetAimSpeed( AIM_SPEED_NORMAL, AIM_SPEED_FAST );
            SetAttackDelay( RandomFloat( 0.01f, 0.3f ) );
            SetFavoriteHitbox( RandomInt( HITGROUP_HEAD, HITGROUP_CHEST ) );
            SetAggression( RandomInt( 60.0f, 70.0f ) );
            break;

        case SKILL_ULTRA_HARD:
            SetMemoryDuration( 13.0f );
            SetReactionDelay( RandomFloat( 0.01f, 0.1f ) );
            SetAlertDuration( RandomFloat( 8.0f, 13.0f ) );
            SetAimSpeed( AIM_SPEED_NORMAL, AIM_SPEED_VERYFAST );
            SetAttackDelay( RandomFloat( 0.005f, 0.1f ) );
            SetFavoriteHitbox( RandomInt( HITGROUP_HEAD, HITGROUP_CHEST ) );
            SetAggression( RandomInt( 80.0f, 90.0f ) );
            break;

        case SKILL_IMPOSIBLE:
            SetMemoryDuration( 13.0f );
            SetReactionDelay( RandomFloat( 0.0f, 0.01f ) );
            SetAlertDuration( RandomFloat( 8.0f, 15.0f ) );
            SetAimSpeed( AIM_SPEED_FAST, AIM_SPEED_VERYFAST );
            SetAttackDelay( RandomFloat( 0.001f, 0.01f ) );
            SetFavoriteHitbox( RandomInt( HITGROUP_HEAD, HITGROUP_CHEST ) );
            SetAggression( 100.0f );
            break;
#endif
    }

    m_iSkillLevel = skill;
}

//================================================================================
// Returns the name of the difficulty level
//================================================================================
const char *CBotProfile::GeSkillName()
{
    switch ( m_iSkillLevel ) {
        case SKILL_EASY:
            return "EASY";
            break;

        case SKILL_MEDIUM:
            return "MEDIUM";
            break;

        case SKILL_HARD:
            return "HARD";
            break;

#ifdef INSOURCE_DLL
        case SKILL_VERY_HARD:
            return "VERY HARD";
            break;

        case SKILL_ULTRA_HARD:
            return "ULTRA HARD";
            break;

        case SKILL_IMPOSIBLE:
            return "IMPOSIBLE";
            break;
#endif
    }

    return "UNKNOWN";
}