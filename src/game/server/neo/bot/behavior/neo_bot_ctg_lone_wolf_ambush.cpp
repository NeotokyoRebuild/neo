#include "cbase.h"
#include "bot/neo_bot.h"
#include "bot/behavior/neo_bot_attack.h"
#include "bot/behavior/neo_bot_ctg_lone_wolf_ambush.h"
#include "bot/behavior/neo_bot_ctg_lone_wolf_seek.h"
#include "bot/behavior/neo_bot_retreat_to_cover.h"
#include "bot/neo_bot_path_compute.h"
#include "nav_mesh.h"
#include "neo_detpack.h"
#include "neo_gamerules.h"
#include "neo_player.h"
#include "weapon_detpack.h"
#include "weapon_ghost.h"

//---------------------------------------------------------------------------------------------
ActionResult< CNEOBot >	CNEOBotCtgLoneWolfAmbush::OnStart( CNEOBot *me, Action< CNEOBot > *priorAction )
{
	BaseClass::OnStart( me, priorAction );
	m_lookAroundTimer.Invalidate();
	m_vecAmbushGoal = GetNearestEnemyCapPoint( me );

	m_bIs1v1 = false;
	m_1v1Timer.Invalidate();
	m_1v1TransitionTimer.Start( RandomFloat( 5.0f, 30.0f ) );

	return Continue();
}

//---------------------------------------------------------------------------------------------
ActionResult< CNEOBot >	CNEOBotCtgLoneWolfAmbush::Update( CNEOBot *me, float interval )
{
	CWeaponGhost *pGhost = NEORules()->m_pGhost;
	if ( !pGhost )
	{
		return Done( "Ghost not found" );
	}

	if ( me->DropGhost() )
	{
		return Continue(); // ghost drop in progress
	}

	const CBaseCombatCharacter *const pGhostOwner = pGhost->GetOwner();
	ActionResult< CNEOBot > ghostAction = ConsiderGhostInterception( me, pGhostOwner );
	if ( !ghostAction.IsContinue() )
	{
		return ghostAction;
	}

	const CKnownEntity *threat = me->GetVisionInterface()->GetPrimaryKnownThreat(true);
	if ( threat && threat->GetEntity() )
	{
		return ChangeTo( new CNEOBotAttack(), "Engaging enemy from ambush" );
	}

	if ( !threat && me->GetActiveWeapon() )
	{
		// Aggressively reload due to lack of backup
		me->ReloadIfLowClip(true); // force reload true
	}

	if ( pGhostOwner && !me->InSameTeam( pGhostOwner ) )
	{
		if ( me->IsDebugging( NEXTBOT_BEHAVIOR ) )
		{
			DevMsg( "Lone Wolf Ambush: Intercepting enemy ghost carrier\n" );
		}
		// Don't interrupt enemy chasing with ambush pathing
		return Continue();
	}

	if ( m_1v1TransitionTimer.IsElapsed() && Is1v1( me ) )
	{
		return ChangeTo( new CNEOBotCtgLoneWolfSeek(), "Searching for other lone wolf" );
	}

	// Wait far enough from the ghost and out of sight, but not too far away that it's hard to intercept
	const float flDistToGoalSq = ( m_vecAmbushGoal != CNEO_Player::VECTOR_INVALID_WAYPOINT ) ? me->GetAbsOrigin().DistToSqr( m_vecAmbushGoal ) : FLT_MAX;
	const Vector vecGhostPos = NEORules()->GetGhostPos();
	const float flDistToGhostSq = me->GetAbsOrigin().DistToSqr( vecGhostPos );

	bool bShouldHoldPosition = ( flDistToGoalSq < Square( 200.0f ) );
	if ( !bShouldHoldPosition )
	{
		const float flMinSafeDistSq = Square( NEO_DETPACK_DAMAGE_RADIUS * 2.0f );
		const float flMaxLurkDistSq = Square( NEO_DETPACK_DAMAGE_RADIUS * 3.0f );

		if ( flDistToGhostSq > flMinSafeDistSq && flDistToGhostSq < flMaxLurkDistSq )
		{
			CNavArea *ghostArea = TheNavMesh->GetNearestNavArea( vecGhostPos );
			CNavArea *myArea = me->GetLastKnownArea();
			bShouldHoldPosition = ( !ghostArea || !myArea || !myArea->IsPotentiallyVisible( ghostArea ) );
		}
	}

	// Wait here in ambush by invalidating path to nearest enemy cap zone
	if ( bShouldHoldPosition )
	{
		if ( m_path.IsValid() && me->IsDebugging( NEXTBOT_BEHAVIOR ) )
		{
			DevMsg( "Lone Wolf Ambush: Holding position at %f %f %f\n", m_vecAmbushGoal.x, m_vecAmbushGoal.y, m_vecAmbushGoal.z );
		}
		m_path.Invalidate();
		me->GetLocomotionInterface()->Stop();
		me->PressCrouchButton( 0.3f );
		return UpdateLookAround( me );
	}

	if ( m_vecAmbushGoal == CNEO_Player::VECTOR_INVALID_WAYPOINT )
	{
		return Done( "No ambush spot found" );
	}

	if ( !m_repathTimer.HasStarted() || m_repathTimer.IsElapsed() || !m_path.IsValid() )
	{
		CNEOBotPathCompute( me, m_path, m_vecAmbushGoal, SAFEST_ROUTE );
		m_path.Update( me );
		m_repathTimer.Start( RandomFloat( 0.5f, 1.5f ) );
	}
	else
	{
		m_path.Update( me );
	}

	return Continue();
}

