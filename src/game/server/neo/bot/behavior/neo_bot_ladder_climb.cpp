#include "cbase.h"
#include "bot/behavior/neo_bot_attack.h"
#include "bot/behavior/neo_bot_ladder_climb.h"
#include "nav_ladder.h"
#include "NextBot/Path/NextBotPathFollow.h"

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

//---------------------------------------------------------------------------------------------
CNEOBotLadderClimb::CNEOBotLadderClimb( const CNavLadder *ladder, bool goingUp )
	: m_ladder( ladder ), m_bGoingUp( goingUp ), m_flLastZ( 0.0f ),
	m_bDismountPhase( false ), m_bJumpedOffLadder( false ), m_pExitArea( nullptr )
{
	m_exitAreaCenter = vec3_origin;
	m_ladderForward = ladder ? -ladder->GetNormal() : vec3_origin;
}

//---------------------------------------------------------------------------------------------
ActionResult<CNEOBot> CNEOBotLadderClimb::OnStart( CNEOBot *me, Action<CNEOBot> *priorAction )
{
	if ( !m_ladder )
	{
		return Done( "No ladder specified" );
	}

	// Don't interfere with look direction
	// Can exit out of behavior if threat is present
	me->StopLookingAroundForEnemies();
	me->SetAttribute( CNEOBot::IGNORE_ENEMIES );

	// Timeout based on ladder length
	float estimatedClimbTime = m_ladder->m_length / MAX_CLIMB_SPEED + 3.0f;
	m_timeoutTimer.Start( estimatedClimbTime );

	ILocomotion *mover = me->GetLocomotionInterface();
	m_flLastZ = mover->GetFeet().z;
	m_stuckTimer.Start( STUCK_CHECK_INTERVAL );

	if ( m_bGoingUp )
	{
		// Hull trace check: Ensure clear path to climb in the intended direction.
		Vector traceStart = mover->GetFeet();

		// Trace upwards from feet to check the space the bot would occupy.
		// e.g. ladder going through opening in ceiling
		float targetZ = m_ladder->m_top.z;
		Vector traceEnd = traceStart;
		traceEnd.z = MIN( targetZ, traceStart.z + 100.0f );

		trace_t tr;
		UTIL_TraceHull( traceStart, traceEnd, me->WorldAlignMins(), me->WorldAlignMaxs(), MASK_NPCSOLID, me, COLLISION_GROUP_NONE, &tr );

		// If vertical path blocked by world geometry due to failed approach alignment,
		// teleport bot to ideal position, assuming approach placed bot close enough
		// Ideally we only transition into this behavior when bot has attached to ladder
		if ( tr.DidHit() && tr.fraction < 1.0f )
		{
			if ( me->IsDebugging( NEXTBOT_PATH ) )
			{
				DevMsg( "%s: Ladder climb path obstructed (fraction %.2f), teleporting to ideal position\n",
					me->GetDebugIdentifier(), tr.fraction );
				NDebugOverlay::Box( traceStart, me->WorldAlignMins(), me->WorldAlignMaxs(), 255, 0, 0, 0, 2.0f );
				NDebugOverlay::Box( traceEnd, me->WorldAlignMins(), me->WorldAlignMaxs(), 255, 0, 0, 0, 2.0f );
			}

			// Fallback: Teleport to a center position on the ladder.
			mover->Reset(); // clear velocity cache in locomotion interface
			me->SetAbsVelocity( vec3_origin );

			Vector idealPos = m_ladder->GetPosAtHeight( m_flLastZ );
			// Offset slightly from the ladder surface based on the bot's collision box
			float offsetDist = me->CollisionProp()->OBBSize().x / 2.0f + 2.0f;
			idealPos += m_ladder->GetNormal() * offsetDist;
			idealPos.z = m_flLastZ;

			// Face perpendicularly straight on to the ladder (-normal)
			Vector idealLookDir = -m_ladder->GetNormal();
			QAngle idealAngles;
			VectorAngles( idealLookDir, idealAngles );

			// Teleport the bot
			me->SetAbsOrigin( idealPos );
			me->SetAbsAngles( idealAngles );

			// Update mover feet to new teleported position for stuck checking
			m_flLastZ = idealPos.z;
		}
		else if ( me->IsDebugging( NEXTBOT_PATH ) )
		{
			NDebugOverlay::Line( traceStart, traceEnd, 0, 255, 0, true, 2.0f );
		}
	}

	if ( me->IsDebugging( NEXTBOT_PATH ) )
	{
		DevMsg( "%s: Starting ladder climb (%s), length %.1f\n",
			me->GetDebugIdentifier(),
			m_bGoingUp ? "up" : "down",
			m_ladder->m_length );
	}

	// Try to resolve the exit area from the current path early
	ResolveExitArea( me );

	return Continue();
}

