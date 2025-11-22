#ifndef NEO_BOT_PAUSE_H
#define NEO_BOT_PAUSE_H
#ifdef _WIN32
#pragma once
#endif

#include "bot/neo_bot.h"

class CNEOBotPause : public Action< CNEOBot >
{
public:
	virtual ActionResult< CNEOBot >	OnStart( CNEOBot *me, Action< CNEOBot > *priorAction );
	virtual ActionResult< CNEOBot >	Update( CNEOBot* me, float interval );
	virtual void					OnEnd( CNEOBot *me, Action< CNEOBot > *nextAction );

	virtual const char* GetName( void ) const { return "Pause"; };
};

#endif // NEO_BOT_PAUSE_H
