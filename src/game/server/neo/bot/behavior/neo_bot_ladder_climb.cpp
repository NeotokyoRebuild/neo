#include "cbase.h"
#include "bot/behavior/neo_bot_attack.h"
#include "bot/behavior/neo_bot_ladder_climb.h"
#include "nav_ladder.h"
#include "NextBot/Path/NextBotPathFollow.h"

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

// The engine grabs a ladder within this distance of its face (CGameMovement::LadderDistance())
static constexpr float LADDER_GRAB_DIST = 2.0f;
// A hull placed off a ladder starts this far clear of the face,
static constexpr float LADDER_PLACE_GAP = 2.0f;
// and is slid at most this far towards the face to find it.
static constexpr float LADDER_PLACE_SEEK = 16.0f;

//---------------------------------------------------------------------------------------------
CNEOBotLadderClimb::CNEOBotLadderClimb( const CNavLadder *ladder, bool goingUp )
	: m_ladder( ladder ), m_bGoingUp( goingUp ), m_flLastZ( 0.0f ),
	m_bDismountPhase( false ), m_bJumpedOffLadder( false ), m_pExitArea( nullptr )
{
	m_dismountPos = vec3_origin;
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

			// On a ladder turned off the world axes, half a hull width off the face leaves the hull's corner in the brush,
			// so clear the corners first, then slide in until the hull touches the face
			const Vector &normal = m_ladder->GetNormal();
			const float flReach = me->CollisionProp()->OBBSize().x / 2.0f * ( fabsf( normal.x ) + fabsf( normal.y ) );
			Vector clearPos = m_ladder->GetPosAtHeight( m_flLastZ ) + normal * ( flReach + LADDER_PLACE_GAP );
			clearPos.z = m_flLastZ;

			trace_t trFace;
			UTIL_TraceHull( clearPos, clearPos - normal * LADDER_PLACE_SEEK, me->WorldAlignMins(), me->WorldAlignMaxs(),
				MASK_PLAYERSOLID, me, COLLISION_GROUP_PLAYER_MOVEMENT, &trFace );
			if ( !trFace.startsolid )
			{
				idealPos = trFace.DidHit() ? trFace.endpos + normal * ( LADDER_GRAB_DIST * 0.5f ) : clearPos;
			}

			// The floor there can stand higher than the feet were, and a hull sunk in a displacement does not start solid,
			// so stand the hull on that floor: unducking on the ladder lowers the feet, and from inside the floor wedges the bot
			trace_t trFloor;
			UTIL_TraceHull( idealPos + Vector( 0.0f, 0.0f, mover->GetStepHeight() ), idealPos, me->WorldAlignMins(), me->WorldAlignMaxs(),
				MASK_PLAYERSOLID, me, COLLISION_GROUP_PLAYER_MOVEMENT, &trFloor );
			if ( !trFloor.startsolid && trFloor.DidHit() )
			{
				idealPos.z = trFloor.endpos.z;
			}

			// Face perpendicularly straight on to the ladder (-normal)
			Vector idealLookDir = -m_ladder->GetNormal();
			QAngle idealAngles;
			VectorAngles( idealLookDir, idealAngles );

			// A spot still in solid, such as inside a ladder brush that stands in front of its nav line, would hold the bot
			// there for good, so the bot climbs from where it is instead
			trace_t trSpot;
			UTIL_TraceHull( idealPos, idealPos, me->WorldAlignMins(), me->WorldAlignMaxs(), MASK_PLAYERSOLID, me, COLLISION_GROUP_PLAYER_MOVEMENT, &trSpot );
			if ( !trSpot.startsolid )
			{
				// Teleport the bot
				me->SetAbsOrigin( idealPos );
				me->SetAbsAngles( idealAngles );
				// the view too: a forward press with the view off the face's normal slides the bot off the side of a narrow ladder
				me->SnapEyeAngles( idealAngles );

				// Update mover feet to new teleported position for stuck checking
				m_flLastZ = idealPos.z;
			}
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

	ClaimLadder( me );

	return Continue();
}

