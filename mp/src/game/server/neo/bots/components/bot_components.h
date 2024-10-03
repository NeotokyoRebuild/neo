//========= Copyright © 1996-2005, Valve Corporation, All rights reserved. ============//
// Authors: 
// Michael S. Booth (linkedin.com/in/michaelbooth), 2003
// Iván Bravo Bravo (linkedin.com/in/ivanbravobravo), 2017

#ifndef BOT_COMPONENTS_H
#define BOT_COMPONENTS_H

#ifdef _WIN32
#pragma once
#endif

#include "bots\interfaces\ibotcomponent.h"
#include "bots\interfaces\ibotvision.h"
#include "bots\interfaces\ibotlocomotion.h"
#include "bots\interfaces\ibotfollow.h"
#include "bots\interfaces\ibotmemory.h"
#include "bots\interfaces\ibotattack.h"
#include "bots\interfaces\ibotdecision.h"

//================================================================================
// Macros
//================================================================================

#define ADD_COMPONENT( className ) AddComponent( new className(this) );

//================================================================================
// Vision component
// Everything related to the aim system.
//================================================================================
class CBotVision : public IBotVision
{
public:
    DECLARE_CLASS_GAMEROOT( CBotVision, IBotVision );
    DECLARE_COMPONENT( BOT_COMPONENT_VISION );

    CBotVision( IBot *bot ) : BaseClass( bot )
    {
    }

    virtual void Update();
    virtual void Process();

public:
    virtual void GetEntityBestAimPosition( CBaseEntity *pTarget, Vector &vecResult );
    virtual int GetAimSpeed();
    virtual float GetStiffness();

    virtual bool LookAt( const char *pDesc, CBaseEntity *pTarget, int priority = PRIORITY_VERY_LOW, float duration = 1.0f, float cosTolerance = 1.0f );
    virtual bool LookAt( const char *pDesc, CBaseEntity *pTarget, const Vector &vecGoal, int priority = PRIORITY_VERY_LOW, float duration = 1.0f, float cosTolerance = 1.0f );
    virtual bool LookAt( const char *pDesc, const Vector &goal, int priority = PRIORITY_VERY_LOW, float duration = 1.0f, float cosTolerance = 1.0f );

    virtual void LookAtThreat();

public:
    virtual void LookNavigation();
    virtual void LookAround();

    virtual void LookDanger();
    virtual void LookInterestingSpot();
    virtual void LookRandomSpot();
    virtual void LookSquadMember();
};

//================================================================================
// Locomotion component
// Everything related to movement and navigation.
//================================================================================
class CBotLocomotion : public IBotLocomotion
{
public:
    DECLARE_CLASS_GAMEROOT( CBotLocomotion, IBotLocomotion );
    DECLARE_COMPONENT( BOT_COMPONENT_LOCOMOTION );

    CBotLocomotion( IBot *bot ) : BaseClass( bot )
    {
    }

    virtual void Reset();
    virtual void Update();

public:
    virtual void UpdateCommands();

    virtual bool DriveTo( const char *pDesc, const Vector &vecGoal, int priority = PRIORITY_VERY_LOW, float tolerance = -1.0f );
    virtual bool DriveTo( const char *pDesc, CBaseEntity *pTarget, int priority = PRIORITY_VERY_LOW, float tolerance = -1.0f );
    virtual bool DriveTo( const char *pDesc, CNavArea *pTargetArea, int priority = PRIORITY_VERY_LOW, float tolerance = -1.0f );

    virtual bool Approach( const Vector &vecGoal, float tolerance, int priority = PRIORITY_VERY_LOW );
    virtual bool Approach( CBaseEntity *pTarget, float tolerance, int priority = PRIORITY_VERY_LOW );

    virtual void StopDrive();
    virtual void Wiggle();

    virtual bool ShouldComputePath();
    virtual void CheckPath();
    virtual void ComputePath();

    virtual bool IsUnreachable() const;
    virtual bool IsStuck() const;
    virtual float GetStuckDuration() const;
    virtual void ResetStuck();

    virtual bool IsOnGround() const;
    virtual CBaseEntity *GetGround() const;
    virtual Vector& GetGroundNormal() const;

    virtual float GetTolerance() const;
    virtual Vector& GetVelocity() const;
    virtual float GetSpeed() const;
    virtual float GetStepHeight() const;
    virtual float GetMaxJumpHeight() const;
    virtual float GetDeathDropHeight() const;
    virtual float GetRunSpeed() const;
    virtual float GetWalkSpeed() const;

    virtual bool IsOnTolerance() const;

    virtual bool IsAreaTraversable( const CNavArea *area ) const;
    virtual bool IsAreaTraversable( const CNavArea *from, const CNavArea *to ) const;
    virtual bool IsTraversable( const Vector &from, const Vector &to ) const;
    virtual bool IsEntityTraversable( CBaseEntity *ent ) const;

