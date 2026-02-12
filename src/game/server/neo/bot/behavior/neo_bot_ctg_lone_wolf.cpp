#include "cbase.h"
#include "neo_player.h"
#include "bot/neo_bot.h"
#include "bot/behavior/neo_bot_attack.h"
#include "bot/behavior/neo_bot_ctg_capture.h"
#include "bot/behavior/neo_bot_ctg_lone_wolf.h"
#include "bot/behavior/neo_bot_seek_weapon.h"
#include "bot/behavior/neo_bot_retreat_to_cover.h"
#include "bot/neo_bot_path_compute.h"
#include "neo_gamerules.h"
#include "neo_ghost_cap_point.h"
#include "weapon_ghost.h"


//---------------------------------------------------------------------------------------------
CNEOBotCtgLoneWolf::CNEOBotCtgLoneWolf( void )
{
	m_hGhost = nullptr;
	m_bPursuingDropThreat = false;
	m_bHasRetreatedFromGhost = false;
	m_vecDropThreatPos = CNEO_Player::VECTOR_INVALID_WAYPOINT;
	m_closestCapturePoint = CNEO_Player::VECTOR_INVALID_WAYPOINT;
}

//---------------------------------------------------------------------------------------------
ActionResult< CNEOBot >	CNEOBotCtgLoneWolf::OnStart( CNEOBot *me, Action< CNEOBot > *priorAction )
{
	m_hGhost = nullptr;
	m_bHasRetreatedFromGhost = false;
	m_bPursuingDropThreat = false;
	m_useAttemptTimer.Invalidate();
	m_lookAroundTimer.Invalidate();
	m_repathTimer.Invalidate();
	m_stalemateTimer.Invalidate();
	m_capPointUpdateTimer.Invalidate();
	m_vecDropThreatPos = CNEO_Player::VECTOR_INVALID_WAYPOINT;
	m_closestCapturePoint = CNEO_Player::VECTOR_INVALID_WAYPOINT;
	m_hPursueTarget = nullptr;

	return Continue();
}


