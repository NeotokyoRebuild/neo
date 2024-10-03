//========= Copyright © 1996-2005, Valve Corporation, All rights reserved. ============//
// Authors: 
// Iván Bravo Bravo (linkedin.com/in/ivanbravobravo), 2017

#ifndef BOT_H
#define BOT_H

#ifdef _WIN32
#pragma once
#endif

#include "utlflags.h."

#ifdef INSOURCE_DLL
#include "in_player.h"
#endif

#include "bots\bot_defs.h"
#include "bots\interfaces\ibot.h"
#include "bots\components\bot_components.h"
#include "bots\schedules\bot_schedules.h"

#include "squad.h"

class CBotProfile;
class CEntityMemory;

//================================================================================
// Artificial Intelligence to create a player controlled by the computer
//================================================================================
class CBot : public IBot
{
public:
    DECLARE_CLASS_GAMEROOT( CBot, IBot );

    CBot( CBasePlayer *parent );

public:
    virtual void Spawn();
    virtual void Update();
    virtual void PlayerMove( CUserCmd *cmd );

    virtual bool CanRunAI();
    virtual void Upkeep();
    virtual void RunAI();

    virtual void UpdateComponents( bool important = false );

    virtual void MimicThink( int );
    virtual void Kick();

    virtual void InjectMovement( NavRelativeDirType direction );
    virtual void InjectButton( int btn );
    
    CAI_Senses *GetSenses() { return GetHost()->GetSenses(); }
    const CAI_Senses *GetSenses() const { return GetHost()->GetSenses(); }

    virtual void OnLooked( int iDistance );
    virtual void OnLooked( CBaseEntity *pEntity );
    virtual void OnListened();

    virtual void OnTakeDamage( const CTakeDamageInfo &info );
    virtual void OnDeath( const CTakeDamageInfo &info );

    // Skill
    virtual void SetSkill( int level );
    
    virtual void SetState( BotState state, float duration = 3.0f );
    virtual void CleanState();

    virtual void Panic( float duration = -1.0f );
    virtual void Alert( float duration = -1.0f );
    virtual void Idle();
    virtual void Combat();

    virtual void SetCondition( BCOND condition );
    virtual void ClearCondition( BCOND condition );
    virtual bool HasCondition( BCOND condition ) const;

    virtual void AddComponent( IBotComponent *pComponent );

    template<typename COMPONENT>
    COMPONENT GetComponent( int id ) const;

    template<typename COMPONENT>
    COMPONENT GetComponent( int id );

    virtual void SetUpComponents();
    virtual void SetUpSchedules();

    virtual IBotSchedule *GetSchedule( int schedule );
    virtual int GetActiveScheduleID();

    virtual int SelectIdealSchedule();
    virtual int TranslateSchedule( int schedule ) { return schedule; }
    virtual void UpdateSchedule();

	virtual bool TaskStart( BotTaskInfo_t *info ) { return false; }
	virtual bool TaskRun( BotTaskInfo_t *info ) { return false; }
	virtual void TaskComplete();
	virtual void TaskFail( const char *pWhy );

    virtual void GatherConditions();

    virtual void GatherHealthConditions();
    virtual void GatherWeaponConditions();
    virtual void GatherEnemyConditions();
    virtual void GatherAttackConditions();
    virtual void GatherLocomotionConditions();

    virtual CBaseEntity *GetEnemy() const;
    virtual CEntityMemory *GetPrimaryThreat() const;

    virtual void SetEnemy( CBaseEntity * pEnemy, bool bUpdate = false );
    virtual void SetPeaceful( bool enabled );

    virtual CSquad *GetSquad();
    virtual void SetSquad( CSquad *pSquad );
    virtual void SetSquad( const char *name );

    virtual bool IsSquadLeader();
    virtual CPlayer *GetSquadLeader();
    virtual IBot *GetSquadLeaderAI();

    virtual void OnMemberTakeDamage( CPlayer *pMember, const CTakeDamageInfo &info );
    virtual void OnMemberDeath( CPlayer *pMember, const CTakeDamageInfo &info );
    virtual void OnMemberReportEnemy( CPlayer *pMember, CBaseEntity *pEnemy );

