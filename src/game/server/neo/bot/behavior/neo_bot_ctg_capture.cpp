#include "cbase.h"
#include "bot/behavior/neo_bot_ctg_capture.h"
#include "bot/behavior/neo_bot_ctg_lone_wolf_seek.h"
#include "bot/behavior/neo_bot_seek_weapon.h"
#include "bot/neo_bot_path_compute.h"
#include "weapon_ghost.h"


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
	m_captureAttemptTimer.Start( CAPTURE_ATTEMPT_TIME );
	m_useTapTimer.Invalidate();
	m_dislodgeTimer.Invalidate();
	m_bTriedDislodge = false;
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
        m_captureAttemptTimer.Start( CAPTURE_ATTEMPT_TIME );
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

	// While dislodging, leave the sidearm out: the primary is dropped afterwards
	CBaseCombatWeapon *pPrimary = me->Weapon_GetSlot( 0 );
	if ( pPrimary && !m_dislodgeTimer.HasStarted() )
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
	
	// A ghost that cannot be walked onto can still be picked up the way players do it:
	// look at it and press use
	const Vector vecGhostCenter = m_hObjective->WorldSpaceCenter();
	const bool bGhostInUseRange = ( me->EyePosition().DistToSqr( vecGhostCenter ) < Square( PLAYER_USE_RADIUS ) )
		&& me->IsLineOfSightClear( m_hObjective, CBaseCombatCharacter::IGNORE_ACTORS );

	if ( bGhostInUseRange && !m_dislodgeTimer.HasStarted() )
	{
		me->GetBodyInterface()->AimHeadTowards( vecGhostCenter, IBody::MANDATORY, 0.1f, nullptr, "Looking at the ghost to use it" );

		// Same facing test as CNEOBotJgrCapture, but use only registers on the press, so tap it
		Vector vecToGhostDir = vecGhostCenter - me->EyePosition();
		vecToGhostDir.NormalizeInPlace();

		Vector vecEyeDirection;
		me->EyeVectors( &vecEyeDirection );
		const bool bIsFacing = vecEyeDirection.Dot( vecToGhostDir ) > USE_FACING_DOT;

		if ( bIsFacing && me->GetBodyInterface()->IsHeadAimingOnTarget() && m_useTapTimer.IsElapsed() )
		{
			me->PressUseButton( BUTTON_TAP_HOLD );
			m_useTapTimer.Start( USE_TAP_INTERVAL );
		}
	}
	else
	{
		me->ReleaseUseButton();
	}

	// Players shoot a lodged ghost so physics moves it: try that once before giving up
	if ( m_dislodgeTimer.HasStarted() )
	{
		if ( m_dislodgeTimer.IsElapsed() )
		{
			m_dislodgeTimer.Invalidate();
			m_captureAttemptTimer.Start( CAPTURE_ATTEMPT_TIME );
			return Continue();
		}

		CBaseCombatWeapon *pSidearm = me->Weapon_GetSlot( 1 );
		if ( pSidearm && me->GetActiveWeapon() != pSidearm )
		{
			me->Weapon_Switch( pSidearm );
		}

		me->GetBodyInterface()->AimHeadTowards( vecGhostCenter, IBody::CRITICAL, 0.2f, nullptr, "Aiming at the lodged ghost" );

		if ( pSidearm && me->GetActiveWeapon() == pSidearm && me->GetBodyInterface()->IsHeadAimingOnTarget()
			&& me->IsLineOfFireClear( vecGhostCenter, CNEOBot::LINE_OF_FIRE_FLAGS_DEFAULT ) )
		{
			me->PressFireButton( BUTTON_TAP_HOLD );
		}

		return Continue();
	}

	if ( m_captureAttemptTimer.IsElapsed() )
	{
		const bool bCanSeeGhost = me->IsLineOfSightClear( m_hObjective, CBaseCombatCharacter::IGNORE_ACTORS );
		if ( !m_bTriedDislodge && bCanSeeGhost )
		{
			m_bTriedDislodge = true;
			m_dislodgeTimer.Start( DISLODGE_TIME );
			return Continue();
		}

		// If the bot fails to capture the ghost, it's sometimes because the ghost is lodged into an awkward position
		// Have the bot search around the location instead, to avoid cycle of failing to pick up ghost and retrying
		return ChangeTo( new CNEOBotCtgLoneWolf(), "Failed to pick up ghost in time, searching around nearest areas" );
	}

	return Continue();
}
