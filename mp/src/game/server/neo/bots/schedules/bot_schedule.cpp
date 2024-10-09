//========= Copyright © 1996-2005, Valve Corporation, All rights reserved. ============//
// Authors: 
// Iván Bravo Bravo (linkedin.com/in/ivanbravobravo), 2017

#include "cbase.h"

#include "bots\bot.h"
#include "bots\interfaces\ibotschedule.h"

#ifdef INSOURCE_DLL
#include "in_utils.h"
#else
#include "bots\in_utils.h"
#endif

#include "in_buttons.h"

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

//================================================================================
//================================================================================
void IBotSchedule::Reset()
{
    BaseClass::Reset();

    m_bFailed = false;
    m_bStarted = false;
    m_bFinished = false;

    m_nActiveTask = NULL;
    Assert( m_Tasks.Count() == 0 );
    m_iScheduleOnFail = SCHEDULE_NONE;

    m_WaitTimer.Invalidate();
    m_FailTimer.Invalidate();
}

//================================================================================
//================================================================================
void IBotSchedule::Start()
{
    Reset();

    m_bStarted = true;
    m_StartTimer.Start();

    if ( GetLocomotion() ) {
        GetLocomotion()->StopDrive();
        GetLocomotion()->Walk();
        GetLocomotion()->StandUp();
    }

    Install_Tasks();
    GetBot()->DebugAddMessage( "[%s] Started", g_BotSchedules[GetID()] );
}

//================================================================================
//================================================================================
void IBotSchedule::Finish()
{
    m_bFinished = true;
    m_bStarted = false;

    m_nActiveTask = NULL;
    m_flLastDesire = BOT_DESIRE_NONE;
    m_StartTimer.Invalidate();

    if ( ItsImportant() ) {
        Assert( m_Tasks.Count() == 0 );
    }

    m_Tasks.Purge();

    if ( GetLocomotion() ) {
        GetLocomotion()->StopDrive();
        GetLocomotion()->Walk();
        GetLocomotion()->StandUp();
    }

    GetBot()->DebugAddMessage( "[%s] Finished", g_BotSchedules[GetID()] );
}

//================================================================================
//================================================================================
void IBotSchedule::Fail( const char *pWhy )
{
	m_bFailed = true;
    m_FailTimer.Start();

    if ( m_iScheduleOnFail != SCHEDULE_NONE ) {
        GetMemory()->UpdateDataMemory( "NextSchedule", m_iScheduleOnFail );
    }

    GetBot()->DebugAddMessage("[%s:%s] Failed: %s", g_BotSchedules[GetID()], GetActiveTaskName(), pWhy);
}

//================================================================================
//================================================================================
BCOND IBotSchedule::GetInterruption()
{
    m_Interrupts.Purge();
    Install_Interruptions();

    FOR_EACH_VEC( m_Interrupts, it )
    {
        BCOND cond = m_Interrupts.Element( it );

        if ( HasCondition( cond ) ) {
            return cond;
        }
    }

    return BCOND_NONE;
}

//================================================================================
//================================================================================
bool IBotSchedule::ShouldInterrupted()
{
    if ( HasFinished() )
        return true;

    if ( HasFailed() )
        return true;

    if ( !GetHost()->IsAlive() )
        return true;

    BCOND interrupt = GetInterruption();

    if ( interrupt != BCOND_NONE ) {
        m_bFailed = true;
        m_FailTimer.Start();
        GetBot()->DebugAddMessage( "[%s:%s] Interrupted by Condition: %s", g_BotSchedules[GetID()], GetActiveTaskName(), g_Conditions[interrupt] );
        return true;
    }

    return false;
}

//================================================================================
//================================================================================
float IBotSchedule::GetInternalDesire()
{
    if ( HasStarted() ) {
        // We have to interrupt it
        if ( ShouldInterrupted() )
            return BOT_DESIRE_NONE;

        // It is important: no matter the current level of desire, it is necessary to complete all tasks.
        if ( ItsImportant() )
            return m_flLastDesire;
    }
    else {
        // We failed less than 2s ago
        if ( HasFailed() && GetElapsedTimeSinceFail() < 2.0f )
            return BOT_DESIRE_NONE;

        // One of the interrupts is active
        if ( GetInterruption() != BCOND_NONE )
            return BOT_DESIRE_NONE;

        if ( GetMemory() ) {
            int nextSchedule = GetDataMemoryInt( "NextSchedule" );

            // Another schedule has asked to activate this
            if ( GetID() == nextSchedule ) {
                GetMemory()->ForgetData( "NextSchedule" );

                m_flLastDesire = BOT_DESIRE_FORCED;
                return m_flLastDesire;
            }
        }
    }

	m_flLastDesire = GetDesire();
	return m_flLastDesire;
}

