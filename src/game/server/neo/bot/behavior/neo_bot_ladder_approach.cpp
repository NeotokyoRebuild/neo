#include "cbase.h"
#include "bot/behavior/neo_bot_ladder_approach.h"
#include "bot/behavior/neo_bot_ladder_climb.h"
#include "bot/behavior/neo_bot_attack.h"
#include "nav_ladder.h"

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

//---------------------------------------------------------------------------------------------
CNEOBotLadderApproach::CNEOBotLadderApproach( const CNavLadder *ladder, bool goingUp )
	: m_ladder( ladder ), m_bGoingUp( goingUp ), m_bOverTop( false )
{
	m_ladderCenter = ladder ? ( ladder->m_top + ladder->m_bottom ) * 0.5f : vec3_origin;
}

//---------------------------------------------------------------------------------------------
ActionResult<CNEOBot> CNEOBotLadderApproach::OnStart( CNEOBot *me, Action<CNEOBot> *priorAction )
{
	if ( !m_ladder )
	{
		return Done( "No ladder specified" );
	}

	// Timeout for approach phase
	m_timeoutTimer.Start( 3.0f );

	// Going down from behind the ladder's plane to a top more than a step over the floor the descent starts from
	// (a parapet, a handrail or a wall cap between): mount over the top edge. The floor, since the bot may be on the barrier
	if ( !m_bGoingUp )
	{
		ILocomotion *mover = me->GetLocomotionInterface();
		const Vector &feet = mover->GetFeet();
		const CNavArea *area = me->GetLastKnownArea();
		const float floorZ = area ? MIN( feet.z, area->GetZ( feet.x, feet.y ) ) : feet.z;
		const bool bBehind = DotProduct2D( ( feet - m_ladder->m_top ).AsVector2D(), m_ladder->GetNormal().AsVector2D() ) < 0.0f;

		m_bOverTop = bBehind && floorZ < m_ladder->m_top.z - mover->GetStepHeight();
		if ( m_bOverTop )
		{
			m_timeoutTimer.Start( OVER_TOP_TIMEOUT );
		}
	}

	if ( me->IsDebugging( NEXTBOT_PATH ) )
	{
		DevMsg( "%s: Starting ladder approach (%s), length %.1f\n",
			me->GetDebugIdentifier(),
			m_bGoingUp ? "up" : "down",
			m_ladder->m_length );
	}

	// Don't interfere with look direction
	// Can exit out of behavior if threat is present
	me->StopLookingAroundForEnemies();
	me->SetAttribute( CNEOBot::IGNORE_ENEMIES );

	return Continue();
}

