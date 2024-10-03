//========= Copyright © 1996-2005, Valve Corporation, All rights reserved. ============//
// Authors: 
// Iván Bravo Bravo (linkedin.com/in/ivanbravobravo), 2017

#ifndef IBOT_MEMORY_H
#define IBOT_MEMORY_H

#pragma once

class CEntityMemory;
class CDataMemory;

//================================================================================
// Useful macros
//================================================================================

// These macros allow you to obtain a value type from the information memory, 
// if the memory does not exist it will be returned an empty one (never NULL).
#define GetDataMemoryVector(name) GetMemory()->GetDataMemory(name, true)->GetVector()
#define GetDataMemoryFloat(name) GetMemory()->GetDataMemory(name, true)->GetFloat()
#define GetDataMemoryInt(name) GetMemory()->GetDataMemory(name, true)->GetInt()
#define GetDataMemoryString(name) GetMemory()->GetDataMemory(name, true)->GetString()
#define GetDataMemoryEntity(name) GetMemory()->GetDataMemory(name, true)->GetEntity()

//================================================================================
// Memory component
// The bot memory about the position of friends and enemies / data memory.
// You can create a custom memory component using this interface.
//================================================================================
abstract_class IBotMemory : public IBotComponent
{
public:
    DECLARE_CLASS_GAMEROOT( IBotMemory, IBotComponent );

    IBotMemory( IBot *bot ) : BaseClass( bot )
    {
        SetDefLessFunc( m_Memory );
        SetDefLessFunc( m_DataMemory );
    }

public:
    virtual void UpdateMemory() = 0;
    virtual void UpdateIdealThreat() = 0;
    virtual void UpdateThreat() = 0;

    virtual void MaintainThreat() = 0;
    virtual float GetPrimaryThreatDistance() const = 0;
    virtual void SetEnemy( CBaseEntity *pEnt, bool bUpdate = false ) = 0;

    virtual CEntityMemory *UpdateEntityMemory( CBaseEntity *pEnt, const Vector &vecPosition, CBaseEntity *pInformer = NULL ) = 0;

    virtual void ForgetEntity( CBaseEntity *pEnt ) = 0;
    virtual void ForgetEntity( int index ) = 0;
    virtual void ForgetAllEntities() = 0;
    
    virtual CEntityMemory *GetEntityMemory( CBaseEntity *pEnt = NULL ) const = 0;
    virtual CEntityMemory *GetEntityMemory( int index ) const = 0;

    virtual float GetMemoryDuration() const = 0;

    virtual CEntityMemory *GetClosestThreat( float *distance = NULL ) const = 0;
    virtual int GetThreatCount( float range ) const = 0;
    virtual int GetThreatCount() const = 0;

    virtual CEntityMemory *GetClosestFriend( float *distance = NULL ) const = 0;
    virtual int GetFriendCount( float range ) const = 0;
    virtual int GetFriendCount() const = 0;

    virtual CEntityMemory *GetClosestKnown( int teamnum, float *distance = NULL ) const = 0;
    virtual int GetKnownCount( int teamnum, float range = MAX_TRACE_LENGTH ) const = 0;

    virtual int GetKnownCount() const {
        return m_Memory.Count();
    }

    virtual float GetTimeSinceVisible( int teamnum ) const = 0;

public:
    virtual CDataMemory *UpdateDataMemory( const char *name, const Vector &value, float forgetTime = -1.0f ) = 0;
    virtual CDataMemory *UpdateDataMemory( const char *name, float value, float forgetTime = -1.0f ) = 0;
    virtual CDataMemory *UpdateDataMemory( const char *name, int value, float forgetTime = -1.0f ) = 0;
    virtual CDataMemory *UpdateDataMemory( const char *name, const char *value, float forgetTime = -1.0f ) = 0;
    virtual CDataMemory *UpdateDataMemory( const char *name, CBaseEntity *value, float forgetTime = -1.0f ) = 0;

    virtual CDataMemory *AddDataMemoryList( const char *name, CDataMemory *value, float forgetTime = -1.0f ) = 0;
    virtual CDataMemory *RemoveDataMemoryList( const char *name, CDataMemory *value, float forgetTime = -1.0f ) = 0;

    virtual CDataMemory *GetDataMemory( const char *name, bool forceIfNotExists = false ) const = 0;

    virtual void ForgetData( const char *name ) = 0;
    virtual void ForgetAllData() = 0;

public:
    virtual void Reset()
    {
        BaseClass::Reset();

        m_bEnabled = true;
        m_pPrimaryThreat = NULL;
        m_pIdealThreat = NULL;
        m_flNearbyDistance = 1000.0;

        m_Memory.Purge();
        m_DataMemory.Purge();
    }

    virtual bool ItsImportant() const {
        return true;
    }

    virtual bool IsEnabled() const {
        return m_bEnabled;
    }

    virtual void Disable() {
        m_bEnabled = false;
    }

    virtual void Enable() {
        m_bEnabled = true;
    }

    virtual CBaseEntity *GetEnemy() const {
        if ( !GetPrimaryThreat() )
            return NULL;

        return GetPrimaryThreat()->GetEntity();
    }

    virtual CEntityMemory *GetPrimaryThreat() const {
        return m_pPrimaryThreat;
    }

    virtual CEntityMemory *GetIdealThreat() const {
        return m_pIdealThreat;
    }

    virtual void SetNearbyDistance( float distance ) {
        m_flNearbyDistance = distance;
    }

protected:
    bool m_bEnabled;
    CEntityMemory *m_pPrimaryThreat;
    CEntityMemory *m_pIdealThreat;

    float m_flNearbyDistance;

    CUtlMap<int, CEntityMemory *> m_Memory;
    CUtlMap<string_t, CDataMemory *> m_DataMemory;

    friend class CBot;
};

#endif