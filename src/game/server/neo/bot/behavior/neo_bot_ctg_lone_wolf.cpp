#include "cbase.h"
#include "neo_player.h"
#include "bot/neo_bot.h"
#include "bot/behavior/neo_bot_attack.h"
#include "bot/behavior/neo_bot_ctg_lone_wolf.h"
#include "bot/behavior/neo_bot_ctg_lone_wolf_ambush.h"
#include "bot/behavior/neo_bot_ctg_lone_wolf_seek.h"
#include "bot/behavior/neo_bot_detpack_deploy.h"
#include "bot/neo_bot_path_compute.h"
#include "neo_gamerules.h"
#include "neo_ghost_cap_point.h"
#include "weapon_detpack.h"
#include "weapon_ghost.h"


//---------------------------------------------------------------------------------------------
ActionResult< CNEOBot >	CNEOBotCtgLoneWolf::OnStart( CNEOBot *me, Action< CNEOBot > *priorAction )
{
	m_repathTimer.Invalidate();
	return Continue();
}


//---------------------------------------------------------------------------------------------
ActionResult< CNEOBot >	CNEOBotCtgLoneWolf::Update( CNEOBot *me, float interval )
{
	if ( me->DropGhost() )
	{
		return Continue(); // ghost drop in progress
	}

	ActionResult< CNEOBot > interceptionResult = ConsiderGhostInterception( me );
	if ( interceptionResult.IsRequestingChange() )
	{
		return interceptionResult;
	}

	const CKnownEntity *threat = me->GetVisionInterface()->GetPrimaryKnownThreat( true );
	if ( threat && threat->GetEntity() )
	{
		return ChangeTo( new CNEOBotAttack(), "Engaging enemy" );
	}

	if ( !threat && me->GetActiveWeapon() )
	{
		// Aggressively reload due to lack of backup
		me->ReloadIfLowClip(true); // force reload true
	}

	CWeaponDetpack *const pDetpackWeapon = assert_cast<CWeaponDetpack*>( me->Weapon_OwnsThisType( "weapon_remotedet" ) );

	if ( pDetpackWeapon && pDetpackWeapon->m_bThisDetpackHasBeenThrown && !pDetpackWeapon->m_bRemoteHasBeenTriggered )
	{
		return ChangeTo( new CNEOBotCtgLoneWolfAmbush(), "Detpack deployed, transitioning to ambush" );
	}

	CNavArea *const ghostArea = TheNavMesh->GetNearestNavArea( NEORules()->GetGhostPos() );
	CNavArea *const myArea = me->GetLastKnownArea();
	if ( ghostArea && myArea && ghostArea->IsPotentiallyVisible( myArea ) )
	{
		if ( pDetpackWeapon && !pDetpackWeapon->m_bThisDetpackHasBeenThrown && NEORules()->m_pGhost )
		{
			return ChangeTo( new CNEOBotDetpackDeploy( NEORules()->GetGhostPos(), new CNEOBotCtgLoneWolfAmbush() ), "Moving to plant detpack" );
		}
		return ChangeTo( new CNEOBotCtgLoneWolfAmbush(), "Waiting in ambush near ghost" );
	}

	return ConsiderGhostVisualCheck( me );
}


//---------------------------------------------------------------------------------------------
ActionResult< CNEOBot > CNEOBotCtgLoneWolf::ConsiderGhostInterception( CNEOBot *me, const CBaseCombatCharacter *pGhostOwner )
{
	if ( !pGhostOwner )
	{
		CWeaponGhost *pGhostWeapon = NEORules()->m_pGhost;
		pGhostOwner = pGhostWeapon ? pGhostWeapon->GetOwner() : nullptr;
	}

	bool bGhostHeldByEnemy = ( pGhostOwner && !me->InSameTeam( pGhostOwner ) );
	if ( !bGhostHeldByEnemy )
	{
		return Continue();
	}

	// intercept enemy carrier
	const CKnownEntity *threat = me->GetVisionInterface()->GetPrimaryKnownThreat( true );
	if ( threat && threat->GetEntity() == pGhostOwner && me->IsLineOfFireClear( threat->GetEntity()->WorldSpaceCenter(), CNEOBot::LINE_OF_FIRE_FLAGS_DEFAULT ) )
	{
		me->EnableCloak( 3.0f );
		return SuspendFor( new CNEOBotAttack, "Attacking the ghost carrier!" );
	}

	Vector vecInterceptGoal = NEORules()->GetGhostPos();
	if ( vecInterceptGoal != CNEO_Player::VECTOR_INVALID_WAYPOINT )
	{
		if ( !m_repathTimer.HasStarted() || m_repathTimer.IsElapsed() )
		{
			CNEOBotPathCompute( me, m_path, vecInterceptGoal, FASTEST_ROUTE );
			m_path.Update( me );
			m_repathTimer.Start( RandomFloat( 0.3f, 1.0f ) );
		}
		else
		{
			m_path.Update( me );
		}
	}
	return Continue();
}


