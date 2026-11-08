#pragma once

#include "nav_pathfind.h"

class CNEOBot;

//---------------------------------------------------------------------------------------------
// A route start-first with the travel distance to each area, summed between area centers: only
// comparable with the same measure, not with a PathFollower's length
struct CNEOBotPredictedRoute
{
	CUtlVector< CNavArea * > areas;
	CUtlVector< float > travel;
};

// Shortest route from pStartArea to vecGoal. NavAreaBuildPath leaves it in the areas' parent
// pointers, which the next search overwrites, so it is copied out into route.
bool PredictRoute( CNavArea *pStartArea, const Vector &vecGoal, CNEOBotPredictedRoute &route );

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

	virtual bool ShouldSearch( CNavArea *adjArea, CNavArea *currentArea, float travelDistanceSoFar ) override;

	virtual void IterateAdjacentAreas( CNavArea *area, CNavArea *priorArea, float travelDistanceSoFar ) override;

protected:
	virtual bool ShouldSearchAtCost( CNavArea *adjArea, CNavArea *currentArea,
		float travelDistanceSoFar, float flCost )
	{
		return ShouldSearch( adjArea, currentArea, travelDistanceSoFar );
	}

private:
	CNEOBot *m_me;
};
