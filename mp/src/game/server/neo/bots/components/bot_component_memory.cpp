//========= Copyright © 1996-2005, Valve Corporation, All rights reserved. ============//
// Authors: 
// Iván Bravo Bravo (linkedin.com/in/ivanbravobravo), 2017
//
// Purpose: It is in charge of managing the memory of the entities 
// and the memory of the information besides considering who will be the best enemy of the bot.
// 
// If you are an expert in C++ and pointers, please get this code out of misery.

#include "cbase.h"
#include "bots\bot.h"
#include "bots\bot_manager.h"

#ifdef INSOURCE_DLL
#include "in_utils.h"
#include "in_player.h"
#else
#include "bots\in_utils.h"
#endif

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

//================================================================================
//================================================================================
void CBotMemory::Update()
{
    VPROF_BUDGET( "CBotMemory::Update", VPROF_BUDGETGROUP_BOTS );

    if ( !IsEnabled() )
        return;

    UpdateMemory();
    UpdateIdealThreat();
    UpdateThreat();
}

//================================================================================
// Update memory, get information about enemies and friends.
//================================================================================
void CBotMemory::UpdateMemory()
{
    int nearbyThreats = 0;
    int nearbyFriends = 0;
    int nearbyDangerousThreats = 0;

    UpdateDataMemory( "NearbyThreats", 0 );
    UpdateDataMemory( "NearbyFriends", 0 );
    UpdateDataMemory( "NearbyDangerousThreats", 0 );

    FOR_EACH_MAP_FAST( m_DataMemory, it )
    {
        CDataMemory *memory = m_DataMemory[it];
        Assert( memory );

        if ( memory->IsExpired() ) {
            m_DataMemory.RemoveAt( it );
            delete memory;
            memory = NULL;
            continue;
        }
    }

    FOR_EACH_MAP_FAST( m_Memory, it )
    {
        CEntityMemory *memory = m_Memory[it];
        Assert( memory );

        CBaseEntity *pEntity = memory->GetEntity();

        // The entity has been deleted.
        if ( pEntity == NULL || pEntity->IsMarkedForDeletion() || !pEntity->IsAlive() ) {
            ForgetEntity( it );
            continue;
        }

        // New frame, we need to recheck if we can see it.
        memory->UpdateVisibility( false );

        if ( !memory->IsLost() ) {
            // The last known position of this entity is close to us.
            // We mark how many allied/enemy entities are close to us to make better decisions.
            if ( memory->IsInRange( m_flNearbyDistance ) ) {
                if ( memory->IsEnemy() ) {
                    if ( GetDecision()->IsDangerousEnemy( pEntity ) ) {
                        ++nearbyDangerousThreats;
                    }

                    ++nearbyThreats;
                }
                else if ( memory->IsFriend() ) {
                    ++nearbyFriends;
                }
            }
        }
    }

    UpdateDataMemory( "NearbyThreats", nearbyThreats );
    UpdateDataMemory( "NearbyFriends", nearbyFriends );
    UpdateDataMemory( "NearbyDangerousThreats", nearbyDangerousThreats );

    // We see, we smell, we feel
    if ( GetHost()->GetSenses() ) {
        GetHost()->GetSenses()->PerformSensing();
    }
}

//================================================================================
// Update who should be our main threat.
//================================================================================
void CBotMemory::UpdateIdealThreat()
{
    CEntityMemory *pIdeal = NULL;

   FOR_EACH_MAP_FAST( m_Memory, it )
    {
        CEntityMemory *memory = m_Memory[it];
        Assert( memory );

        if ( memory->IsLost() )
            continue;

        if ( !memory->IsEnemy() )
            continue;

        CBaseEntity *pEnt = memory->GetEntity();
        Assert( pEnt );

        if ( !pIdeal || GetDecision()->IsBetterEnemy( pEnt, pIdeal->GetEntity() ) ) {
            pIdeal = memory;
        }
    }

    m_pIdealThreat = pIdeal;
}

