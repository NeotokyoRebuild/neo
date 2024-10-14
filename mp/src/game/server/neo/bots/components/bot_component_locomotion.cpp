//========= Copyright © 1996-2005, Valve Corporation, All rights reserved. ============//
// Authors: 
// Michael S. Booth (linkedin.com/in/michaelbooth), 2003
// Iván Bravo Bravo (linkedin.com/in/ivanbravobravo), 2017

#include "cbase.h"
#include "bots\bot.h"

#ifdef INSOURCE_DLL
#include "in_utils.h"
#else
#include "bots\in_utils.h"
#endif

#include "nav.h"
#include "nav_mesh.h"
#include "nav_area.h"

#include "in_buttons.h"
#include "basetypes.h"

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

//================================================================================
// Commands
//================================================================================

DECLARE_REPLICATED_COMMAND( bot_locomotion_allow_teleport, "1", "Indica si los Bots pueden teletransportarse al quedar atascados." )
DECLARE_REPLICATED_COMMAND( bot_locomotion_hiddden_teleport, "1", "Indica si los Bots pueden teletransportarse solo si ningun Jugador los esta mirando" )
DECLARE_REPLICATED_COMMAND( bot_locomotion_tolerance, "60", "" )
DECLARE_REPLICATED_COMMAND( bot_locomotion_allow_wiggle, "1", "" )

extern ConVar bot_debug;
extern ConVar bot_debug_locomotion;

//================================================================================
//================================================================================
void CBotLocomotion::Reset()
{
    BaseClass::Reset();

    m_bCrouching = false;
    m_bJumping = false;
    m_bSneaking = false;
    m_bRunning = false;
    m_bUsingLadder = false;
}

//================================================================================
//================================================================================
void CBotLocomotion::Update()
{
    VPROF_BUDGET( "CBotLocomotion::Update", VPROF_BUDGETGROUP_BOTS );

    // We were jumping but we have already touched the ground.
    if ( IsJumping() && IsOnGround() ) {
        m_bJumping = false;
    }

    UpdateCommands();

    // We have nowhere to go
    if ( !HasDestination() )
        return;

    // Our decisions tell us that we should not move
    if ( !GetDecision()->ShouldUpdateNavigation() )
        return;

    // We check if we have a valid path to reach our destination
    CheckPath();

    if ( !HasValidPath() )
        return;

    if ( GetBot()->ShouldShowDebug() ) {
        GetPathFollower()->Debug( bot_debug_locomotion.GetBool() );
    }
    else {
        GetPathFollower()->Debug( false );
    }

    if ( !IsUnreachable() ) {
        GetPathFollower()->Update( m_flTickInterval );

        // We got stuck, we started to move randomly
        if ( GetDecision()->ShouldWiggle() ) {
            Wiggle();
        }
    }
}

void CBotLocomotion::UpdateCommands()
{
    if ( GetDecision()->ShouldSneak() ) {
        InjectButton( IN_WALK );
    }
    else if ( GetDecision()->ShouldRun() ) {
        InjectButton( IN_SPEED );
    }
    if ( GetDecision()->ShouldCrouch() ) {
        InjectButton( IN_DUCK );
    }

    if ( GetDecision()->ShouldJump() ) {
        Jump();
    }
}

bool CBotLocomotion::DriveTo( const char * pDesc, const Vector & vecGoal, int priority, float tolerance )
{
    if ( IsDisabled() )
        return false;

    if ( !vecGoal.IsValid() )
        return false;

    if ( HasDestination() ) {
        if ( GetPriority() > priority )
            return false;

        if ( GetDestination().DistTo( vecGoal ) <= 30.0f )
            return false;
    }

    float flDistance = GetAbsOrigin().DistTo( vecGoal );
    float flTolerance =  (tolerance > 0.0f) ? tolerance : GetTolerance();

    if ( flDistance <= flTolerance )
        return false;

    m_vecDestination = vecGoal;
    m_vecNextSpot.Invalidate();
    m_pDescription = pDesc;

    SetPriority( priority );
    SetTolerance( tolerance );
    CheckPath();

    return true;
}

