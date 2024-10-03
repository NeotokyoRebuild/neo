//========= Copyright © 1996-2005, Valve Corporation, All rights reserved. ============//
// Authors: 
// Iván Bravo Bravo (linkedin.com/in/ivanbravobravo), 2017

#ifndef IBOT_DECISION_H
#define IBOT_DECISION_H

#pragma once

//================================================================================
// Decision component
// Everything related to the decisions that the bot must take.
// You can create a custom decision component using this interface.
//================================================================================
abstract_class IBotDecision : public IBotComponent
{
public:
    DECLARE_CLASS_GAMEROOT( IBotDecision, IBotComponent );

    IBotDecision( IBot *bot ) : BaseClass( bot )
    {
    }

    enum FieldOfViewCheckType
    {
        USE_FOV, 
        DISREGARD_FOV
    };

    enum LineOfSightCheckType
    {
        IGNORE_NOTHING,
        IGNORE_ACTORS
    };

public:
    virtual bool CanLookNoVisibleSpots() const {
        return true;
    }

    virtual bool CanLookSurroundings() const {
        return true;
    }

    virtual bool ShouldLookDangerSpot() const = 0;
    virtual bool ShouldLookInterestingSpot() const = 0;
    virtual bool ShouldLookRandomSpot() const = 0;
    virtual bool ShouldLookSquadMember() const = 0;

    virtual bool ShouldLookThreat() const = 0;

    virtual bool ShouldFollow() const = 0;

    virtual bool ShouldUpdateNavigation() const = 0;
    virtual bool ShouldTeleport( const Vector &vecGoal ) const = 0;
    virtual bool ShouldWiggle() const = 0;

    virtual bool ShouldRun() const = 0;
    virtual bool ShouldSneak() const = 0;
    virtual bool ShouldCrouch() const = 0;
    virtual bool ShouldJump() const = 0;

    virtual bool CanHuntThreat() const = 0;
    virtual bool ShouldInvestigateSound() const = 0;
    virtual bool ShouldCover() const = 0;
    virtual bool ShouldGrabWeapon( CBaseWeapon *pWeapon ) const = 0;
    virtual bool ShouldSwitchToWeapon( CBaseWeapon *pWeapon ) const = 0;
    virtual bool ShouldHelpFriends() const = 0;
    virtual bool ShouldHelpDejectedFriend( CPlayer *pDejected ) const = 0;

    virtual bool IsLowHealth() const = 0;
    virtual bool CanMove() const = 0;

    virtual bool IsUsingFiregun() const = 0;

    virtual bool CanAttack() const = 0;
    virtual bool CanCrouchAttack() const = 0;
    virtual bool ShouldCrouchAttack() const = 0;

    virtual bool IsEnemy( CBaseEntity *pEntity ) const = 0;
    virtual bool IsFriend( CBaseEntity *pEntity ) const = 0;
    virtual bool IsSelf( CBaseEntity *pEntity ) const = 0;

    virtual bool IsBetterEnemy( CBaseEntity *pEnemy, CBaseEntity *pPrevious ) const = 0;
    virtual bool CanBeEnemy( CBaseEntity *pEnemy ) const = 0;
    virtual bool IsDangerousEnemy( CBaseEntity *pEnemy = NULL ) const = 0;
    virtual bool IsImportantEnemy( CBaseEntity *pEnemy = NULL ) const = 0;
    virtual bool IsPrimaryThreatLost() const = 0;

    virtual bool ShouldMustBeCareful() const = 0;

    virtual void SwitchToBestWeapon() = 0;
    virtual bool GetNearestCover( float radius = GET_COVER_RADIUS, Vector *vecCoverSpot = NULL ) const = 0;
    virtual bool IsInCoverPosition() const = 0;

    virtual float GetWeaponIdealRange( CBaseWeapon *pWeapon = NULL ) const = 0;

    virtual BCOND ShouldRangeAttack1() = 0;
    virtual BCOND ShouldRangeAttack2() = 0;
    virtual BCOND ShouldMeleeAttack1() = 0;
    virtual BCOND ShouldMeleeAttack2() = 0;

    virtual bool IsAbleToSee( CBaseEntity *entity, FieldOfViewCheckType checkFOV = USE_FOV ) const = 0;
    virtual bool IsAbleToSee( const Vector &pos, FieldOfViewCheckType checkFOV = USE_FOV ) const = 0;

    virtual bool IsInFieldOfView( CBaseEntity *entity ) const = 0;
    virtual bool IsInFieldOfView( const Vector &pos ) const = 0;

    virtual bool IsLineOfSightClear( CBaseEntity *entity, CBaseEntity **hit = NULL ) const = 0;
    virtual bool IsLineOfSightClear( const Vector &pos, CBaseEntity *entityToIgnore = NULL, CBaseEntity **hit = NULL ) const = 0;
};

#endif // IBOT_DECISION_H