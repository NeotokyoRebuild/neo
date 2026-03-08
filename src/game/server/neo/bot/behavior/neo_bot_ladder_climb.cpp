#include "cbase.h"
#include "bot/behavior/neo_bot_ladder_climb.h"
#include "nav_ladder.h"
#include "NextBot/Path/NextBotPathFollow.h"

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

//---------------------------------------------------------------------------------------------
CNEOBotLadderClimb::CNEOBotLadderClimb( const CNavLadder *ladder, bool goingUp )
	: m_ladder( ladder ), m_bGoingUp( goingUp ), m_bHasBeenOnLadder( false ),
	  m_flLastZ( 0.0f ), m_flHighestZ( -FLT_MAX ), m_bHasLeftGround( false ),
	  m_bDismountPhase( false ), m_pExitArea( nullptr )
{
}

//---------------------------------------------------------------------------------------------
ActionResult<CNEOBot> CNEOBotLadderClimb::OnStart( CNEOBot *me, Action<CNEOBot> *priorAction )
{
	if ( !m_ladder )
	{
		return Done( "No ladder specified" );
	}

	// Ignore enemies while climbing
	me->StopLookingAroundForEnemies();

	// Timeout based on ladder length
	float estimatedClimbTime = m_ladder->m_length / MAX_CLIMB_SPEED + 2.0f;
	m_timeoutTimer.Start( estimatedClimbTime );

	m_bHasBeenOnLadder = false;

	ILocomotion *mover = me->GetLocomotionInterface();
	m_flLastZ = mover->GetFeet().z;
	m_flHighestZ = m_flLastZ;
	m_bHasLeftGround = !mover->GetGround();
	m_stuckTimer.Start( STUCK_CHECK_INTERVAL );

	if ( me->IsDebugging( NEXTBOT_PATH ) )
	{
		DevMsg( "%s: Starting ladder climb (%s), length %.1f\n",
			me->GetDebugIdentifier(),
			m_bGoingUp ? "up" : "down",
			m_ladder->m_length );
	}

	return Continue();
}

//---------------------------------------------------------------------------------------------
void CNEOBotLadderClimb::EnterDismountPhase( CNEOBot *me )
{
	m_bDismountPhase = true;
	m_dismountTimer.Start( DISMOUNT_TIMEOUT );

	// Try to resolve the exit area from the current path
	const PathFollower *path = me->GetCurrentPath();
	if ( path && path->IsValid() )
	{
		const Path::Segment *seg = path->GetCurrentGoal();
		// Walk forward past any ladder segments to find the ground exit
		while ( seg && seg->ladder )
		{
			seg = path->NextSegment( seg );
		}
		if ( seg && seg->area )
		{
			m_pExitArea = seg->area;
		}
	}

	if ( me->IsDebugging( NEXTBOT_PATH ) )
	{
		DevMsg( "%s: Entering dismount phase (exit area %s)\n",
			me->GetDebugIdentifier(),
			m_pExitArea ? "found" : "NOT found" );
	}
}