bool CBotLocomotion::DriveTo( const char * pDesc, CBaseEntity * pTarget, int priority, float tolerance )
{
    if ( !pTarget )
        return false;

    Vector vecGoal( pTarget->GetAbsOrigin() );

    if ( GetMemory() ) {
        CEntityMemory *memory = GetMemory()->GetEntityMemory( pTarget );

        // We have information about this entity in our memory!
        if ( memory ) {
            vecGoal = memory->GetLastKnownPosition();
        }
    }

    return DriveTo( pDesc, vecGoal, priority, tolerance );
}

bool CBotLocomotion::DriveTo( const char * pDesc, CNavArea * pTargetArea, int priority, float tolerance )
{
    if ( !pTargetArea )
        return false;

    // TODO: Something better?
    return DriveTo( pDesc, pTargetArea->GetRandomPoint(), priority, tolerance );
}

bool CBotLocomotion::Approach( const Vector & vecGoal, float tolerance, int priority )
{
    return DriveTo( "Approach", vecGoal, priority, tolerance );
}

bool CBotLocomotion::Approach( CBaseEntity * pTarget, float tolerance, int priority )
{
    return DriveTo( "Approach", pTarget, priority, tolerance );
}

void CBotLocomotion::StopDrive()
{
    // TODO: Maybe here we can do more.
    Reset();
    //GetBot()->DebugAddMessage( "Navigation Stopped" );
}

void CBotLocomotion::Wiggle()
{
    if ( !m_WiggleTimer.HasStarted() || m_WiggleTimer.IsElapsed() ) {
        m_WiggleDirection = (NavRelativeDirType)RandomInt( 0, 3 );
        m_WiggleTimer.Start( RandomFloat( 0.5f, 2.0f ) );
    }

    Vector vecForward, vecRight;
    GetHost()->EyeVectors( &vecForward, &vecRight );

    const float lookRange = 15.0f;

    Vector vecPos;
    float flGround;

    switch ( m_WiggleDirection ) {
        case LEFT:
            vecPos = GetAbsOrigin() - (lookRange * vecRight);
            break;

        case RIGHT:
            vecPos = GetAbsOrigin() + (lookRange * vecRight);
            break;

        case FORWARD:
        default:
            vecPos = GetAbsOrigin() + (lookRange * vecForward);
            break;

        case BACKWARD:
            vecPos = GetAbsOrigin() - (lookRange * vecForward);
            break;
    }

    if ( GetSimpleGroundHeightWithFloor( vecPos, &flGround ) ) {
        GetBot()->InjectMovement( m_WiggleDirection );
    }
    
    if ( GetStuckDuration() >= 2.5f ) {
        GetBot()->InjectButton( IN_DUCK );

        // Sometimes we jump
        if ( RandomInt( 0, 100 ) > 80 ) {
            Jump();
        }
    }
}

bool CBotLocomotion::ShouldComputePath()
{
    if ( !HasValidPath() )
        return true;

    // Building a path is very expensive for the engine, we limit this to once every 3s.
    if ( GetPath()->GetElapsedTimeSinceBuild() < 3.0f )
        return false;

    if ( IsStuck() && GetStuckDuration() >= 4.0f )
        return true;

    const Vector vecGoal = GetDestination();
    const float range = 100.0f;

    // Our destination has changed enough so that we 
    // must recompute the route we must take.
    if ( GetPath()->GetEndpoint().DistTo( vecGoal ) > range ) {
        return true;
    }

    // TODO: Take into account other things: Obstacles, elevators, etc.
    return false;
}

void CBotLocomotion::CheckPath()
{
    if ( !HasDestination() )
        return;

    // We override the current route to recompute.
    if ( ShouldComputePath() ) {
        ComputePath();
    }
}

