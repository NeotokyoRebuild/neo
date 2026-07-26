#pragma once

#include "bot/neo_bot.h"

//--------------------------------------------------------------------------------------------------------
class CNEOBotCtgEnemyInterceptCapPath : public Action< CNEOBot >
{
public:
	CNEOBotCtgEnemyInterceptCapPath( void );

	virtual ActionResult< CNEOBot >	OnStart( CNEOBot *me, Action< CNEOBot > *priorAction ) override;
	virtual ActionResult< CNEOBot >	Update( CNEOBot *me, float interval ) override;

	virtual const char *GetName( void ) const override { return "ctgEnemyInterceptCapPath"; }

private:
	bool RecomputeTargetCapPoint( CNEOBot *me );

	// The spot being intercepted to: normally a cut-off node picked off the carrier's predicted
	// route, falling back to the enemy capture zone itself when no cut-off can be picked.
	CNavArea *m_targetCapArea;
	Vector m_targetCapPos;
	bool m_bHoldingPosition;			// true once parked on the cut-off, watching for the carrier
	CUtlVector< CNavArea * > m_predictedCarrierRoute;	// nav areas of the carrier's last predicted fastest path to the cap, cap-first order
	int m_targetRouteIndex = 0;		// index of m_targetCapArea within m_predictedCarrierRoute (for the along-route "carrier slipped past" test)
	CountdownTimer m_recomputeTimer;	// periodic cut-off recompute cadence
	CountdownTimer m_minRecomputeInterval;	// floor between pathfinds (recompute + path repair) so neither can storm
	CountdownTimer m_contactCheckTimer;	// cadence for the "carrier in sight" hand-off check
	CountdownTimer m_holdGiveUpTimer;	// upper bound on time spent in this behaviour
	PathFollower m_path;
};