	virtual bool ShouldShowDebug();
    virtual void DebugDisplay();
    virtual void DebugScreenText( const char *pText, Color color = Color(255, 255, 255, 150), float yPosition = -1, float duration = 0.15f );
    virtual void DebugAddMessage( char *format, ... );

    virtual IBotVision *GetVision() const {
        return GetComponent<IBotVision *>( BOT_COMPONENT_VISION );
    }

    virtual IBotAttack *GetAttack() const {
        return GetComponent<IBotAttack *>( BOT_COMPONENT_ATTACK );
    }

    virtual IBotMemory *GetMemory() const {
        return GetComponent<IBotMemory *>( BOT_COMPONENT_MEMORY );
    }

    virtual IBotLocomotion *GetLocomotion() const {
        return GetComponent<IBotLocomotion *>( BOT_COMPONENT_LOCOMOTION );
    }

    virtual IBotFollow *GetFollow() const {
        return GetComponent<IBotFollow *>( BOT_COMPONENT_FOLLOW );
    }

    virtual IBotDecision *GetDecision() const {
        return GetComponent<IBotDecision *>( BOT_COMPONENT_DECISION );
    }

public:
    virtual void ApplyDebugCommands();
    virtual void Possess( CPlayer *pPlayer );
    virtual bool IsLocalPlayerWatchingMe();

    // Functions to debug if attempts have been made to use conditions before collecting them.
    virtual void BlockConditions() {
        m_bConditionsBlocked = true;
        m_nConditions.ClearAll();
    }

    virtual void UnblockConditions() {
        m_bConditionsBlocked = true;
    }

    virtual bool IsConditionsAllowed() const {
        return (!m_nConditions.IsAllClear() && m_bConditionsBlocked == false);
    }

private:
    CBot( const CBot & );

protected:
    bool m_bConditionsBlocked;

    // Damage
    int m_iRepeatedDamageTimes;
    float m_flDamageAccumulated;

    // Timer
    CFastTimer m_RunTimer;
    CFastTimer m_ScheduleTimer;

    friend class DirectorManager;
    friend class IBotSchedule;
};

//================================================================================
// Author: Michael S. Booth (linkedin.com/in/michaelbooth), 2003
//================================================================================
class CSimpleBotPathCost
{
public:
    CSimpleBotPathCost( IBot *bot )
    {
        m_pBot = bot;
    }