void CBotLocomotion::ComputePath()
{
    VPROF_BUDGET( "CBotLocomotion::ComputePath", VPROF_BUDGETGROUP_BOTS );

    Assert( HasDestination() );

    Vector from = GetAbsOrigin();
    Vector to = GetDestination();

    CSimpleBotPathCost cost( GetBot() );

    GetPathFollower()->Reset();
    GetPath()->Compute( from, to, cost );
}

bool CBotLocomotion::IsUnreachable() const
{
    return GetPath()->IsUnreachable();
}

bool CBotLocomotion::IsStuck() const
{
    return GetPathFollower()->IsStuck();
}

float CBotLocomotion::GetStuckDuration() const
{
    return GetPathFollower()->GetStuckDuration();
}

void CBotLocomotion::ResetStuck()
{
    GetPathFollower()->ResetStuck();
}

bool CBotLocomotion::IsOnGround() const
{
#ifdef INSOURCE_DLL
    return GetHost()->IsOnGround();
#else
    return (GetHost()->GetFlags() & FL_ONGROUND) ? true : false;
#endif
}

CBaseEntity * CBotLocomotion::GetGround() const
{
    if ( !IsOnGround() ) {
        return NULL;
    }

    Vector vecFloor( GetFeet() );
    vecFloor.z -= 100.0f;

    trace_t tr;
    UTIL_TraceLine( GetFeet(), vecFloor, MASK_SOLID, GetHost(), COLLISION_GROUP_NONE, &tr );

    // TODO: Something better?
    return tr.m_pEnt;
}

Vector & CBotLocomotion::GetGroundNormal() const
{
    if ( !IsOnGround() ) {
        Vector vecNormal( vec3_invalid );
        return vecNormal;
    }

    Vector vecFloor( GetFeet() );
    vecFloor.z -= 100.0f;

    trace_t tr;
    UTIL_TraceLine( GetFeet(), vecFloor, MASK_SOLID, GetHost(), COLLISION_GROUP_NONE, &tr );

    // TODO: Something better?
    return tr.plane.normal;
}

float CBotLocomotion::GetTolerance() const
{
    if ( m_flTolerance > 0.0f )
        return m_flTolerance;

    float flTolerance = bot_locomotion_tolerance.GetFloat();
    return flTolerance;
}

Vector & CBotLocomotion::GetVelocity() const
{
    Vector vecVelocity( GetHost()->GetAbsVelocity() );
    return vecVelocity;
}

float CBotLocomotion::GetSpeed() const
{
    return GetVelocity().Length2D();
}

float CBotLocomotion::GetStepHeight() const
{
    return StepHeight;
}

float CBotLocomotion::GetMaxJumpHeight() const
{
    return JumpCrouchHeight;
}

float CBotLocomotion::GetDeathDropHeight() const
{
    return DeathDrop;
}

float CBotLocomotion::GetRunSpeed() const
{
#ifdef INSOURCE_DLL
    ConVarRef sv_player_sprint_speed( "sv_player_sprint_speed" );
    return sv_player_sprint_speed.GetFloat();
#else
    // This goes according to your mod
    return 200.0f;
#endif
}

float CBotLocomotion::GetWalkSpeed() const
{
#ifdef INSOURCE_DLL
    ConVarRef sv_player_walk_speed( "sv_player_walk_speed" );
    return sv_player_walk_speed.GetFloat();
#else
    // This goes according to your mod
    return 80.0f;
#endif
}

bool CBotLocomotion::IsOnTolerance() const
{
    if ( !HasDestination() )
        return false;

    float flDistance = GetDistanceToDestination();
    float flTolerance = GetTolerance();

    return (flDistance <= flTolerance);
}

bool CBotLocomotion::IsAreaTraversable( const CNavArea * area ) const
{
    if ( area == NULL )
        return false;

    if ( area->IsBlocked( TEAM_ANY ) || area->IsBlocked( GetHost()->GetTeamNumber() ) )
        return false;

    // TODO: More checks!
    return true;
}