//---------------------------------------------------------------------------------------------
void CNEOBotLadderClimb::ResolveExitArea( CNEOBot *me )
{
	// Clear potentially stale exit area
	m_pExitArea = nullptr;

	const PathFollower *path = me->GetCurrentPath();
	if ( path && path->IsValid() )
	{
		const Path::Segment *seg = path->GetCurrentGoal();
		if ( !seg )
		{
			return;
		}

		constexpr int MAX_PATH_SEARCH_STEPS = 100;
		int safetyCounter = 0;
		while ( seg && safetyCounter < MAX_PATH_SEARCH_STEPS )
		{
			if ( !seg->ladder )
			{
				break;
			}

			seg = path->NextSegment( seg );
			safetyCounter++;
		}
		Assert( safetyCounter < MAX_PATH_SEARCH_STEPS );

		if ( seg && !seg->ladder && seg->area )
		{
			m_pExitArea = seg->area;
			m_exitAreaCenter = m_pExitArea->GetCenter();
		}
	}
}


//---------------------------------------------------------------------------------------------
// Implementation based on ladder climbing logic in https://github.com/Dragoteryx/drgbase/
ActionResult<CNEOBot> CNEOBotLadderClimb::Update( CNEOBot *me, float /*interval*/ )
{
	if ( m_timeoutTimer.IsElapsed() )
	{
		return Done( "Ladder climb timeout" );
	}

	const CKnownEntity *threat = me->GetVisionInterface()->GetPrimaryKnownThreat(true);
	if ( threat )
	{
		if ( me->IsDebugging( NEXTBOT_PATH ) )
		{
			DevMsg( "%s: Threat detected during ladder climb - engaging\n", me->GetDebugIdentifier() );
		}
		// Detach from ladder to ready weapon
		me->PressJumpButton();
		me->PressBackwardButton(0.1f);
		// ChangeTo: We may move away from ladder when fighting, reevaluate later
		return ChangeTo( new CNEOBotAttack, "Interrupting climb to engage enemy" );
	}

	ILocomotion *mover = me->GetLocomotionInterface();
	IBody *body = me->GetBodyInterface();
	const Vector& myPos = mover->GetFeet();
	bool bExitIsBehind = false;
	Vector toExit = vec3_origin;

	if ( m_pExitArea )
	{
		toExit = m_exitAreaCenter - myPos;
		toExit.z = 0.0f;
		if ( toExit.Length2DSqr() > 0.01f )
		{
			toExit.NormalizeInPlace();
		}
		else
		{
			toExit = m_ladderForward; // Fallback direction
		}

		if ( DotProduct( toExit, m_ladderForward ) < 0.0f )
		{
			bExitIsBehind = true;
		}
	}

	// Check if we're on the ladder (MOVETYPE_LADDER or locomotion says so)
	bool onLadder = me->IsBotOnLadder();

	if ( !onLadder )
	{
		if ( mover->IsOnGround() )
		{
			return Done( "Reached the ground, ending climb" );
		}
		
		if ( !m_bDismountPhase && !m_bJumpedOffLadder )
		{
			Vector ladderClosestPoint;
			CalcClosestPointOnLineSegment( myPos, m_ladder->m_bottom, m_ladder->m_top, ladderClosestPoint );
			if ( myPos.DistToSqr( ladderClosestPoint ) > Square( MAX_DEVIATION_DIST ) )
			{
				return Done( "Fallen too far from ladder, resetting" );
			}
		}
	}

	//------------------------------------------------------------
	// Ladder climbing phase
	//------------------------------------------------------------
	if ( !m_bDismountPhase )
	{
		float currentZ = myPos.z;
		float targetZ = m_bGoingUp ? m_ladder->m_top.z : m_ladder->m_bottom.z;

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
					return Continue();
				}
				else
				{
					// We are stuck mid-climb. Reset scenario by jumping backwards and ending condition
					if ( me->IsDebugging( NEXTBOT_PATH ) )
					{
						DevMsg( "%s: Ladder climb stuck - jumping backwards to reset (delta %.1f)\n",
							me->GetDebugIdentifier(), verticalDelta );
					}
					me->PressJumpButton();
					me->PressBackwardButton(0.1f);
					return Done( "Got stuck on something climbing the ladder, jumping off to reset." );
				}
			}
			m_flLastZ = currentZ;
			m_stuckTimer.Start( STUCK_CHECK_INTERVAL );
		}

		// Early jump-off
		if ( m_pExitArea )
		{
			float zDistToExit = currentZ - m_exitAreaCenter.z;

			if ( zDistToExit > 0.0f && zDistToExit <= SAFE_FALL_DIST )
			{
				EnterDismountPhase( me );
				return Continue();
			}
		}

		if ( m_bGoingUp )
		{
			body->SetDesiredPosture( IBody::STAND );
		}

		float dismountZ = targetZ;
		if ( m_pExitArea )
		{
			if ( m_bGoingUp )
			{
				if ( bExitIsBehind )
				{
					dismountZ = Min( m_exitAreaCenter.z, targetZ );
				}
			}
			else
			{
				// Allow early drop-off at intermediate floors
				dismountZ = Max( m_exitAreaCenter.z, targetZ );
			}
		}

		// Check if we've reached the target exit height
		if ( m_bGoingUp ? ( currentZ >= dismountZ ) : ( currentZ <= dismountZ ) )
		{
			EnterDismountPhase( me );
			return Continue();
		}

		// Adjust vertical height based on target exit area
		bool bIsClimbingDown = false;
		if ( onLadder )
		{
			bool bShouldGoUp = m_bGoingUp;
			if ( m_pExitArea )
			{
				bShouldGoUp = ( currentZ < m_exitAreaCenter.z );
			}

			if ( bShouldGoUp )
			{
				me->PressMoveUpButton();
			}
			else
			{
				me->PressMoveDownButton();
				bIsClimbingDown = true;
			}
		}


		// Look at and move to the dismount height, slightly behind the ladder
		Vector lookTarget = m_ladder->GetPosAtHeight( dismountZ );
		lookTarget -= m_ladder->GetNormal() * 50.0f;
		body->AimHeadTowards( lookTarget, IBody::MANDATORY, 0.1f, nullptr,
			m_bGoingUp ? "Climbing up (looking at dismount position)" : "Climbing down (looking at dismount position)" );
		me->PressForwardButton(0.1f);
	}

	//------------------------------------------------------------
	// Ladder dismount phase
	//------------------------------------------------------------
	if ( m_bDismountPhase )
	{
		// Reached the target NavArea after the ladder
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
		if ( m_pExitArea )
		{
			bool bDroppingEarly = ( myPos.z >= m_exitAreaCenter.z ); 
			// Maintain Z-height while on the ladder in the dismount phase
			if ( onLadder )
			{
				if ( myPos.z < m_exitAreaCenter.z )
				{
					me->PressMoveUpButton();
				}
				else
				{
					me->PressMoveDownButton();
				}
			}
			else
			{
				bDroppingEarly = true;
			}

			body->AimHeadTowards( m_exitAreaCenter, IBody::MANDATORY, 0.1f, nullptr, "Walking to exit area" );

			// Jump to detach from ladder if exit is not straight ahead, or if we have reached the exit height
			float dot = DotProduct( toExit, m_ladderForward );

			if ( dot < 0.5f || bDroppingEarly )
			{
				// Velocity kick to simulate a jump to the next NavArea
				if ( !m_bJumpedOffLadder )
				{
					me->PressJumpButton(); // mostly to trigger animation if possible

					mover->Reset(); // clear velocity cache in locomotion interface

					Vector jumpVelocity = toExit * 150.0f;
					jumpVelocity.z = 150.0f - me->GetAbsVelocity().z;

					me->ApplyAbsVelocityImpulse( jumpVelocity );
					m_bJumpedOffLadder = true;
				}
				else if ( !onLadder )
				{
					me->PressForwardButton();
				}
			}
			else
			{
				// Forward exit, just clamber over
				me->PressForwardButton();
			}

			if ( me->IsDebugging( NEXTBOT_PATH ) )
			{
				NDebugOverlay::Line( myPos, m_exitAreaCenter, 0, 255, 255, true, 0.1f );
			}
		}
		else
		{
			// No exit area resolved - just push forward along the ladder normal
			Vector pushDir = m_ladderForward;
			Vector pushTarget = myPos + 100.0f * pushDir;
			body->AimHeadTowards( pushTarget, IBody::MANDATORY, 0.1f, nullptr, "Dismount push forward" );
			me->PressForwardButton();
			
			if ( mover->IsOnGround() )
			{
				return Done( "Dismounted to ground" );
			}
		}
	}

	return Continue();
}

//---------------------------------------------------------------------------------------------
void CNEOBotLadderClimb::EnterDismountPhase( CNEOBot *me )
{
	me->SetAbsVelocity( vec3_origin ); // stop momentum
	m_bDismountPhase = true;
	m_dismountTimer.Start( DISMOUNT_TIMEOUT );
	ResolveExitArea( me );

	if ( me->IsDebugging( NEXTBOT_PATH ) )
	{
		DevMsg( "%s: Entering dismount phase (exit area %s)\n",
			me->GetDebugIdentifier(),
			m_pExitArea ? "found" : "NOT found" );
	}
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
