#include "cbase.h"
#include "neo_player.h"
#include "bot/neo_bot.h"
#include "bot/behavior/neo_bot_ctg_carrier.h"
#include "bot/behavior/neo_bot_ctg_lone_wolf.h"
#include "bot/neo_bot_ctg_carry_block.h"
#include "bot/neo_bot_ghost_callout.h"
#include "bot/neo_bot_path_reservation.h"
#include "bot/neo_bot_path_compute.h"
#include "nav_mesh.h"
#include "nav_pathfind.h"
#include "neo_gamerules.h"
#include "debugoverlay_shared.h"
#include "weapon_ghost.h"

// A carrier with no capture zone counts as arrived at its hold area this close to the area's center
static constexpr float CARRIER_HOLD_ARRIVE_RANGE = 48.0f;

// The carrier looks for a less exposed area among the areas its own area can see within this distance,
static constexpr float CARRIER_CLIMB_STEP_DIST = 600.0f;
// at most this many steps
static constexpr int CARRIER_CLIMB_MAX_STEPS = 8;
// or this many while a blocked route holds the carrier, which stays near its way to the zone for the break
static constexpr int CARRIER_BLOCKED_CLIMB_MAX_STEPS = 2;
// and only where a path is at most this much longer than the straight line, so a step does not lead around the map
static constexpr float CARRIER_CLIMB_MAX_DETOUR = 2.0f;
// and tests the walk to at most this many of the best candidates a step
static constexpr int CARRIER_CLIMB_MAX_PATH_TESTS = 8;

// A carrier holds only an area at least a player wide each way:
// nav_generate leaves narrow strips along walls, where a bot often cannot reach the center
static constexpr float HOLD_AREA_MIN_WIDTH = 2.0f * HalfHumanWidth;

static bool IsWideEnoughToHold( const CNavArea *area )
{
	return area->GetSizeX() >= HOLD_AREA_MIN_WIDTH && area->GetSizeY() >= HOLD_AREA_MIN_WIDTH;
}


//-----------------------------------------------------------------------------------------------------
// A step of the carrier's climb: an area the current area can see, its exposure and its distance to the enemy zones
struct ClimbCandidate_t
{
	CNavArea *area;
	int nVisible;
	float flZoneDist;
};

static int CompareClimbCandidates( const ClimbCandidate_t *a, const ClimbCandidate_t *b )
{
	// least exposed first, then farther from the enemy zones, then lower ID so the choice is the same every round
	if ( a->nVisible != b->nVisible )
	{
		return a->nVisible < b->nVisible ? -1 : 1;
	}

	if ( a->flZoneDist != b->flZoneDist )
	{
		return a->flZoneDist > b->flZoneDist ? -1 : 1;
	}

	return a->area->GetID() < b->area->GetID() ? -1 : 1;
}

//-----------------------------------------------------------------------------------------------------
// Collects the areas the current area can see within a step that are less exposed and no closer to the enemy zones
class CCollectLessExposedAreas
{
public:
	CCollectLessExposedAreas( CNEOBot *me, CNavArea *pCurrentArea, CUtlVector< ClimbCandidate_t > *pCandidates )
		: m_me( me ), m_pCurrent( pCurrentArea ), m_pCandidates( pCandidates )
	{
		m_iEnemyTeam = GetEnemyTeam( me->GetTeamNumber() );
		m_flCurrentZoneDist = GetZoneDist( pCurrentArea );
	}

	bool operator() ( CNavArea *area )
	{
		// strictly less exposed, so the climb cannot go back and forth between equal areas
		const int nVisible = area->GetPotentiallyVisibleAreaCount();
		if ( nVisible == 0 || nVisible >= m_pCurrent->GetPotentiallyVisibleAreaCount() )
		{
			return true;
		}

		if ( area->GetCenter().DistToSqr( m_pCurrent->GetCenter() ) > Square( CARRIER_CLIMB_STEP_DIST ) )
		{
			return true;
		}

		// the carrier stands upright, where it can move freely
		if ( !IsWideEnoughToHold( area ) || ( area->GetAttributes() & ( NAV_MESH_CROUCH | NAV_MESH_JUMP | NAV_MESH_AVOID ) ) )
		{
			return true;
		}

		const AreaPenalty_t penalty = CNEOBotPathReservations()->GetAreaPenalty( area->GetID() );
		if ( penalty.IsStuckProne() || CNEOBotPathReservations()->IsAreaHazardous( area->GetID(), m_me ) )
		{
			return true;
		}

		// carry the ghost away from the enemy zones, never toward them
		const float flZoneDist = GetZoneDist( area );
		if ( flZoneDist < m_flCurrentZoneDist )
		{
			return true;
		}

		m_pCandidates->AddToTail( { area, nVisible, flZoneDist } );
		return true;
	}

private:
	float GetZoneDist( const CNavArea *area ) const
	{
		const Vector vecZone = NEORules()->GetNearestGhostCapPoint( m_iEnemyTeam, area->GetCenter() );
		if ( vecZone == CNEO_Player::VECTOR_INVALID_WAYPOINT )
		{
			return FLT_MAX;
		}

		return vecZone.DistTo( area->GetCenter() );
	}