//================================================================================
// Update who will be our primary threat.
//================================================================================
void CBotMemory::UpdateThreat()
{
    // We totally lost it
    if ( m_pPrimaryThreat && m_pPrimaryThreat->IsLost() ) {
        m_pPrimaryThreat = NULL;
    }

    // We do not have a primary threat, 
    // if we have an ideal threat then we use it as the primary,
    // otherwise there is no enemy.
    if ( !m_pPrimaryThreat ) {
        if ( m_pIdealThreat ) {
            m_pPrimaryThreat = m_pIdealThreat;
        }
        else {
            return;
        }
    }
    else {
        // We already have a primary threat but it is low priority, 
        // the ideal threat is better.
        if ( m_pIdealThreat && m_pPrimaryThreat != m_pIdealThreat ) {
            if ( GetDecision()->IsBetterEnemy( m_pIdealThreat->GetEntity(), m_pPrimaryThreat->GetEntity() ) ) {
                m_pPrimaryThreat = m_pIdealThreat;
            }
        }
    }

    // We update the location of hitboxes
    m_pPrimaryThreat->UpdateHitboxAndVisibility();
}

//================================================================================
// It maintains the current primary threat, preventing it from being forgotten.
//================================================================================
void CBotMemory::MaintainThreat()
{
    CEntityMemory *memory = GetPrimaryThreat();

    if ( memory != NULL ) {
        memory->Maintain();
    }
}

//================================================================================
//================================================================================
float CBotMemory::GetPrimaryThreatDistance() const
{
    CEntityMemory *memory = GetPrimaryThreat();

    if ( !memory )
        return -1.0f;

    return memory->GetDistance();
}

//================================================================================
//================================================================================
void CBotMemory::SetEnemy( CBaseEntity * pEnt, bool bUpdate )
{
    if ( pEnt == NULL ) {
        m_pPrimaryThreat = NULL;
        return;
    }

    if ( pEnt->IsMarkedForDeletion() )
        return;

    // What?
    if ( pEnt == GetHost() )
        return;

    if ( !GetDecision()->CanBeEnemy( pEnt ) )
        return;

    CEntityMemory *memory = GetEntityMemory( pEnt );

    if ( bUpdate || !memory ) {
        memory = UpdateEntityMemory( pEnt, pEnt->WorldSpaceCenter() );
    }

    if ( m_pPrimaryThreat == memory )
        return;

    m_pPrimaryThreat = memory;
    SetCondition( BCOND_NEW_ENEMY );
}

//================================================================================
//================================================================================
CEntityMemory * CBotMemory::UpdateEntityMemory( CBaseEntity * pEnt, const Vector & vecPosition, CBaseEntity * pInformer )
{
    VPROF_BUDGET( "CBotMemory::UpdateEntityMemory", VPROF_BUDGETGROUP_BOTS );

    if ( pEnt == NULL || pEnt->IsMarkedForDeletion() )
        return NULL;

    // We do not need to remember ourselves...
    if ( pEnt == GetHost() )
        return NULL;

    if ( !IsEnabled() )
        return NULL;

    CEntityMemory *memory = GetEntityMemory( pEnt );

    if ( memory ) {
        // We avoid updating the memory several times in the same frame.
        if ( memory->GetFrameLastUpdate() == gpGlobals->absoluteframetime ) {
            return memory;
        }

        // Someone else is informing us about this entity.
        if ( pInformer ) {
            // We already have vision
            if ( memory->IsVisible() ) {
                return memory;
            }

            // Optimization: If you are informing us, 
            // we will avoid updating until 1 second after the last report.
            if ( memory->IsUpdatedRecently( 1.0f ) ) {
                return memory;
            }
        }
    }

    // I have seen this entity with my own eyes, 
    // we must communicate it to our squad.
    if ( !pInformer && GetBot()->GetSquad() && GetDecision()->IsEnemy(pEnt) ) {
        GetBot()->GetSquad()->ReportEnemy( GetHost(), pEnt );
    }

    if ( !memory ) {
        memory = new CEntityMemory( GetBot(), pEnt, pInformer );
        m_Memory.Insert( pEnt->entindex(), memory );
    }

    memory->UpdatePosition( vecPosition );
    memory->SetInformer( pInformer );
    memory->MarkLastFrame();

    return memory;
}

