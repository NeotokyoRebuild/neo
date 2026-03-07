#include "cbase.h"
#include "bot/behavior/neo_bot_ctg_capture.h"
#include "bot/behavior/neo_bot_ctg_lone_wolf_seek.h"
#include "bot/behavior/neo_bot_seek_weapon.h"
#include "bot/neo_bot_path_compute.h"
#include "neo_detpack.h"
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
			return Done( "Unable to find a path to the ghost capture target" );
		}
		m_repathTimer.Start( RandomFloat( 0.3f, 0.6f ) );
	}
	m_path.Update( me );

	if ( !m_captureAttemptTimer.HasStarted() )
	{
		// If this timer expires, give up
		m_captureAttemptTimer.Start( 3.0f );
	}

	// Check if there is a detpack that risks exploding if I pick up the ghost
	// NEO Jank: It may be more proper to check for line of sight,
	// but triggering an entity search at the last moment may involve fewer overall calculations
	// with the lampshade explanation as the bot being able to notice the detpack along the path
	// even if we didn't check constantly along the same path
	CBaseEntity *pEnts[256];
	int numEnts = UTIL_EntitiesInSphere( pEnts, 256, me->GetAbsOrigin(), NEO_DETPACK_DAMAGE_RADIUS, 0 );
	bool bDetpackNear = false;
	for ( int i = 0; i < numEnts; ++i )
	{
		if ( pEnts[i] && FClassnameIs( pEnts[i], "neo_deployed_detpack" ) )
		{
			bDetpackNear = true;
			break;
		}
	}

	if ( bDetpackNear )
	{
		// NEO JANK: Putting the bot into seek mode will have it search the map for enemies for the rest of the round
		// but for now this could be fine as it may indicate an entrenched enemy
		// or a friendly that is setting up an ambush, where either scenario indicates ghost capture is too dangerous
		return ChangeTo( new CNEOBotCtgLoneWolfSeek(), "Found detpack: skipping ghost capture to search for enemies" );
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
		return ChangeTo( new CNEOBotSeekWeapon, "Failed to capture ghost in time" );
	}

	return Continue();
}
