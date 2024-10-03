//========= Copyright © 1996-2005, Valve Corporation, All rights reserved. ============//
// Authors: 
// Michael S. Booth (linkedin.com/in/michaelbooth), 2003
// Iván Bravo Bravo (linkedin.com/in/ivanbravobravo), 2017

#ifndef IBOT_VISION_H
#define IBOT_VISION_H

#pragma once

//================================================================================
// Vision component
// Everything related to the aim system.
// You can create a custom vision component using this interface.
//================================================================================
abstract_class IBotVision : public IBotComponent
{
public:
    DECLARE_CLASS_GAMEROOT( IBotVision, IBotComponent );

    IBotVision( IBot *bot ) : BaseClass( bot )
    {
    }

public:
    virtual void GetEntityBestAimPosition( CBaseEntity *pTarget, Vector &vecResult ) = 0;
    virtual int GetAimSpeed() = 0;
    virtual float GetStiffness() = 0;

    virtual bool LookAt( const char *pDesc, CBaseEntity *pTarget, int priority = PRIORITY_VERY_LOW, float duration = 1.0f, float cosTolerance = 1.0f ) = 0;
    virtual bool LookAt( const char *pDesc, CBaseEntity *pTarget, const Vector &vecGoal, int priority = PRIORITY_VERY_LOW, float duration = 1.0f, float cosTolerance = 1.0f ) = 0;
    virtual bool LookAt( const char *pDesc, const Vector &goal, int priority = PRIORITY_VERY_LOW, float duration = 1.0f, float cosTolerance = 1.0f ) = 0;

    virtual void LookAtThreat() = 0;
public:
    virtual void Reset() {
        BaseClass::Reset();

        m_vecLookGoal.Invalidate();
        m_pLookingAt = NULL;
        m_pDescription = "INVALID";
        m_iPriority = PRIORITY_INVALID;

        m_bAimReady = false;
        m_flLookYawVel = 0.0f;
        m_flLookPitchVel = 0.0f;
        m_flDuration = 0.0f;
        m_flCosTolerance = 1.5f;
        m_VisionTimer.Invalidate();
    }

    virtual CountdownTimer GetTimer() {
        return m_VisionTimer;
    }

    virtual CBaseEntity *GetAimTarget() {
        return m_pLookingAt.Get();
    }

    virtual const Vector GetAimGoal() {
        return m_vecLookGoal;
    }

    virtual bool HasAimGoal() {
        return m_vecLookGoal.IsValid();
    }

    virtual bool IsVisionTimeExpired() {
        return (m_VisionTimer.HasStarted() && m_VisionTimer.IsElapsed());
    }

    virtual bool IsAimReady() {
        return m_bAimReady;
    }

    virtual const char *GetDescription() {
        return m_pDescription;
    }

    virtual int GetPriority() {
        return m_iPriority;
    }

    virtual void SetPriority( int priority, int onlyIfIsLower = PRIORITY_UNINTERRUPTABLE ) {
        if ( m_iPriority > onlyIfIsLower )
            return;

        m_iPriority = priority;
    }
public:

protected:
    Vector m_vecLookGoal;
    EHANDLE m_pLookingAt;

    const char *m_pDescription;
    int m_iPriority;

    bool m_bAimReady;
    float m_flCosTolerance;

    float m_flLookYawVel;
    float m_flLookPitchVel;

    float m_flDuration;
    CountdownTimer m_VisionTimer;
};

#endif // IBOT_VISION_H