//================================================================================
// Removes the entity from memory
//================================================================================
void CBotMemory::ForgetEntity( CBaseEntity * pEnt )
{
    int index = m_Memory.Find( pEnt->entindex() );
    ForgetEntity( index );
}

//================================================================================
// Removes index from memory
// Notes:
// 1. It is the index in the array, not the index of the entity.
// 2. Always use this function to safely remove an entity from memory and its known pointers.
//================================================================================
void CBotMemory::ForgetEntity( int index )
{
    if ( !m_Memory.IsValidIndex( index ) )
        return;

    CEntityMemory *memory = m_Memory.Element( index );
    Assert( memory );

    // Remove from array
    m_Memory.RemoveAt( index );

    // We check and set null all known pointers
    // Iván: Noob question...
    // "delete memory" would not have to do exactly this? Blessed pointers...

    if ( memory == m_pPrimaryThreat ) {
        m_pPrimaryThreat = NULL;
    }

    if ( memory == m_pIdealThreat ) {
        m_pIdealThreat = NULL;
    }

    //m_Threats.FindAndRemove( memory );
    //m_Friends.FindAndRemove( memory );

    DevMsg(2, "Deleting index %i from memory...\n", index);

    // Delete pointer ???
    delete memory;
    memory = NULL;
}

//================================================================================
//================================================================================
void CBotMemory::ForgetAllEntities()
{
    Reset();
}

//================================================================================
//================================================================================
CEntityMemory * CBotMemory::GetEntityMemory( CBaseEntity * pEnt ) const
{
    if ( pEnt == NULL ) {
        if ( GetPrimaryThreat() == NULL )
            return NULL;

        return GetPrimaryThreat();
    }

    Assert( pEnt );

    if ( pEnt == NULL )
        return NULL;

    return GetEntityMemory( pEnt->entindex() );
}

//================================================================================
//================================================================================
CEntityMemory * CBotMemory::GetEntityMemory( int entindex ) const
{
    int index = m_Memory.Find( entindex );

    if ( !m_Memory.IsValidIndex( index ) )
        return NULL;

    return m_Memory.Element( index );
}

//================================================================================
//================================================================================
float CBotMemory::GetMemoryDuration() const
{
    return GetProfile()->GetMemoryDuration();
}

//================================================================================
//================================================================================
CEntityMemory * CBotMemory::GetClosestThreat( float *distance ) const
{
    float closest = MAX_TRACE_LENGTH;
    CEntityMemory *closestMemory = NULL;

    FOR_EACH_MAP_FAST( m_Memory, it )
    {
        CEntityMemory *memory = m_Memory[it];
        Assert( memory );

        if ( memory->IsLost() )
            continue;

        if ( !memory->IsEnemy() )
            continue;

        float distance = memory->GetDistance();

        if ( distance < closest ) {
            closest = distance;
            closestMemory = memory;
        }
    }

    if ( distance ) {
        distance = &closest;
    }

    return closestMemory;
}

//================================================================================
//================================================================================
int CBotMemory::GetThreatCount( float range ) const
{
    int count = 0;
    
    FOR_EACH_MAP_FAST( m_Memory, it )
    {
        CEntityMemory *memory = m_Memory[it];
        Assert( memory );

        if ( memory->IsLost() )
            continue;

        if ( !memory->IsEnemy() )
            continue;

        float distance = memory->GetDistance();

        if ( distance > range )
            continue;

        ++count;
    }

    return count;
}