//---------------------------------------------------------------------------------------------
// Implementation based on ladder climbing implementation in https://github.com/Dragoteryx/drgbase/
ActionResult<CNEOBot> CNEOBotLadderApproach::Update( CNEOBot *me, float )
{
	if ( !m_ladder )
	{
		return Done( "Ladder is invalid" );
	}

	if ( m_timeoutTimer.IsElapsed() )
	{
		return Done( "Ladder approach timeout" );
	}

	const CKnownEntity *threat = me->GetVisionInterface()->GetPrimaryKnownThreat(true);
	if ( threat )
	{
		if ( me->IsDebugging( NEXTBOT_PATH ) )
		{
			DevMsg( "%s: Threat detected during ladder approach - engaging\n", me->GetDebugIdentifier() );
		}
		// ChangeTo: We may move away from ladder when fighting, so don't want to get stuck in ladder approach behavior
		return ChangeTo( new CNEOBotAttack(), "Engaging enemy before climbing" );
	}

	ILocomotion *mover = me->GetLocomotionInterface();
	IBody *body = me->GetBodyInterface();

	const Vector& myPos = mover->GetFeet();

	// If we end up closer to exit point than entry point,
	// may not need to continue climb, exit to reevaluate situation
	Vector entryPos = m_bGoingUp ? m_ladder->m_bottom : m_ladder->m_top;
	Vector exitPos = m_bGoingUp ? m_ladder->m_top : m_ladder->m_bottom;
	float distToEntrySq = myPos.DistToSqr( entryPos );
	float distToExitSq = myPos.DistToSqr( exitPos );

	if ( distToExitSq < distToEntrySq )
	{
		return Done( "Closer to ladder exit than entry, assuming goal reached accidentally" );
	}

	// Are we climbing up or down the ladder?
	Vector targetPos = m_bGoingUp ? m_ladder->m_bottom : m_ladder->m_top;

	// Going down, head for where the bot will hang on the ladder: in front of its face, over the drop
	if ( !m_bGoingUp )
	{
		targetPos += m_ladder->GetNormal() * ( body->GetHullWidth() * 0.5f + HANG_CLEARANCE );
	}

	// Calculate 2D vector from bot to ladder mount point
	Vector2D to = ( targetPos - myPos ).AsVector2D();
	float range = to.NormalizeInPlace();

	// Get ladder's outward normal in 2D
	Vector2D ladderNormal2D = m_ladder->GetNormal().AsVector2D();
	float dot = DotProduct2D( ladderNormal2D, to );

	if ( m_bOverTop )
	{
		return UpdateOverTop( me, targetPos, range );
	}

	// Aim at the ladder center at eye level to carefully attach to ladder
	// Eye level in order to not accidentally move and detach between behavior transition
	// If bot was looking up or down, sometimes the fast climb movement causes a detachment
	Vector lookTarget = m_ladderCenter;
	lookTarget.z = me->EyePosition().z;

	// Going down, look down the ladder instead: the forward press that grabs it stays held for a few
	// ticks, and with a level view it climbs the bot up and off the top
	if ( !m_bGoingUp )
	{
		lookTarget = m_ladder->m_bottom;
	}

	body->AimHeadTowards( lookTarget, IBody::MANDATORY, 0.1f, nullptr, "Stare at ladder center" );

	if ( me->IsDebugging( NEXTBOT_PATH ) )
	{
		NDebugOverlay::Cross3D( targetPos, 5.0f, 255, 255, 0, true, 0.1f );
		NDebugOverlay::Line( myPos, targetPos, 255, 255, 0, true, 0.1f );
	}

	// Going down, mount only once off the top: grabbed while still up there, on the landing or on the
	// ladder's own top, the bot cannot press its way down, so it keeps walking out over the drop
	const float flTopZ = m_ladder->m_top.z;
	const bool bOnTop = !m_bGoingUp
		&& ( myPos.z >= flTopZ || ( mover->IsOnGround() && myPos.z > flTopZ - mover->GetStepHeight() ) );

	// Within mount range and on the ladder: start climbing
	if ( range < MOUNT_RANGE && me->IsOnLadder() && !bOnTop )
	{
		if ( me->IsDebugging( NEXTBOT_PATH ) )
		{
			DevMsg( "%s: Starting ladder climb\n", me->GetDebugIdentifier() );
		}

		// Stop the bot before behavior transition to prevent falling off the ladder
		// there can be a delay in the state change, so momentum can cause a fall
		me->SetAbsVelocity( vec3_origin );
		// ChangeTo: if something goes wrong during climb, reevaluate situation
		return ChangeTo( new CNEOBotLadderClimb( m_ladder, m_bGoingUp ), "Mounting ladder" );
	}

	// Are we aligned and close enough to mount the ladder? Going down, line up only from in front of the
	// face (at an angle the bot ends up on the floor beside the drop); from elsewhere walk straight on
	const bool bAligned = dot < ALIGN_DOT_THRESHOLD;
	const bool bLineUp = m_bGoingUp ? range >= MOUNT_RANGE
		: ( dot < 0.0f && ( range >= MOUNT_RANGE || !bAligned ) );
	if ( bLineUp )
	{
		// Perpendicular alignment line
		Vector2D alignNormal = ladderNormal2D;
		if ( dot > 0.0f )
		{
			alignNormal = -alignNormal; // Target behind ladder
		}

		Vector goal = targetPos;
		
		// Pull the goal point outwards along the ladder's normal
		// to guide bot movement along approach
		float offsetDist = Clamp( range * 0.8f, 10.0f, ALIGN_RANGE );
		goal.x += alignNormal.x * offsetDist;
		goal.y += alignNormal.y * offsetDist;

		mover->Approach( goal );

		if ( me->IsDebugging( NEXTBOT_PATH ) )
		{
			NDebugOverlay::Cross3D( goal, 5.0f, 255, 0, 255, true, 0.1f );
		}
	}
	else if ( m_bGoingUp ? bAligned : !bOnTop )
	{
		// Aligned (or going down and off the top), push forward to attach to the ladder
		me->PressForwardButton();
		mover->Approach( targetPos );
	}
	else
	{
		// Close but not aligned (or going down and still on the top) - continue approaching
		mover->Approach( targetPos );
	}

	return Continue();
}

