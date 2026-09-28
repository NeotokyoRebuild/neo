#include "cbase.h"
#include "bot/behavior/neo_bot_ctg_capture.h"
#include "bot/behavior/neo_bot_ctg_lone_wolf_seek.h"
#include "bot/behavior/neo_bot_seek_weapon.h"
#include "bot/neo_bot_path_compute.h"
#include "weapon_ghost.h"

namespace
{
constexpr float CTG_CAPTURE_ATTEMPT_TIME = 3.0f;	// Per nav area, before the ghost counts as lodged
constexpr float CTG_CAPTURE_BUTTON_TAP_HOLD = 0.1f;
constexpr float CTG_CAPTURE_USE_TAP_INTERVAL = 0.3f;
constexpr float CTG_CAPTURE_USE_JUMP_INTERVAL = 1.0f;

bool IsGhostInSight( CNEOBot *me, const Vector &vecEye, CWeaponGhost *pGhost, const Vector &vecGhostCenter )
{
	trace_t trace;
	NextBotTraceFilterIgnoreActors filter( me, COLLISION_GROUP_NONE );
	UTIL_TraceLine( vecEye, vecGhostCenter, MASK_PLAYERSOLID, &filter, &trace );
	return trace.m_pEnt == pGhost || trace.fraction == 1.0f;
}
}


//---------------------------------------------------------------------------------------------
CNEOBotCtgCapture::CNEOBotCtgCapture( CWeaponGhost *pObjective )
{
	m_hObjective = pObjective;
}


//---------------------------------------------------------------------------------------------
ActionResult<CNEOBot> CNEOBotCtgCapture::OnStart( CNEOBot *me, Action<CNEOBot> *priorAction )
{
	m_path.Invalidate();
	m_repathTimer.Invalidate();
	m_captureAttemptTimer.Start( CTG_CAPTURE_ATTEMPT_TIME );
	m_useTapTimer.Invalidate();
	m_useJumpTimer.Invalidate();
	m_previousKnownArea = me->GetLastKnownArea();
	
	if ( !m_hObjective )
	{
		return Done( "No ghost capture target specified." );
	}

	return Continue();
}


//---------------------------------------------------------------------------------------------
ActionResult<CNEOBot> CNEOBotCtgCapture::Update( CNEOBot *me, float interval )
{
	if ( me->IsDead() )
	{
		return Done( "I died before I could capture the ghost" );
	}

	if ( !m_hObjective )
	{
		return Done( "Ghost capture target lost" );
	}

	if ( me->IsCarryingGhost() )
	{
		return Done( "Captured ghost" );
	}

	if ( m_hObjective->GetOwner() )
	{
		return Done( "Ghost was taken by someone else" );
	}

	// Add more capture time if we progressed to the next NavArea
    CNavArea *pCurrentKnownArea = me->GetLastKnownArea();
    if ( pCurrentKnownArea != m_previousKnownArea )
    {
        m_captureAttemptTimer.Start( CTG_CAPTURE_ATTEMPT_TIME );
        m_previousKnownArea = pCurrentKnownArea;
    }

	if ( !m_repathTimer.HasStarted() || m_repathTimer.IsElapsed() )
	{
		if ( !CNEOBotPathCompute( me, m_path, m_hObjective->GetAbsOrigin(), FASTEST_ROUTE ) )
		{
			// Something has gone wrong if we can't path to a ghost that we by convention started close to
			// Transition to search around the ghost behavior to avoid cycle of ghost search failure and retry attempts
			return ChangeTo( new CNEOBotCtgLoneWolf(), "Unable to find a path to the ghost capture target, searching around nearest areas" );
		}
		m_repathTimer.Start( RandomFloat( 1.0f, 2.0f ) );
	}
	m_path.Update( me );

	CBaseCombatWeapon *pPrimary = me->Weapon_GetSlot( 0 );
	if ( pPrimary )
	{
		// Switch to primary weapon to drop it, if not already active
		if ( me->GetActiveWeapon() != pPrimary )
		{
			me->Weapon_Switch( pPrimary );
		}
		else
		{
			me->PressDropButton( 0.1f );
		}
	}
	
	// A ghost that cannot be walked onto can still be picked up the way players do it: look at it
	// and press use. Not while a threat is in view, as that aim would keep the head off the threat
	const Vector vecEye = me->EyePosition();
	const Vector vecGhostCenter = m_hObjective->WorldSpaceCenter();
	if ( vecEye.DistToSqr( vecGhostCenter ) < Square( PLAYER_USE_RADIUS )
		&& !me->GetVisionInterface()->GetPrimaryKnownThreat( true ) )
	{
		TryUseGhost( me, vecEye, vecGhostCenter );
	}

	if ( m_captureAttemptTimer.IsElapsed() )
	{
		// If the bot fails to capture the ghost, it's sometimes because the ghost is lodged into an awkward position
		// Have the bot search around the location instead, to avoid cycle of failing to pick up ghost and retrying
		return ChangeTo( new CNEOBotCtgLoneWolf(), "Failed to pick up ghost in time, searching around nearest areas" );
	}

	return Continue();
}


//---------------------------------------------------------------------------------------------
void CNEOBotCtgCapture::TryUseGhost( CNEOBot *me, const Vector &vecEye, const Vector &vecGhostCenter )
{
	me->GetBodyInterface()->AimHeadTowards( vecGhostCenter, IBody::MANDATORY, 0.1f, nullptr, "Looking at the ghost to use it" );

	// Use registers on the press, so tap it, but only when a press would pick the ghost
	if ( m_useTapTimer.IsElapsed() && me->GetBodyInterface()->IsHeadAimingOnTarget() )
	{
		m_useTapTimer.Start( CTG_CAPTURE_USE_TAP_INTERVAL );
		if ( me->FindUseEntity() == m_hObjective )
		{
			me->PressUseButton( CTG_CAPTURE_BUTTON_TAP_HOLD );
			return;
		}
	}

	// At the foot of a crate the ghost rests on, the top edge hides it: jump to see over
	ILocomotion *pMover = me->GetLocomotionInterface();
	if ( vecGhostCenter.z <= vecEye.z || !pMover->IsOnGround() || !m_useJumpTimer.IsElapsed() )
	{
		return;
	}

	m_useJumpTimer.Start( CTG_CAPTURE_USE_JUMP_INTERVAL );
	if ( !IsGhostInSight( me, vecEye, m_hObjective, vecGhostCenter ) )
	{
		pMover->Jump();
	}
}
