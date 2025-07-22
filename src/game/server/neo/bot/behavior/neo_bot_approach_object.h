#pragma once

#include "Path/NextBotPathFollow.h"

class CNEOBotApproachObject : public Action< CNEOBot >
{
public:
	CNEOBotApproachObject( CBaseEntity *loot, float range = 10.0f );

	virtual ActionResult< CNEOBot >	OnStart( CNEOBot *me, Action< CNEOBot > *priorAction );
	virtual ActionResult< CNEOBot >	Update( CNEOBot *me, float interval );

	virtual const char *GetName( void ) const	{ return "ApproachObject"; };

private:
	CHandle< CBaseEntity > m_loot;		// what we are collecting
	float m_range;						// how close should we get
	PathFollower m_path;				// how we get to the loot
	CountdownTimer m_repathTimer;
};
