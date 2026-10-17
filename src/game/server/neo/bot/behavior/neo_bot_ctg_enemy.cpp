#include "cbase.h"
#include "neo_player.h"
#include "bot/neo_bot.h"
#include "bot/behavior/neo_bot_ctg_enemy.h"
#include "bot/behavior/neo_bot_ctg_enemy_chase.h"
#include "bot/behavior/neo_bot_ctg_enemy_cutoff.h"
#include "neo_gamerules.h"
#include "nav_mesh.h"
#include "nav_pathfind.h"

//---------------------------------------------------------------------------------------------
// A route start-first with the travel distance to each area, summed between area centers: only
// comparable with the same measure, not with a PathFollower's length
struct CNEOBotPredictedRoute
{
	CUtlVector< CNavArea * > areas;
	CUtlVector< float > travel;
};

//---------------------------------------------------------------------------------------------
// Shortest route from pStartArea to vecGoal. NavAreaBuildPath leaves it in the areas' parent
// pointers, which the next search overwrites, so it is copied out here.
static bool PredictRoute( CNavArea *pStartArea, const Vector &vecGoal, CNEOBotPredictedRoute &route )
{
	CNavArea *pGoalArea = TheNavMesh->GetNearestNavArea( vecGoal );
	if ( !pStartArea || !pGoalArea )
	{
		return false;
	}

	ShortestPathCost cost;
	if ( !NavAreaBuildPath( pStartArea, pGoalArea, &vecGoal, cost ) )
	{
		return false;
	}

	// The parent chain runs goal -> start
	for ( CNavArea *pArea = pGoalArea; pArea; pArea = pArea->GetParent() )
	{
		route.areas.AddToTail( pArea );
	}

	route.areas.Reverse();

	route.travel.AddToTail( 0.0f );
	for ( int i = 1; i < route.areas.Count(); ++i )
	{
		route.travel.AddToTail( route.travel[ i - 1 ]
			+ ( route.areas[i]->GetCenter() - route.areas[ i - 1 ]->GetCenter() ).Length() );
	}

	return true;
}

//---------------------------------------------------------------------------------------------
// Floods outward over the areas this bot can traverse, leaving each reached area marked with its
// shortest travel distance (IsMarked, GetCostSoFar) until the next nav search. Ladders are not followed.
class CNEOBotTraversableFlood : public ISearchSurroundingAreasFunctor
{
public:
	CNEOBotTraversableFlood( CNEOBot *me ) : m_me( me ) {}

	virtual bool operator()( CNavArea *area, CNavArea *priorArea, float travelDistanceSoFar ) override
	{
		return true;
	}

	virtual bool ShouldSearch( CNavArea *adjArea, CNavArea *currentArea, float travelDistanceSoFar ) override
	{
		return m_me->GetLocomotionInterface()->IsAreaTraversable( adjArea );
	}

	// Dijkstra in place of the base breadth-first flood: the open list is ordered by distance and an
	// area's distance drops when a shorter way in turns up, so GetCostSoFar is the shortest travel
	virtual void IterateAdjacentAreas( CNavArea *area, CNavArea *priorArea, float travelDistanceSoFar ) override
	{
		for ( int dir = 0; dir < NUM_DIRECTIONS; ++dir )
		{
			const int count = area->GetAdjacentCount( (NavDirType)dir );
			for ( int i = 0; i < count; ++i )
			{
				CNavArea *adjArea = area->GetAdjacentArea( (NavDirType)dir, i );
				if ( !ShouldSearch( adjArea, area, travelDistanceSoFar ) )
				{
					continue;
				}

				const float flCost = travelDistanceSoFar + ( adjArea->GetCenter() - area->GetCenter() ).Length();
				if ( !adjArea->IsMarked() )
				{
					adjArea->Mark();
					adjArea->SetParent( area );
					adjArea->SetCostSoFar( flCost );
					adjArea->SetTotalCost( flCost );
					adjArea->AddToOpenList();
				}
				else if ( adjArea->IsOpen() && flCost < adjArea->GetCostSoFar() )
				{
					adjArea->SetParent( area );
					adjArea->SetCostSoFar( flCost );
					adjArea->SetTotalCost( flCost );
					adjArea->UpdateOnOpenList();
				}
			}
		}
	}

private:
	CNEOBot *m_me;
};