//================================================================================
//================================================================================
void IBotSchedule::Update()
{
    VPROF_BUDGET( "IBotSchedule::Update", VPROF_BUDGETGROUP_BOTS );

    Assert( m_Tasks.Count() > 0 );
    BotTaskInfo_t *idealTask = m_Tasks.Element( 0 );

    if ( idealTask != m_nActiveTask ) {
        m_nActiveTask = idealTask;
        TaskStart();
        return;
    }

    TaskRun();
}

//================================================================================
//================================================================================
void IBotSchedule::Wait( float seconds )
{
    m_WaitTimer.Start( seconds );
    GetBot()->DebugAddMessage( "[%s:%s] %.2fs", g_BotSchedules[GetID()], GetActiveTaskName(), seconds );
}

//================================================================================
//================================================================================
bool IBotSchedule::SavePosition( const Vector &position, float duration )
{
    Assert( GetMemory() );

    if ( !GetMemory() ) {
        Fail( "SavePosition() without memory." );
        return false;
    }

    GetMemory()->UpdateDataMemory( "SavedPosition", position, duration );
    return true;
}

//================================================================================
//================================================================================
const Vector &IBotSchedule::GetSavedPosition()
{
    Assert( GetMemory() );

    if ( !GetMemory() ) {
        Fail( "GetSavedPosition() without memory." );
        return vec3_invalid;
    }

    return GetDataMemoryVector( "SavedPosition" );
}

//================================================================================
//================================================================================
const char *IBotSchedule::GetActiveTaskName() const
{
    BotTaskInfo_t *info = GetActiveTask();

    if ( !info ) {
        return "UNKNOWN";
    }

    if ( info->task >= BLAST_TASK ) {
        return UTIL_VarArgs( "CUSTOM: %i", info->task );
    }

    return g_BotTasks[ info->task ];
}

