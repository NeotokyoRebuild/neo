#pragma once

#include "bot/neo_bot.h"
#include "bot/behavior/neo_bot_ctg_lone_wolf.h"

class CWeaponDetpack;

//--------------------------------------------------------------------------------------------------------
class CNEOBotCtgLoneWolfAmbush : public CNEOBotCtgLoneWolf
{
public:
	typedef CNEOBotCtgLoneWolf BaseClass;
	CNEOBotCtgLoneWolfAmbush( void ) = default;

	virtual ActionResult< CNEOBot > OnStart( CNEOBot *me, Action< CNEOBot > *priorAction ) override;
	virtual ActionResult< CNEOBot > Update( CNEOBot *me, float interval ) override;
	virtual ActionResult<CNEOBot> OnSuspend( CNEOBot *me, Action<CNEOBot> *interruptingAction ) override;
	virtual ActionResult<CNEOBot> OnResume( CNEOBot *me, Action<CNEOBot> *interruptingAction ) override;

	virtual EventDesiredResult< CNEOBot > OnStuck( CNEOBot *me ) override;
	virtual EventDesiredResult< CNEOBot > OnMoveToSuccess( CNEOBot *me, const Path *path ) override;
	virtual EventDesiredResult< CNEOBot > OnMoveToFailure( CNEOBot *me, const Path *path, MoveToFailureType reason ) override;

	virtual const char *GetName( void ) const override { return "ctgLoneWolfAmbush"; }

protected:
	ActionResult< CNEOBot > UpdateLookAround( CNEOBot *me );
	bool Is1v1( CNEOBot *me );

private:
	CountdownTimer m_lookAroundTimer;

	bool m_bIs1v1{ false };
	CountdownTimer m_1v1Timer;
	CountdownTimer m_1v1TransitionTimer;

	CUtlVector< CNavArea * > m_visibleAreas;
	Vector m_vecAmbushGoal;
};
