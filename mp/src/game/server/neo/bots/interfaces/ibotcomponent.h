//========= Copyright © 1996-2005, Valve Corporation, All rights reserved. ============//
// Authors: 
// Iván Bravo Bravo (linkedin.com/in/ivanbravobravo), 2017

#ifndef IBOT_COMPONENT_H
#define IBOT_COMPONENT_H

#ifdef _WIN32
#pragma once
#endif

#include "bots\interfaces\ibot.h"

class IBotVision;
class IBotAttack;
class IBotMemory;
class IBotLocomotion;
class IBotFollow;
class IBotDecision;

//================================================================================
// Macros
//================================================================================

#define FOR_EACH_COMPONENT FOR_EACH_MAP( m_nComponents, it )
#define FOR_EACH_IMPORTANT_COMPONENT FOR_EACH_MAP( m_nComponents, it ) if ( !m_nComponents[it]->ItsImportant() ) continue; else

#define DECLARE_COMPONENT( id ) virtual int GetID() const { return id; }

//================================================================================
// Artificial Intelligence Component.
// Base for creating components and schedules.
//================================================================================
class IBotComponent : public CPlayerInfo
{
public:
    IBotComponent( IBot *bot )
    {
        m_nBot = bot;
        m_pParent = m_nBot->m_pParent;
    }

    virtual bool IsSchedule() const {
        return false;
    }

    // Returns whether the component is important and must be updated before gathering conditions
    // If true, make sure that the component does not use "HasCondition"
    virtual bool ItsImportant() const {
        return false;
    }

    virtual IBot *GetBot() const {
        return m_nBot;
    }

    virtual CPlayer *GetHost() const {
        return ToInPlayer( m_pParent );
    }

    virtual void Reset() {
        m_flTickInterval = gpGlobals->interval_per_tick;
        m_flUpdateCost = 0.0f;
    }

    virtual CBotProfile *GetProfile() const {
        return m_nBot->GetProfile();
    }

    virtual void SetCondition( BCOND condition ) {
        m_nBot->SetCondition( condition );
    }

    virtual void ClearCondition( BCOND condition ) {
        m_nBot->ClearCondition( condition );
    }

    virtual bool HasCondition( BCOND condition ) const {
        return m_nBot->HasCondition( condition );
    }

    virtual void InjectButton( int btn ) {
        m_nBot->InjectButton( btn );
    }

    virtual bool IsIdle() const {
        return m_nBot->IsIdle();
    }

    virtual bool IsAlerted() const {
        return m_nBot->IsAlerted();
    }

    virtual bool IsCombating() const {
        return m_nBot->IsCombating();
    }

    virtual bool IsPanicked() const {
        return m_nBot->IsPanicked();
    }

    virtual float GetUpdateCost() const {
        return m_flUpdateCost;
    }

    virtual void SetUpdateCost( float time ) {
        m_flUpdateCost = time;
    }

    virtual IBotVision *GetVision() const {
        return m_nBot->GetVision();
    }

    virtual IBotAttack *GetAttack() const {
        return m_nBot->GetAttack();
    }

    virtual IBotMemory *GetMemory() const {
        return m_nBot->GetMemory();
    }

    virtual IBotLocomotion *GetLocomotion() const {
        return m_nBot->GetLocomotion();
    }

    virtual IBotFollow *GetFollow() const {
        return m_nBot->GetFollow();
    }

    virtual IBotDecision *GetDecision() const {
        return m_nBot->GetDecision();
    }

public:
    virtual int GetID() const = 0;
    virtual void Update() = 0;

public:
    float m_flTickInterval;
    float m_flUpdateCost;

protected:
    IBot *m_nBot;
};

#endif // IBOT_COMPONENT_H