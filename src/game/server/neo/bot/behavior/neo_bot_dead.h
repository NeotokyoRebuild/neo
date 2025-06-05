#pragma once

#include "Path/NextBotChasePath.h"

class CNEOBotDead : public Action< CNEOBot >
{
public:
	virtual ActionResult< CNEOBot >	OnStart( CNEOBot *me, Action< CNEOBot > *priorAction );
	virtual ActionResult< CNEOBot >	Update( CNEOBot *me, float interval );

	virtual const char *GetName( void ) const	{ return "Dead"; };

private:
	IntervalTimer m_deadTimer;
};