bool CBotLocomotion::IsAreaTraversable( const CNavArea * from, const CNavArea * to ) const
{
    if ( from == NULL || to == NULL )
        return false;

    if ( !IsAreaTraversable( from ) || !IsAreaTraversable( to ) )
        return false;

    if ( !from->IsConnected( to, NUM_DIRECTIONS ) )
        return false;

    // Do not go from one jump area to another
    if ( (from->GetAttributes() & NAV_MESH_JUMP) && (to->GetAttributes() & NAV_MESH_JUMP) )
        return false;

    // TODO: More checks!
    return true;
}

//================================================================================
// Returns if we have a valid destination path
// NOTE: Very heavy duty for the engine, use with care.
//================================================================================
bool CBotLocomotion::IsTraversable( const Vector & from, const Vector & to ) const
{
    CSimpleBotPathCost pathCost( GetBot() );

    CNavPath testPath;
    return testPath.Compute( from, to, pathCost );
}

//================================================================================
//================================================================================
bool CBotLocomotion::IsEntityTraversable( CBaseEntity * ent ) const
{
    // TODO
    return false;
}


//================================================================================
//================================================================================
const Vector &CBotLocomotion::GetCentroid() const
{
    static Vector centroid;

    const Vector &mins = GetHost()->WorldAlignMins();
    const Vector &maxs = GetHost()->WorldAlignMaxs();

    centroid = GetFeet();
    centroid.z += (maxs.z - mins.z) / 2.0f;

    return centroid;
}

//================================================================================
//================================================================================
const Vector &CBotLocomotion::GetFeet() const
{
    static Vector feet;
    feet = GetHost()->GetAbsOrigin();

    return feet;
}

//================================================================================
//================================================================================
const Vector &CBotLocomotion::GetEyes() const
{
    static Vector eyes;
    eyes = GetHost()->EyePosition();

    return eyes;
}

//================================================================================
//================================================================================
float CBotLocomotion::GetMoveAngle() const
{
    return GetHost()->GetAbsAngles().y;
}

//================================================================================
//================================================================================
CNavArea *CBotLocomotion::GetLastKnownArea() const
{
    return GetHost()->GetLastKnownArea();
}

//================================================================================
//  Find "simple" ground height, treating current nav area as part of the floo
//================================================================================
bool CBotLocomotion::GetSimpleGroundHeightWithFloor( const Vector &pos, float *height, Vector *normal )
{
    if ( TheNavMesh->GetSimpleGroundHeight( pos, height, normal ) ) {
        // our current nav area also serves as a ground polygon
        if ( GetLastKnownArea() && GetLastKnownArea()->IsOverlapping( pos ) ) {
            *height = MAX( (*height), GetLastKnownArea()->GetZ( pos ) );
        }

        return true;
    }

    return false;
}

//================================================================================
//================================================================================
void CBotLocomotion::Crouch()
{
    m_bCrouching = true;
    //GetBot()->DebugAddMessage( "Crouch" );
}

//================================================================================
//================================================================================
void CBotLocomotion::StandUp()
{
    m_bCrouching = false;
    //GetBot()->DebugAddMessage( "StandUp" );
}

//================================================================================
//================================================================================
bool CBotLocomotion::IsCrouching() const
{
    return m_bCrouching;
}

//================================================================================
//================================================================================
void CBotLocomotion::Jump()
{
    InjectButton( IN_JUMP );
    m_bJumping = true;
    //GetBot()->DebugAddMessage( "Jump" );
}

//================================================================================
//================================================================================
bool CBotLocomotion::IsJumping() const
{
    return m_bJumping;
}

//================================================================================
//================================================================================
void CBotLocomotion::Run()
{
    m_bRunning = true;
    m_bSneaking = false;
    //GetBot()->DebugAddMessage( "Run" );
}

//================================================================================
//================================================================================
void CBotLocomotion::Walk()
{
    m_bRunning = false;
    m_bSneaking = false;
    //GetBot()->DebugAddMessage( "Walk" );
}

