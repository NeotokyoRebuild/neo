#ifndef NEO_BOT_PAUSE_H
#define NEO_BOT_PAUSE_H
#ifdef _WIN32
#pragma once
#endif

#include "bot/neo_bot.h"

class CNEOBotPause : public Action< CNEOBot >
{
public:
	virtual ActionResult< CNEOBot >	OnStart( CNEOBot *me, Action< CNEOBot > *priorAction ) override;
	virtual ActionResult< CNEOBot >	Update( CNEOBot* me, float interval ) override;
	virtual void					OnEnd( CNEOBot *me, Action< CNEOBot > *nextAction ) override;

	virtual const char* GetName( void ) const override { return "Pause"; };
};

#endif // NEO_BOT_PAUSE_H
