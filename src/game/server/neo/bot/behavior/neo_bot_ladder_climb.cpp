#include "cbase.h"
#include "bot/behavior/neo_bot_ladder_climb.h"
#include "nav_ladder.h"
#include "NextBot/Path/NextBotPathFollow.h"

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

//---------------------------------------------------------------------------------------------
CNEOBotLadderClimb::CNEOBotLadderClimb( const CNavLadder *ladder, bool goingUp )
	: m_ladder( ladder ), m_bGoingUp( goingUp ), m_bHasBeenOnLadder( false ),
	  m_flLastZ( 0.0f ), m_flHighestZ( -FLT_MAX ), m_bHasLeftGround( false )
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
		// We were on the ladder and are now firmly on ground - climb is complete
		return Done( "Dismounted ladder - on solid ground" );
	}

	// Exit if we touch ground after having actually left it (e.g. at the bottom or top exit)
	// This helps prevent "ladder tunnel vision" where the bot stays attached to the 
	// ladder state while standing on a ledge.
	if ( m_bHasBeenOnLadder && onGround && m_bHasLeftGround )
	{
		return Done( "Touched ground after progress - ladder climb finished" );
	}

	// Get current position and target
	const Vector& myPos = mover->GetFeet();
	float currentZ = myPos.z;
	float targetZ = m_bGoingUp ? m_ladder->m_top.z : m_ladder->m_bottom.z;

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

		// Check if we've reached the top
		if ( currentZ >= targetZ - mover->GetStepHeight() && onGround )
		{
			return Done( "Reached top of ladder" );
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
		if ( currentZ <= targetZ + mover->GetStepHeight() && onGround )
		{
			return Done( "Reached bottom of ladder" );
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
	me->StartLookingAroundForEnemies();
	return Continue();
}

//---------------------------------------------------------------------------------------------
ActionResult<CNEOBot> CNEOBotLadderClimb::OnResume( CNEOBot *me, Action<CNEOBot> *interruptingAction )
{
	me->StopLookingAroundForEnemies();
	return Continue();
}
