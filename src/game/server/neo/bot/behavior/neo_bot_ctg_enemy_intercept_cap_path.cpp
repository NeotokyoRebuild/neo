#include "cbase.h"
#include "neo_player.h"
#include "neo_gamerules.h"
#include "neo_ghost_cap_point.h"
#include "bot/neo_bot.h"
#include "bot/behavior/neo_bot_ctg_enemy_intercept_cap_path.h"
#include "bot/neo_bot_path_compute.h"
#include "nav_pathfind.h"

// NavAreaBuildPath only maintains CNavArea::GetPathLengthSoFar() when it is called with a positive
// maxPathLength -- SetPathLengthSoFar is gated on bHaveMaxPathLength in nav_pathfind.h. The
// interception pathfinds below deliberately pass no length cap, so that field holds stale data from
// some earlier unrelated search. Measure a finished route ourselves, the same way NavAreaBuildPath's
// own internal PathLength() helper does: sum segment lengths along the parent chain from the search's
// goal area back to its start area.
static float RouteLengthAlongParents( CNavArea *pEndArea )
{
	float flLength = 0.0f;
	for ( CNavArea *pArea = pEndArea; pArea && pArea->GetParent(); pArea = pArea->GetParent() )
	{
		flLength += ( pArea->GetCenter() - pArea->GetParent()->GetCenter() ).Length();
	}
	return flLength;
}

//---------------------------------------------------------------------------------------------
CNEOBotCtgEnemyInterceptCapPath::CNEOBotCtgEnemyInterceptCapPath( void )
	: m_targetCapArea( nullptr ),
	  m_targetCapPos( vec3_origin ),
	  m_bHoldingPosition( false )
{
}