//---------------------------------------------------------------------------------------------
CNEO_Player *CNEOBotCtgEnemy::EnemyGhostCarrier( const CNEOBot *me )
{
	if ( !NEORules()->GhostExists() )
	{
		return nullptr;
	}

	const int iGhoster = NEORules()->GetGhosterPlayer();
	if ( iGhoster <= 0 || iGhoster > gpGlobals->maxClients )
	{
		return nullptr;
	}

	CNEO_Player *pCarrier = ToNEOPlayer( UTIL_PlayerByIndex( iGhoster ) );
	if ( !pCarrier || !pCarrier->IsAlive() || pCarrier->GetTeamNumber() == me->GetTeamNumber() )
	{
		return nullptr;
	}

	return pCarrier;
}

//---------------------------------------------------------------------------------------------
CNavArea *CNEOBotCtgEnemy::FindCutOff( CNEOBot *me, CNEO_Player *pGhostCarrier )
{
	CNavArea *pMyArea = me->GetLastKnownArea();
	if ( !pGhostCarrier || !pMyArea )
	{
		return nullptr;
	}

	// The carrier's goal as a defender reads it off the ghost marker: the nearest zone it can score in
	const Vector vecGoal = NEORules()->GetNearestGhostCapPoint( pGhostCarrier->GetTeamNumber(), pGhostCarrier->GetAbsOrigin() );
	if ( vecGoal == CNEO_Player::VECTOR_INVALID_WAYPOINT )
	{
		return nullptr;
	}

	// A naive shortest route: the carrier's class, loadout and intent are not observable
	CNEOBotPredictedRoute carrierRoute;
	if ( !PredictRoute( pGhostCarrier->GetLastKnownArea(), vecGoal, carrierRoute ) )
	{
		return nullptr;
	}

	// Already in its cap's area: nothing to cut off, and a zero search range would mean no limit
	if ( carrierRoute.areas.Count() < 2 )
	{
		return nullptr;
	}

	// One flood prices every area of the route by our own travel. It must come after the route is
	// copied out: both searches use the nav areas' shared search state.
	CNEOBotTraversableFlood flood( me );
	SearchSurroundingAreas( pMyArea, flood, carrierRoute.travel.Tail() );

	// The earliest area we reach no later than the carrier is the furthest from its cap we can stand in its way
	for ( int i = 0; i < carrierRoute.areas.Count(); ++i )
	{
		CNavArea *pArea = carrierRoute.areas[i];
		if ( pArea->IsMarked() && pArea->GetCostSoFar() <= carrierRoute.travel[i] )
		{
			return pArea;
		}
	}

	return nullptr;
}

//---------------------------------------------------------------------------------------------
ActionResult< CNEOBot > CNEOBotCtgEnemy::Update( CNEOBot *me, float interval )
{
	CNEO_Player *pGhostCarrier = EnemyGhostCarrier( me );
	if ( !pGhostCarrier )
	{
		return Done( "No enemy ghost carrier" );
	}

	// No cut-off means the carrier is ahead of us, and a detour would only give up more ground
	CNavArea *pCutOff = FindCutOff( me, pGhostCarrier );
	if ( !pCutOff )
	{
		return ChangeTo( new CNEOBotCtgEnemyChase( FASTEST_ROUTE ), "Chasing the ghost carrier down" );
	}

	if ( pCutOff == me->GetLastKnownArea() )
	{
		return ChangeTo( new CNEOBotCtgEnemyChase( DEFAULT_ROUTE ), "Standing on the cut-off - chasing" );
	}

	return ChangeTo( new CNEOBotCtgEnemyCutOff( pCutOff ), "Cutting the carrier off short of its cap" );
}

//---------------------------------------------------------------------------------------------
QueryResultType CNEOBotCtgEnemy::CarrierUrgency( const CNEOBot *me )
{
	CNEO_Player *pCarrier = EnemyGhostCarrier( me );
	if ( !pCarrier )
	{
		return ANSWER_UNDEFINED;
	}

	const Vector vecCarrierGoal = NEORules()->GetNearestGhostCapPoint( pCarrier->GetTeamNumber(), pCarrier->GetAbsOrigin() );
	if ( vecCarrierGoal == CNEO_Player::VECTOR_INVALID_WAYPOINT )
	{
		return ANSWER_UNDEFINED;
	}

	const float flCarrierToGoalSq = pCarrier->GetAbsOrigin().DistToSqr( vecCarrierGoal );
	const float flMeToGoalSq = me->GetAbsOrigin().DistToSqr( vecCarrierGoal );

	return ( flMeToGoalSq > flCarrierToGoalSq ) ? ANSWER_YES : ANSWER_UNDEFINED;
}
