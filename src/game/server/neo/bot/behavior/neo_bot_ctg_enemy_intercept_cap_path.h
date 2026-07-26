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

	CNavArea *m_targetCapArea;
	CountdownTimer m_visibilityCheckTimer;
	CountdownTimer m_recomputeTimer;
	PathFollower m_path;
	Vector m_targetCapPos;
};
