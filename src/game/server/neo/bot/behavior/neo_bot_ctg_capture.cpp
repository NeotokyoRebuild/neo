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
	m_captureAttemptTimer.Invalidate();
	m_repathTimer.Invalidate();
	m_path.Invalidate();
	
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

	if ( !m_repathTimer.HasStarted() || m_repathTimer.IsElapsed() )
	{
		if ( !CNEOBotPathCompute( me, m_path, m_hObjective->GetAbsOrigin(), FASTEST_ROUTE ) )
		{
			// Something has gone wrong if we can't path to a ghost that we by convention started close to
			// Transition to search around the ghost behavior to avoid cycle of ghost search failure and retry attempts
			return ChangeTo( new CNEOBotCtgLoneWolf(), "Unable to find a path to the ghost capture target, searching around nearest areas" );
		}
		m_repathTimer.Start( RandomFloat( 0.3f, 0.6f ) );
	}
	m_path.Update( me );

	if ( !m_captureAttemptTimer.HasStarted() )
	{
		// If this timer expires, give up
		m_captureAttemptTimer.Start( 3.0f );
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
	
	if ( m_captureAttemptTimer.IsElapsed() )
	{
		// If the bot fails to capture the ghost, it's sometimes because the ghost is lodged into an awkward position
		// Have the bot search around the location instead, to avoid cycle of failing to pick up ghost and retrying
		return ChangeTo( new CNEOBotCtgLoneWolf(), "Failed to pick up ghost in time, searching around nearest areas" );
	}

	return Continue();
}
