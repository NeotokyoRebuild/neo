#pragma once

#include "Path/NextBotChasePath.h"

class CNEOBotMeleeAttack : public Action< CNEOBot >
{
public:
	CNEOBotMeleeAttack( float giveUpRange = -1.0f );

	virtual ActionResult< CNEOBot >	OnStart( CNEOBot *me, Action< CNEOBot > *priorAction );
	virtual ActionResult< CNEOBot >	Update( CNEOBot *me, float interval );

	virtual const char *GetName( void ) const	{ return "MeleeAttack"; };

private:
	float m_giveUpRange;			// if non-negative and if threat is farther than this, give up our melee attack
	ChasePath m_path;
};