    virtual void OnLeaveGround( CBaseEntity *pGround ) { }
    virtual void OnLandOnGround( CBaseEntity *pGround ) { }

public:
    // CImprovLocomotor
    virtual const Vector &GetCentroid() const;
    virtual const Vector &GetFeet() const;
    virtual const Vector &GetEyes() const;
    virtual float GetMoveAngle() const;

    virtual CNavArea *GetLastKnownArea() const;
    virtual bool GetSimpleGroundHeightWithFloor( const Vector &pos, float *height, Vector *normal = NULL );

    virtual void Crouch();
    virtual void StandUp();
    virtual bool IsCrouching() const;

    virtual void Jump();
    virtual bool IsJumping() const;

    virtual void Run();
    virtual void Walk();
    virtual void Sneak();

    virtual bool IsRunning() const;
    virtual bool IsWalking() const;
    virtual bool IsSneaking() const;

    virtual void StartLadder( const CNavLadder *ladder, NavTraverseType how, const Vector &approachPos, const Vector &departPos );
    virtual bool TraverseLadder( const CNavLadder *ladder, NavTraverseType how, const Vector &approachPos, const Vector &departPos, float deltaT );
    virtual bool IsUsingLadder() const;

    virtual void TrackPath( const Vector &pathGoal, float deltaT );
    virtual void OnMoveToSuccess( const Vector &goal );
    virtual void OnMoveToFailure( const Vector &goal, MoveToFailureType reason );

protected:
    bool m_bCrouching;
    bool m_bJumping;
    bool m_bSneaking;
    bool m_bRunning;
    bool m_bUsingLadder;
};

//================================================================================
// Follow component
// Everything related to following another entity.
//================================================================================
class CBotFollow : public IBotFollow
{
public:
    DECLARE_CLASS_GAMEROOT( CBotFollow, IBotFollow );
    DECLARE_COMPONENT( BOT_COMPONENT_FOLLOW );

    CBotFollow( IBot *bot ) : BaseClass( bot )
    {
    }
    
    virtual void Update();

public:
    virtual void Start( CBaseEntity *pEntity, bool enabled = true );
    virtual void Start( const char *pEntityName, bool enabled = true );
    virtual void Stop();

    virtual float GetTolerance();

    virtual bool IsFollowingPlayer();
    virtual bool IsFollowingBot();
    virtual bool IsFollowingHuman();
};

//================================================================================
// Memory component
// The bot memory about the position of friends and enemies / data memory.
//================================================================================
class CBotMemory : public IBotMemory
{
public:
    DECLARE_CLASS_GAMEROOT( CBotMemory, IBotMemory );
    DECLARE_COMPONENT( BOT_COMPONENT_MEMORY );

    CBotMemory( IBot *bot ) : BaseClass( bot )
    {
        UpdateDataMemory( "NearbyThreats", 0 );
        UpdateDataMemory( "NearbyFriends", 0 );
        UpdateDataMemory( "NearbyDangerousThreats", 0 );
    }

    virtual void Update();

public:
    virtual void UpdateMemory();
    virtual void UpdateIdealThreat();
    virtual void UpdateThreat();

    virtual void MaintainThreat();

    virtual float GetPrimaryThreatDistance() const;

    virtual void SetEnemy( CBaseEntity *pEnt, bool bUpdate = false );

    virtual CEntityMemory *UpdateEntityMemory( CBaseEntity *pEnt, const Vector &vecPosition, CBaseEntity *pInformer = NULL );
    virtual void ForgetEntity( CBaseEntity *pEnt );
    virtual void ForgetEntity( int index );
    virtual void ForgetAllEntities();

    virtual CEntityMemory *GetEntityMemory( CBaseEntity *pEnt = NULL ) const;
    virtual CEntityMemory *GetEntityMemory( int index ) const;

    virtual float GetMemoryDuration() const;

    virtual CEntityMemory *GetClosestThreat( float *distance = NULL ) const;
    virtual int GetThreatCount( float range ) const;
    virtual int GetThreatCount() const;

    virtual CEntityMemory *GetClosestFriend( float *distance = NULL ) const;
    virtual int GetFriendCount( float range ) const;
    virtual int GetFriendCount() const;

    virtual CEntityMemory *GetClosestKnown( int teamnum, float *distance = NULL ) const;
    virtual int GetKnownCount( int teamnum, float range = MAX_TRACE_LENGTH ) const;

    virtual float GetTimeSinceVisible( int teamnum ) const;

public:
    virtual CDataMemory *UpdateDataMemory( const char *name, const Vector &value, float forgetTime = -1.0f );
    virtual CDataMemory *UpdateDataMemory( const char *name, float value, float forgetTime = -1.0f );
    virtual CDataMemory *UpdateDataMemory( const char *name, int value, float forgetTime = -1.0f );
    virtual CDataMemory *UpdateDataMemory( const char *name, const char *value, float forgetTime = -1.0f );
    virtual CDataMemory *UpdateDataMemory( const char *name, CBaseEntity *value, float forgetTime = -1.0f );

    virtual CDataMemory *AddDataMemoryList( const char *name, CDataMemory *value, float forgetTime = -1.0f );
    virtual CDataMemory *RemoveDataMemoryList( const char *name, CDataMemory *value, float forgetTime = -1.0f );