//---------------------------------------------------------------------------------------------
// PlayerLocomotion lets go of a ladder it was never asked to use, and once the contact persists
// takes it over by the nearer end - at the foot, the bottom. Tell it this climb is wanted.
void CNEOBotLadderClimb::ClaimLadder( CNEOBot *me ) const
{
	ILocomotion *mover = me->GetLocomotionInterface();
	if ( mover->IsUsingLadder() )
	{
		return;
	}

	const CNavArea *pExitArea = m_pExitArea;
	if ( !pExitArea )
	{
		pExitArea = m_bGoingUp ? m_ladder->GetTopArea() : m_ladder->m_bottomArea;
	}

	// The locomotion's dismount walks to this area, so there is nothing to claim with without one
	if ( !pExitArea )
	{
		return;
	}

	if ( m_bGoingUp )
	{
		mover->ClimbLadder( m_ladder, pExitArea );
	}
	else
	{
		mover->DescendLadder( m_ladder, pExitArea );
	}
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

		// The path's current goal can still be the area at the ladder's foot when the climb reaches the
		// top, and that area is no exit: the exit is the first area after this ladder
		const Path::Segment *ladderSeg = seg;
		for ( int i = 0; ladderSeg && ladderSeg->ladder != m_ladder && i < MAX_PATH_SEARCH_STEPS; ++i )
		{
			ladderSeg = path->NextSegment( ladderSeg );
		}

		if ( ladderSeg && ladderSeg->ladder == m_ladder )
		{
			seg = ladderSeg;
		}

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
			m_pExitArea = FindLanding( path, seg->area );
			m_dismountPos = GetDismountPos( me );
		}
	}
}

//---------------------------------------------------------------------------------------------
// By the dismount the path's goal can already be past the area this ladder lands on,
// so take a landing the path passes through, or else the ladder's own end area
const CNavArea *CNEOBotLadderClimb::FindLanding( const PathFollower *path, const CNavArea *pathExit ) const
{
	const CNavLadder::LadderDirectionType dir = m_bGoingUp ? CNavLadder::LADDER_UP : CNavLadder::LADDER_DOWN;
	if ( m_ladder->IsConnected( pathExit, dir ) )
	{
		return pathExit;
	}

	for ( const Path::Segment *seg = path->FirstSegment(); seg; seg = path->NextSegment( seg ) )
	{
		if ( seg->area && m_ladder->IsConnected( seg->area, dir ) )
		{
			return seg->area;
		}
	}

	const CNavArea *ladderEnd = m_bGoingUp ? m_ladder->GetTopArea() : m_ladder->m_bottomArea;
	return ladderEnd ? ladderEnd : pathExit;
}

//---------------------------------------------------------------------------------------------
// A landing shallower than half a hull is a wall cap the climb crosses, not a floor to stand on
bool CNEOBotLadderClimb::IsNarrowLanding( CNEOBot *me ) const
{
	const float flMinDepth = me->GetBodyInterface()->GetHullWidth() * NARROW_LANDING_HULLS;
	return Min( m_pExitArea->GetSizeX(), m_pExitArea->GetSizeY() ) < flMinDepth;
}

