#include "cbase.h"
#include "neo_player.h"
#include "bot/neo_bot.h"
#include "bot/behavior/neo_bot_ctg_carrier.h"
#include "bot/behavior/neo_bot_ctg_lone_wolf.h"
#include "bot/neo_bot_path_compute.h"
#include "neo_gamerules.h"
#include "neo_ghost_cap_point.h"
#include "debugoverlay_shared.h"
#include "weapon_ghost.h"

ConVar neo_debug_ghost_carrier( "neo_debug_ghost_carrier", "0", FCVAR_CHEAT );


//---------------------------------------------------------------------------------------------
static void CollectPlayers( CNEOBot *me, CUtlVector<CNEO_Player*> *pOutTeammates, CUtlVector<CNEO_Player*> *pOutEnemies = nullptr )
{
	for ( int i = 1; i <= gpGlobals->maxClients; i++ )
	{
		CNEO_Player *pPlayer = ToNEOPlayer( UTIL_PlayerByIndex( i ) );
		if ( !pPlayer || !pPlayer->IsAlive() )
		{
			continue;
		}

		if ( pPlayer->GetTeamNumber() == me->GetTeamNumber() )
		{
			if ( pPlayer != me && pOutTeammates ) 
			{
				pOutTeammates->AddToTail( pPlayer );
			}
		}
		else if ( pOutEnemies )
		{
			pOutEnemies->AddToTail( pPlayer );
		}
	}
}


//---------------------------------------------------------------------------------------------
CNEOBotGhostEquipmentHandler::CNEOBotGhostEquipmentHandler()
{
	m_hCurrentFocusEnemy = nullptr;
	m_enemyUpdateTimer.Invalidate();

	for ( int i = 0; i < MAX_PLAYERS_ARRAY_SAFE; ++i )
	{
		m_enemyLastPos[i] = CNEO_Player::VECTOR_INVALID_WAYPOINT;
	}
}

void CNEOBotGhostEquipmentHandler::Update( CNEOBot *me )
{
	if ( !me->IsAlive() )
	{
		return;
	}

	if ( !me->IsCarryingGhost() )
	{
		return;
	}

	if ( !m_enemyUpdateTimer.HasStarted() )
	{
		m_enemyUpdateTimer.Start( GetUpdateInterval( me ) );
	}

	EquipBestWeaponForGhoster( me );

	bool bUpdateCallout = m_enemyUpdateTimer.IsElapsed();
	if ( bUpdateCallout )
	{
		m_enemies.RemoveAll();
		CollectPlayers( me, nullptr, &m_enemies );
		UpdateGhostCarrierCallout( me, m_enemies );
		m_enemyUpdateTimer.Start( GetUpdateInterval( me ) );
	}

	// Debug: Highlight the location of the enemy a bot ghost carrier is calling out
	if ( neo_debug_ghost_carrier.GetBool() )
	{
		CBaseEntity *pFocus = m_hCurrentFocusEnemy.Get();
		if ( pFocus && pFocus->IsPlayer() && pFocus->IsAlive() )
		{
			NDebugOverlay::Cross3D( pFocus->GetAbsOrigin(), 20.0f, 255, 0, 0, true, 0.1f );
		}
	}

	CBaseEntity *pFocus = m_hCurrentFocusEnemy.Get();
	if ( pFocus && pFocus->IsAlive() )
	{
		// Notify teammates to look at the enemy
		for ( int i = 1; i <= gpGlobals->maxClients; i++ )
		{
			CNEO_Player *pTeammate = ToNEOPlayer( UTIL_PlayerByIndex( i ) );
			if ( !pTeammate || !pTeammate->IsAlive() || pTeammate == me || pTeammate->GetTeamNumber() != me->GetTeamNumber() )
			{
				continue;
			}

			CNEOBot *pBot = ToNEOBot( pTeammate );
			if ( pBot )
			{
				if ( pBot->IsLineOfFireClear( pFocus, CNEOBot::LINE_OF_FIRE_FLAGS_DEFAULT ) )
				{
					// NEO Jank: Urge relevant teammate bots look at the enemy
					pBot->GetBodyInterface()->AimHeadTowards( pFocus, IBody::IMPORTANT, 0.5f, nullptr, "Ghost carrier teammate look override" );
				}
				else
				{
					// Force updates to known but not visible entity by forgetting them first
					pBot->GetVisionInterface()->ForgetEntity( pFocus );
				}
				pBot->GetVisionInterface()->AddKnownEntity( pFocus ); // keep after ForgetEntity
			}
		}

		me->GetBodyInterface()->AimHeadTowards( pFocus->WorldSpaceCenter(), IBody::IMPORTANT, 1.0f, nullptr, "Ghost carrier focus look" );
	}
}