//---------------------------------------------------------------------------------------------
bool CNEOBotCtgEnemyInterceptCapPath::RecomputeTargetCapPoint( CNEOBot *me )
{
	m_targetCapPos = vec3_origin;
	m_targetCapArea = nullptr;
	m_predictedCarrierRoute.RemoveAll();

	if ( !NEORules()->GhostExists() )
	{
		return false;
	}

	const int iGhosterPlayer = NEORules()->GetGhosterPlayer();
	if ( iGhosterPlayer <= 0 || iGhosterPlayer > gpGlobals->maxClients )
	{
		return false;
	}

	CNEO_Player *pGhostCarrier = ToNEOPlayer( UTIL_PlayerByIndex( iGhosterPlayer ) );
	if ( !pGhostCarrier || !pGhostCarrier->IsAlive() || pGhostCarrier->GetTeamNumber() == me->GetTeamNumber() )
	{
		return false;
	}

	float flMinDistSq = FLT_MAX;
	CNEOGhostCapturePoint *pBestCap = nullptr;

	for ( int i = 0; i < NEORules()->m_pGhostCaps.Count(); ++i )
	{
		CNEOGhostCapturePoint *pCapPoint = dynamic_cast< CNEOGhostCapturePoint* >( UTIL_EntityByIndex( NEORules()->m_pGhostCaps[i] ) );
		// Skip a disabled cap zone -- it cannot be captured this round, so it is not a real
		// interception target. Same guard as neo_bot_seek_and_destroy.cpp's cap scan.
		if ( !pCapPoint || !pCapPoint->GetActive() )
		{
			continue;
		}

		if ( pCapPoint->owningTeamAlternate() != me->GetTeamNumber() )
		{
			const float distSq = pGhostCarrier->GetAbsOrigin().DistToSqr( pCapPoint->GetAbsOrigin() );
			if ( distSq < flMinDistSq )
			{
				flMinDistSq = distSq;
				pBestCap = pCapPoint;
			}
		}
	}

	if ( !pBestCap )
	{
		return false;
	}

	const Vector vecCapPos = pBestCap->GetAbsOrigin();
	CNavArea *pCapArea = TheNavMesh->GetNearestNavArea( vecCapPos );
	if ( !pCapArea )
	{
		return false;
	}

	CNavArea *pCarrierArea = pGhostCarrier->GetLastKnownArea();
	if ( !pCarrierArea )
	{
		return false;
	}

	// Default fallback to the capture zone position and area
	m_targetCapPos = vecCapPos;
	m_targetCapArea = pCapArea;

	// Predict the enemy ghost carrier's fastest path to their target cap zone. (pCarrierArea is
	// non-null here -- checked above.)
	if ( pCarrierArea != pCapArea )
	{
		ShortestPathCost cost;
		if ( NavAreaBuildPath( pCarrierArea, pCapArea, &vecCapPos, cost ) )
		{
			// The predicted route is the parent chain from pCapArea (goal) back to pCarrierArea
			// (start). Measure it directly -- GetPathLengthSoFar() is not maintained for this call
			// (see RouteLengthAlongParents above).
			const float flTotalRouteLen = RouteLengthAlongParents( pCapArea );
			const Vector vecBotPos = me->GetAbsOrigin();

			CUtlVector< CNavArea * > candidateAreas;	// route nodes worth trying as a cut-off
			CUtlVector< float > candidateCarrierDist;	// matching carrier->node distance along the route
			CUtlVector< float > candidateBotDistSq;	// matching straight-line bot->node distance^2 (reach-check order)
			CNavArea *pNearestToBotArea = nullptr;		// straight-line fallback when no cut-off is beatable
			float flMinDistSqToBot = FLT_MAX;

			// Walk cap -> carrier, accumulating distance-from-cap so each node knows both how far it
			// is from the cap and (total - that) how far the carrier still is from it.
			float flNodeToCap = 0.0f;
			CNavArea *pPrevArea = nullptr;
			for ( CNavArea *pArea = pCapArea; pArea; pArea = pArea->GetParent() )
			{
				if ( pPrevArea )
				{
					flNodeToCap += ( pArea->GetCenter() - pPrevArea->GetCenter() ).Length();
				}
				pPrevArea = pArea;

				// Cache the route so Update can cheaply tell -- by scanning these area pointers, no
				// pathfind -- whether the carrier has since diverged onto an alternative path.
				m_predictedCarrierRoute.AddToTail( pArea );

				const float distSqToBot = vecBotPos.DistToSqr( pArea->GetCenter() );
				if ( distSqToBot < flMinDistSqToBot )
				{
					flMinDistSqToBot = distSqToBot;
					pNearestToBotArea = pArea;
				}

				const float flCarrierToNode = flTotalRouteLen - flNodeToCap;
				// (a) well ahead of the carrier, and (b) not already in the goal-defence stretch
				// just before the cap -- that last bit is CNEOBotCtgEnemy's job, not a cut-off.
				if ( flCarrierToNode < 200.0f || flNodeToCap < 250.0f )
				{
					continue;
				}

				candidateAreas.AddToTail( pArea );
				candidateCarrierDist.AddToTail( flCarrierToNode );
				candidateBotDistSq.AddToTail( distSqToBot );
			}

			// Pick a cut-off the bot can hold, not just any point ahead of the carrier. Check the
			// candidates NEAREST THE BOT first and take the first one it can nav-reach clearly
			// before the carrier would. An earlier furthest-forward-first walk tended to land on a
			// node right by the enemy cap -- ~4000 u of travel, so the bot burned its whole give-up
			// window walking there and never arrived. Nearest-first minimises bot travel => the bot
			// actually gets there with hold time to spare. Real path length, not a straight line
			// through walls. Bounded reach checks so a bad recompute cannot pathfind forever.
			CNavArea *pForwardCutoffArea = nullptr;
			const int iMaxReachChecks = 4;
			const float flReachRatio = 1.15f;	// bot route may be at most 15% longer than the carrier's to that node
			CNavArea *pBotArea = me->GetLastKnownArea();
			if ( !pBotArea )
			{
				pBotArea = TheNavMesh->GetNearestNavArea( vecBotPos );
			}

			CUtlVector< bool > candidateChecked;
			candidateChecked.SetCount( candidateAreas.Count() );
			for ( int c = 0; c < candidateChecked.Count(); ++c )
			{
				candidateChecked[c] = false;
			}

			for ( int iPass = 0; iPass < iMaxReachChecks && pBotArea; ++iPass )
			{
				// next-nearest unchecked candidate
				int iBest = -1;
				float flBestDistSq = FLT_MAX;
				for ( int c = 0; c < candidateAreas.Count(); ++c )
				{
					if ( !candidateChecked[c] && candidateBotDistSq[c] < flBestDistSq )
					{
						flBestDistSq = candidateBotDistSq[c];
						iBest = c;
					}
				}
				if ( iBest < 0 )
				{
					break;
				}
				candidateChecked[iBest] = true;

				CNavArea *pArea = candidateAreas[iBest];
				const Vector vecNode = pArea->GetCenter();
				ShortestPathCost botCost;
				if ( !NavAreaBuildPath( pBotArea, pArea, &vecNode, botCost ) )
				{
					continue;
				}

				const float flBotRouteLen = RouteLengthAlongParents( pArea );
				if ( flBotRouteLen <= candidateCarrierDist[iBest] * flReachRatio )
				{
					pForwardCutoffArea = pArea;
					break;
				}
			}

			if ( pForwardCutoffArea )
			{
				m_targetCapArea = pForwardCutoffArea;
				m_targetCapPos = pForwardCutoffArea->GetCenter();
			}
			else if ( pNearestToBotArea )
			{
				m_targetCapArea = pNearestToBotArea;
				m_targetCapPos = pNearestToBotArea->GetCenter();
			}
		}
	}

	// Where the chosen cut-off sits on the cached route (cap-first order, so a lower index is
	// closer to the cap). Update uses it for an along-route "has the carrier passed this?" test
	// instead of a straight-line one that trips through walls.
	m_targetRouteIndex = m_predictedCarrierRoute.Find( m_targetCapArea );

	return ( m_targetCapArea != nullptr );
}