//---------------------------------------------------------------------------------------------
ActionResult<CNEOBot> CNEOBotCtgLoneWolfAmbush::OnSuspend( CNEOBot *me, Action<CNEOBot > *interruptingAction )
{
	m_path.Invalidate();
	BaseClass::OnSuspend( me, interruptingAction );
	return Continue();
}

//---------------------------------------------------------------------------------------------
ActionResult<CNEOBot> CNEOBotCtgLoneWolfAmbush::OnResume( CNEOBot *me, Action<CNEOBot > *interruptingAction )
{
	BaseClass::OnResume( me, interruptingAction );
	return Continue();
}

//---------------------------------------------------------------------------------------------
EventDesiredResult< CNEOBot > CNEOBotCtgLoneWolfAmbush::OnStuck( CNEOBot *me )
{
	m_path.Invalidate();
	return TryContinue();
}

//---------------------------------------------------------------------------------------------
EventDesiredResult< CNEOBot > CNEOBotCtgLoneWolfAmbush::OnMoveToSuccess( CNEOBot *me, const Path *path )
{
	return TryContinue();
}

//---------------------------------------------------------------------------------------------
EventDesiredResult< CNEOBot > CNEOBotCtgLoneWolfAmbush::OnMoveToFailure( CNEOBot *me, const Path *path, MoveToFailureType reason )
{
	m_path.Invalidate();
	return TryContinue();
}

//---------------------------------------------------------------------------------------------
// Helper for "UpdateLookAround" - inspired from how CNavArea CollectPotentiallyVisibleAreas works
class CCollectPotentiallyVisibleAreas
{
public:
	CCollectPotentiallyVisibleAreas( CUtlVector< CNavArea * > *collection )
	{
		m_collection = collection;
	}

	bool operator() ( CNavArea *baseArea )
	{
		m_collection->AddToTail( baseArea );
		return true;
	}

	CUtlVector< CNavArea * > *m_collection;
};

//---------------------------------------------------------------------------------------------
ActionResult< CNEOBot > CNEOBotCtgLoneWolfAmbush::UpdateLookAround( CNEOBot *me )
{
	if ( !m_lookAroundTimer.HasStarted() || m_lookAroundTimer.IsElapsed() )
	{
		// NEO Jank Cheat: Bots don't have a good intuition for where to look for threats
		// So the compromise is to have them retreat from a threat when the latter shows up
		// The looking around logic below is performative for spectators to justify why a bot might incidentally turn to see threat
		for ( int i = 1; i <= gpGlobals->maxClients; i++ )
		{
			CNEO_Player *pPlayer = ToNEOPlayer( UTIL_PlayerByIndex( i ) );
			if ( pPlayer && pPlayer->IsAlive() && !me->InSameTeam( pPlayer ) )
			{
				if ( me->IsLineOfFireClear( pPlayer->WorldSpaceCenter(), CNEOBot::LINE_OF_FIRE_FLAGS_DEFAULT ) )
				{
					me->GetVisionInterface()->AddKnownEntity( pPlayer );
					me->GetBodyInterface()->AimHeadTowards( pPlayer->WorldSpaceCenter(), IBody::CRITICAL, 0.2f, nullptr, "Ambush Cheat: Reacting to enemy in LOF" );
					return SuspendFor( new CNEOBotRetreatToCover(), "Ambush Prep: Retreating from sensed enemy" );
				}
			}
		}

		m_lookAroundTimer.Start( 0.2f );

		// Logic inspired from neo_bot.cpp UpdateLookingAroundForIncomingPlayers
		// Update our view to watch where enemies might be coming from
		CNavArea *myArea = me->GetLastKnownArea();
		if ( myArea )
		{
			m_visibleAreas.RemoveAll();
			CCollectPotentiallyVisibleAreas collect( &m_visibleAreas );
			myArea->ForAllPotentiallyVisibleAreas( collect );

			if ( m_visibleAreas.Count() > 0 )
			{
				// Pick a random area
				int which = RandomInt( 0, m_visibleAreas.Count()-1 );
				CNavArea *area = m_visibleAreas[ which ];

				// Look at a spot in it
				int retryCount = 5;
				for( int i=0; i<retryCount; ++i )
				{
					Vector spot = area->GetRandomPoint() + Vector( 0, 0, HumanEyeHeight * 0.75f );
					
					// Ensure we can see it
					if ( me->GetVisionInterface()->IsLineOfSightClear( spot ) )
					{
						me->GetBodyInterface()->AimHeadTowards( spot, IBody::IMPORTANT, 1.0f, nullptr, "Ambush: Scanning area" );
						m_lookAroundTimer.Start(RandomFloat(0.5f, 2.0f));
						return Continue();
					}
				}
			}
		}

		// Fallback scanning delay if we failed to find a spot
		m_lookAroundTimer.Start(RandomFloat(0.3f, 1.0f));
	}

	return Continue();
}

//---------------------------------------------------------------------------------------------
bool CNEOBotCtgLoneWolfAmbush::Is1v1( CNEOBot *me )
{
	if ( m_bIs1v1 )
	{
		return true;
	}

	// NEO JANK: Assume I have no teammates given that
	// I entered this function because my teammates are dead
	if ( !m_1v1Timer.HasStarted() || m_1v1Timer.IsElapsed() )
	{
		int iAliveEnemyCount = 0;
		for ( int i = 1; i <= gpGlobals->maxClients; i++ )
		{
			CNEO_Player *pPlayer = ToNEOPlayer( UTIL_PlayerByIndex( i ) );
			if ( pPlayer && pPlayer->IsAlive() && !me->InSameTeam( pPlayer ) )
			{
				if ( ++iAliveEnemyCount > 1 )
				{
					break;
				}
			}
		}
		m_bIs1v1 = ( iAliveEnemyCount == 1 );
		m_1v1Timer.Start( 2.0f );
	}

	return m_bIs1v1;
}