void CNEOBotGhostEquipmentHandler::EquipBestWeaponForGhoster( CNEOBot *me )
{
	const CKnownEntity *threat = me->GetVisionInterface()->GetPrimaryKnownThreat();

	CNEOBaseCombatWeapon *pActive =    assert_cast<CNEOBaseCombatWeapon*>( me->GetActiveWeapon() );
	CNEOBaseCombatWeapon *pGhost =     assert_cast<CNEOBaseCombatWeapon*>( me->Weapon_GetSlot( 0 ) );
	CNEOBaseCombatWeapon *pSecondary = assert_cast<CNEOBaseCombatWeapon*>( me->Weapon_GetSlot( 1 ) );

	// Sanity check: if we don't have the ghost, we shouldn't be in this behavior, but handle gracefully
	if ( !pGhost ) 
	{
		// Fallback to normal behavior if ghost is missing
		me->EquipBestWeaponForThreat( threat );
		return;
	}

	// See if we need to defend ourselves
	bool bHasThreat = ( threat && threat->GetEntity() && threat->GetEntity()->IsAlive() );
	if ( bHasThreat )
	{
		bool bCanSeeThreat = me->GetVisionInterface()->IsLineOfSightClear( threat->GetEntity()->WorldSpaceCenter() );
		if ( bCanSeeThreat )
		{
			if ( pSecondary && pSecondary->HasAmmo() )
			{
				// We can still call out enemies the old fashioned way ;)
				me->Weapon_Switch( pSecondary );
				return;
			}
		}
	}

	// Equip ghost to call out enemies behind walls
	if ( pActive != pGhost )
	{
		me->Weapon_Switch( pGhost );
	}
}

float CNEOBotGhostEquipmentHandler::GetUpdateInterval( CNEOBot *me ) const
{
	switch ( me->GetDifficulty() )
	{
	case CNEOBot::EASY:		return 2.5f;
	case CNEOBot::NORMAL:	return 2.0f;
	case CNEOBot::HARD:		return 1.5f;
	case CNEOBot::EXPERT:	return 1.0f;
	}

	return 3.0f;
}

