#include "cbase.h"
#include "bot/behavior/neo_bot_ladder_approach.h"
#include "bot/behavior/neo_bot_ladder_climb.h"
#include "bot/behavior/neo_bot_attack.h"
#include "nav_ladder.h"

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

//---------------------------------------------------------------------------------------------
CNEOBotLadderApproach::CNEOBotLadderApproach( const CNavLadder *ladder, bool goingUp )
	: m_ladder( ladder ), m_bGoingUp( goingUp )
{
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

	if ( me->IsDebugging( NEXTBOT_PATH ) )
	{
		DevMsg( "%s: Starting ladder approach (%s), length %.1f\n",
			me->GetDebugIdentifier(),
			m_bGoingUp ? "up" : "down",
			m_ladder->m_length );
	}

	return Continue();
}

//---------------------------------------------------------------------------------------------
// Implementation based on ladder climbing implementation in https://github.com/Dragoteryx/drgbase/
ActionResult<CNEOBot> CNEOBotLadderApproach::Update( CNEOBot *me, float interval )
{
	if ( m_timeoutTimer.IsElapsed() )
	{
		return Done( "Ladder approach timeout" );
	}

	const CKnownEntity *threat = me->GetVisionInterface()->GetPrimaryKnownThreat();
	if ( threat && threat->IsVisibleRecently() )
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

	// Are we climbing up or down the ladder?
	const Vector& targetPos = m_bGoingUp ? m_ladder->m_bottom : m_ladder->m_top;

	// Calculate 2D vector from bot to ladder mount point
	Vector2D to = ( targetPos - myPos ).AsVector2D();
	float range = to.NormalizeInPlace();

	// Get ladder's outward normal in 2D
	Vector2D ladderNormal2D = m_ladder->GetNormal().AsVector2D();
	float dot = DotProduct2D( ladderNormal2D, to );

	if ( me->IsDebugging( NEXTBOT_PATH ) )
	{
		NDebugOverlay::Line( myPos, targetPos, 255, 255, 0, true, 0.1f );
	}

	// Are we aligned and close enough to mount the ladder?
	if ( range >= ALIGN_RANGE )
	{
		// Far from ladder - just approach the target position
		body->AimHeadTowards( targetPos, IBody::CRITICAL, 0.5f, nullptr, "Moving toward ladder" );
		mover->Approach( targetPos );
	}
	else if ( range >= MOUNT_RANGE )
	{
		// Within alignment range but not mount range - need to align with ladder
		if ( dot >= ALIGN_DOT_THRESHOLD )
		{
			// Not aligned - rotate around the ladder to get aligned
			Vector2D myPerp( -to.y, to.x );
			Vector2D ladderPerp2D( -ladderNormal2D.y, ladderNormal2D.x );

			Vector goal = targetPos;
			
			// Calculate offset to circle around
			float alignRange = MOUNT_RANGE + (1.0f + dot) * (ALIGN_RANGE - MOUNT_RANGE);
			goal.x -= alignRange * to.x;
			goal.y -= alignRange * to.y;

			// Choose direction to circle based on perpendicular alignment
			if ( DotProduct2D( to, ladderPerp2D ) < 0.0f )
			{
				goal.x += 10.0f * myPerp.x;
				goal.y += 10.0f * myPerp.y;
			}
			else
			{
				goal.x -= 10.0f * myPerp.x;
				goal.y -= 10.0f * myPerp.y;
			}

			body->AimHeadTowards( goal, IBody::CRITICAL, 0.3f, nullptr, "Aligning with ladder" );
			mover->Approach( goal );

			if ( me->IsDebugging( NEXTBOT_PATH ) )
			{
				NDebugOverlay::Cross3D( goal, 5.0f, 255, 0, 255, true, 0.1f );
			}
		}
		else
		{
			// Aligned - approach the ladder base directly
			body->AimHeadTowards( targetPos, IBody::CRITICAL, 0.3f, nullptr, "Approaching ladder" );
			mover->Approach( targetPos );
		}
	}
	else
	{
		// Within mount range - check if aligned to start climbing
		if ( dot < ALIGN_DOT_THRESHOLD )
		{
			if ( me->IsDebugging( NEXTBOT_PATH ) )
			{
				DevMsg( "%s: Starting ladder climb\n", me->GetDebugIdentifier() );
			}

			// ChangeTo: if something goes wrong during climb, reevaluate situation
			return ChangeTo( new CNEOBotLadderClimb( m_ladder, m_bGoingUp ), "Mounting ladder" );
		}
		else
		{
			// Close but not aligned - continue approaching to align
			body->AimHeadTowards( targetPos, IBody::CRITICAL, 0.3f, nullptr, "Approaching ladder" );
			mover->Approach( targetPos );
		}
	}

	return Continue();
}
