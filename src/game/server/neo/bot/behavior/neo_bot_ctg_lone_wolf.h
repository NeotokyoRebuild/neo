#pragma once

#include "bot/neo_bot.h"

//--------------------------------------------------------------------------------------------------------
class CNEOBotCtgLoneWolf : public Action< CNEOBot >
{
public:
	CNEOBotCtgLoneWolf( void ) = default;

	virtual ActionResult< CNEOBot > OnStart( CNEOBot *me, Action< CNEOBot > *priorAction ) override;
	virtual ActionResult< CNEOBot > Update( CNEOBot *me, float interval ) override;
	virtual ActionResult< CNEOBot > OnSuspend( CNEOBot *me, Action< CNEOBot > *interruptingAction ) override;
	virtual ActionResult< CNEOBot > OnResume( CNEOBot *me, Action< CNEOBot > *interruptingAction ) override;

	virtual EventDesiredResult< CNEOBot > OnStuck( CNEOBot *me ) override;

	virtual const char *GetName( void ) const override { return "ctgLoneWolf"; }

protected:
	virtual ActionResult< CNEOBot > ConsiderGhostInterception( CNEOBot *me, const CBaseCombatCharacter *pGhostOwner = nullptr );
	virtual ActionResult< CNEOBot > ConsiderGhostVisualCheck( CNEOBot *me );

	Vector GetNearestEnemyCapPoint( CNEOBot *me );

	CountdownTimer m_repathTimer;
	PathFollower m_path;
};