void CNEOBotGhostEquipmentHandler::UpdateGhostCarrierCallout( CNEOBot *me, const CUtlVector<CNEO_Player*> &enemies )
{
	CNEO_Player *pBestCallout = nullptr;
	float flBestCalloutMoved = -1.0f;
	float flBestCalloutDistSq = FLT_MAX;
	bool bBestCalloutIsNew = false;
	
	bool bConsideringOnlyLoSEnemies = false;
	
	const Vector& vecMyPos = me->GetAbsOrigin();

	for ( int i = 0; i < enemies.Count(); ++i )
	{
		CNEO_Player *pPlayer = enemies[i];

		if ( pPlayer->IsEffectActive( EF_NODRAW ) ) 
		{
			continue;
		}

		int idx = pPlayer->entindex();
		if ( !IsIndexIntoPlayerArrayValid( idx ) )
		{
			continue;
		}

		// IsAbleToSee already checks if ghost is booted up to see enemies behind walls
		if ( !me->GetVisionInterface()->IsAbleToSee( pPlayer, IVision::DISREGARD_FOV ) )
		{
			continue;
		}

		float flDistToMeSq = vecMyPos.DistToSqr( pPlayer->GetAbsOrigin() );

		// Also take into account how much the enemy has moved since we last reported them
		Vector vecLast = m_enemyLastPos[ idx ];
		float flMoved = 0.0f;
		bool bIsNew = ( vecLast == CNEO_Player::VECTOR_INVALID_WAYPOINT );

		if ( !bIsNew )
		{
			flMoved = ( pPlayer->GetAbsOrigin() - vecLast ).Length();
		}

		// Any enemies in line of sight have the top priority
		if ( me->GetVisionInterface()->IsLineOfSightClear( pPlayer->WorldSpaceCenter() ) )
		{
			if ( !bConsideringOnlyLoSEnemies )
			{
				bConsideringOnlyLoSEnemies = true;
				pBestCallout = pPlayer;
				bBestCalloutIsNew = bIsNew;
				flBestCalloutMoved = flMoved;
				flBestCalloutDistSq = flDistToMeSq;
			}
			else
			{
				if ( flDistToMeSq < flBestCalloutDistSq )
				{
					pBestCallout = pPlayer;
					bBestCalloutIsNew = bIsNew;
					flBestCalloutMoved = flMoved;
					flBestCalloutDistSq = flDistToMeSq;
				}
			}
			continue;
		}

		if ( bConsideringOnlyLoSEnemies )
		{
			// we don't need to consider other criteria anymore if we are only considering LoS enemies
			continue;
		}

		if ( !pBestCallout )
		{
			pBestCallout = pPlayer;
			bBestCalloutIsNew = bIsNew;
			flBestCalloutMoved = flMoved;
			flBestCalloutDistSq = flDistToMeSq;
			continue;
		}

		// Consider an enemy we haven't called out yet
		if ( bIsNew && !bBestCalloutIsNew )
		{
			pBestCallout = pPlayer;
			bBestCalloutIsNew = true;
			flBestCalloutDistSq = flDistToMeSq;
			flBestCalloutMoved = 0;
			continue;
		}

		if ( !bIsNew && bBestCalloutIsNew )
		{
			// Existing candidate is new, current is old -> ignore current
			continue;
		}

		// Tie breakers between candidates
		if ( bIsNew )
		{
			// Both New: Tie breaker is distance to me (closest first)
			if ( flDistToMeSq < flBestCalloutDistSq )
			{
				pBestCallout = pPlayer;
				flBestCalloutDistSq = flDistToMeSq;
			}
		}
		else
		{
			// Both Old: Prioritize one that has moved more since last callout
			float diff = flMoved - flBestCalloutMoved;
			
			// Check absolute difference to see if it is significant or not
			if ( FloatMakePositive( diff ) < 100.0f )
			{
				// Roughly same movement, tie breaker is closest distance
				if ( flDistToMeSq < flBestCalloutDistSq )
				{
					pBestCallout = pPlayer;
					flBestCalloutMoved = flMoved;
					flBestCalloutDistSq = flDistToMeSq;
				}
			}
			else if ( diff > 0.0f )
			{
				// Distinctly moved more
				pBestCallout = pPlayer;
				flBestCalloutMoved = flMoved;
				flBestCalloutDistSq = flDistToMeSq;
			}
		}
	}

	if ( pBestCallout )
	{
		// NEO Jank: Ideally we could detect the enemy in the middle of the bot's screen, but they tend to look erratically
		// It's easier to just set an enemy handle to appromimate a human focusing on calling out one enemy
		m_hCurrentFocusEnemy = pBestCallout;
		
		// Update cache for this enemy so we know how much they moved next time
		int idx = pBestCallout->entindex();
		if ( IsIndexIntoPlayerArrayValid( idx ) )
		{
			m_enemyLastPos[ idx ] = pBestCallout->GetAbsOrigin();
		}
	}
}



//---------------------------------------------------------------------------------------------
// CNEOBotCtgCarrier Implementation
//---------------------------------------------------------------------------------------------

CNEOBotCtgCarrier::CNEOBotCtgCarrier( void )
{
	m_closestCapturePoint = CNEO_Player::VECTOR_INVALID_WAYPOINT;
}

//---------------------------------------------------------------------------------------------
ActionResult< CNEOBot >	CNEOBotCtgCarrier::OnStart( CNEOBot *me, Action< CNEOBot > *priorAction )
{
	m_chasePath.Invalidate();
	m_aloneTimer.Invalidate();
	m_repathTimer.Invalidate();
	m_path.SetMinLookAheadDistance( me->GetDesiredPathLookAheadRange() );

	m_teammates.RemoveAll();
	CollectPlayers( me, &m_teammates );

	m_closestCapturePoint = GetNearestCapPoint( me );

	UpdateFollowPath( me, m_teammates );
	return Continue();
}


//---------------------------------------------------------------------------------------------
ActionResult< CNEOBot >	CNEOBotCtgCarrier::Update( CNEOBot *me, float interval )
{
	if ( !me->IsAlive() )
	{
		return Done( "I am dead" );
	}

	if ( !me->IsCarryingGhost() )
	{
		return Done( "No longer carrying the ghost" );
	}
	
	m_teammates.RemoveAll();
	CollectPlayers( me, &m_teammates );

	if ( m_teammates.Count() == 0 )
	{
		return SuspendFor( new CNEOBotCtgLoneWolf, "I'm the last one!" );
	}

	UpdateFollowPath( me, m_teammates );
	
	m_ghostEquipmentHandler.Update( me );

	return Continue();
}