//---------------------------------------------------------------------------------------------
// Going down a ladder whose top stands on a barrier the bot reaches from behind the ladder's plane:
//
//                 hang point
//           face -> x |#|  <- parapet, handrail or wall cap, level with the ladder's top
//                     |#|_______________________   floor the descent starts from
//                     |                             <- bot backs out this way
//
// Walking off the top forward, the bot falls past the face moving away from it, which LadderMove() never grabs.
// So it faces the face, hops the barrier and backs out, and the locomotion presses into the face as it drops.
ActionResult<CNEOBot> CNEOBotLadderApproach::UpdateOverTop( CNEOBot *me, const Vector &hangPos, float range )
{
	CNEOBotLocomotion *mover = me->GetLocomotionInterface();
	const Vector &feet = mover->GetFeet();

	// Caught below the top, hanging in front of the face: climb down. Standing on the ladder's own top
	// the engine may hold the bot too, but that is no mount: the locomotion lets go of it
	if ( me->IsOnLadder() && !mover->IsOnGround() && feet.z < m_ladder->m_top.z && range < MOUNT_RANGE )
	{
		mover->StopCatchingLadder();
		me->SetAbsVelocity( vec3_origin );
		return ChangeTo( new CNEOBotLadderClimb( m_ladder, m_bGoingUp ), "Mounting ladder over its top" );
	}

	// Face the ladder from far behind it, just below level: forward then points into the face and reaches furthest,
	// and the back press points a little up, never into the ladder's own top
	const Vector intoFace = -m_ladder->GetNormal();
	Vector lookAt = m_ladder->m_top + FACE_LOOK_RANGE * intoFace;
	lookAt.z = me->EyePosition().z - FACE_LOOK_DROP;
	me->GetBodyInterface()->AimHeadTowards( lookAt, IBody::MANDATORY, 0.1f, nullptr, "Facing the ladder to back onto it" );

	mover->CatchLadderBelowTop( m_ladder );

	// In the air, hopping onto the barrier or dropping past the face: keep backing out
	if ( !mover->IsOnGround() )
	{
		me->PressBackwardButton();
		return Continue();
	}

	// How far to the side of the ladder's middle the bot is, and how far out in front of its face
	const Vector2D toFeet = ( feet - m_ladder->m_top ).AsVector2D();
	const float side = toFeet.x * intoFace.y - toFeet.y * intoFace.x;
	const float out = -DotProduct2D( toFeet, intoFace.AsVector2D() );
	const float halfHull = me->GetBodyInterface()->GetHullWidth() * 0.5f;
	const float rise = m_ladder->m_top.z - feet.z;
	const bool bOnTop = rise <= mover->GetStepHeight();

	if ( fabsf( side ) > MAX( m_ladder->m_width * 0.5f, LINEUP_TOLERANCE ) )
	{
		// Line up behind the ladder first: hopping or dropping from beside it, the bot comes down beside it. Near
		// the edge, sideways only: a press with any part into the face dips into the ladder's top, which grabs the bot
		const bool bNearEdge = bOnTop && out > -( halfHull + EDGE_ZONE );
		const float depth = bNearEdge ? -out : halfHull + ( bOnTop ? 0.0f : LINEUP_DEPTH );
		Vector lineUp = m_ladder->m_top + intoFace * depth;
		lineUp.z = feet.z;
		mover->Approach( lineUp );
		return Continue();
	}

	if ( !bOnTop )
	{
		// On the floor behind the barrier: hop onto it from a standstill, which goes straight up, and back out
		// over it. No sidestep: square to the face, backing out heads straight for the hang point
		if ( !mover->IsClimbingOrJumping() && rise <= mover->GetMaxJumpHeight() && IsBarrierAhead( me, hangPos ) )
		{
			me->SetAbsVelocity( vec3_origin );
			mover->Jump();
		}

		me->PressBackwardButton();
		return Continue();
	}

	// On the barrier's top: back out crouched, so the bot leaves the top slowly and drops within reach of the face
	me->PressCrouchButton( CROUCH_HOLD );

	// Over the ladder's own top, a back press with the view still pointing up (coming down from a climb)
	// points into that top: the engine grabs the bot there and slides it off at climbing speed
	if ( out > -( halfHull + EDGE_ZONE ) && me->EyeAngles().x < 0.0f )
	{
		return Continue();
	}

	// Sidesteps only while the bot's middle is behind the face: one still held as it drops would skew the
	// press into the face
	if ( out < 0.0f )
	{
		mover->Approach( hangPos );
	}
	else
	{
		me->PressBackwardButton();
	}

	return Continue();
}

