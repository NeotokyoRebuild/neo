//========= Copyright © 1996-2005, Valve Corporation, All rights reserved. ============//
// Authors: 
// Iván Bravo Bravo (linkedin.com/in/ivanbravobravo), 2017

#ifndef IBOT_FOLLOW_H
#define IBOT_FOLLOW_H

#pragma once

//================================================================================
// Follow component
// Everything related to following another entity.
// You can create a custom follow component using this interface.
//================================================================================
abstract_class IBotFollow : public IBotComponent
{
public:
    DECLARE_CLASS_GAMEROOT( IBotFollow, IBotComponent );

    IBotFollow( IBot *bot ) : BaseClass( bot )
    {
    }

public:
    virtual void Start( CBaseEntity *pEntity, bool enabled = true ) = 0;
    virtual void Start( const char *pEntityName, bool enabled = true ) = 0;
    virtual void Stop() = 0;

    virtual float GetTolerance() = 0;

    virtual bool IsFollowingPlayer() = 0;
    virtual bool IsFollowingBot() = 0;
    virtual bool IsFollowingHuman() = 0;

public:
    virtual CBaseEntity *GetEntity() {
        return m_Entity.Get();
    }

    virtual bool IsFollowing() {
        return (GetEntity() != NULL);
    }

    virtual bool IsFollowingActive() {
        return (IsFollowing() && IsEnabled());
    }

    virtual bool IsEnabled() {
        return m_bEnabled;
    }

    virtual void SetEnabled( bool enabled ) {
        m_bEnabled = enabled;
    }

protected:
    bool m_bEnabled;
    EHANDLE m_Entity;
};

#endif