	CNEOBot *m_me;
	CNavArea *m_pCurrent;
	CUtlVector< ClimbCandidate_t > *m_pCandidates;
	int m_iEnemyTeam;
	float m_flCurrentZoneDist;
};


//-----------------------------------------------------------------------------------------------------
// Can a bot walk from one area to the other without a long detour
static bool IsShortWalk( CNavArea *pFrom, CNavArea *pTo )
{
	const float flMaxLength = CARRIER_CLIMB_MAX_DETOUR * pFrom->GetCenter().DistTo( pTo->GetCenter() );
	ShortestPathCost costFunc;
	return NavAreaBuildPath( pFrom, pTo, nullptr, costFunc, nullptr, flMaxLength );
}


//-----------------------------------------------------------------------------------------------------
static CNavArea *FindLeastExposedArea( CNEOBot *me, CNavArea *pStartArea, int nMaxSteps )
{
	// Without nav_analyze visibility data there is nothing to judge by, so stay put
	if ( !pStartArea || pStartArea->GetPotentiallyVisibleAreaCount() == 0 )
	{
		return pStartArea;
	}

	CNavArea *pCurrent = pStartArea;
	for ( int iStep = 0; iStep < nMaxSteps; iStep++ )
	{
		CUtlVector< ClimbCandidate_t > candidates;
		CCollectLessExposedAreas collect( me, pCurrent, &candidates );
		pCurrent->ForAllPotentiallyVisibleAreas( collect );
		candidates.Sort( CompareClimbCandidates );

		// take the least exposed candidate the carrier can walk to without a long detour
		CNavArea *pNext = nullptr;
		for ( int i = 0; i < candidates.Count() && i < CARRIER_CLIMB_MAX_PATH_TESTS; i++ )
		{
			if ( IsShortWalk( pCurrent, candidates[i].area ) )
			{
				pNext = candidates[i].area;
				break;
			}
		}

		// a local minimum: no visible area within a step is less exposed and reachable
		if ( !pNext )
		{
			break;
		}

		pCurrent = pNext;
	}

	return pCurrent;
}

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

	// Pick a focus enemy on the timer, or at once when the ghost stops showing the current one
	CBaseEntity *pFocus = m_hCurrentFocusEnemy.Get();
	const bool bFocusLost = pFocus && !IsValidFocusEnemy( me, pFocus );
	if ( m_enemyUpdateTimer.IsElapsed() || bFocusLost )
	{
		m_enemies.RemoveAll();
		CollectPlayers( me, nullptr, &m_enemies );
		UpdateGhostCarrierCallout( me, m_enemies );
		m_enemyUpdateTimer.Start( GetUpdateInterval( me ) );
	}

	// Check if currently focused enemy has disappeared from beacon range
	pFocus = m_hCurrentFocusEnemy.Get();
	if ( !IsValidFocusEnemy( me, pFocus ) )
	{
		m_hCurrentFocusEnemy = nullptr;
		pFocus = nullptr;
	}

	// Debug: Highlight the location of the enemy a bot ghost carrier is calling out
	if ( neo_debug_ghost_carrier.GetBool() && pFocus && pFocus->IsPlayer() )
	{
		NDebugOverlay::Cross3D( pFocus->GetAbsOrigin(), 20.0f, 255, 0, 0, true, 0.1f );
	}

	if ( pFocus )
	{
		// NEOBotGhostCallout tells teammates about the enemy the carrier looks at
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

bool CNEOBotGhostEquipmentHandler::IsValidFocusEnemy( CNEOBot *me, CBaseEntity *pFocus ) const
{
	if ( !pFocus || !pFocus->IsAlive() || pFocus->IsEffectActive( EF_NODRAW ) )
	{
		return false;
	}

	if ( !me->IsEnemy( pFocus ) )
	{
		return false;
	}

	return me->IsRevealedByMyGhost( pFocus );
}

float CNEOBotGhostEquipmentHandler::GetUpdateInterval( CNEOBot *me ) const
{
	float flInterval = 3.0f;
	switch ( me->GetDifficulty() )
	{
	case CNEOBot::EASY:		flInterval = 2.5f; break;
	case CNEOBot::NORMAL:	flInterval = 2.0f; break;
	case CNEOBot::HARD:		flInterval = 1.5f; break;
	case CNEOBot::EXPERT:	flInterval = 1.0f; break;
	}

	// Look at each focus enemy long enough for at least one callout to read the carrier's aim
	return Max( flInterval, NEOBotGhostCallout::GetMinAimHoldTime() );
}

void CNEOBotGhostEquipmentHandler::UpdateGhostCarrierCallout( CNEOBot *me, const CUtlVector<CNEO_Player*> &enemies )
{
	CNEO_Player *pBestCallout = nullptr;
	float flBestCalloutMoved = -1.0f;
	float flBestCalloutDistSq = FLT_MAX;
	bool bBestCalloutIsNew = false;

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

		// Only the enemies the ghost shows, as a human carrier calls out the beacons on their screen
		if ( !me->IsRevealedByMyGhost( pPlayer ) )
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
		// The carrier looks at its focus enemy, and NEOBotGhostCallout reads that aim as the callout
		m_hCurrentFocusEnemy = pBestCallout;
		
		// Update cache for this enemy so we know how much they moved next time
		int idx = pBestCallout->entindex();
		if ( IsIndexIntoPlayerArrayValid( idx ) )
		{
			m_enemyLastPos[ idx ] = pBestCallout->GetAbsOrigin();
		}
	}
	else
	{
		// Nobody is in beacon range
		m_hCurrentFocusEnemy = nullptr;
	}
}



