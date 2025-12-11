#pragma once

#include "NextBot/Path/NextBotPath.h"
#include "nav_pathfind.h"
#include "tier1/utlmap.h"

class CNEOBot;
class CNavArea;
class CNavLadder;
class CFuncElevator;

#include "neo_bot_path_reservation.h"
#include "tier1/utlstack.h"

//----------------------------------------------------------------------------------------------------------------
/**
 * Functor used with NavAreaBuildPath()
 */
class CNEOBotPathCost : public IPathCost
{
public:
	CNEOBotPathCost(CNEOBot* me, RouteType routeType);

	virtual float operator()(CNavArea* baseArea, CNavArea* fromArea, const CNavLadder* ladder, const CFuncElevator* elevator, float length) const override;

	bool m_bIgnoreReservations;

private:
	CNEOBot* m_me;
	RouteType m_routeType;
	float m_stepHeight;
	float m_maxJumpHeight;
	float m_maxDropHeight;
};