//---------------------------------------------------------------------------------------------
// Implementation based on ladder climbing logic in https://github.com/Dragoteryx/drgbase/
ActionResult<CNEOBot> CNEOBotLadderClimb::Update( CNEOBot *me, float interval )
{
	if ( m_timeoutTimer.IsElapsed() )
	{
		return Done( "Ladder climb timeout" );
	}

	ILocomotion *mover = me->GetLocomotionInterface();
	IBody *body = me->GetBodyInterface();

	// Check if we're on the ground
	bool onGround = ( mover->GetGround() != nullptr );

	//------------------------------------------------------------
	// Dismount phase: we've left the ladder, walk to exit NavArea
	//------------------------------------------------------------
	if ( m_bDismountPhase )
	{
		// Done: reached the target NavArea
		if ( m_pExitArea && me->GetLastKnownArea() == m_pExitArea )
		{
			return Done( "Reached next NavArea after dismount" );
		}

		// Safety timeout
		if ( m_dismountTimer.IsElapsed() )
		{
			return Done( "Dismount walk timed out" );
		}

		// Build look target toward exit area center with vertical bias preserved
		const Vector& myPos = mover->GetFeet();
		if ( m_pExitArea )
		{
			Vector exitCenter = m_pExitArea->GetCenter();

			// Flatten the direction to horizontal, then re-add vertical look bias
			Vector dir = exitCenter - myPos;
			dir.z = 0.0f;

			Vector lookTarget = myPos + dir;
			if ( m_bGoingUp )
			{
				lookTarget.z += 64.0f;	// Look slightly upward while dismounting up
			}
			else
			{
				lookTarget.z -= 64.0f;	// Look slightly downward while dismounting down
			}

			body->AimHeadTowards( lookTarget, IBody::MANDATORY, 0.1f, nullptr, "Walking to exit area" );
			mover->Approach( exitCenter, 9999999.9f );

			if ( me->IsDebugging( NEXTBOT_PATH ) )
			{
				NDebugOverlay::Line( myPos, exitCenter, 0, 255, 255, true, 0.1f );
			}
		}
		else
		{
			// No exit area resolved - just push forward along the ladder normal
			Vector pushDir = -m_ladder->GetNormal();
			Vector pushTarget = myPos + 100.0f * pushDir;
			body->AimHeadTowards( pushTarget, IBody::MANDATORY, 0.1f, nullptr, "Dismount push forward" );
			mover->Approach( pushTarget, 9999999.9f );
		}

		return Continue();
	}

	//------------------------------------------------------------
	// Normal ladder climbing phase
	//------------------------------------------------------------

	// Get current position and target (needed by both status checks and movement logic below)
	const Vector& myPos = mover->GetFeet();
	float currentZ = myPos.z;
	float targetZ = m_bGoingUp ? m_ladder->m_top.z : m_ladder->m_bottom.z;

	// Update onLadder status
	// Also treat being in the air (no ground entity) as "still on ladder" to prevent
	// premature exit when the bot briefly pops off at the top of a ladder.
	bool inAir = !onGround;
	if ( inAir )
	{
		m_bHasLeftGround = true;
	}

	bool onLadder = ( me->GetMoveType() == MOVETYPE_LADDER ) ||
	                mover->IsUsingLadder() ||
	                mover->IsAscendingOrDescendingLadder() ||
	                inAir;

	if ( onLadder )
	{
		m_bHasBeenOnLadder = true;
	}
	else if ( m_bHasBeenOnLadder )
	{
		// We were on the ladder and are now firmly on ground.
		// For downward descents, only trigger dismount once we're near the bottom;
		// the bot may still be standing on the upper staging platform when it first
		// touches the ladder, so guard against a premature transition.
		const float bottomZ = m_ladder->m_bottom.z;
		const float nearBottomThreshold = mover->GetStepHeight() * 4.0f;
		if ( m_bGoingUp || ( myPos.z <= bottomZ + nearBottomThreshold ) )
		{
			EnterDismountPhase( me );
			return Continue();
		}
	}

	// Transition to dismount if we touch ground after having left it.
	// Same downward guard: don't fire while still on the upper platform.
	if ( m_bHasBeenOnLadder && onGround && m_bHasLeftGround )
	{
		const float bottomZ = m_ladder->m_bottom.z;
		const float nearBottomThreshold = mover->GetStepHeight() * 4.0f;
		if ( m_bGoingUp || ( myPos.z <= bottomZ + nearBottomThreshold ) )
		{
			EnterDismountPhase( me );
			return Continue();
		}
	}

	// Track peak height for progress detection
	if ( currentZ > m_flHighestZ )
	{
		m_flHighestZ = currentZ;
	}

	// Stuck detection: if we haven't made vertical progress, bail out gracefully
	if ( m_stuckTimer.IsElapsed() )
	{
		float verticalDelta = fabsf( currentZ - m_flLastZ );
		if ( verticalDelta < STUCK_Z_TOLERANCE )
		{
			// No vertical progress - if we're close enough to the target, consider it done
			float distToTarget = fabsf( currentZ - targetZ );
			if ( distToTarget < mover->GetStepHeight() * 2.0f )
			{
				return Done( "Near target height with no vertical progress - considering climb complete" );
			}

			if ( me->IsDebugging( NEXTBOT_PATH ) )
			{
				DevMsg( "%s: Ladder climb stuck - no vertical progress (delta %.1f)\n",
					me->GetDebugIdentifier(), verticalDelta );
			}
		}
		m_flLastZ = currentZ;
		m_stuckTimer.Start( STUCK_CHECK_INTERVAL );
	}

	if ( m_bGoingUp )
	{
		body->SetDesiredPosture( IBody::STAND );

		// Check if we've reached the top - transition to dismount instead of exiting
		if ( currentZ >= targetZ - mover->GetStepHeight() && onGround )
		{
			EnterDismountPhase( me );
			return Continue();
		}

		// Near the top of the ladder: try to push toward the navmesh exit area
		// This handles cases where the exit is behind or offset from the ladder normal
		float distToTop = targetZ - currentZ;
		if ( distToTop <= DISMOUNT_PUSH_DISTANCE && m_bHasBeenOnLadder )
		{
			// Try to find the dismount area from the current path
			const PathFollower *path = me->GetCurrentPath();
			if ( path && path->IsValid() )
			{
				const Path::Segment *goal = path->GetCurrentGoal();
				if ( goal )
				{
					// Find the next non-ladder segment (the exit area)
					const Path::Segment *exitSeg = goal;
					while ( exitSeg && exitSeg->ladder )
					{
						exitSeg = path->NextSegment( exitSeg );
					}

					if ( exitSeg && exitSeg->area )
					{
						// Push toward the exit area center instead of just the ladder normal
						Vector exitPos = exitSeg->area->GetCenter();
						exitPos.z = Max( exitPos.z, currentZ );
						body->AimHeadTowards( exitPos, IBody::MANDATORY, 0.1f, nullptr, "Dismounting ladder toward exit" );
						mover->Approach( exitPos, 9999999.9f );

						if ( me->IsDebugging( NEXTBOT_PATH ) )
						{
							NDebugOverlay::Line( myPos, exitPos, 0, 255, 0, true, 0.1f );
						}

						return Continue();
					}
				}
			}
		}

		// Climb up: look up and push into ladder
		Vector goal = myPos + 100.0f * ( -m_ladder->GetNormal() + Vector( 0, 0, 2 ) );
		body->AimHeadTowards( goal, IBody::MANDATORY, 0.1f, nullptr, "Climbing ladder" );
		mover->Approach( goal, 9999999.9f );
	}
	else
	{
		// Check if we've reached the bottom - transition to dismount instead of exiting
		if ( currentZ <= targetZ + mover->GetStepHeight() && onGround )
		{
			EnterDismountPhase( me );
			return Continue();
		}

		// Climb down: Stare at bottom of ladder while moving forward to it
		Vector goal = m_ladder->m_bottom;
		body->AimHeadTowards( goal, IBody::MANDATORY, 0.1f, nullptr, "Descending ladder fast" );
		mover->Approach( goal, 9999999.9f );
	}

	return Continue();
}

//---------------------------------------------------------------------------------------------
void CNEOBotLadderClimb::OnEnd( CNEOBot *me, Action<CNEOBot> *nextAction )
{
	me->StartLookingAroundForEnemies();

	if ( me->IsDebugging( NEXTBOT_PATH ) )
	{
		DevMsg( "%s: Finished ladder climb\n", me->GetDebugIdentifier() );
	}
}

//---------------------------------------------------------------------------------------------
ActionResult<CNEOBot> CNEOBotLadderClimb::OnSuspend( CNEOBot *me, Action<CNEOBot> *interruptingAction )
{
	return Done( "OnSuspend: Cancel out of ladder climb, situation will likely become stale." );
}

//---------------------------------------------------------------------------------------------
ActionResult<CNEOBot> CNEOBotLadderClimb::OnResume( CNEOBot *me, Action<CNEOBot> *interruptingAction )
{
	return Done( "OnResume: Cancel out of ladder climb, situation is likely stale." );
}
