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
// How long to shoot a lodged ghost, including the 0.5-2 s it takes to draw and aim the sidearm
constexpr float CTG_CAPTURE_DISLODGE_TIME = 3.0f;
// Missed shots fly past the ghost, so keep teammates clear of the line this far beyond it
constexpr float CTG_CAPTURE_DISLODGE_OVERSHOOT = 256.0f;

bool IsGhostInSight( CNEOBot *me, const Vector &vecEye, CWeaponGhost *pGhost, const Vector &vecGhostCenter )
{
	trace_t trace;
	NextBotTraceFilterIgnoreActors filter( me, COLLISION_GROUP_NONE );
	UTIL_TraceLine( vecEye, vecGhostCenter, MASK_PLAYERSOLID, &filter, &trace );
	return trace.m_pEnt == pGhost || trace.fraction == 1.0f;
}

// The sidearm while its clip is loaded, then the primary if the bot has not dropped it yet
CNEOBaseCombatWeapon *GetLoadedDislodgeWeapon( CNEOBot *me )
{
	for ( const int iSlot : { 1, 0 } )
	{
		auto *pWeapon = static_cast<CNEOBaseCombatWeapon *>( me->Weapon_GetSlot( iSlot ) );
		if ( pWeapon && pWeapon->Clip1() > 0 )
		{
			return pWeapon;
		}
	}

	return nullptr;
}

bool IsDislodgeShotSafe( CNEOBot *me, const Vector &vecEye, CWeaponGhost *pGhost, const Vector &vecGhostCenter )
{
	if ( !me->IsLineOfFireClear( vecEye, pGhost, CNEOBot::LINE_OF_FIRE_FLAGS_DEFAULT ) )
	{
		return false;
	}

	Vector vecToGhostDir = vecGhostCenter - vecEye;
	vecToGhostDir.NormalizeInPlace();

	const Vector vecPastGhost = vecGhostCenter + vecToGhostDir * CTG_CAPTURE_DISLODGE_OVERSHOOT;
	return !me->IsFriendlyNearLineOfFire( vecEye, vecPastGhost );
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

	// The ghost aims below would hold the head away from a visible threat, so fighting comes first
	const bool bThreatInView = me->GetVisionInterface()->GetPrimaryKnownThreat( true ) != nullptr;
	const Vector vecEye = me->EyePosition();
	const Vector vecGhostCenter = m_hObjective->WorldSpaceCenter();

	if ( m_dislodgeTimer.HasStarted() )
	{
		UpdateDislodge( me, vecEye, vecGhostCenter, bThreatInView );
		return Continue();
	}

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
	
	// A ghost that cannot be walked onto can still be picked up the way players do it:
	// look at it and press use
	if ( !bThreatInView && vecEye.DistToSqr( vecGhostCenter ) < Square( PLAYER_USE_RADIUS ) )
	{
		TryUseGhost( me, vecEye, vecGhostCenter );
	}

	if ( m_captureAttemptTimer.IsElapsed() )
	{
		// Players shoot a lodged ghost so physics moves it: give that one go before giving up
		if ( !m_bTriedDislodge && GetLoadedDislodgeWeapon( me ) && IsGhostInSight( me, vecEye, m_hObjective, vecGhostCenter ) )
		{
			m_bTriedDislodge = true;
			m_dislodgeTimer.Start( CTG_CAPTURE_DISLODGE_TIME );
			return Continue();
		}

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


//---------------------------------------------------------------------------------------------
// The ghost is heavy, so keep shooting it until the window closes or the loaded clips run dry
void CNEOBotCtgCapture::UpdateDislodge( CNEOBot *me, const Vector &vecEye, const Vector &vecGhostCenter, bool bThreatInView )
{
	CNEOBaseCombatWeapon *pGun = GetLoadedDislodgeWeapon( me );
	if ( m_dislodgeTimer.IsElapsed() || !pGun )
	{
		m_dislodgeTimer.Invalidate();
		me->ReleaseFireButton();
		m_captureAttemptTimer.Start( CTG_CAPTURE_ATTEMPT_TIME );
		return;
	}

	if ( bThreatInView )
	{
		return;
	}

	if ( me->GetActiveWeapon() != pGun && !me->Weapon_Switch( pGun ) )
	{
		me->ReleaseFireButton();
		return;
	}

	me->GetBodyInterface()->AimHeadTowards( vecGhostCenter, IBody::CRITICAL, 0.2f, nullptr, "Aiming at the lodged ghost" );

	if ( !me->GetBodyInterface()->IsHeadAimingOnTarget() || !IsDislodgeShotSafe( me, vecEye, m_hObjective, vecGhostCenter ) )
	{
		me->ReleaseFireButton();
		return;
	}

	if ( me->IsContinuousFireWeapon( pGun ) )
	{
		me->PressFireButton( CTG_CAPTURE_BUTTON_TAP_HOLD );
	}
	else if ( me->m_nButtons & IN_ATTACK )
	{
		me->ReleaseFireButton(); // semi-auto needs the trigger released between shots
	}
	else
	{
		me->PressFireButton();
	}
}
