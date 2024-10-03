//========= Copyright © 1996-2005, Valve Corporation, All rights reserved. ============//
// Authors: 
// Michael S. Booth (linkedin.com/in/michaelbooth), 2003
// Iván Bravo Bravo (linkedin.com/in/ivanbravobravo), 2017

#ifndef IBOT_NAVIGATION_H
#define IBOT_NAVIGATION_H

#pragma once

//================================================================================
// Locomotion component
// Everything related to movement and navigation.
// You can create a custom locomotion component using this interface.
//================================================================================
abstract_class IBotLocomotion : public IBotComponent, public CImprov
{
public:
    DECLARE_CLASS_GAMEROOT( IBotLocomotion, IBotComponent );

    IBotLocomotion( IBot *bot ) : BaseClass( bot )
    {
    }

public:
    virtual void UpdateCommands() = 0;

    virtual bool DriveTo( const char *pDesc, const Vector &vecGoal, int priority = PRIORITY_VERY_LOW, float tolerance = -1.0f ) = 0;
    virtual bool DriveTo( const char *pDesc, CBaseEntity *pTarget, int priority = PRIORITY_VERY_LOW, float tolerance = -1.0f ) = 0;
    virtual bool DriveTo( const char *pDesc, CNavArea *pTargetArea, int priority = PRIORITY_VERY_LOW, float tolerance = -1.0f ) = 0;

    virtual bool Approach( const Vector &vecGoal, float tolerance, int priority = PRIORITY_VERY_LOW ) = 0;
    virtual bool Approach( CBaseEntity *pTarget, float tolerance, int priority = PRIORITY_VERY_LOW ) = 0;

    virtual void StopDrive() = 0;
    virtual void Wiggle() = 0;

    virtual bool ShouldComputePath() = 0;
    virtual void CheckPath() = 0;
    virtual void ComputePath() = 0;

    virtual bool IsUnreachable() const = 0;
    virtual bool IsStuck() const = 0;
    virtual float GetStuckDuration() const = 0;
    virtual void ResetStuck() = 0;

    virtual bool IsOnGround() const = 0;
    virtual CBaseEntity *GetGround() const = 0;
    virtual Vector& GetGroundNormal() const = 0;

    virtual float GetTolerance() const = 0;
    virtual Vector& GetVelocity() const = 0;
    virtual float GetSpeed() const = 0;
    virtual float GetStepHeight() const = 0;
    virtual float GetMaxJumpHeight() const = 0;
    virtual float GetDeathDropHeight() const = 0;
    virtual float GetRunSpeed() const = 0;
    virtual float GetWalkSpeed() const = 0;

    virtual bool IsOnTolerance() const = 0;

    virtual bool IsAreaTraversable( const CNavArea *area ) const = 0;
    virtual bool IsAreaTraversable( const CNavArea *from, const CNavArea *to ) const = 0;
    virtual bool IsTraversable( const Vector &from, const Vector &to ) const = 0;
    virtual bool IsEntityTraversable( CBaseEntity *ent ) const = 0;

    virtual void OnLeaveGround( CBaseEntity *pGround ) = 0;
    virtual void OnLandOnGround( CBaseEntity *pGround ) = 0;

    virtual void Sneak() = 0;
    virtual bool IsSneaking() const = 0;
    virtual bool IsWalking() const = 0;

public:
    virtual void Reset()
    {
        m_bDisabled = false;
        m_flTolerance = -1.0f;

        m_vecDestination.Invalidate();
        m_vecNextSpot.Invalidate();

        m_pDescription = "INVALID";
        m_iPriority = PRIORITY_INVALID;

        m_WiggleDirection = BACKWARD;
        m_WiggleTimer.Invalidate();

        m_NavPath = new CNavPath();

        m_NavPathFollower = new CNavPathFollower();
        m_NavPathFollower->Reset();
        m_NavPathFollower->SetPath( m_NavPath );
        m_NavPathFollower->SetImprov( this );
        m_NavPathFollower->SetFollowPathExactly( false );
    }

    virtual bool HasDestination() const {
        return m_vecDestination.IsValid();
    }

    virtual bool HasValidPath() {
        return m_NavPath->IsValid();
    }

    virtual bool IsDisabled() const {
        return m_bDisabled;
    }

    virtual void SetDisabled( bool disabled ) {
        m_bDisabled = disabled;
    }

    virtual const Vector& GetDestination() {
        return m_vecDestination;
    }

    virtual const Vector& GetNextSpot() {
        return m_vecNextSpot;
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

    virtual void SetTolerance( float tolerance ) {
        m_flTolerance = tolerance;
    }

    virtual float GetDistanceToDestination() const {
        if ( !HasDestination() )
            return -1.0f;

        return GetFeet().DistTo( m_vecDestination );
    }

    virtual float GetDistanceSquaredToDestination() const {
        if ( !HasDestination() )
            return -1.0f;

        return GetFeet().DistToSqr( m_vecDestination );
    }

    virtual CNavPath *GetPath() const {
        return m_NavPath;
    }

    virtual CNavPathFollower *GetPathFollower() const {
        return m_NavPathFollower;
    }

protected:
    bool m_bDisabled;
    float m_flTolerance;

    CNavPath *m_NavPath;
    CNavPathFollower *m_NavPathFollower;

    Vector m_vecDestination;
    Vector m_vecNextSpot;

    const char *m_pDescription;
    int m_iPriority;

    NavRelativeDirType m_WiggleDirection;
    CountdownTimer m_WiggleTimer;
};

#endif // IBOT_NAVIGATION_H