//---------------------------------------------------------------------------------------------
Vector CNEOBotCtgCarrier::GetNearestCapPoint( const CNEOBot *me ) const
{
	if ( !me )
		return CNEO_Player::VECTOR_INVALID_WAYPOINT;

	const int iMyTeam = me->GetTeamNumber();

	if ( NEORules()->m_pGhostCaps.Count() > 0 )
	{
		Vector bestPos = CNEO_Player::VECTOR_INVALID_WAYPOINT;
		float flNearestCapDistSq = FLT_MAX;
		for( int i=0; i<NEORules()->m_pGhostCaps.Count(); ++i )
		{
			CNEOGhostCapturePoint *pCapPoint = dynamic_cast<CNEOGhostCapturePoint*>( UTIL_EntityByIndex( NEORules()->m_pGhostCaps[i] ) );
			if (!pCapPoint)
			{
				continue;
			}

			int iCapTeam = pCapPoint->owningTeamAlternate();
			if ( iCapTeam == iMyTeam )
			{
				float distanceToCap = me->GetAbsOrigin().DistToSqr( pCapPoint->GetAbsOrigin() );
				if ( distanceToCap < flNearestCapDistSq )
				{
					flNearestCapDistSq = distanceToCap;
					bestPos = pCapPoint->GetAbsOrigin();
				}
			}
		}
		return bestPos;
	}

	return CNEO_Player::VECTOR_INVALID_WAYPOINT;
}


//---------------------------------------------------------------------------------------------
void CNEOBotCtgCarrier::UpdateFollowPath( CNEOBot *me, const CUtlVector<CNEO_Player*> &teammates )
{
	// Strategy:
	// 1. Identify valid goal (Capture Point).
	// 2. Scan all teammates:
	//    - Track "Nearest Spatial Teammate" (fallback if no LOS).
	//    - Track "Teammate Closest to Goal" (primary target if LOS exists).
	// 3. Decision:
	//    - If I see any teammate -> Chase the one closest to the goal.
	//    - If I see NO teammates -> Chase the one spatially closest to me (regroup).
	//    - If no teammates exist -> Fallback to Goal (though Update() should catch this as a transition to LoneWolf).

	// Identify the goal first
	Vector vecGoalPos = m_closestCapturePoint;
	bool bFoundGoal = ( vecGoalPos != CNEO_Player::VECTOR_INVALID_WAYPOINT );

	if ( vecGoalPos == CNEO_Player::VECTOR_INVALID_WAYPOINT )
	{
		bFoundGoal = false;
	}

	CNEO_Player *pTeammateNearestToMe = nullptr;
	float flTeammateNearestToMeDistSq = FLT_MAX;

	CNEO_Player *pBestGoalTeammate = nullptr;
	float flBestGoalTeammateDistSq = FLT_MAX;

	for ( int i = 0; i < teammates.Count(); i++ )
	{
		CNEO_Player *pPlayer = teammates[i];
			
		// Find the teammate nearest to me
		float distanceToMe = pPlayer->GetAbsOrigin().DistToSqr( me->GetAbsOrigin() );
		if ( distanceToMe < flTeammateNearestToMeDistSq )
		{
			flTeammateNearestToMeDistSq = distanceToMe;
			pTeammateNearestToMe = pPlayer;
		}

		// Find the teammate closest to the goal
		if ( bFoundGoal )
		{
			float distanceToGoal = pPlayer->GetAbsOrigin().DistToSqr( vecGoalPos );
			if ( distanceToGoal < flBestGoalTeammateDistSq )
			{
				flBestGoalTeammateDistSq = distanceToGoal;
				pBestGoalTeammate = pPlayer;
			}
		}
	}

	if ( !bFoundGoal )
	{
		// If we have no goal (defending team on asymmetric map), we should just stay put and call out targets.
		m_chasePath.Invalidate();
		m_path.Invalidate();
		return;
	}

	// If we are safe to cap (closer than enemies), just go for it!
	if ( bFoundGoal )
	{
		// We need to know where enemies are to determine if we are safe to cap
		CWeaponGhost *pGhost = dynamic_cast<CWeaponGhost*>( me->Weapon_GetSlot( 0 ) );
		if ( pGhost && pGhost->IsGhost() && pGhost->IsBootupCompleted() )
		{
			float flDistMeToGoalSq = me->GetAbsOrigin().DistToSqr( vecGoalPos );
			
			bool bAnyEnemyCloser = false;
			
			for ( int i = 1; i <= gpGlobals->maxClients; i++ )
			{
				CNEO_Player *pPlayer = ToNEOPlayer( UTIL_PlayerByIndex( i ) );
				if ( pPlayer && pPlayer->IsAlive() && pPlayer->GetTeamNumber() != me->GetTeamNumber() )
				{
					float dSq = pPlayer->GetAbsOrigin().DistToSqr( vecGoalPos );
					if ( dSq <= flDistMeToGoalSq )
					{
						bAnyEnemyCloser = true;
						break;
					}
				}
			}

			if ( !bAnyEnemyCloser )
			{
				m_chasePath.Invalidate();

				if ( !m_path.IsValid() || !m_repathTimer.HasStarted() || m_repathTimer.IsElapsed() )
				{
					CNEOBotPathCompute( me, m_path, vecGoalPos, FASTEST_ROUTE );
					m_repathTimer.Start( RandomFloat( 0.3f, 0.5f ) );
				}
				
				m_path.Update( me );
				return;
			}
		}
	}

	// Choose which teammate to follow
	CNEO_Player *pTargetTeammate = nullptr;

	// Check if we are relatively protected by teammates before running after the vanguard
	bool bIsNearEscort = false;
	if ( pTeammateNearestToMe )
	{
		if ( me->GetVisionInterface()->IsLineOfSightClear( pTeammateNearestToMe->WorldSpaceCenter() ) )
		{
			bIsNearEscort = true;
		}
	}

	if ( bIsNearEscort )
	{
		m_aloneTimer.Invalidate();
		
		// If we are near a teammate, advance towards goal with vanguard
		if ( pBestGoalTeammate )
		{
			pTargetTeammate = pBestGoalTeammate;
		}
		else
		{
			// Fallback to nearest if no goal teammate
			// (unlikely if we are near a teammate, but in case there's a logic bug)
			pTargetTeammate = pTeammateNearestToMe; 
		}
	}
	else
	{
		// We are isolated from our teammates
		if ( !m_aloneTimer.HasStarted() )
		{
			m_aloneTimer.Start( 3.0f );
		}

		if ( !m_aloneTimer.IsElapsed() )
		{
			// grace period: keep running towards the vanguard in case we can catch up with them
			// helps keep following vanguard in case they round a corner or some other visual obstacle
			if ( pBestGoalTeammate )
			{
				pTargetTeammate = pBestGoalTeammate;
			}
			else
			{
				pTargetTeammate = pTeammateNearestToMe;
			}
		}
		else
		{
			// regroup with nearest friendlies if alone for too long
			pTargetTeammate = pTeammateNearestToMe;
		}
	}

	if ( pTargetTeammate )
	{
		float flDistToTeammateSq = me->GetAbsOrigin().DistToSqr( pTargetTeammate->GetAbsOrigin() );
		if ( flDistToTeammateSq < ( 100.0f * 100.0f ) )
		{
			m_chasePath.Invalidate();
			return;
		}

		m_path.Invalidate();
		
		// chase after the chosen teammate
		CNEOBotPathUpdateChase( me, m_chasePath, pTargetTeammate, SAFEST_ROUTE );
		return;
	}

	// No teammates alive?
	// Update() should handle the LastStand transition next tick, but just in case we fall through here:
	m_chasePath.Invalidate();

	if ( bFoundGoal )
	{
		if ( !m_path.IsValid() || !m_repathTimer.HasStarted() || m_repathTimer.IsElapsed() )
		{
			CNEOBotPathCompute( me, m_path, vecGoalPos, SAFEST_ROUTE );
			m_repathTimer.Start( RandomFloat( 0.5f, 1.0f ) );
		}
		m_path.Update( me );
	}
	else
	{
		// Usually defending the ghost with no capture point
		m_path.Invalidate();
	}
}

