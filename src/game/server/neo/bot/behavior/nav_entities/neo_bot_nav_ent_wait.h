#pragma once

#include "NextBot/NavMeshEntities/func_nav_prerequisite.h"


class CNEOBotNavEntWait : public Action< CNEOBot >
{
public:
	CNEOBotNavEntWait( const CFuncNavPrerequisite *prereq );

	virtual ActionResult< CNEOBot >	OnStart( CNEOBot *me, Action< CNEOBot > *priorAction );
	virtual ActionResult< CNEOBot >	Update( CNEOBot *me, float interval );

	virtual const char *GetName( void ) const	{ return "NavEntWait"; };

private:
	CHandle< CFuncNavPrerequisite > m_prereq;
	CountdownTimer m_timer;
};