    virtual CDataMemory *GetDataMemory( const char *name, bool forceIfNotExists = false ) const;

    virtual void ForgetData( const char *name );
    virtual void ForgetAllData();
};

//================================================================================
// Attack component
// Everything related to attacking
//================================================================================
class CBotAttack : public IBotAttack
{
public:
    DECLARE_CLASS_GAMEROOT( CBotAttack, IBotAttack );
    DECLARE_COMPONENT( BOT_COMPONENT_ATTACK );

    CBotAttack( IBot *bot ) : BaseClass( bot )
    {
    }

    virtual void Update();

public:
    virtual void FiregunAttack();
    virtual void MeleeWeaponAttack();
    //virtual void OnAttack( int type );
};

//================================================================================
// Decision component
// Everything related to the decisions that the bot must take.
//================================================================================
class CBotDecision : public IBotDecision
{
public:
    DECLARE_CLASS_GAMEROOT( CBotDecision, IBotDecision );
    DECLARE_COMPONENT( BOT_COMPONENT_DECISION );

    CBotDecision( IBot *bot ) : BaseClass( bot )
    {
    }

    virtual void Update() {

    }

public:
    virtual bool ShouldLookDangerSpot() const;
    virtual bool ShouldLookInterestingSpot() const;
    virtual bool ShouldLookRandomSpot() const;
    virtual bool ShouldLookSquadMember() const;

    virtual bool ShouldLookThreat() const;

    virtual bool ShouldFollow() const;

    virtual bool ShouldUpdateNavigation() const;
    virtual bool ShouldTeleport( const Vector &vecGoal ) const;
    virtual bool ShouldWiggle() const;

    virtual bool ShouldRun() const;
    virtual bool ShouldSneak() const;
    virtual bool ShouldCrouch() const;
    virtual bool ShouldJump() const;

    virtual bool CanHuntThreat() const;
    virtual bool ShouldInvestigateSound() const;
    virtual bool ShouldCover() const;
    virtual bool ShouldGrabWeapon( CBaseWeapon *pWeapon ) const;
    virtual bool ShouldSwitchToWeapon( CBaseWeapon *pWeapon ) const;
    virtual bool ShouldHelpFriends() const;
    virtual bool ShouldHelpDejectedFriend( CPlayer *pDejected ) const;

    virtual bool IsLowHealth() const;
    virtual bool CanMove() const;

    virtual bool IsUsingFiregun() const;

    virtual bool CanShoot() const {
        return (!m_ShotRateTimer.HasStarted() || m_ShotRateTimer.IsElapsed());
    }

    virtual bool CanAttack() const;
    virtual bool CanCrouchAttack() const;
    virtual bool ShouldCrouchAttack() const;

    virtual bool IsEnemy( CBaseEntity *pEntity ) const;
    virtual bool IsFriend( CBaseEntity *pEntity ) const;
    virtual bool IsSelf( CBaseEntity *pEntity ) const;

    virtual bool IsBetterEnemy( CBaseEntity *pEnemy, CBaseEntity *pPrevious ) const;

    virtual bool CanBeEnemy( CBaseEntity *pEnemy ) const {
        return true;
    }

    virtual bool IsDangerousEnemy( CBaseEntity *pEnemy = NULL ) const;
    virtual bool IsImportantEnemy( CBaseEntity *pEnemy = NULL ) const;
    virtual bool IsPrimaryThreatLost() const;

    virtual bool ShouldMustBeCareful() const;

    virtual void SwitchToBestWeapon();
    virtual bool GetNearestCover( float radius = GET_COVER_RADIUS, Vector *vecCoverSpot = NULL ) const;
    virtual bool IsInCoverPosition() const;

    virtual float GetWeaponIdealRange( CBaseWeapon *pWeapon = NULL ) const;

    virtual BCOND ShouldRangeAttack1();
    virtual BCOND ShouldRangeAttack2();
    virtual BCOND ShouldMeleeAttack1();
    virtual BCOND ShouldMeleeAttack2();

    virtual bool IsAbleToSee( CBaseEntity *entity, FieldOfViewCheckType checkFOV = USE_FOV ) const;
    virtual bool IsAbleToSee( const Vector &pos, FieldOfViewCheckType checkFOV = USE_FOV ) const;

    virtual bool IsInFieldOfView( CBaseEntity *entity ) const;
    virtual bool IsInFieldOfView( const Vector &pos ) const;

    virtual bool IsLineOfSightClear( CBaseEntity *entity, CBaseEntity **hit = NULL ) const;
    virtual bool IsLineOfSightClear( const Vector &pos, CBaseEntity *entityToIgnore = NULL, CBaseEntity **hit = NULL ) const;

public:
    CountdownTimer m_RandomAimTimer;
    CountdownTimer m_IntestingAimTimer;
    CountdownTimer m_BlockLookAroundTimer;
    CountdownTimer m_ShotRateTimer;
};


#endif // BOT_COMPONENTS_H