//---------------------------------------------------------------------------------------------
ActionResult< CNEOBot >	CNEOBotCtgLoneWolf::Update( CNEOBot *me, float interval )
{
	const CKnownEntity *threat = me->GetVisionInterface()->GetPrimaryKnownThreat( true );

	CBaseCombatWeapon *pWeapon = me->GetActiveWeapon();
	if ( !threat && pWeapon && pWeapon->UsesClipsForAmmo1() )
	{
		if ( pWeapon->Clip1() < pWeapon->GetMaxClip1() && me->GetAmmoCount( pWeapon->GetPrimaryAmmoType() ) > 0 )
		{
			// Aggressively reload due to lack of backup
			me->PressReloadButton();
		}
	}

	// We dropped the ghost to hunt a threat.
	if ( m_bPursuingDropThreat )
	{
		// First, ensure we have a weapon.
		if ( !me->Weapon_GetSlot( 0 ) )
		{
			return SuspendFor( new CNEOBotSeekWeapon(), "Scavenging for weapon to hunt threat" );
		}

		// We have a weapon. Investigate the last known location.
		float flDistSq = me->GetAbsOrigin().DistToSqr( m_vecDropThreatPos );
		if ( flDistSq < Square( 100.0f ) || m_vecDropThreatPos == CNEO_Player::VECTOR_INVALID_WAYPOINT )
		{
			// We arrived at threat's last known position, but didn't find them.
			m_bPursuingDropThreat = false;
		}
		else
		{
			// Move to investigate
			if ( threat && threat->GetEntity() && me->GetVisionInterface()->IsAbleToSee( threat->GetEntity(), CNEOBotVision::DISREGARD_FOV, nullptr ) )
			{
				return SuspendFor( new CNEOBotAttack, "Found the threat I was hunting!" );
			}

			CNEOBotPathCompute( me, m_path, m_vecDropThreatPos, FASTEST_ROUTE );
			m_path.Update( me );
			return Continue();
		}
	}

	// Always need to find the ghost to act on it
	if (!m_hGhost)
	{
		m_hGhost = dynamic_cast<CWeaponGhost*>( gEntList.FindEntityByClassname(nullptr, "weapon_ghost") );
	}

	if (!m_hGhost)
	{
		return Done( "Ghost not found" );
	}

	// Occasionally reconsider which cap zone is our goal
	if ( !m_capPointUpdateTimer.HasStarted() || m_capPointUpdateTimer.IsElapsed() )
	{
		m_closestCapturePoint = CNEO_Player::VECTOR_INVALID_WAYPOINT;
		float flNearestCapDistSq = FLT_MAX;
		
		if ( NEORules()->m_pGhostCaps.Count() > 0 )
		{
			const Vector& vecStart = me->IsCarryingGhost() ? me->GetAbsOrigin() : m_hGhost->GetAbsOrigin();
			
			for( int i=0; i<NEORules()->m_pGhostCaps.Count(); ++i )
			{
				CNEOGhostCapturePoint *pCapPoint = dynamic_cast<CNEOGhostCapturePoint*>( UTIL_EntityByIndex( NEORules()->m_pGhostCaps[i] ) );
				if ( !pCapPoint ) continue;

				if ( pCapPoint->owningTeamAlternate() == me->GetTeamNumber() )
				{
					float distSq = vecStart.DistToSqr( pCapPoint->GetAbsOrigin() );
					if ( distSq < flNearestCapDistSq )
					{
						flNearestCapDistSq = distSq;
						m_closestCapturePoint = pCapPoint->GetAbsOrigin();
					}
				}
			}
		}
		m_capPointUpdateTimer.Start( RandomFloat( 0.5f, 1.0f ) );
	}

	float flDistGhostToGoal = FLT_MAX;
	if ( m_closestCapturePoint != CNEO_Player::VECTOR_INVALID_WAYPOINT )
	{
		const Vector& vecStart = me->IsCarryingGhost() ? me->GetAbsOrigin() : m_hGhost->GetAbsOrigin();
		flDistGhostToGoal = vecStart.DistTo( m_closestCapturePoint );
	}

	// Safe to cap: We are closer to the goal than the nearest enemy is to the goal.
	// NEO Jank Cheat: We're intentionally cheating here compared to the neo_bot_ctg_carrier behavior by not checking if the ghost is booted.
	// The reason is that we want to avoid spectators getting frustrated with bots choosing to ambush at the ghost instead of capping it,
	// when it's apparent that the enemy is too far behind to catch up (and ambushing would give them the opportunity to do so).
	// Our bots so far have poor intuition about where unseen enemies could come from,
	// so it's easier to cheat with distance checks than to anticipate where enemies are.
	float flMyTotalDist = flDistGhostToGoal; 
	if ( !me->IsCarryingGhost() )
	{
		flMyTotalDist += me->GetAbsOrigin().DistTo( m_hGhost->GetAbsOrigin() );
	}

	// Count enemies and find if one is closer to our goal
	int iEnemyTeamCount = 0;
	float flClosestEnemyDistToGoalSq = FLT_MAX;
	float flMyTotalDistSq = ( flMyTotalDist >= FLT_MAX ) ? FLT_MAX : ( flMyTotalDist * flMyTotalDist );
	
	for ( int i = 1; i <= gpGlobals->maxClients; i++ )
	{
		CNEO_Player *pPlayer = ToNEOPlayer( UTIL_PlayerByIndex( i ) );
		if ( pPlayer && pPlayer->IsAlive() && pPlayer->GetTeamNumber() != me->GetTeamNumber() )
		{
			iEnemyTeamCount++;
			if ( m_closestCapturePoint != CNEO_Player::VECTOR_INVALID_WAYPOINT )
			{
				float distSq = pPlayer->GetAbsOrigin().DistToSqr( m_closestCapturePoint );
				if ( distSq < flClosestEnemyDistToGoalSq )
				{
					flClosestEnemyDistToGoalSq = distSq;
					if ( iEnemyTeamCount > 1 && flClosestEnemyDistToGoalSq < flMyTotalDistSq )
					{
						// We already know it's not a 1v1 (count > 1)
						// And we know it's not safe to cap (enemy closer than us)
						// So we can stop checking.
						break;
					}
				}
			}
		}
	}
	
	// Tie breaker: If it's a 1v1, it's boring for human observers to wait forever
	// Just try to grab the ghost, even if it might not be the best tactic
	bool bIs1v1 = (iEnemyTeamCount == 1);
	
	bool bSafeToCap = ((m_closestCapturePoint != CNEO_Player::VECTOR_INVALID_WAYPOINT) && (flMyTotalDistSq < flClosestEnemyDistToGoalSq));
	
	CWeaponGhost *pGhostWeapon = m_hGhost.Get();
	CBaseCombatCharacter *pGhostOwner = pGhostWeapon ? pGhostWeapon->GetOwner() : nullptr;
	bool bGhostHeldByEnemy = (pGhostOwner && pGhostOwner->GetTeamNumber() != me->GetTeamNumber());

	// Consider next action
	if ( me->IsCarryingGhost() )
	{
		if ( bSafeToCap )
		{
			if ( m_closestCapturePoint != CNEO_Player::VECTOR_INVALID_WAYPOINT )
			{
				if ( !m_repathTimer.HasStarted() || m_repathTimer.IsElapsed() )
				{
					CNEOBotPathCompute( me, m_path, m_closestCapturePoint, SAFEST_ROUTE );
					m_path.Update( me );
					m_repathTimer.Start( RandomFloat( 0.3f, 0.5f ) );
				}
				else
				{
					m_path.Update( me );
				}
			}
			return Continue();
		}
		else
		{
			// Enemy is closer to goal (blocking us) or gaining on us.

			// If we see a weapon nearby, drop the ghost and take it
			CBaseEntity *pNearbyWeapon = FindNearestPrimaryWeapon( me->GetAbsOrigin(), true );
			if ( pNearbyWeapon )
			{
				CBaseCombatWeapon *pGhostWep = me->Weapon_GetSlot( 0 );
				if ( pGhostWep )
				{
					if ( me->GetActiveWeapon() != pGhostWep )
					{
						me->Weapon_Switch( pGhostWep );
						return Continue();
					}

					me->PressDropButton( 0.1f );
					return ChangeTo( new CNEOBotSeekWeapon(), "Dropping ghost to scavenge nearby weapon" );
				}
			}

			CBaseCombatWeapon *pActiveWeapon = me->GetActiveWeapon();
			CBaseCombatWeapon *pGhostWep = me->Weapon_GetSlot( 0 );
			
			// If we know where the threat is, drop and hunt.
			if ( threat && threat->GetLastKnownPosition() != CNEO_Player::VECTOR_INVALID_WAYPOINT )
			{
				m_vecDropThreatPos = threat->GetLastKnownPosition();
				m_bPursuingDropThreat = true;
				m_hPursueTarget = threat->GetEntity();

				if ( pGhostWep )
				{
					if ( pActiveWeapon != pGhostWep )
					{
						me->Weapon_Switch( pGhostWep );
					}
					else
					{
						me->EnableCloak( 3.0f );
						me->PressDropButton( 0.1f );
					}
				}
				return Continue();
			}
			
			// Else continue moving ghost towards goal
			if ( m_closestCapturePoint != CNEO_Player::VECTOR_INVALID_WAYPOINT )
			{
				if ( !m_repathTimer.HasStarted() || m_repathTimer.IsElapsed() )
				{
					CNEOBotPathCompute( me, m_path, m_closestCapturePoint, SAFEST_ROUTE );
					m_path.Update( me );
					m_repathTimer.Start( RandomFloat( 0.3f, 0.5f ) );
				}
				else
				{
					m_path.Update( me );
				}
			}
			return Continue();
		}
	}
	else if ( bGhostHeldByEnemy )
	{
		// intercept enemy carrier
		if ( threat && threat->GetEntity() == pGhostOwner && me->IsLineOfFireClear( threat->GetEntity()->WorldSpaceCenter(), CNEOBot::LINE_OF_FIRE_FLAGS_DEFAULT ))
		{
			me->EnableCloak( 3.0f );
			return SuspendFor(new CNEOBotAttack, "Attacking the ghost carrier!");
		}

		if ( !m_repathTimer.HasStarted() || m_repathTimer.IsElapsed() )
		{
			CNEOBotPathCompute(me, m_path, m_hGhost->GetAbsOrigin(), FASTEST_ROUTE);
			m_path.Update(me);
			m_repathTimer.Start( RandomFloat( 0.3f, 0.5f ) );
		}
		else
		{
			m_path.Update(me);
		}
		return Continue();
	}
	else
	{
		// Ghost is free for taking
		if ( bSafeToCap || (bIs1v1 && m_stalemateTimer.HasStarted() && m_stalemateTimer.IsElapsed()) )
		{
			// Try to cap before enemy can stop us.
			float flDistToGhostSq = me->GetAbsOrigin().DistToSqr(m_hGhost->GetAbsOrigin());
			if ( flDistToGhostSq < 100.0f * 100.0f )
			{
				return SuspendFor(new CNEOBotCtgCapture(m_hGhost.Get()), "Picking up ghost to make a run for it!");
			}
			
			if ( !m_repathTimer.HasStarted() || m_repathTimer.IsElapsed() )
			{
				CNEOBotPathCompute(me, m_path, m_hGhost->GetAbsOrigin(), FASTEST_ROUTE);
				m_path.Update(me);
				m_repathTimer.Start( RandomFloat( 0.2f, 0.5f ) );
			}
			else
			{
				m_path.Update(me);
			}
			return Continue();
		}
		else
		{
			// Not safe. Enemy is closer to goal or blocking.
			// Try to ambush them
			
			if ( bIs1v1 && !m_stalemateTimer.HasStarted() )
			{
				m_stalemateTimer.Start( RandomFloat( 10.0f, 20.0f ) );
			}

			if ( m_bHasRetreatedFromGhost )
			{
				// Waiting in ambush/cover
				if (threat && me->IsLineOfFireClear( threat->GetEntity()->WorldSpaceCenter(), CNEOBot::LINE_OF_FIRE_FLAGS_DEFAULT ))
				{
					me->EnableCloak( 3.0f );
					return SuspendFor(new CNEOBotAttack, "Ambushing enemy near ghost!");
				}
				return UpdateLookAround( me, m_hGhost->GetAbsOrigin() ); 
			}
			else
			{
				// Hide out of sight of ghost to ambush anyone that picks up the ghost
				float flDistToGhostSq = me->GetAbsOrigin().DistToSqr(m_hGhost->GetAbsOrigin());
				if (flDistToGhostSq < 300.0f * 300.0f)
				{
					m_bHasRetreatedFromGhost = true;
					return SuspendFor(new CNEOBotRetreatToCover(), "Finding a hiding spot near the ghost");
				}
				else
				{
					// Get near the ghost first before surveying hiding spots
					if ( !m_repathTimer.HasStarted() || m_repathTimer.IsElapsed() )
					{
						CNEOBotPathCompute(me, m_path, m_hGhost->GetAbsOrigin(), FASTEST_ROUTE);
						m_path.Update(me);
						m_repathTimer.Start( RandomFloat( 0.5f, 1.0f ) );
					}
					else
					{
						m_path.Update(me);
					}
					return Continue();
				}
			}
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
	if ( m_bPursuingDropThreat && m_hPursueTarget.Get() )
	{
		if ( !m_hPursueTarget->IsAlive() )
		{
			// Target dead, stop pursuit
			m_bPursuingDropThreat = false;
			m_hPursueTarget = nullptr;
		}
		else
		{
			// Remember where we last saw the threat
			const CKnownEntity *known = me->GetVisionInterface()->GetKnown( m_hPursueTarget );
			if ( known )
			{
				m_vecDropThreatPos = known->GetLastKnownPosition();
			}
		}
	}

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
EventDesiredResult< CNEOBot > CNEOBotCtgLoneWolf::OnMoveToSuccess( CNEOBot *me, const Path *path )
{
	return TryContinue();
}

//---------------------------------------------------------------------------------------------
EventDesiredResult< CNEOBot > CNEOBotCtgLoneWolf::OnMoveToFailure( CNEOBot *me, const Path *path, MoveToFailureType reason )
{
	m_path.Invalidate();
	return TryContinue();
}


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
ActionResult< CNEOBot > CNEOBotCtgLoneWolf::UpdateLookAround( CNEOBot *me, const Vector &anchorPos )
{
	if ( !m_lookAroundTimer.HasStarted() || m_lookAroundTimer.IsElapsed() )
	{
		// NEO Jank Cheat: Bots don't have a good intuition for where to look for threats
		// So the compromise is to have them retreat from a threat when the latter shows up
		// The looking around logic below is performative for spectators to justify why a bot might incidentally turn to see threat
		for ( int i = 1; i <= gpGlobals->maxClients; i++ )
		{
			CNEO_Player *pPlayer = ToNEOPlayer( UTIL_PlayerByIndex( i ) );
			if ( pPlayer && pPlayer->IsAlive() && pPlayer->GetTeamNumber() != me->GetTeamNumber() )
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
						
						const float maxLookInterval = 2.0f;
						m_lookAroundTimer.Start(RandomFloat(0.5f, maxLookInterval));
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
