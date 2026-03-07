#include "cbase.h"
#include "bot/behavior/neo_bot_attack.h"
#include "bot/behavior/neo_bot_ladder_climb.h"
#include "nav_ladder.h"
#include "NextBot/Path/NextBotPathFollow.h"

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

//---------------------------------------------------------------------------------------------
CNEOBotLadderClimb::CNEOBotLadderClimb( const CNavLadder *ladder, bool goingUp )
	: m_ladder( ladder ), m_bGoingUp( goingUp ), m_bHasBeenOnLadder( false ),
	  m_flLastZ( 0.0f ), m_bDismountPhase( false ), m_pExitArea( nullptr )
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
	me->SetAttribute( CNEOBot::IGNORE_ENEMIES );

	// Timeout based on ladder length
	float estimatedClimbTime = m_ladder->m_length / MAX_CLIMB_SPEED + 2.0f;
	m_timeoutTimer.Start( estimatedClimbTime );

	m_bHasBeenOnLadder = false;

	ILocomotion *mover = me->GetLocomotionInterface();
	m_flLastZ = mover->GetFeet().z;
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
	me->ReleaseForwardButton(); // pause to reorient
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

	const CKnownEntity *threat = me->GetVisionInterface()->GetPrimaryKnownThreat(true);
	if ( threat && threat->IsVisibleRecently() )
	{
		if ( me->IsDebugging( NEXTBOT_PATH ) )
		{
			DevMsg( "%s: Threat detected during ladder approach - engaging\n", me->GetDebugIdentifier() );
		}
		// ChangeTo: We may move away from ladder when fighting, so don't want to get stuck in ladder climb behavior
		return ChangeTo( new CNEOBotAttack, "Interrupting climb to engage enemy" );
	}

	ILocomotion *mover = me->GetLocomotionInterface();
	IBody *body = me->GetBodyInterface();


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
			Vector dir = exitCenter - myPos;
			Vector lookTarget = myPos + dir;

			body->AimHeadTowards( lookTarget, IBody::MANDATORY, 0.1f, nullptr, "Walking to exit area" );
			me->PressForwardButton();

			// Strafe or jump to detach from ladder if exit is not straight ahead
			Vector toExit = exitCenter - myPos;
			toExit.z = 0.0f;
			VectorNormalize( toExit );

			Vector ladderForward = -m_ladder->GetNormal();
			ladderForward.z = 0.0f;
			VectorNormalize( ladderForward );

			float dot = DotProduct( toExit, ladderForward );
			// 2D cross product: positive = exit is to the right, negative = to the left
			float cross = ladderForward.x * toExit.y - ladderForward.y * toExit.x;

			if ( dot < 0.5f )
			{
				if ( fabsf( cross ) > 0.3f )
				{
					// Exit is to the side — strafe toward it
					if ( cross > 0.0f )
					{
						me->PressRightButton();
					}
					else
					{
						me->PressLeftButton();
					}
				}
				else
				{
					// Exit is nearly directly behind — jump to detach
					me->PressJumpButton();
				}
			}

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
			me->PressForwardButton();
		}

		return Continue();
	}

	//------------------------------------------------------------
	// Normal ladder climbing phase
	//------------------------------------------------------------
	const Vector& myPos = mover->GetFeet();
	float currentZ = myPos.z;
	float targetZ = m_bGoingUp ? m_ladder->m_top.z : m_ladder->m_bottom.z;

	bool onLadder = ( me->GetMoveType() == MOVETYPE_LADDER ) ||
	                mover->IsUsingLadder() ||
	                mover->IsAscendingOrDescendingLadder();

	if ( onLadder )
	{
		m_bHasBeenOnLadder = true;
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
				EnterDismountPhase( me );
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
		if ( currentZ >= targetZ )
		{
			EnterDismountPhase( me );
			return Continue();
		}

		// Climb up: look up and push into ladder
		Vector goal = myPos + 100.0f * ( -m_ladder->GetNormal() + Vector( 0, 0, 2 ) );
		body->AimHeadTowards( goal, IBody::MANDATORY, 0.1f, nullptr, "Climbing ladder" );
		mover->Approach( goal, 9999999.9f );
	}
	else
	{
		// Check if we've reached the bottom - transition to dismount instead of exiting
		if ( currentZ <= targetZ + mover->GetStepHeight() )
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
	me->ClearAttribute( CNEOBot::IGNORE_ENEMIES );

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
