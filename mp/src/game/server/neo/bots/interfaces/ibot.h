//========= Copyright © 1996-2005, Valve Corporation, All rights reserved. ============//
// Authors: 
// Iván Bravo Bravo (linkedin.com/in/ivanbravobravo), 2017

#ifndef IBOT_H
#define IBOT_H

#ifdef _WIN32
#pragma once
#endif

#include "bots\bot_defs.h"
#include "bots\bot_utils.h"

class IBotSchedule;
class IBotComponent;

class IBotVision;
class IBotAttack;
class IBotMemory;
class IBotLocomotion;
class IBotFollow;
class IBotDecision;

class CSquad;

#ifdef INSOURCE_DLL
#include "in_shareddefs.h"

class CPlayer;
#endif

//================================================================================
// Information
//================================================================================

static int g_botID = 0;

//================================================================================
// Macros
//================================================================================

// VPROF
#define VPROF_BUDGETGROUP_BOTS _T("Bots") 

//================================================================================
// Bot Artificial Intelligence Interface
// You can create a custom bot central system using this interface.
//================================================================================
abstract_class IBot : public CPlayerInfo
{
public:
    IBot( CBasePlayer *parent )
    {
        SetDefLessFunc( m_nComponents );
        SetDefLessFunc( m_nSchedules );

        m_pProfile = new CBotProfile();
        m_iPerformance = BOT_PERFORMANCE_AWAKE;
        m_pParent = parent;
    }

    virtual CPlayer *GetHost() const {
        return (CPlayer *)m_pParent;
    }

    virtual BotPerformance GetPerformance() {
        return m_iPerformance;
    }

    virtual void SetPerformance( BotPerformance value ) {
        m_iPerformance = value;
    }

    virtual CUserCmd *GetUserCommand() {
        return m_cmd;
    }

    virtual CUserCmd *GetLastCmd() {
        return m_lastCmd;
    }

    virtual void SetTacticalMode( int attitude ) {
        m_iTacticalMode = attitude;
    }

    virtual int GetTacticalMode() {
        return m_iTacticalMode;
    }

    virtual CBotProfile *GetProfile() const {
        return m_pProfile;
    }

    virtual float GetStateDuration() {
        return (m_iStateTimer.HasStarted()) ? m_iStateTimer.GetRemainingTime() : -1;
    }

    virtual BotState GetState() const {
        return m_iState;
    }

    virtual bool IsIdle() const {
        return (GetState() == STATE_IDLE);
    }

    virtual bool IsAlerted() const {
        return (GetState() == STATE_ALERT);
    }

    virtual bool IsCombating() const {
        return (GetState() == STATE_COMBAT);
    }

    virtual bool IsPanicked() const {
        return (GetState() == STATE_PANIC);
    }

    virtual IBotSchedule *GetActiveSchedule() const {
        return m_nActiveSchedule;
    }

    virtual void Spawn() = 0;
    virtual void Update() = 0;
    virtual void PlayerMove( CUserCmd *cmd ) = 0;

    virtual bool CanRunAI() = 0;
    virtual void Upkeep() = 0;
    virtual void RunAI() = 0;

    virtual void UpdateComponents( bool important = false ) = 0;

    virtual void MimicThink( int ) = 0;
    virtual void Kick() = 0;

    virtual void InjectMovement( NavRelativeDirType direction ) = 0;
    virtual void InjectButton( int btn ) = 0;

    virtual CAI_Senses *GetSenses() = 0;
    virtual const CAI_Senses *GetSenses() const = 0;

    virtual void OnLooked( int iDistance ) = 0;
    virtual void OnLooked( CBaseEntity *pEntity ) = 0;
    virtual void OnListened() = 0;

    virtual void OnTakeDamage( const CTakeDamageInfo &info ) = 0;
    virtual void OnDeath( const CTakeDamageInfo &info ) = 0;

    virtual void SetSkill( int level ) = 0;

    virtual void SetState( BotState state, float duration = 3.0f ) = 0;
    virtual void CleanState() = 0;

    virtual void Panic( float duration = -1.0f ) = 0;
    virtual void Alert( float duration = -1.0f ) = 0;
    virtual void Idle() = 0;
    virtual void Combat() = 0;

    virtual void SetCondition( BCOND condition ) = 0;
    virtual void ClearCondition( BCOND condition ) = 0;
    virtual bool HasCondition( BCOND condition ) const = 0;

    virtual void AddComponent( IBotComponent *pComponent ) = 0;

    template<typename COMPONENT>
    COMPONENT GetComponent( int id ) const = 0;

    template<typename COMPONENT>
    COMPONENT GetComponent( int id ) = 0;

    virtual void SetUpComponents() = 0;
    virtual void SetUpSchedules() = 0;

    virtual IBotSchedule *GetSchedule( int schedule ) = 0;
    virtual int GetActiveScheduleID() = 0;

    virtual int SelectIdealSchedule() = 0;
    virtual int TranslateSchedule( int schedule ) = 0;
    virtual void UpdateSchedule() = 0;

    virtual bool TaskStart( BotTaskInfo_t *info ) = 0;
    virtual bool TaskRun( BotTaskInfo_t *info ) = 0;
    virtual void TaskComplete() = 0;
    virtual void TaskFail( const char *pWhy ) = 0;

    virtual void GatherConditions() = 0;

    virtual void GatherHealthConditions() = 0;
    virtual void GatherWeaponConditions() = 0;
    virtual void GatherEnemyConditions() = 0;
    virtual void GatherAttackConditions() = 0;
    virtual void GatherLocomotionConditions() = 0;

    virtual CBaseEntity *GetEnemy() const = 0;
    virtual CEntityMemory *GetPrimaryThreat() const = 0;

    virtual void SetEnemy( CBaseEntity * pEnemy, bool bUpdate = false ) = 0;
    virtual void SetPeaceful( bool enabled ) = 0;

    virtual CSquad *GetSquad() = 0;
    virtual void SetSquad( CSquad *pSquad ) = 0;
    virtual void SetSquad( const char *name ) = 0;

    virtual bool IsSquadLeader() = 0;
    virtual CPlayer *GetSquadLeader() = 0;
    virtual IBot *GetSquadLeaderAI() = 0;

    virtual void OnMemberTakeDamage( CPlayer *pMember, const CTakeDamageInfo &info ) = 0;
    virtual void OnMemberDeath( CPlayer *pMember, const CTakeDamageInfo &info ) = 0;
    virtual void OnMemberReportEnemy( CPlayer *pMember, CBaseEntity *pEnemy ) = 0;

    virtual bool ShouldShowDebug() = 0;
    virtual void DebugDisplay() = 0;
    virtual void DebugScreenText( const char *pText, Color color = Color( 255, 255, 255, 150 ), float yPosition = -1, float duration = 0.15f ) = 0;
    virtual void DebugAddMessage( char *format, ... ) = 0;

    virtual IBotVision *GetVision() const = 0;
    virtual IBotAttack *GetAttack() const = 0;
    virtual IBotMemory *GetMemory() const = 0;
    virtual IBotLocomotion *GetLocomotion() const = 0;
    virtual IBotFollow *GetFollow() const = 0;
    virtual IBotDecision *GetDecision() const = 0;

protected:
    BotState m_iState;
    CBotProfile *m_pProfile;
    int m_iTacticalMode;
    BotPerformance m_iPerformance;
    CountdownTimer m_iStateTimer;

    // Components
    CUtlMap<int, IBotComponent *> m_nComponents;

    // Schedules
    IBotSchedule *m_nActiveSchedule;
    CUtlMap<int, IBotSchedule *> m_nSchedules;

    // Cmd
    CUserCmd *m_lastCmd;
    CUserCmd *m_cmd;

    // Conditions
    CFlagsBits m_nConditions;

    // Debug
    CUtlVector<DebugMessage> m_debugMessages;
    float m_flDebugYPosition;

    friend class CBotSpawn;
    friend class DirectorManager;
};

#endif