//================================================================================
//================================================================================
void CBotLocomotion::Sneak()
{
    m_bRunning = false;
    m_bSneaking = true;
    //GetBot()->DebugAddMessage( "Sneak" );
}

//================================================================================
//================================================================================
bool CBotLocomotion::IsRunning() const
{
    return m_bRunning;
}

//================================================================================
//================================================================================
bool CBotLocomotion::IsWalking() const
{
    return (!m_bRunning && !m_bSneaking);
}

//================================================================================
//================================================================================
bool CBotLocomotion::IsSneaking() const
{
    return m_bSneaking;
}

//================================================================================
//================================================================================
void CBotLocomotion::StartLadder( const CNavLadder *ladder, NavTraverseType how, const Vector &approachPos, const Vector &departPos )
{
    // TODO: This feels like when you build a piece of furniture and you do not use all the parts/screws...
}

//================================================================================
// This will be called in each frame while we are moving on a ladder.
// TODO: Everything about ladder's still has a lot of work to do!
//================================================================================
bool CBotLocomotion::TraverseLadder( const CNavLadder *ladder, NavTraverseType how, const Vector &approachPos, const Vector &departPos, float deltaT )
{
    Vector vecStart;
    Vector vecEnd;
    Vector vecLookAt;

    //
    GetPathFollower()->m_stuckMonitor.Update( this );

    // Debug
    if ( GetBot()->ShouldShowDebug() ) {
        ladder->DrawLadder();
    }

    if ( how == GO_LADDER_UP ) {
        vecStart = ladder->m_bottom;
        vecEnd = ladder->m_top;

        if ( ladder->m_topForwardArea ) {
            vecLookAt = ladder->m_topForwardArea->GetCenter();
        }
        else if ( ladder->m_topLeftArea ) {
            vecLookAt = ladder->m_topLeftArea->GetCenter();
        }
        else if ( ladder->m_topRightArea ) {
            vecLookAt = ladder->m_topRightArea->GetCenter();
        }
        else {
            Assert( 0 );
        }

        vecEnd.z += JumpHeight;
    }
    else if ( how == GO_LADDER_DOWN ) {
        Assert( ladder->m_bottomArea );

        vecStart = ladder->m_top;
        vecEnd = ladder->m_bottom;
        vecLookAt = ladder->m_bottomArea->GetCenter();
    }
    else {
        Assert( !"How the hell did I use this ladder??" );
        return true;
    }

    // We got stuck on a ladder for more than 5 seconds... 
    // We're not ashamed.
    if ( IsStuck() && GetStuckDuration() > 5.0f ) {
        OnMoveToFailure( vecEnd, FAIL_STUCK );
        return true;
    }

    vecLookAt.x = vecEnd.x;
    vecLookAt.z += JumpCrouchHeight;

    if ( !IsUsingLadder() ) {
        if ( departPos.IsValid() ) {
            if ( how == GO_LADDER_UP ) {
                if ( GetAbsOrigin().z >= departPos.z ) {
                    return true;
                }
            }
            else {
                if ( GetAbsOrigin().z <= departPos.z ) {
                    return true;
                }
            }
        }

        // We move to the place where the ladder begins
        TrackPath( vecStart, deltaT );
        GetBot()->DebugAddMessage( "Going to ladder... %.2f", GetAbsOrigin().DistTo( vecStart ) );
    }
    else {
        // We looked to the end of the stairs. (Up or down)
        if ( GetVision() ) {
            GetVision()->LookAt( "Look Ladder End", vecLookAt, PRIORITY_CRITICAL );
        }

        float flDistance = GetAbsOrigin().DistTo( vecEnd );

        // We move forward to cross the ladder
        GetBot()->InjectMovement( FORWARD );
        //GetBot()->DebugAddMessage( UTIL_VarArgs( "Moving through the ladder... %.2f \n", flDistance ) );

        // We are coming down and we almost reach the end of the ladder, 
        // we jump to release!
        if ( flDistance <= 80.0f && how == GO_LADDER_DOWN ) {
            Jump();
        }
    }

    if ( GetBot()->ShouldShowDebug() ) {
        NDebugOverlay::Text( approachPos, "ApproachPos", false, 0.1f );
        NDebugOverlay::Text( departPos, "DepartPos", false, 0.1f );

        NDebugOverlay::Text( ladder->m_top, "Top!", false, 0.1f );
        NDebugOverlay::Text( ladder->m_bottom, "Bottom!", false, 0.1f );
    }

    return false;
}

