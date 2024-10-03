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
#include "basecombatweapon.h"
#endif

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

extern ConVar bot_optimize;
extern ConVar bot_far_distance;

//================================================================================
// It is called when we already have information about Bot vision.
//================================================================================
void CBot::OnLooked( int iDistance )
{
    VPROF_BUDGET( "OnLooked", VPROF_BUDGETGROUP_BOTS );

    AISightIter_t iter;
    CBaseEntity *pSightEnt = GetSenses()->GetFirstSeenEntity( &iter );

    if ( !pSightEnt )
        return;

    // TODO: This "optimization" works?
    int limit = 25;

    while ( pSightEnt ) {
        OnLooked( pSightEnt );

        pSightEnt = GetSenses()->GetNextSeenEntity( &iter );
        --limit;

        if ( limit <= 0 )
            break;
    }
}

//================================================================================
// We can see the specified entity
//================================================================================
void CBot::OnLooked( CBaseEntity *pSightEnt )
{
    if ( !GetMemory() )
        return;

    CEntityMemory *memory = GetMemory()->UpdateEntityMemory( pSightEnt, pSightEnt->WorldSpaceCenter() );
    Assert( memory );

    if ( !memory )
        return;

    memory->UpdateVisibility( true );

    if ( memory->IsEnemy() ) {
        int priority = GetHost()->IRelationPriority( pSightEnt );

        if ( priority < 0 ) {
            SetCondition( BCOND_SEE_DISLIKE );
        }
        else {
            SetCondition( BCOND_SEE_HATE );
        }
    }
    else if ( memory->IsFriend() ) {
        SetCondition( BCOND_SEE_FRIEND );

        if ( pSightEnt->IsPlayer() ) {
            CPlayer *pSightPlayer = ToInPlayer( pSightEnt );

            if ( GetDecision()->ShouldHelpDejectedFriend( pSightPlayer ) ) {
                GetMemory()->UpdateDataMemory( "DejectedFriend", pSightEnt, 30.0f );
                SetCondition( BCOND_SEE_DEJECTED_FRIEND );
            }
        }
    }
    else {
        // A weapon, maybe we can use it...
        if ( pSightEnt->IsBaseCombatWeapon() ) {
            CBaseWeapon *pWeapon = ToBaseWeapon( pSightEnt );
            Assert( pWeapon );

            if ( GetDecision()->ShouldGrabWeapon( pWeapon ) ) {
                GetMemory()->UpdateDataMemory( "BestWeapon", pSightEnt, 30.0f );
                SetCondition( BCOND_BETTER_WEAPON_AVAILABLE );
            }
        }
    }
}

//================================================================================
// It is called when we already have information about Bot hearing
//================================================================================
void CBot::OnListened()
{
    VPROF_BUDGET( "OnListened", VPROF_BUDGETGROUP_BOTS );

    AISoundIter_t iter;
    CSound *pCurrentSound = GetSenses()->GetFirstHeardSound( &iter );

    while ( pCurrentSound ) {
        if ( pCurrentSound->IsSoundType( SOUND_DANGER ) ) {
            if ( pCurrentSound->IsSoundType( SOUND_CONTEXT_BULLET_IMPACT ) ) {
                SetCondition( BCOND_HEAR_MOVE_AWAY );

                if ( pCurrentSound->IsSoundType( SOUND_CONTEXT_FROM_SNIPER ) ) {
                    SetCondition( BCOND_HEAR_BULLET_IMPACT_SNIPER );
                }
                else {
                    SetCondition( BCOND_HEAR_BULLET_IMPACT );
                }
            }

            if ( pCurrentSound->IsSoundType( SOUND_CONTEXT_EXPLOSION ) ) {
                SetCondition( BCOND_HEAR_SPOOKY );
                SetCondition( BCOND_HEAR_MOVE_AWAY );
            }

            SetCondition( BCOND_HEAR_DANGER );
        }

        if ( pCurrentSound->IsSoundType( SOUND_COMBAT ) ) {
            if ( pCurrentSound->IsSoundType( SOUND_CONTEXT_GUNFIRE ) ) {
                SetCondition( BCOND_HEAR_SPOOKY );
            }

            SetCondition( BCOND_HEAR_COMBAT );
        }

        if ( pCurrentSound->IsSoundType( SOUND_WORLD ) ) {
            SetCondition( BCOND_HEAR_WORLD );
        }

#ifdef INSOURCE_DLL
        if ( pCurrentSound->IsSoundType( SOUND_PLAYER ) ) {
            if ( pCurrentSound->IsSoundType( SOUND_CONTEXT_FOOTSTEP ) ) {
                SetCondition( BCOND_HEAR_ENEMY_FOOTSTEP );
            }

            SetCondition( BCOND_HEAR_ENEMY );
        }
#endif

        if ( pCurrentSound->IsSoundType( SOUND_CARCASS ) ) {
            SetCondition( BCOND_SMELL_CARCASS );
        }

        if ( pCurrentSound->IsSoundType( SOUND_MEAT ) ) {
            SetCondition( BCOND_SMELL_MEAT );
        }

        if ( pCurrentSound->IsSoundType( SOUND_GARBAGE ) ) {
            SetCondition( BCOND_SMELL_GARBAGE );
        }

        pCurrentSound = GetSenses()->GetNextHeardSound( &iter );
    }
}

//================================================================================
// We have received damage
//================================================================================
void CBot::OnTakeDamage( const CTakeDamageInfo &info )
{
    CBaseEntity *pAttacker = info.GetAttacker();
    float farDistance = bot_far_distance.GetFloat();

    // The attacker is a character
    if ( pAttacker && pAttacker->MyCombatCharacterPointer() ) {
        if ( pAttacker == GetEnemy() )
            return;

        if ( GetDecision()->IsEnemy( pAttacker ) ) {
            float distance = GetAbsOrigin().DistTo( pAttacker->GetAbsOrigin() );
            bool visible = GetDecision()->IsAbleToSee( pAttacker );
            Vector vecPosition = pAttacker->WorldSpaceCenter();

            // We have no vision of the attacker! We can not know the exact position
            if ( !visible ) {
                float errorRange = 200.0f;

                // It is very far!
                if ( distance >= farDistance ) {
                    errorRange = 500.0f;
                }

                vecPosition.x += RandomFloat( -errorRange, errorRange );
                vecPosition.y += RandomFloat( -errorRange, errorRange );
            }

            // We were calm, without hurting anyone...
            if ( GetState() == STATE_IDLE ) {
                // We can not see it! We panicked!
                if ( !GetDecision()->IsAbleToSee( pAttacker ) ) {
                    Panic();
                }
            }

            GetMemory()->UpdateEntityMemory( pAttacker, vecPosition );

            if ( !visible ) {
                // We try to look at where we've been hurt
                GetVision()->LookAt( "Unknown Threat Spot", vecPosition, PRIORITY_VERY_HIGH, 0.2f );
            }
        }
    }

#ifdef INSOURCE_DLL
    // El último daño recibido fue hace menos de 2s
    // Al parecer estamos recibiendo daño continuo
    if ( GetHost()->GetLastDamageTimer().IsLessThen( 2.0f ) ) {
        ++m_iRepeatedDamageTimes;
        m_flDamageAccumulated += info.GetDamage();
    }
#endif
}

//================================================================================
// Dead x(
//================================================================================
void CBot::OnDeath( const CTakeDamageInfo &info ) 
{
    if ( GetActiveSchedule() ) {
        GetActiveSchedule()->Fail( "Player Death" );
        GetActiveSchedule()->Finish();
        m_nActiveSchedule = NULL;
    }
}