//---------------------------------------------------------------------------------------------
ActionResult< CNEOBot > CNEOBotCtgCarrier::OnResume( CNEOBot *me, Action< CNEOBot > *interruptingAction )
{
	m_teammates.RemoveAll();
	CollectPlayers( me, &m_teammates );

	// Re-evaluate nearest cap point on resume (in case we moved significantly while interrupted)
	m_closestCapturePoint = GetNearestCapPoint( me );

	m_repathTimer.Invalidate();
	UpdateFollowPath( me, m_teammates );
	return Continue();
}

//---------------------------------------------------------------------------------------------
EventDesiredResult< CNEOBot > CNEOBotCtgCarrier::OnStuck( CNEOBot *me )
{
	m_teammates.RemoveAll();
	CollectPlayers( me, &m_teammates );
	UpdateFollowPath( me, m_teammates );
	return TryContinue();
}

//---------------------------------------------------------------------------------------------
EventDesiredResult< CNEOBot > CNEOBotCtgCarrier::OnMoveToSuccess( CNEOBot *me, const Path *path )
{
	return TryContinue();
}

//---------------------------------------------------------------------------------------------
EventDesiredResult< CNEOBot > CNEOBotCtgCarrier::OnMoveToFailure( CNEOBot *me, const Path *path, MoveToFailureType reason )
{
	return TryContinue();
}
