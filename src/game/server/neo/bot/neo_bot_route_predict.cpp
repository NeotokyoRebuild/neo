#include "cbase.h"
#include "bot/neo_bot.h"
#include "bot/neo_bot_route_predict.h"
#include "nav_mesh.h"

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

//---------------------------------------------------------------------------------------------
bool PredictRoute( CNavArea *pStartArea, const Vector &vecGoal, CNEOBotPredictedRoute &route )
{
	CNavArea *pGoalArea = TheNavMesh->GetNearestNavArea( vecGoal );
	if ( !pStartArea || !pGoalArea )
	{
		return false;
	}

	ShortestPathCost cost;
	CNavArea *pReachedArea = nullptr;
	if ( !NavAreaBuildPath( pStartArea, pGoalArea, &vecGoal, cost, &pReachedArea ) )
	{
		return false;
	}

	// The parent chain runs goal -> start, from the area the search reached: a blocked pGoalArea is
	// passed over for another area containing vecGoal, and its own parent links are stale
	for ( CNavArea *pArea = pReachedArea; pArea; pArea = pArea->GetParent() )
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
bool CNEOBotTraversableFlood::ShouldSearch( CNavArea *adjArea, CNavArea *currentArea, float travelDistanceSoFar )
{
	return m_me->GetLocomotionInterface()->IsAreaTraversable( adjArea );
}

//---------------------------------------------------------------------------------------------
// Dijkstra in place of the base breadth-first flood: the open list is ordered by distance and an
// area's distance drops when a shorter way in turns up, so GetCostSoFar is the shortest travel
void CNEOBotTraversableFlood::IterateAdjacentAreas( CNavArea *area, CNavArea *priorArea, float travelDistanceSoFar )
{
	for ( int dir = 0; dir < NUM_DIRECTIONS; ++dir )
	{
		const int count = area->GetAdjacentCount( (NavDirType)dir );
		for ( int i = 0; i < count; ++i )
		{
			CNavArea *adjArea = area->GetAdjacentArea( (NavDirType)dir, i );
			// Settled distances cannot improve in this nonnegative-cost flood.
			if ( adjArea->IsMarked() && !adjArea->IsOpen() )
			{
				continue;
			}

			const float flCost = travelDistanceSoFar + ( adjArea->GetCenter() - area->GetCenter() ).Length();
			if ( adjArea->IsMarked() && flCost >= adjArea->GetCostSoFar() )
			{
				continue;
			}

			if ( !ShouldSearchAtCost( adjArea, area, travelDistanceSoFar, flCost ) )
			{
				continue;
			}
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
