#include "cbase.h"
#include "bot/behavior/neo_bot_ladder_climb.h"
#include "nav_ladder.h"

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

//---------------------------------------------------------------------------------------------
CNEOBotLadderClimb::CNEOBotLadderClimb( const CNavLadder *ladder, bool goingUp )
	: m_ladder( ladder ), m_bGoingUp( goingUp ), m_bHasBeenOnLadder( false )
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
	float estimatedClimbTime = m_ladder->m_length / MAX_CLIMB_SPEED + 1.0f;
	m_timeoutTimer.Start( estimatedClimbTime );

	m_bHasBeenOnLadder = false;

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

	// Check if we're on the ladder (MOVETYPE_LADDER or locomotion says so)
	bool onLadder = ( me->GetMoveType() == MOVETYPE_LADDER ) || 
	                mover->IsUsingLadder() || 
	                mover->IsAscendingOrDescendingLadder();

	if ( onLadder )
	{
		m_bHasBeenOnLadder = true;
	}
	else if ( m_bHasBeenOnLadder )
	{
		// We were on the ladder but got knocked off - return to reevaluate situation
		// Since ladder approach should use ChangeTo climbing behavior, this Done will return to the state BEFORE ladder approach
		return Done( "Knocked off ladder - reevaluating situation" );
	}

	// Get current position and target
	const Vector& myPos = mover->GetFeet();
	float currentZ = myPos.z;
	float targetZ = m_bGoingUp ? m_ladder->m_top.z : m_ladder->m_bottom.z;

	if ( m_bGoingUp )
	{
		if ( currentZ >= targetZ - mover->GetStepHeight() )
		{
			return Done( "Reached top of ladder" );
		}

		// Climb up: look up and push into ladder
		Vector goal = myPos + 100.0f * ( -m_ladder->GetNormal() + Vector( 0, 0, 2 ) );
		body->AimHeadTowards( goal, IBody::MANDATORY, 0.1f, nullptr, "Climbing ladder" );
		mover->Approach( goal, 9999999.9f );
	}
	else
	{
		if ( currentZ <= targetZ + mover->GetStepHeight() )
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