//================================================================================
//================================================================================
void IBotSchedule::TaskStart()
{
    BotTaskInfo_t *pTask = GetActiveTask();

    if ( GetBot()->TaskStart( pTask ) ) {
        return;
    }

    switch ( pTask->task ) {
        case BTASK_MOVE_DESTINATION:
        case BTASK_AIM:
        case BTASK_CALL_FOR_BACKUP:
        case BTASK_HUNT_ENEMY:
        {
            // We place them to not generate an assert
            break;
        }

        case BTASK_WAIT:
        {
            Wait( pTask->flValue );
            break;
        }

        case BTASK_SET_TOLERANCE:
        {
            if ( GetLocomotion() ) {
                GetLocomotion()->SetTolerance( pTask->flValue );
            }

            TaskComplete();
            break;
        }

        case BTASK_PLAY_ANIMATION:
        {
            Activity activity = (Activity)pTask->iValue;
#ifdef INSOURCE_DLL
            GetHost()->DoAnimationEvent( PLAYERANIMEVENT_CUSTOM, activity );
#else
            Assert( !"Implement in your mod" );
            TaskComplete();
#endif
            break;
        }

        case BTASK_PLAY_GESTURE:
        {
            Activity activity = (Activity)pTask->iValue;
#ifdef INSOURCE_DLL
            GetHost()->DoAnimationEvent( PLAYERANIMEVENT_CUSTOM_GESTURE, activity );
#else
            Assert( !"Implement in your mod" );
#endif
            TaskComplete();
            break;
        }

        case BTASK_PLAY_SEQUENCE:
        {
#ifdef INSOURCE_DLL
            GetHost()->DoAnimationEvent( PLAYERANIMEVENT_CUSTOM_SEQUENCE, pTask->iValue );
#else
            Assert( !"Implement in your mod" );
            TaskComplete();
#endif
            break;
        }

        case BTASK_SAVE_POSITION:
        {
            SavePosition( GetAbsOrigin() );
            TaskComplete();
            break;
        }

        case BTASK_RESTORE_POSITION:
        {
            Assert( GetSavedPosition().IsValid() );

            if ( GetFollow() && GetFollow()->IsFollowingActive() ) {
                TaskComplete();
                return;
            }

            break;
        }

        case BTASK_SAVE_SPAWN_POSITION:
        {
            Assert( GetMemory() );

            if ( !GetMemory() ) {
                Fail( "Without memory." );
                return;
            }

            SavePosition( GetDataMemoryVector( "SpawnPosition" ) );
            break;
        }

        case BTASK_MOVE_LIVE_DESTINATION:
        {
            // TODO
            break;
        }

        case BTASK_SAVE_ASIDE_SPOT:
        {
            CNavArea *pArea = GetHost()->GetLastKnownArea();

            if ( pArea == NULL ) {
                pArea = TheNavMesh->GetNearestNavArea( GetHost() );
            }

            if ( pArea == NULL ) {
                Fail( "Without last known area." );
                return;
            }

            SavePosition( pArea->GetRandomPoint(), 5.0f );
            TaskComplete();

            break;
        }

        case BTASK_SAVE_COVER_SPOT:
        {
            // We are already in a cover position, 
            // we skip this task and the next one (which should be MOVE_DESTINATION)
            if ( GetDecision()->IsInCoverPosition() ) {
                TaskComplete();
                TaskComplete();
                return;
            }

            Vector vecGoal;
            float radius = pTask->flValue;

            if ( radius <= 0.0f ) {
                radius = GET_COVER_RADIUS;
            }

            if ( !GetDecision()->GetNearestCover( radius, &vecGoal ) ) {
                Fail( "No cover spot found" );
                return;
            }

            Assert( vecGoal.IsValid() );
            SavePosition( vecGoal );

            TaskComplete();
            break;
        }

        case BTASK_SAVE_FAR_COVER_SPOT:
        {
            Vector vecGoal;
            float minRadius = pTask->flValue;

            if ( minRadius <= 0 ) {
                minRadius = (GET_COVER_RADIUS * 2);
            }

            float maxRadius = (minRadius * 3);

            CSpotCriteria criteria;
            criteria.SetMaxRange( maxRadius );
            criteria.SetMinDistanceAvoid( minRadius );
            criteria.UseNearest( false );
            criteria.UseRandom( true );
            criteria.OutOfVisibility( true );
            criteria.AvoidTeam( GetBot()->GetEnemy() );

            if ( !Utils::FindCoverPosition( &vecGoal, GetHost(), criteria ) ) {
                Fail( "No far cover spot found" );
                return;
            }

            Assert( vecGoal.IsValid() );
            SavePosition( vecGoal );

            TaskComplete();
            break;
        }

        case BTASK_USE:
        {
            InjectButton( IN_USE );
            TaskComplete();
            break;
        }

        case BTASK_JUMP:
        {
            if ( GetLocomotion() ) {
                GetLocomotion()->Jump();
            }
            else {
                InjectButton( IN_JUMP );
            }

            TaskComplete();
            break;
        }

        case BTASK_CROUCH:
        {
            if ( !GetLocomotion() ) {
                Fail( "Without Locomotion" );
                return;
            }

            GetLocomotion()->Crouch();
            TaskComplete();
            break;
        }

        case BTASK_STANDUP:
        {
            if ( !GetLocomotion() ) {
                Fail( "Without Locomotion" );
                return;
            }

            GetLocomotion()->StandUp();
            TaskComplete();
            break;
        }

        case BTASK_RUN:
        {
            if ( !GetLocomotion() ) {
                Fail( "Without Locomotion" );
                return;
            }

            GetLocomotion()->Run();
            TaskComplete();
            break;
        }

        case BTASK_SNEAK:
        {
            if ( !GetLocomotion() ) {
                Fail( "Without Locomotion" );
                return;
            }

            GetLocomotion()->Sneak();
            TaskComplete();
            break;
        }

        case BTASK_WALK:
        {
            if ( !GetLocomotion() ) {
                Fail( "Without Locomotion" );
                return;
            }

            GetLocomotion()->Walk();
            TaskComplete();
            break;
        }

        case BTASK_RELOAD:
        {
            InjectButton( IN_RELOAD );

            // Async
            if ( pTask->iValue == 1 ) {
                TaskComplete();
            }
            break;
        }

        case BTASK_RELOAD_SAFE:
        {
            bool reload = false;

            if ( HasCondition( BCOND_WITHOUT_ENEMY ) ) {
                reload = true;
            }
            else {
                if ( HasCondition( BCOND_ENEMY_LOST ) && HasCondition( BCOND_ENEMY_LAST_POSITION_VISIBLE ) )
                    reload = true;

                if ( HasCondition( BCOND_ENEMY_TOO_FAR ) || HasCondition( BCOND_ENEMY_FAR ) )
                    reload = true;
            }

            if ( reload ) {
                InjectButton( IN_RELOAD );

                // Async
                if ( pTask->iValue == 1 ) {
                    TaskComplete();
                }
            }
            else {
                TaskComplete();
            }

            break;
        }

        // NOTE: This is only a test
        case BTASK_HEAL:
        {
            GetHost()->TakeHealth( 30.0f, DMG_GENERIC );
            TaskComplete();
            break;
        }

        case BTASK_SET_FAIL_SCHEDULE:
        {
            if ( !GetMemory() ) {
                Fail( "Without memory" );
                return;
            }

            m_iScheduleOnFail = pTask->iValue;
            TaskComplete();
            break;
        }

        case BTASK_SET_SCHEDULE:
        {
            if ( !GetMemory() ) {
                Fail( "Without memory" );
                return;
            }

            GetMemory()->UpdateDataMemory( "NextSchedule", pTask->iValue );

            TaskComplete();
            break;
        }

        default:
        {
            Assert( !"TaskStart(): Task not handled!" );
            break;
        }
    }
}

