#pragma once

#include "bot/neo_bot.h"
#include "bot/behavior/neo_bot_ctg_lone_wolf.h"
#include "utlmap.h"

class CWeaponDetpack;

//--------------------------------------------------------------------------------------------------------
class CNEOBotCtgLoneWolfSeek : public CNEOBotCtgLoneWolf
{
public:
	typedef CNEOBotCtgLoneWolf BaseClass;
	CNEOBotCtgLoneWolfSeek( void ) = default;

	virtual ActionResult< CNEOBot > OnStart( CNEOBot *me, Action< CNEOBot > *priorAction ) override;
	virtual ActionResult< CNEOBot > Update( CNEOBot *me, float interval ) override;

	virtual EventDesiredResult< CNEOBot > OnStuck( CNEOBot *me ) override;
	virtual EventDesiredResult< CNEOBot > OnMoveToSuccess( CNEOBot *me, const Path *path ) override;
	virtual EventDesiredResult< CNEOBot > OnMoveToFailure( CNEOBot *me, const Path *path, MoveToFailureType reason ) override;

	virtual const char *GetName( void ) const override { return "ctgLoneWolfSeek"; }

private:
	static constexpr float DEBUG_OVERLAY_DURATION = 3.0f;
	static constexpr float ENEMY_LAST_KNOWN_DIST = 100.0f;
	static constexpr float PATH_RECOMPUTE_DIST = 64.0f;
	static constexpr float SEARCH_WAYPOINT_REACHED_DIST = 200.0f;

	Vector m_vecLastGhostPos{ CNEO_Player::VECTOR_INVALID_WAYPOINT };
	CNavArea *m_pCachedGhostArea{ nullptr };

	CUtlMap<int, bool> m_exploredAreaIds;
	int m_iExplorationTargetId{-1};
	Vector m_vecSearchWaypoint{CNEO_Player::VECTOR_INVALID_WAYPOINT};

	void MarkVisibleAreasAsExplored( CNEOBot *me, CNavArea *currentArea, CNavArea *ghostArea = nullptr );
};
