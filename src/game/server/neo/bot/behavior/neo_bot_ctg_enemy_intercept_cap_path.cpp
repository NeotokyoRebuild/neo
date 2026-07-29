#include "cbase.h"
#include "neo_player.h"
#include "neo_gamerules.h"
#include "neo_ghost_cap_point.h"
#include "bot/neo_bot.h"
#include "bot/behavior/neo_bot_ctg_enemy_intercept_cap_path.h"
#include "bot/behavior/neo_bot_ctg_enemy.h"
#include "bot/neo_bot_path_compute.h"
#include "nav_pathfind.h"

//---------------------------------------------------------------------------------------------
CNEOBotCtgEnemyInterceptCapPath::CNEOBotCtgEnemyInterceptCapPath( void )
	: m_targetCapPos( vec3_origin ),
	  m_targetCapArea( nullptr )
{
}

//---------------------------------------------------------------------------------------------
bool CNEOBotCtgEnemyInterceptCapPath::RecomputeTargetCapPoint( CNEOBot *me )
{
	m_targetCapPos = vec3_origin;
	m_targetCapArea = nullptr;

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
		if ( !pCapPoint )
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

	// Predict hypothetical fastest path for the enemy ghost carrier to their target cap zone
	if ( pCarrierArea && pCarrierArea != pCapArea )
	{
		ShortestPathCost cost;
		if ( NavAreaBuildPath( pCarrierArea, pCapArea, &vecCapPos, cost ) )
		{
			CUtlVector< CNavArea * > pathAreas;
			for ( CNavArea *area = pCapArea; area; area = area->GetParent() )
			{
				pathAreas.AddToTail( area );
			}

			// Find the nav area along the predicted carrier path closest to this bot
			CNavArea *pBestInterceptArea = nullptr;
			float flMinDistSqToBot = FLT_MAX;
			const Vector vecBotPos = me->GetAbsOrigin();

			for ( int i = 0; i < pathAreas.Count(); ++i )
			{
				CNavArea *pArea = pathAreas[i];
				if ( !pArea )
				{
					continue;
				}

				const float distSq = vecBotPos.DistToSqr( pArea->GetCenter() );
				if ( distSq < flMinDistSqToBot )
				{
					flMinDistSqToBot = distSq;
					pBestInterceptArea = pArea;
				}
			}

			if ( pBestInterceptArea )
			{
				m_targetCapArea = pBestInterceptArea;
				m_targetCapPos = pBestInterceptArea->GetCenter();
			}
		}
	}

	return ( m_targetCapArea != nullptr );
}

//---------------------------------------------------------------------------------------------
ActionResult< CNEOBot > CNEOBotCtgEnemyInterceptCapPath::OnStart( CNEOBot *me, Action< CNEOBot > *priorAction )
{
	m_path.SetMinLookAheadDistance( me->GetDesiredPathLookAheadRange() );

	m_visibilityCheckTimer.Start( RandomFloat( 1.0f, 3.0f ) );
	m_recomputeTimer.Start( RandomFloat( 2.0f, 5.0f ) );

	if ( !RecomputeTargetCapPoint( me ) )
	{
		return ChangeTo( new CNEOBotCtgEnemy, "No enemy capture point available" );
	}

	if ( !CNEOBotPathCompute( me, m_path, m_targetCapPos, FASTEST_ROUTE ) )
	{
		return ChangeTo( new CNEOBotCtgEnemy, "Initial path compute failed" );
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
		return ChangeTo( new CNEOBotCtgEnemy, "Ghost does not exist" );
	}

	const int iGhosterPlayer = NEORules()->GetGhosterPlayer();
	if ( iGhosterPlayer <= 0 || iGhosterPlayer > gpGlobals->maxClients )
	{
		return ChangeTo( new CNEOBotCtgEnemy, "Ghost carrier invalid" );
	}

	CNEO_Player *pGhostCarrier = ToNEOPlayer( UTIL_PlayerByIndex( iGhosterPlayer ) );
	if ( !pGhostCarrier || !pGhostCarrier->IsAlive() || pGhostCarrier->GetTeamNumber() == me->GetTeamNumber() )
	{
		return ChangeTo( new CNEOBotCtgEnemy, "Ghost carrier dead or friendly" );
	}

	// Check if we need to reroute
	if ( m_recomputeTimer.IsElapsed() )
	{
		m_recomputeTimer.Start( RandomFloat( 2.0f, 5.0f ) );
		const Vector vecOldTargetPos = m_targetCapPos;
		if ( RecomputeTargetCapPoint( me ) )
		{
			if ( m_targetCapPos != vecOldTargetPos )
			{
				if ( !CNEOBotPathCompute( me, m_path, m_targetCapPos, FASTEST_ROUTE ) )
				{
					return ChangeTo( new CNEOBotCtgEnemy, "Path failed on recompute" );
				}
			}
		}
		else
		{
			return ChangeTo( new CNEOBotCtgEnemy, "No capture point found on recompute" );
		}
	}

	// Visibility / arrival stop-condition check (every 1-3s)
	if ( m_visibilityCheckTimer.IsElapsed() )
	{
		m_visibilityCheckTimer.Start( RandomFloat( 1.0f, 3.0f ) );
		CNavArea *botArea = me->GetLastKnownArea();
		if ( botArea && m_targetCapArea && ( botArea == m_targetCapArea || botArea->IsPotentiallyVisible( m_targetCapArea ) ) )
		{
			return ChangeTo( new CNEOBotCtgEnemy, "Reached interception visibility" );
		}
	}

	m_path.Update( me );
	if ( !m_path.IsValid() )
	{
		return ChangeTo( new CNEOBotCtgEnemy, "Path failed" );
	}

	return Continue();
}
