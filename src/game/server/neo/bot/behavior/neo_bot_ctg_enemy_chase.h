#pragma once

#include "bot/neo_bot.h"
#include "Path/NextBotChasePath.h"
#include "nav_pathfind.h"

//--------------------------------------------------------------------------------------------------------
// Runs the enemy ghost carrier down until there is no longer a living enemy carrier. FASTEST_ROUTE
// from behind it; DEFAULT_ROUTE from ahead, where the reservation penalty spreads defenders out.
class CNEOBotCtgEnemyChase : public Action< CNEOBot >
{
public:
	CNEOBotCtgEnemyChase( RouteType route ) : m_routeType( route ) {}

	virtual ActionResult< CNEOBot >	OnStart( CNEOBot *me, Action< CNEOBot > *priorAction ) override;
	virtual ActionResult< CNEOBot >	Update( CNEOBot *me, float interval ) override;
	virtual ActionResult< CNEOBot >	OnResume( CNEOBot *me, Action< CNEOBot > *interruptingAction ) override;

	virtual QueryResultType ShouldHurry( const INextBot *me ) const override;

	virtual const char *GetName( void ) const override { return "ctgEnemyChase"; }

private:
	ChasePath m_chasePath;
	CountdownTimer m_routeTypeTimer;		// throttles re-checking whether we are ahead of the carrier
	RouteType m_routeType;
};