//================================================================================
//================================================================================
int CBotMemory::GetThreatCount() const
{
    int count = 0;

    FOR_EACH_MAP_FAST( m_Memory, it )
    {
        CEntityMemory *memory = m_Memory[it];
        Assert( memory );

        if ( memory->IsLost() )
            continue;

        if ( !memory->IsEnemy() )
            continue;

        ++count;
    }

    return count;
}

//================================================================================
//================================================================================
CEntityMemory * CBotMemory::GetClosestFriend( float *distance ) const
{
    float closest = MAX_TRACE_LENGTH;
    CEntityMemory *closestMemory = NULL;

    FOR_EACH_MAP_FAST( m_Memory, it )
    {
        CEntityMemory *memory = m_Memory[it];
        Assert( memory );

        if ( memory->IsLost() )
            continue;

        if ( !memory->IsFriend() )
            continue;

        float distance = memory->GetDistance();

        if ( distance < closest ) {
            closest = distance;
            closestMemory = memory;
        }
    }

    if ( distance ) {
        distance = &closest;
    }

    return closestMemory;
}

//================================================================================
//================================================================================
int CBotMemory::GetFriendCount( float range ) const
{
    int count = 0;

    FOR_EACH_MAP_FAST( m_Memory, it )
    {
        CEntityMemory *memory = m_Memory[it];
        Assert( memory );

        if ( memory->IsLost() )
            continue;

        if ( !memory->IsFriend() )
            continue;

        float distance = memory->GetDistance();

        if ( distance > range )
            continue;

        ++count;
    }

    return count;
}

//================================================================================
//================================================================================
int CBotMemory::GetFriendCount() const
{
    int count = 0;

    FOR_EACH_MAP_FAST( m_Memory, it )
    {
        CEntityMemory *memory = m_Memory[it];
        Assert( memory );

        if ( memory->IsLost() )
            continue;

        if ( !memory->IsFriend() )
            continue;

        ++count;
    }

    return count;
}

//================================================================================
//================================================================================
CEntityMemory * CBotMemory::GetClosestKnown( int teamnum, float *distance ) const
{
    float closest = MAX_TRACE_LENGTH;
    CEntityMemory *closestMemory = NULL;

    FOR_EACH_MAP_FAST( m_Memory, it )
    {
        CEntityMemory *memory = m_Memory[it];
        Assert( memory );

        if ( memory->IsLost() )
            continue;

        if ( memory->GetEntity()->GetTeamNumber() != teamnum )
            continue;

        float distance = memory->GetDistance();

        if ( distance < closest ) {
            closest = distance;
            closestMemory = memory;
        }
    }

    if ( distance ) {
        distance = &closest;
    }

    return closestMemory;
}

//================================================================================
//================================================================================
int CBotMemory::GetKnownCount( int teamnum, float range ) const
{
    int count = 0;

    FOR_EACH_MAP_FAST( m_Memory, it )
    {
        CEntityMemory *memory = m_Memory[it];
        Assert( memory );

        if ( memory->IsLost() )
            continue;

        if ( memory->GetEntity()->GetTeamNumber() != teamnum )
            continue;

        float distance = memory->GetDistance();

        if ( distance > range )
            continue;

        ++count;
    }

    return count;
}

//================================================================================
//================================================================================
float CBotMemory::GetTimeSinceVisible( int teamnum ) const
{
    float closest = -1.0f;

    FOR_EACH_MAP_FAST( m_Memory, it )
    {
        CEntityMemory *memory = m_Memory[it];
        Assert( memory );

        if ( memory->IsLost() )
            continue;

        if ( memory->GetEntity()->GetTeamNumber() != teamnum )
            continue;

        float time = memory->GetTimeLastVisible();

        if ( time > closest ) {
            closest = time;
        }
    }

    return closest;
}