//---------------------------------------------------------------------------------------------
ActionResult< CNEOBot > CNEOBotCtgEnemyInterceptCapPath::OnStart( CNEOBot *me, Action< CNEOBot > *priorAction )
{
	m_path.SetMinLookAheadDistance( me->GetDesiredPathLookAheadRange() );

	m_bHoldingPosition = false;
	m_contactCheckTimer.Start( RandomFloat( 0.5f, 1.5f ) );
	// Periodic recompute is a backstop; the cheap per-tick deviation check in Update drives the
	// fast reaction to an unexpected carrier route. m_minRecomputeInterval keeps that from turning
	// into a pathfind storm -- NavAreaBuildPath is not free and this runs for every intercepting bot.
	m_recomputeTimer.Start( RandomFloat( 3.0f, 5.0f ) );
	m_minRecomputeInterval.Start( 1.0f );
	m_holdGiveUpTimer.Start( RandomFloat( 14.0f, 20.0f ) );

	if ( !RecomputeTargetCapPoint( me ) )
	{
		return Done( "No enemy capture point available" );
	}

	if ( !CNEOBotPathCompute( me, m_path, m_targetCapPos, FASTEST_ROUTE ) )
	{
		return Done( "Initial path compute failed" );
	}

	return Continue();
}

//---------------------------------------------------------------------------------------------
ActionResult< CNEOBot > CNEOBotCtgEnemyInterceptCapPath::Update( CNEOBot *me, float interval )
{
	if ( NEORules()->GetGameType() != NEO_GAME_TYPE_CTG )
	{
		return Done( "Game mode is no longer CTG" );
	}

	if ( !NEORules()->GhostExists() )
	{
		return Done( "Ghost does not exist" );
	}

	const int iGhosterPlayer = NEORules()->GetGhosterPlayer();
	if ( iGhosterPlayer <= 0 || iGhosterPlayer > gpGlobals->maxClients )
	{
		return Done( "Ghost carrier invalid" );
	}

	CNEO_Player *pGhostCarrier = ToNEOPlayer( UTIL_PlayerByIndex( iGhosterPlayer ) );
	if ( !pGhostCarrier || !pGhostCarrier->IsAlive() || pGhostCarrier->GetTeamNumber() == me->GetTeamNumber() )
	{
		return Done( "Ghost carrier dead or friendly" );
	}

	// Hand off to the direct chaser once the intercept has served its purpose. These are the
	// meaningful "the repositioning is over" signals -- not PVS potential-visibility, which trips
	// almost everywhere on a compact map and made this behaviour a sub-second no-op.
	//
	// "Carrier slipped past the cut-off": test it along the cached route, not straight-line. The
	// route is stored cap-first, so the carrier has passed our cut-off once its last-known area
	// sits at a lower index (closer to the cap) than the cut-off does. A straight-line compare
	// here fired on tick one for any cut-off the carrier happened to be nearer to through a wall,
	// which cancelled a large fraction of episodes instantly on winding maps. If the carrier's
	// area is not on the route at all it has diverged -- the recompute path below handles that.
	if ( m_predictedCarrierRoute.Count() > 0 && m_targetRouteIndex != m_predictedCarrierRoute.InvalidIndex() )
	{
		CNavArea *pCarrierArea = pGhostCarrier->GetLastKnownArea();
		const int iCarrierIdx = pCarrierArea ? m_predictedCarrierRoute.Find( pCarrierArea )
		                                     : m_predictedCarrierRoute.InvalidIndex();
		if ( iCarrierIdx != m_predictedCarrierRoute.InvalidIndex() && iCarrierIdx < m_targetRouteIndex )
		{
			return Done( "Carrier slipped past the cut-off" );
		}
	}

	if ( m_contactCheckTimer.IsElapsed() )
	{
		m_contactCheckTimer.Start( RandomFloat( 0.5f, 1.5f ) );
		if ( me->IsRangeLessThan( pGhostCarrier, 900.0f ) && me->IsLineOfSightClear( pGhostCarrier ) )
		{
			return Done( "Carrier in sight" );
		}
	}

	if ( m_holdGiveUpTimer.IsElapsed() )
	{
		return Done( "Intercept window closed" );
	}

	// Re-pick the cut-off on the periodic backstop timer, or sooner if the carrier has diverged
	// from the route we predicted. The divergence test is cheap -- a linear scan of the cached
	// route's area pointers, no pathfinding -- and m_minRecomputeInterval throttles how often it
	// can actually trigger the expensive RecomputeTargetCapPoint.
	bool bDoRecompute = m_recomputeTimer.IsElapsed();
	if ( !bDoRecompute && m_minRecomputeInterval.IsElapsed() && m_predictedCarrierRoute.Count() > 0 )
	{
		CNavArea *pCarrierArea = pGhostCarrier->GetLastKnownArea();
		if ( pCarrierArea && m_predictedCarrierRoute.Find( pCarrierArea ) == m_predictedCarrierRoute.InvalidIndex() )
		{
			bDoRecompute = true;
		}
	}

	if ( bDoRecompute )
	{
		m_recomputeTimer.Start( RandomFloat( 3.0f, 5.0f ) );
		m_minRecomputeInterval.Start( 1.0f );
		const Vector vecOldTargetPos = m_targetCapPos;
		CNavArea *pOldTargetArea = m_targetCapArea;
		const bool bWasHolding = m_bHoldingPosition;
		if ( !RecomputeTargetCapPoint( me ) )
		{
			return Done( "No capture point found on recompute" );
		}

		// Hold hysteresis: the carrier moves every tick, so the predicted route -- and the node
		// picked off it -- drifts a little on every backstop recompute. Without this a bot that had
		// finally parked on a cut-off gets its hold cleared and its path rebuilt for a sub-200 u
		// retarget and never settles. If we were holding and the fresh pick is within an
		// arrive-radius of where we already stand, and the old cut-off is still on the new route
		// (so the slipped-past test stays valid), keep holding it.
		const float flHoldHysteresis = 200.0f;
		const int iOldOnNewRoute = pOldTargetArea ? m_predictedCarrierRoute.Find( pOldTargetArea )
		                                          : m_predictedCarrierRoute.InvalidIndex();
		if ( bWasHolding
			&& iOldOnNewRoute != m_predictedCarrierRoute.InvalidIndex()
			&& vecOldTargetPos.DistTo( m_targetCapPos ) < flHoldHysteresis )
		{
			m_targetCapArea = pOldTargetArea;
			m_targetCapPos = vecOldTargetPos;
			m_targetRouteIndex = iOldOnNewRoute;
			// m_bHoldingPosition stays true; keep the already-completed path.
		}
		else if ( m_targetCapPos != vecOldTargetPos )
		{
			m_bHoldingPosition = false;
			if ( !CNEOBotPathCompute( me, m_path, m_targetCapPos, FASTEST_ROUTE ) )
			{
				return Done( "Path failed on recompute" );
			}
		}
	}

	// Parked on the cut-off: hold it and watch toward the carrier. CNEOBotMainAction still fires
	// at any visible threat while we hold, so a holding bot is a stationary ambusher, not a duck.
	if ( m_bHoldingPosition )
	{
		me->GetBodyInterface()->AimHeadTowards( pGhostCarrier, IBody::IMPORTANT, 0.3f,
			nullptr, "Intercept: watching for the ghost carrier" );
		return Continue();
	}

	m_path.Update( me );
	if ( !m_path.IsValid() )
	{
		// PathFollower invalidates the path when the goal is reached. If we are at (or nearly at)
		// the cut-off that is success -- hold here. Otherwise the path genuinely broke: try one
		// rebuild before giving up.
		const float flDistToTarget = sqrt( me->GetAbsOrigin().DistToSqr( m_targetCapPos ) );

		const float flArriveRadius = 200.0f;
		if ( flDistToTarget < flArriveRadius )
		{
			m_bHoldingPosition = true;
			return Continue();
		}

		// Rebuilding is a pathfind, so it goes through the same floor as the recompute above. A
		// cut-off whose area centre the bot cannot physically stand on completes its path a little
		// short of the arrive radius every time; without this floor that turns into a rebuild on
		// every tick. Wait it out instead -- m_holdGiveUpTimer still bounds the whole episode.
		if ( !m_minRecomputeInterval.IsElapsed() )
		{
			return Continue();
		}
		m_minRecomputeInterval.Start( 1.0f );

		if ( RecomputeTargetCapPoint( me ) && CNEOBotPathCompute( me, m_path, m_targetCapPos, FASTEST_ROUTE ) )
		{
			m_bHoldingPosition = false;
			return Continue();
		}

		return Done( "Lost the interception path" );
	}

	return Continue();
}