//---------------------------------------------------------------------------------------------
// CNEOBotCtgCarrier Implementation
//---------------------------------------------------------------------------------------------

CNEOBotCtgCarrier::CNEOBotCtgCarrier( void )
{
	m_closestCapturePoint = CNEO_Player::VECTOR_INVALID_WAYPOINT;
	m_holdArea = nullptr;
	m_bHoldAreaChosen = false;
	m_holdState = HOLD_STATE_UNKNOWN;
}

//---------------------------------------------------------------------------------------------
ActionResult< CNEOBot >	CNEOBotCtgCarrier::OnStart( CNEOBot *me, Action< CNEOBot > *priorAction )
{
	m_chasePath.Invalidate();
	m_aloneTimer.Invalidate();
	m_path.SetMinLookAheadDistance( me->GetDesiredPathLookAheadRange() );

	m_teammates.RemoveAll();
	CollectPlayers( me, &m_teammates );

	m_closestCapturePoint = NEORules()->GetNearestGhostCapPoint( me->GetTeamNumber(), me->GetAbsOrigin() );

	m_holdState = HOLD_STATE_UNKNOWN;
	UpdateHoldState( me );

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
	
	if ( NEORules()->IsRoundOver() )
	{
		return Done( "Round Over: no CTG objectives to reach" );
	}

	m_teammates.RemoveAll();
	CollectPlayers( me, &m_teammates );

	// Check if bot should transition into lone wolf behavior,
	// except for an ATK attacker, which still has to take the ghost
	if ( m_teammates.Count() == 0
		&& !( IsAtkAttackingTeam( me->GetTeamNumber() ) && HasGhostCapZone( me->GetTeamNumber() ) ) )
	{
		const CKnownEntity *threat = me->GetVisionInterface()->GetPrimaryKnownThreat( true );
		if ( threat && threat->GetEntity() && threat->GetEntity()->IsAlive() )
		{
			CNavArea *destArea = TheNavMesh->GetNearestNavArea( m_closestCapturePoint );
			CNavArea *myArea = me->GetLastKnownArea();

			if ( !destArea || !myArea || !destArea->IsPotentiallyVisible( myArea ) )
			{
				return SuspendFor( new CNEOBotCtgLoneWolf, "Last one standing and blocked from capturing!" );
			}
		}

		// Lone wolf will drop the ghost and go into enemy seeking behavior
		if ( !HasGhostCapZone( me->GetTeamNumber() )
			|| m_closestCapturePoint == CNEO_Player::VECTOR_INVALID_WAYPOINT )
		{
			return SuspendFor( new CNEOBotCtgLoneWolf, "Looking for enemy since there is no capture point" );
		}
	}

	NEOBotCarryBlock::UpdateCarryBlock( me, IsCapPotentiallyVisible( me ) || HasCleanRunToCap( me ), m_closestCapturePoint );
	UpdateHoldState( me );

	UpdateFollowPath( me, m_teammates );
	
	m_ghostEquipmentHandler.Update( me );

	return Continue();
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
	const bool bHasCapZone = HasGhostCapZone( me->GetTeamNumber() );
	const bool bFoundGoal = bHasCapZone && vecGoalPos != CNEO_Player::VECTOR_INVALID_WAYPOINT;

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

	// A blocked route: hold near here and let the teammates hunt, until the verdict clears
	if ( bFoundGoal && ( m_holdState == HOLD_STATE_HOLD ) )
	{
		m_chasePath.Invalidate();
		UpdateHoldPath( me );
		return;
	}

	if ( !bFoundGoal )
	{
		// Without a capture zone, carry the ghost to cover and call out targets.
		m_chasePath.Invalidate();
		if ( bHasCapZone )
		{
			m_path.Invalidate();
			return;
		}

		UpdateHoldPath( me );
		return;
	}

	// If we are safe to cap (closer than enemies), just go for it!
	if ( bFoundGoal )
	{
		// We need to know where enemies are to determine if we are safe to cap
		if ( me->GetBeaconingGhost() )
		{
			float flDistMeToGoalSq = me->GetAbsOrigin().DistToSqr( vecGoalPos );
			
			bool bAnyEnemyCloser = false;
			
			for ( int i = 1; i <= gpGlobals->maxClients; i++ )
			{
				CNEO_Player *pPlayer = ToNEOPlayer( UTIL_PlayerByIndex( i ) );
				if ( pPlayer && pPlayer->IsAlive() && pPlayer->GetTeamNumber() != me->GetTeamNumber()
					&& me->GetVisionInterface()->IsAbleToSee( pPlayer, IVision::DISREGARD_FOV ) )
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

				if ( !m_path.IsValid() )
				{
					CNEOBotPathCompute( me, m_path, vecGoalPos, FASTEST_ROUTE );
				}
                
				m_path.Update( me );
				return;
			}
		}
	}

	// In ATK the clock runs for the defenders, so head for the zone and let the escorts keep up
	if ( IsAtkAttackingTeam( me->GetTeamNumber() ) && HasGhostCapZone( me->GetTeamNumber() ) )
	{
		m_chasePath.Invalidate();

		if ( !m_path.IsValid() )
		{
			CNEOBotPathCompute( me, m_path, vecGoalPos, SAFEST_ROUTE );
		}

		m_path.Update( me );
		return;
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
		if ( !m_path.IsValid() )
		{
			CNEOBotPathCompute( me, m_path, vecGoalPos, SAFEST_ROUTE );
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
bool CNEOBotCtgCarrier::HasCleanRunToCap( const CNEOBot *me ) const
{
	if ( !me->IsCarryingGhost() )
	{
		return false;
	}

	if ( !HasGhostCapZone( me->GetTeamNumber() )
		|| m_closestCapturePoint == CNEO_Player::VECTOR_INVALID_WAYPOINT )
	{
		return false;
	}

	const float flMyDistToGoalSq = me->GetAbsOrigin().DistToSqr( m_closestCapturePoint );

	CUtlVector< CKnownEntity > knownVector;
	me->GetVisionInterface()->CollectKnownEntities( &knownVector );

	for ( int i = 0; i < knownVector.Count(); ++i )
	{
		const CKnownEntity &known = knownVector[i];
		CBaseEntity *pKnown = known.GetEntity();
		if ( !pKnown || !pKnown->IsPlayer() )
		{
			continue;
		}

		if ( pKnown->GetTeamNumber() == me->GetTeamNumber() )
		{
			continue;
		}

		if ( NEOBotCarryBlock::IsNamedDead( ToBasePlayer( pKnown ) ) )
		{
			continue;
		}

		if ( known.GetLastKnownPosition().DistToSqr( m_closestCapturePoint ) <= flMyDistToGoalSq )
		{
			// A known enemy is at least as close to the cap as we are.
			return false;
		}
	}

	return true;
}

//---------------------------------------------------------------------------------------------
bool CNEOBotCtgCarrier::IsCapPotentiallyVisible( const CNEOBot *me ) const
{
	if ( !me->IsCarryingGhost() )
	{
		return false;
	}

	if ( !HasGhostCapZone( me->GetTeamNumber() )
		|| m_closestCapturePoint == CNEO_Player::VECTOR_INVALID_WAYPOINT )
	{
		return false;
	}

	const CNavArea *capArea = TheNavMesh->GetNearestNavArea( m_closestCapturePoint );
	const CNavArea *myArea = me->GetLastKnownArea();
	if ( !capArea || !myArea )
	{
		return false;
	}

	return capArea->IsPotentiallyVisible( myArea );
}

//---------------------------------------------------------------------------------------------
QueryResultType CNEOBotCtgCarrier::ShouldHurry( const INextBot *me ) const
{
	const CNEOBot *meBot = static_cast<const CNEOBot *>( me );

	// Within sight of the cap zone, run for it regardless of who is closing in
	if ( IsCapPotentiallyVisible( meBot ) || HasCleanRunToCap( meBot ) )
	{
		return ANSWER_YES;
	}

	return ANSWER_UNDEFINED;
}

//---------------------------------------------------------------------------------------------
ActionResult< CNEOBot > CNEOBotCtgCarrier::OnResume( CNEOBot *me, Action< CNEOBot > *interruptingAction )
{
	m_teammates.RemoveAll();
	CollectPlayers( me, &m_teammates );

	// Re-evaluate nearest cap point on resume (in case we moved significantly while interrupted)
	m_closestCapturePoint = NEORules()->GetNearestGhostCapPoint( me->GetTeamNumber(), me->GetAbsOrigin() );

	UpdateHoldState( me );
	UpdateFollowPath( me, m_teammates );
	return Continue();
}

//---------------------------------------------------------------------------------------------
// Follows the team's hold-or-break verdict, the same one the teammates read
void CNEOBotCtgCarrier::UpdateHoldState( CNEOBot *me )
{
	const HoldState_t state = NEOBotCarryBlock::IsTeamHoldingGhost( me->GetTeamNumber() ) ? HOLD_STATE_HOLD : HOLD_STATE_BREAK;
	if ( state == m_holdState )
	{
		return;
	}

	m_holdState = state;

	// A hold spot or a run planned for the other state is stale
	m_path.Invalidate();
	m_chasePath.Invalidate();
	m_holdArea = nullptr;
	m_bHoldAreaChosen = false;

	// NEO-DEF-TEMP
	Msg( "NEO_DEF mode team=%d carrier=%d state=%s\n", me->GetTeamNumber(), me->entindex(),
		( state == HOLD_STATE_HOLD ) ? "hold" : "break" );
}

//---------------------------------------------------------------------------------------------
// A holding carrier walks quietly to cover and holds there
void CNEOBotCtgCarrier::UpdateHoldPath( CNEOBot *me )
{
	if ( !m_bHoldAreaChosen )
	{
		m_bHoldAreaChosen = true;

		const int nMaxSteps = HasGhostCapZone( me->GetTeamNumber() ) ? CARRIER_BLOCKED_CLIMB_MAX_STEPS : CARRIER_CLIMB_MAX_STEPS;
		m_holdArea = FindLeastExposedArea( me, me->GetLastKnownArea(), nMaxSteps );
	}

	const bool bArrived = !m_holdArea || me->GetLastKnownArea() == m_holdArea
		|| me->GetAbsOrigin().AsVector2D().DistToSqr( m_holdArea->GetCenter().AsVector2D() ) < Square( CARRIER_HOLD_ARRIVE_RANGE );
	if ( bArrived )
	{
		m_path.Invalidate();
		return;
	}

	if ( !m_path.IsValid() )
	{
		CNEOBotPathCompute( me, m_path, m_holdArea->GetCenter(), SAFEST_ROUTE );
	}

	// Walk quietly, so enemies cannot hear where the ghost goes
	me->ReleaseRunButton();
	me->PressSneakButton( 0.3f );
	m_path.Update( me );
}

//---------------------------------------------------------------------------------------------
EventDesiredResult< CNEOBot > CNEOBotCtgCarrier::OnStuck( CNEOBot *me )
{
	m_path.Invalidate();
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
	m_path.Invalidate();

	// Hold where I am when the way to the hold area fails
	if ( m_holdArea )
	{
		m_holdArea = nullptr;
	}

	return TryContinue();
}