//---------------------------------------------------------------------------------------------
ActionResult< CNEOBot > CNEOBotCtgLoneWolf::ConsiderGhostVisualCheck( CNEOBot *me )
{
	if ( me->IsCarryingGhost() )
	{
		return Continue();
	}

	CWeaponGhost *pGhostWeapon = NEORules()->m_pGhost;
	CBaseCombatCharacter *pGhostOwner = pGhostWeapon ? pGhostWeapon->GetOwner() : nullptr;
	bool bGhostHeldByEnemy = ( pGhostOwner && !me->InSameTeam( pGhostOwner ) );

	if ( bGhostHeldByEnemy )
	{
		return Continue();
	}

	// Move to ghost's location to gain visual contact
	Vector vecAcquireGoal = NEORules()->GetGhostPos();
	if ( vecAcquireGoal != CNEO_Player::VECTOR_INVALID_WAYPOINT )
	{
		if ( !m_repathTimer.HasStarted() || m_repathTimer.IsElapsed() )
		{
			CNEOBotPathCompute( me, m_path, vecAcquireGoal, FASTEST_ROUTE );
			m_path.Update( me );
			m_repathTimer.Start( RandomFloat( 0.3f, 1.0f ) );
		}
		else
		{
			m_path.Update( me );
		}
	}

	return Continue();
}


//---------------------------------------------------------------------------------------------
ActionResult< CNEOBot > CNEOBotCtgLoneWolf::OnSuspend( CNEOBot *me, Action< CNEOBot > *interruptingAction )
{
	m_path.Invalidate();
	return Continue();
}

//---------------------------------------------------------------------------------------------
ActionResult< CNEOBot > CNEOBotCtgLoneWolf::OnResume( CNEOBot *me, Action< CNEOBot > *interruptingAction )
{
	return Continue();
}

//---------------------------------------------------------------------------------------------
EventDesiredResult< CNEOBot > CNEOBotCtgLoneWolf::OnStuck( CNEOBot *me )
{
	m_path.Invalidate();
	me->GetLocomotionInterface()->Jump();
	return TryContinue();
}


//---------------------------------------------------------------------------------------------
Vector CNEOBotCtgLoneWolf::GetNearestEnemyCapPoint( CNEOBot *me )
{
	if ( !me )
		return CNEO_Player::VECTOR_INVALID_WAYPOINT;

	const int iEnemyTeam = NEORules()->GetOpposingTeam( me->GetTeamNumber() );

	if ( NEORules()->m_pGhostCaps.Count() > 0 )
	{
		Vector bestPos = CNEO_Player::VECTOR_INVALID_WAYPOINT;
		float flNearestSq = FLT_MAX;
		for ( int i = 0; i < NEORules()->m_pGhostCaps.Count(); ++i )
		{
			CNEOGhostCapturePoint *pCapPoint = assert_cast<CNEOGhostCapturePoint*>( UTIL_EntityByIndex( NEORules()->m_pGhostCaps[i] ) );
			if ( !pCapPoint )
			{
				continue;
			}

			int iCapTeam = pCapPoint->owningTeamAlternate();
			if ( iCapTeam == iEnemyTeam || iCapTeam == TEAM_ANY )
			{
				float distSq = me->GetAbsOrigin().DistToSqr( pCapPoint->GetAbsOrigin() );
				if ( distSq < flNearestSq )
				{
					flNearestSq = distSq;
					bestPos = pCapPoint->GetAbsOrigin();
				}
			}
		}
		return bestPos;
	}

	return CNEO_Player::VECTOR_INVALID_WAYPOINT;
}

