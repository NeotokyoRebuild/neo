#ifndef NEO_BOT_PATH_CLEAR_BREAKABLE_H
#define NEO_BOT_PATH_CLEAR_BREAKABLE_H
#pragma once

#include "bot/neo_bot.h"

//--------------------------------------------------------------------------------------------------------
class CNEOBotPathClearBreakable : public Action< CNEOBot >
{
public:
	// Returns the breakable entity if one is blocking the bot's path
	static CBaseEntity *GetBreakableInPath( CNEOBot *me );

	CNEOBotPathClearBreakable( CBaseEntity *breakable );

	virtual ActionResult< CNEOBot > OnStart( CNEOBot *me, Action< CNEOBot > *priorAction ) override;
	virtual ActionResult< CNEOBot > Update( CNEOBot *me, float interval ) override;
	virtual void OnEnd( CNEOBot *me, Action< CNEOBot > *nextAction ) override;

	virtual const char *GetName( void ) const override { return "PathClearBreakable"; }

private:
	CHandle< CBaseEntity > m_hBreakable;
	bool m_isWaitingForFullReload = false;
	bool m_bDidSwitchWeapon = false;
	bool m_bSkipKnife = false;
	bool m_bStrafeRight = false;
	CountdownTimer m_giveUpTimer;
	CountdownTimer m_meleeTimeoutTimer;
};

#endif // NEO_BOT_PATH_CLEAR_BREAKABLE_H