//---------------------------------------------------------------------------------------------
// Is the way to the hang point blocked just ahead, a step above the feet: a barrier, not a curb
bool CNEOBotLadderApproach::IsBarrierAhead( CNEOBot *me, const Vector &hangPos ) const
{
	const ILocomotion *mover = me->GetLocomotionInterface();
	Vector toHang = hangPos - mover->GetFeet();
	toHang.z = 0.0f;
	if ( toHang.NormalizeInPlace() <= 0.0f )
	{
		return false;
	}

	const Vector start = mover->GetFeet() + Vector( 0.0f, 0.0f, mover->GetStepHeight() );
	const Vector end = start + BARRIER_PROBE * toHang;

	trace_t tr;
	UTIL_TraceHull( start, end, me->WorldAlignMins(), me->WorldAlignMaxs(), MASK_PLAYERSOLID, me, COLLISION_GROUP_PLAYER_MOVEMENT, &tr );

	return tr.DidHit();
}

//---------------------------------------------------------------------------------------------
void CNEOBotLadderApproach::OnEnd( CNEOBot *me, Action<CNEOBot> *nextAction )
{
	me->StartLookingAroundForEnemies();
	me->ClearAttribute( CNEOBot::IGNORE_ENEMIES );
	me->GetLocomotionInterface()->StopCatchingLadder();

	if ( me->IsDebugging( NEXTBOT_PATH ) )
	{
		DevMsg( "%s: Finished ladder approach\n", me->GetDebugIdentifier() );
	}
}

//---------------------------------------------------------------------------------------------
ActionResult<CNEOBot> CNEOBotLadderApproach::OnSuspend( CNEOBot *me, Action<CNEOBot> *interruptingAction )
{
	return Done( "OnSuspend: Cancel out of ladder approach, situation will likely become stale." );
}

//---------------------------------------------------------------------------------------------
ActionResult<CNEOBot> CNEOBotLadderApproach::OnResume( CNEOBot *me, Action<CNEOBot> *interruptingAction )
{
	return Done( "OnResume: Cancel out of ladder approach, situation is likely stale." );
}