    float operator() ( CNavArea *area, CNavArea *fromArea, const CNavLadder *ladder, const CFuncElevator *elevator, float length )
    {
        float baseDangerFactor = 100.0f;
        float dangerFactor = (1.0f - (0.95f * m_pBot->GetProfile()->GetAggression())) * baseDangerFactor;
        float dist;

        if ( fromArea == NULL )
            return 0.0f;

        if ( !m_pBot->GetLocomotion()->IsAreaTraversable( fromArea, area ) )
            return -1.0f;

        if ( ladder ) {
            // ladders are slow to use
            const float ladderPenalty = 2.0f;
            dist = ladderPenalty * ladder->m_length;
        }
        else if ( length > 0.0 ) {
            // optimization to avoid recomputing length
            dist = length;
        }
        else {
            dist = (area->GetCenter() - fromArea->GetCenter()).Length();
        }

        // Cost by distance
        float cost = dist + fromArea->GetCostSoFar();

        // check height change
        float deltaZ = fromArea->ComputeAdjacentConnectionHeightChange( area );

        if ( deltaZ >= m_pBot->GetLocomotion()->GetStepHeight() ) {
            if ( deltaZ >= m_pBot->GetLocomotion()->GetMaxJumpHeight() ) {
                return -1.0f;
            }

            // jumping is slower than flat ground
            const float jumpPenalty = 5.0f;
            cost += jumpPenalty * dist;
        }
        else if ( deltaZ < -m_pBot->GetLocomotion()->GetDeathDropHeight() ) {
            // too far to drop
            return -1.0f;
        }





        // We get the height difference
        /*float ourZ = fromArea->GetCenter().z; //fromArea->GetZ( fromArea->GetCenter() );
        float areaZ = area->GetCenter().z;//area->GetZ( area->GetCenter() );

        float fallDistance = ourZ - areaZ;

        if ( ladder && ladder->m_bottom.z < fromArea->GetCenter().z && ladder->m_bottom.z > area->GetCenter().z ) {
            fallDistance = ladder->m_bottom.z - area->GetCenter().z;
        }

        // We add cost if access to this area is necessary to jump from a great distance (and without water in the fall)
        if ( !area->IsUnderwater() && !area->IsConnected( fromArea, NUM_DIRECTIONS ) ) {
#ifdef INSOURCE_DLL
            float fallDamage = m_pBot->GetApproximateFallDamage( fallDistance );
#else
            // empirically discovered height values
            const float slope = 0.2178f;
            const float intercept = 26.0f;

            float fallDamage = slope * fallDistance - intercept;
            fallDamage =  MAX( 0.0f, fallDamage );
#endif

            if ( fallDamage > 0.0f ) {
                // if the fall would kill us, don't use it
                const float deathFallMargin = 10.0f;

                if ( fallDamage + deathFallMargin >= m_pBot->GetHealth() )
                    return -1.0f;

                // if we need to get there in a hurry, ignore minor pain
                //const float painTolerance = 15.0f * m_nBot->GetProfile()->GetAggression() + 10.0f;
                const float painTolerance = 30.0f;

                if ( fallDamage > painTolerance ) {
                    // cost is proportional to how much it hurts when we fall
                    // 10 points - not a big deal, 50 points - ouch!
                    cost += 100.0f * fallDamage * fallDamage;
                }
            }
        }
        */

        if ( area->IsUnderwater() ) {
            float penalty = 20.0f;
            cost += penalty * dist;
        }

        // Is above us
        /*if ( !ladder && fallDistance < 0 ) {
            // We can not reach with our highest jump
            if ( fallDistance < -(JumpCrouchHeight) )
                return -1.0f;

            float penalty = 13.0f;
            cost += penalty * (fabs( fallDistance ) + dist);
        }*/

        if ( area->GetAttributes() & (NAV_MESH_CROUCH | NAV_MESH_WALK) ) {
            // these areas are very slow to move through
            //float penalty = (m_route == FASTEST_ROUTE) ? 20.0f : 5.0f;
            float penalty = 5.0f;
            cost += penalty * dist;
        }

        if ( area->GetAttributes() & NAV_MESH_JUMP ) {
            const float jumpPenalty = 5.0f;
            cost += jumpPenalty * dist;
        }

        if ( area->GetAttributes() & NAV_MESH_AVOID ) {
            const float avoidPenalty = 10.0f;
            cost += avoidPenalty * dist;
        }

        if ( area->HasAvoidanceObstacle() ) {
            const float blockedPenalty = 20.0f;
            cost += blockedPenalty * dist;
        }

        {
            // add in cost of teammates in the way

            // approximate density of teammates based on area
            float size = (area->GetSizeX() + area->GetSizeY()) / 2.0f;

            // degenerate check
            if ( size >= 1.0f ) {
                // cost is proportional to the density of teammates in this area
                const float costPerFriendPerUnit = 50000.0f;
                cost += costPerFriendPerUnit * (float)area->GetPlayerCount( m_pBot->GetHost()->GetTeamNumber() ) / size;
            }
        }

        // add in the danger of this path - danger is per unit length travelled
        cost += dist * baseDangerFactor * area->GetDanger( m_pBot->GetHost()->GetTeamNumber() );

        return cost;
    }

protected:
    IBot *m_pBot;
};

extern CPlayer *CreateBot( const char *pPlayername, const Vector *vecPosition, const QAngle *angles );

template<typename COMPONENT>
inline COMPONENT CBot::GetComponent( int id ) const
{
    int index = m_nComponents.Find( id );

    if ( !m_nComponents.IsValidIndex( index ) )
        return NULL;

    return dynamic_cast<COMPONENT>( m_nComponents.Element( index ) );
}

template<typename COMPONENT>
inline COMPONENT CBot::GetComponent( int id )
{
    int index = m_nComponents.Find( id );

    if ( !m_nComponents.IsValidIndex( index ) )
        return NULL;

    return dynamic_cast<COMPONENT>(m_nComponents.Element( index ));
}


#endif // BOT_H