//================================================================================
//================================================================================
CDataMemory * CBotMemory::UpdateDataMemory( const char * name, const Vector & value, float forgetTime )
{
    CDataMemory *memory = GetDataMemory( name );

    if ( memory ) {
        memory->SetVector( value );
    }
    else {
        memory = new CDataMemory( value );
        m_DataMemory.Insert( AllocPooledString(name), memory );
    }

    memory->ForgetIn( forgetTime );
    return memory;
}

//================================================================================
//================================================================================
CDataMemory * CBotMemory::UpdateDataMemory( const char * name, float value, float forgetTime )
{
    CDataMemory *memory = GetDataMemory( name );

    if ( memory ) {
        memory->SetFloat( value );
    }
    else {
        memory = new CDataMemory( value );
        m_DataMemory.Insert( AllocPooledString( name ), memory );
    }

    memory->ForgetIn( forgetTime );
    return memory;
}

//================================================================================
//================================================================================
CDataMemory * CBotMemory::UpdateDataMemory( const char * name, int value, float forgetTime )
{
    CDataMemory *memory = GetDataMemory( name );

    if ( memory ) {
        memory->SetInt( value );
    }
    else {
        memory = new CDataMemory( value );
        m_DataMemory.Insert( AllocPooledString( name ), memory );
    }

    memory->ForgetIn( forgetTime );
    return memory;
}

//================================================================================
//================================================================================
CDataMemory * CBotMemory::UpdateDataMemory( const char * name, const char * value, float forgetTime )
{
    CDataMemory *memory = GetDataMemory( name );

    if ( memory ) {
        memory->SetString( value );
    }
    else {
        memory = new CDataMemory( value );
        m_DataMemory.Insert( AllocPooledString( name ), memory );
    }

    memory->ForgetIn( forgetTime );
    return memory;
}

//================================================================================
//================================================================================
CDataMemory * CBotMemory::UpdateDataMemory( const char * name, CBaseEntity * value, float forgetTime )
{
    if ( value == NULL || value->IsMarkedForDeletion() )
        return NULL;

    CDataMemory *memory = GetDataMemory( name );

    if ( memory ) {
        memory->SetEntity( value );
    }
    else {
        memory = new CDataMemory( value );
        m_DataMemory.Insert( AllocPooledString( name ), memory );
    }

    memory->ForgetIn( forgetTime );
    return memory;
}

//================================================================================
//================================================================================
CDataMemory * CBotMemory::AddDataMemoryList( const char * name, CDataMemory * value, float forgetTime )
{
    CDataMemory *memory = GetDataMemory( name );

    if ( !memory ) {
        memory = new CDataMemory();
        memory->ForgetIn( forgetTime );

        m_DataMemory.Insert( AllocPooledString( name ), memory );
    }

    memory->Add( value );
    return memory;
}

//================================================================================
//================================================================================
CDataMemory * CBotMemory::RemoveDataMemoryList( const char * name, CDataMemory * value, float forgetTime )
{
    CDataMemory *memory = GetDataMemory( name );

    if ( !memory ) {
        return NULL;
    }

    memory->Remove( value );
    return memory;
}

//================================================================================
//================================================================================
CDataMemory * CBotMemory::GetDataMemory( const char * name, bool forceIfNotExists ) const
{
    string_t szName = AllocPooledString( name );
    int index = m_DataMemory.Find( szName );

    if ( !m_DataMemory.IsValidIndex( index ) ) {
        if ( forceIfNotExists ) {
            return new CDataMemory();
        }
        else {
            return NULL;
        }
    }

    return m_DataMemory.Element( index );
}

//================================================================================
//================================================================================
void CBotMemory::ForgetData( const char * name )
{
    string_t szName = AllocPooledString( name );
    m_DataMemory.Remove( szName );
}

//================================================================================
//================================================================================ 
void CBotMemory::ForgetAllData()
{
    m_DataMemory.Purge();
}