//---------------------------------------------------------------------------------------------
// Step off toward where the landing meets this end of the ladder: a long, narrow landing's center
// can lie off to one side, and the dismount kick toward it carries the bot past the landing's edge
Vector CNEOBotLadderClimb::GetDismountPos( CNEOBot *me ) const
{
	if ( IsNarrowLanding( me ) )
	{
		return m_pExitArea->GetCenter();
	}

	const Vector &ladderEnd = m_bGoingUp ? m_ladder->m_top : m_ladder->m_bottom;
	Vector nearPoint;
	m_pExitArea->GetClosestPointOnArea( ladderEnd, &nearPoint );

	// into the wall at the top, where the landing is, and away from it at the bottom
	Vector inward = m_bGoingUp ? m_ladderForward : -m_ladderForward;
	inward.z = 0.0f;
	if ( inward.NormalizeInPlace() > 0.0f )
	{
		nearPoint += inward * LANDING_INSET;
	}

	Vector dismountPos;
	m_pExitArea->GetClosestPointOnArea( nearPoint, &dismountPos );
	return dismountPos;
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
		toExit = m_dismountPos - myPos;
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
		// The locomotion drops its claim whenever the engine lets go of the bot, even for a tick
		if ( me->GetMoveType() == MOVETYPE_LADDER )
		{
			ClaimLadder( me );
		}

		float currentZ = myPos.z;
		float targetZ = m_bGoingUp ? m_ladder->m_top.z : m_ladder->m_bottom.z;

		// Going down, the bot holds still until its view makes forward take it down: not a stall
		const bool bHoldForView = !m_bGoingUp && !me->GetLocomotionInterface()->IsForwardDownLadder( m_ladder );
		if ( bHoldForView )
		{
			m_flLastZ = currentZ;
			m_stuckTimer.Start( STUCK_CHECK_INTERVAL );
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
					return Continue();
				}
				else if ( m_nNudges < STALL_NUDGES )
				{
					// Usually snagged on something at the ladder's edge, not blocked outright:
					// shimmy sideways, right then left, for longer each time, before giving up
					++m_nNudges;
					m_nudgeTimer.Start( STALL_NUDGE_TIME * m_nNudges );
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

		// Early jump-off. Going down, only once a standing height below the top: from any higher, the
		// kick towards the exit lands the bot back on the floor the descent started from.
		bool bWantsDismount = false;
		if ( m_pExitArea )
		{
			float zDistToExit = currentZ - m_dismountPos.z;
			const bool bBelowTopFloor = m_bGoingUp || currentZ < m_ladder->m_top.z - body->GetStandHullHeight();

			if ( zDistToExit > 0.0f && zDistToExit <= SAFE_FALL_DIST && bBelowTopFloor )
			{
				bWantsDismount = true;
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
					dismountZ = Min( m_dismountPos.z, targetZ );
				}
			}
			else
			{
				// Allow early drop-off at intermediate floors
				dismountZ = Max( m_dismountPos.z, targetZ );
			}
		}

		// Check if we've reached the target exit height
		if ( m_bGoingUp ? ( currentZ >= dismountZ ) : ( currentZ <= dismountZ ) )
		{
			bWantsDismount = true;
		}

		// A parapet between the ladder and a roof exit blocks the step off at floor height:
		// keep climbing until the hull clears it (stuck detection ends it if nothing does)
		const bool bLipInTheWay = bWantsDismount && m_bGoingUp && onLadder
			&& IsDismountBlocked( me, m_pExitArea ? toExit : m_ladderForward );

		if ( bWantsDismount && !bLipInTheWay )
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
				bShouldGoUp = ( currentZ < m_dismountPos.z );
			}

			if ( bLipInTheWay )
			{
				bShouldGoUp = true;
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


		// A descent the locomotion has claimed is aimed by its DescendLadder();
		// a second MANDATORY aim here would hold the view back from it
		if ( m_bGoingUp || !mover->IsUsingLadder() )
		{
			// Look at and move to the dismount height, slightly behind the ladder
			Vector lookTarget = m_ladder->GetPosAtHeight( dismountZ );
			lookTarget -= m_ladder->GetNormal() * 50.0f;
			body->AimHeadTowards( lookTarget, IBody::MANDATORY, 0.1f, nullptr,
				m_bGoingUp ? "Climbing up (looking at dismount position)" : "Climbing down (looking at dismount position)" );
		}

		if ( bHoldForView )
		{
			me->ReleaseForwardButton();
		}
		else
		{
			me->PressForwardButton(0.1f);
		}

		if ( onLadder && m_nudgeTimer.HasStarted() && !m_nudgeTimer.IsElapsed() )
		{
			if ( m_nNudges % 2 )
			{
				me->PressRightButton( 0.1f );
			}
			else
			{
				me->PressLeftButton( 0.1f );
			}
		}
	}

	//------------------------------------------------------------
	// Ladder dismount phase
	//------------------------------------------------------------
	if ( m_bDismountPhase )
	{
		// The last known area becomes the landing while the bot is still in the air beside it,
		// so finish once the bot stands on the landing, or enters a narrow one it crosses
		if ( m_pExitArea && me->GetLastKnownArea() == m_pExitArea
			&& ( IsNarrowLanding( me ) || ( mover->IsOnGround() && m_pExitArea->IsOverlapping( myPos ) ) ) )
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
			bool bDroppingEarly = ( myPos.z >= m_dismountPos.z ); 
			// Maintain Z-height while on the ladder in the dismount phase
			if ( onLadder )
			{
				if ( myPos.z < m_dismountPos.z )
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

			body->AimHeadTowards( m_dismountPos, IBody::MANDATORY, 0.1f, nullptr, "Walking to exit area" );

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
				NDebugOverlay::Line( myPos, m_dismountPos, 0, 255, 255, true, 0.1f );
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
bool CNEOBotLadderClimb::IsDismountBlocked( CNEOBot *me, const Vector &toExit ) const
{
	const Vector &feet = me->GetLocomotionInterface()->GetFeet();
	const float hullWidth = me->GetBodyInterface()->GetHullWidth();
	const Vector traceEnd = feet + toExit * ( DISMOUNT_CLEARANCE_HULLS * hullWidth );

	trace_t tr;
	UTIL_TraceHull( feet, traceEnd, me->WorldAlignMins(), me->WorldAlignMaxs(), MASK_NPCSOLID, me, COLLISION_GROUP_NONE, &tr );

	return tr.DidHit();
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
