//========= Copyright © 1996-2005, Valve Corporation, All rights reserved. ============//
// Authors: 
// Iván Bravo Bravo (linkedin.com/in/ivanbravobravo), 2017

#include "cbase.h"
#include "bots\bot_utils.h"

#include "bots\bot_defs.h"
#include "bots\bot.h"

#ifdef INSOURCE_DLL
#include "in_utils.h"
#include "in_player.h"
#include "in_gamerules.h"
#else
#include "bots\in_utils.h"
#endif

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

//================================================================================
//================================================================================
CEntityMemory::CEntityMemory( IBot *pBot, CBaseEntity *pEntity, CBaseEntity *pInformer )
{
    m_hEntity = pEntity;
    m_hInformer = pInformer;
    m_vecLastPosition.Invalidate();
    m_bVisible = false;
    m_LastVisible.Invalidate();
    m_LastUpdate.Invalidate();
    m_pBot = pBot;
}

//================================================================================
// Returns the amount of time left before this memory is considered lost.
//================================================================================
float CEntityMemory::GetTimeLeft()
{
    return m_pBot->GetMemory()->GetMemoryDuration() - GetElapsedTimeSinceUpdate();
}

//================================================================================
// Returns if the memory should be considered as lost, ie:
// The entity does not exist, has died, is to be eliminated or 
// we have not received any update of it during the time limit of our memory.
//================================================================================
bool CEntityMemory::IsLost()
{
    if ( GetEntity() == NULL || GetEntity()->IsMarkedForDeletion() || !GetEntity()->IsAlive() )
        return true;

    return (GetTimeLeft() <= 0);
}

//================================================================================
//================================================================================
bool CEntityMemory::IsHitboxVisible( HitboxType part )
{
    switch ( part ) {
        case HITGROUP_HEAD:
            return (m_VisibleHitbox.head.IsValid());
            break;

        case HITGROUP_CHEST:
        default:
            return (m_VisibleHitbox.chest.IsValid());
            break;

        case HITGROUP_LEFTLEG:
            return (m_VisibleHitbox.leftLeg.IsValid());
            break;

        case HITGROUP_RIGHTLEG:
            return (m_VisibleHitbox.rightLeg.IsValid());
            break;
    }

    return false;
}

//================================================================================
//================================================================================
float CEntityMemory::GetDistance() const
{
    return m_pBot->GetHost()->GetAbsOrigin().DistTo( GetLastKnownPosition() );
}

//================================================================================
//================================================================================
float CEntityMemory::GetDistanceSquare() const
{
    return m_pBot->GetHost()->GetAbsOrigin().DistToSqr( GetLastKnownPosition() );
}

//================================================================================
//================================================================================
bool CEntityMemory::IsEnemy() const
{
    return m_pBot->GetDecision()->IsEnemy( GetEntity() );
}

//================================================================================
//================================================================================
bool CEntityMemory::IsFriend() const
{
    return m_pBot->GetDecision()->IsFriend( GetEntity() );
}

//================================================================================
// Returns whether the specified hitbox is visible and sets its value to [vecPosition].
// If the specified hitbox (favorite) is not visible, it will try another hitbox with a lower priority.
//================================================================================
bool CEntityMemory::GetVisibleHitboxPosition( Vector & vecPosition, HitboxType favorite )
{
    if ( IsHitboxVisible( favorite ) ) {
        switch ( favorite ) {
            case HITGROUP_HEAD:
            {
                vecPosition = m_VisibleHitbox.head;
                return true;
                break;
            }

            case HITGROUP_CHEST:
            default:
            {
                vecPosition = m_VisibleHitbox.chest;
                return true;
                break;
            }

            case HITGROUP_LEFTLEG:
            {
                vecPosition = m_VisibleHitbox.leftLeg;
                return true;
                break;
            }

            case HITGROUP_RIGHTLEG:
            {
                vecPosition = m_VisibleHitbox.rightLeg;
                return true;
                break;
            }
        }
    }

    if ( favorite != HITGROUP_CHEST && IsHitboxVisible( HITGROUP_CHEST ) ) {
        vecPosition = m_VisibleHitbox.chest;
        return true;
    }

    if ( favorite != HITGROUP_HEAD && IsHitboxVisible( HITGROUP_HEAD ) ) {
        vecPosition = m_VisibleHitbox.head;
        return true;
    }

    if ( favorite != HITGROUP_LEFTLEG && IsHitboxVisible( HITGROUP_LEFTLEG ) ) {
        vecPosition = m_VisibleHitbox.leftLeg;
        return true;
    }

    if ( favorite != HITGROUP_RIGHTLEG && IsHitboxVisible( HITGROUP_RIGHTLEG ) ) {
        vecPosition = m_VisibleHitbox.rightLeg;
        return true;
    }

    return false;
}

//================================================================================
// Update hitbox positions and visibility
//================================================================================
void CEntityMemory::UpdateHitboxAndVisibility()
{
    UpdateVisibility( false );
    m_Hitbox.Reset();
    m_VisibleHitbox.Reset();

    Utils::GetHitboxPositions( GetEntity(), m_Hitbox );

    if ( !m_Hitbox.IsValid() )
        return;

    if ( m_pBot->GetDecision()->IsAbleToSee( m_Hitbox.head ) ) {
        m_VisibleHitbox.head = m_Hitbox.head;
        UpdateVisibility( true );
    }

    if ( m_pBot->GetDecision()->IsAbleToSee( m_Hitbox.chest ) ) {
        m_VisibleHitbox.chest = m_Hitbox.chest;
        UpdateVisibility( true );
    }

    if ( m_pBot->GetDecision()->IsAbleToSee( m_Hitbox.leftLeg ) ) {
        m_VisibleHitbox.leftLeg = m_Hitbox.leftLeg;
        UpdateVisibility( true );
    }

    if ( m_pBot->GetDecision()->IsAbleToSee( m_Hitbox.rightLeg ) ) {
        m_VisibleHitbox.rightLeg = m_Hitbox.rightLeg;
        UpdateVisibility( true );
    }

    // We update the ideal position
    GetVisibleHitboxPosition( m_vecIdealPosition, m_pBot->GetProfile()->GetFavoriteHitbox() );
}