//================================================================================
//================================================================================
void IBotSchedule::TaskRun()
{
    BotTaskInfo_t *pTask = GetActiveTask();

    if ( GetBot()->TaskRun( pTask ) )
        return;

    switch ( pTask->task ) {
        case BTASK_SAVE_POSITION:
        case BTASK_SAVE_COVER_SPOT:
        case BTASK_SAVE_FAR_COVER_SPOT:
        case BTASK_SAVE_SPAWN_POSITION:
        case BTASK_CALL_FOR_BACKUP:
        {
            break;
        }

        case BTASK_WAIT:
        {
            if ( GetMemory() ) {
                GetMemory()->MaintainThreat();
            }

            if ( IsWaitFinished() ) {
                TaskComplete();
            }

            break;
        }

        case BTASK_PLAY_ANIMATION:
        case BTASK_PLAY_SEQUENCE:
        {
            if ( GetMemory() ) {
                GetMemory()->MaintainThreat();
            }

            if ( GetHost()->IsActivityFinished() ) {
                TaskComplete();
            }

            break;
        }

        case BTASK_RESTORE_POSITION:
        {
            if ( GetMemory() ) {
                GetMemory()->MaintainThreat();
            }

            if ( HasCondition( BCOND_SEE_HATE ) ) {
                TaskComplete();
                return;
            }

            Vector vecGoal = GetSavedPosition();

            float distance = GetAbsOrigin().DistTo( vecGoal );
            float tolerance = GetLocomotion()->GetTolerance();

            if ( distance <= tolerance ) {
                TaskComplete();
                return;
            }

            GetLocomotion()->DriveTo( "Restoring Position", vecGoal, PRIORITY_NORMAL );
            break;
        }

        case BTASK_MOVE_DESTINATION:
        {
            Vector vecGoal = GetSavedPosition();
            CBaseEntity *pTarget = pTask->pszValue.Get();

            if ( pTarget ) {
                if ( GetMemory() ) {
                    CEntityMemory *memory = GetMemory()->GetEntityMemory( pTarget );

                    if ( memory ) {
                        vecGoal = memory->GetLastKnownPosition();
                    }
                    else {
                        vecGoal = pTarget->GetAbsOrigin();
                    }
                }
                else {
                    vecGoal = pTarget->GetAbsOrigin();
                }
            }
            else if ( pTask->vecValue.IsValid() ) {
                vecGoal = pTask->vecValue;
            }

            Assert( vecGoal.IsValid() );

            if ( !vecGoal.IsValid() ) {
                Fail( "Invalid goal" );
                return;
            }

            float distance = GetAbsOrigin().DistTo( vecGoal );
            float tolerance = GetLocomotion()->GetTolerance();

            if ( distance <= tolerance ) {
                TaskComplete();
                return;
            }

            GetLocomotion()->DriveTo( "Moving Destination", vecGoal, PRIORITY_HIGH );
            break;
        }

        case BTASK_HUNT_ENEMY:
        {
            if ( !GetLocomotion() ) {
                Fail( "Hunt Enemy: Without Locomotion" );
                return;
            }

            if ( !GetMemory() ) {
                Fail( "Hunt Enemy: Without Memory" );
                return;
            }

            CEntityMemory *memory = GetMemory()->GetPrimaryThreat();

            if ( !memory ) {
                Fail( "Hunt Enemy: Without Enemy Memory" );
                return;
            }

            // We prevent memory from expiring
            memory->Maintain();

            float distance = memory->GetDistance();
            float tolerance = pTask->flValue;

            if ( tolerance < 1.0f ) {
                tolerance = GetLocomotion()->GetTolerance();
            }

            // We are approaching our enemy because our current weapon does not have enough range.
            if ( HasCondition( BCOND_TOO_FAR_TO_ATTACK ) ) {
                CBaseWeapon *pWeapon = GetHost()->GetActiveBaseWeapon();

                if ( pWeapon ) {
                    float range = GetDecision()->GetWeaponIdealRange( pWeapon );

                    if ( pWeapon->IsMeleeWeapon() ) {
                        tolerance = range;
                    }
                    else {
                        tolerance = (range - 100.0f); // Safe
                    }
                }
            }

            bool completed = (distance <= tolerance);

            // We have range to attack, we stop as soon as we have a clear vision of the enemy.
            if ( !HasCondition( BCOND_TOO_FAR_TO_ATTACK ) && !completed ) {
                completed = HasCondition( BCOND_SEE_ENEMY ) && !HasCondition( BCOND_ENEMY_OCCLUDED );
            }

            if ( completed ) {
                TaskComplete();
                return;
            }

            if ( GetBot()->GetTacticalMode() == TACTICAL_MODE_DEFENSIVE && memory->GetInformer() ) {
                GetLocomotion()->DriveTo( "Hunt Threat - Informer", memory->GetInformer(), PRIORITY_HIGH, tolerance );
            }
            else {
                GetLocomotion()->DriveTo( "Hunt Threat", memory->GetEntity(), PRIORITY_HIGH, tolerance );
            }

            break;
        }

        case BTASK_AIM:
        {
            if ( !GetVision() ) {
                Fail( "Aim: Without Vision" );
                return;
            }

            if ( pTask->pszValue.Get() ) {
                GetVision()->LookAt( "Schedule Aim", pTask->pszValue.Get(), PRIORITY_CRITICAL, 1.0f );
            }
            else {
                if ( !pTask->vecValue.IsValid() ) {
                    Fail( "Aim: Invalid goal" );
                    return;
                }

                GetVision()->LookAt( "Schedule Aim", pTask->vecValue, PRIORITY_CRITICAL, 1.0f );
            }

            if ( GetVision()->IsAimReady() ) {
                if ( GetMemory() ) {
                    GetMemory()->UpdateDataMemory( "BlockLookAround", 1, 5.0f );
                }

                TaskComplete();
            }

            break;
        }

        case BTASK_RELOAD:
        case BTASK_RELOAD_SAFE:
        {
            if ( GetMemory() ) {
                GetMemory()->MaintainThreat();
            }

            CBaseWeapon *pWeapon = GetHost()->GetActiveBaseWeapon();

            if ( pWeapon == NULL ) {
                TaskComplete();
                return;
            }

#ifdef INSOURCE_DLL
            if ( !pWeapon->IsReloading() || (!HasCondition( BCOND_EMPTY_CLIP1_AMMO ) && !HasCondition( BCOND_LOW_CLIP1_AMMO )) ) {
#else
            if ( !pWeapon->m_bInReload || (!HasCondition( BCOND_EMPTY_CLIP1_AMMO ) && !HasCondition( BCOND_LOW_CLIP1_AMMO )) ) {
#endif
                TaskComplete();
                return;
            }

            break;
        }

        default:
        {
            Assert( !"TaskRun(): Task not handled!" );
            break;
        }
    }
}

//================================================================================
//================================================================================
void IBotSchedule::TaskComplete()
{
    if ( GetLocomotion()->HasDestination() ) {
        GetLocomotion()->StopDrive();
    }

    if ( m_Tasks.Count() == 0 ) {
        Assert( !"m_Tasks.Count() == 0" );
        return;
    }

    m_Tasks.Remove( 0 );

    if ( m_Tasks.Count() == 0 ) {
        m_bFinished = true;
    }
}