//================================================================================
//================================================================================
bool CBotLocomotion::IsUsingLadder() const
{
    return (GetHost()->GetMoveType() == MOVETYPE_LADDER);
}

//================================================================================
// This is called every frame while we are moving to a destination.
// Michael S. Booth (linkedin.com/in/michaelbooth), 2003
//================================================================================
void CBotLocomotion::TrackPath( const Vector &pathGoal, float deltaT )
{
    Vector myOrigin = GetCentroid();
    m_vecNextSpot = pathGoal;

    // compute our current forward and lateral vectors
    float flAngle = GetHost()->EyeAngles().y;

    Vector2D dir( Utils::BotCOS( flAngle ), Utils::BotSIN( flAngle ) );
    Vector2D lat( -dir.y, dir.x );

    // compute unit vector to goal position
    Vector2D to( pathGoal.x - myOrigin.x, pathGoal.y - myOrigin.y );
    to.NormalizeInPlace();

    // move towards the position independant of our view direction
    float toProj = to.x * dir.x + to.y * dir.y;
    float latProj = to.x * lat.x + to.y * lat.y;

    const float c = 0.25f;    // 0.5

    if ( toProj > c ) {
        GetBot()->InjectMovement( FORWARD );
    }
    else if ( toProj < -c ) {
        GetBot()->InjectMovement( BACKWARD );
    }

    if ( latProj >= c ) {
        GetBot()->InjectMovement( LEFT );
    }
    else if ( latProj <= -c ) {
        GetBot()->InjectMovement( RIGHT );
    }
}

//================================================================================
// We have arrived at our destination
//================================================================================
void CBotLocomotion::OnMoveToSuccess( const Vector &goal )
{
    StopDrive();
    //GetBot()->DebugAddMessage( "Move Success" );
}

//================================================================================
// Something has prevented us from reaching our destination
//================================================================================
void CBotLocomotion::OnMoveToFailure( const Vector &goal, MoveToFailureType reason )
{
    // We can teleport
    if ( GetDecision()->ShouldTeleport( goal ) ) {
        Vector theGoal = goal;
        theGoal.z += 20.0f;
        
        GetHost()->Teleport( &theGoal, NULL, NULL );
        StopDrive();

        // Debug
        if ( reason == FAIL_STUCK )
            GetBot()->DebugAddMessage( "FAIL_STUCK! Teleporting..." );
        if ( reason == FAIL_INVALID_PATH )
            GetBot()->DebugAddMessage( "FAIL_INVALID_PATH! Teleporting..." );
        if ( reason == FAIL_FELL_OFF )
            GetBot()->DebugAddMessage( "FAIL_FELL_OFF! Teleporting..." );
    }
    else {
        // We invalidate the current route, 
        // maybe we can calculate a valid new route.
        // TODO: Detect if we have stayed in a loop or something...
        GetPath()->Invalidate();

        // Debug
        if ( reason == FAIL_STUCK )
            GetBot()->DebugAddMessage( "FAIL_STUCK! I cant teleport!" );
        if ( reason == FAIL_INVALID_PATH )
            GetBot()->DebugAddMessage( "FAIL_INVALID_PATH! I cant teleport!" );
        if ( reason == FAIL_FELL_OFF )
            GetBot()->DebugAddMessage( "FAIL_FELL_OFF! I cant teleport!" );
    }
}