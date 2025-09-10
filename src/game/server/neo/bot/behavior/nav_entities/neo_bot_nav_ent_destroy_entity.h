#pragma once

#include "Path/NextBotPathFollow.h"
#include "NextBot/NavMeshEntities/func_nav_prerequisite.h"

class CNEOBotNavEntDestroyEntity : public Action< CNEOBot >
{
public:
	CNEOBotNavEntDestroyEntity( const CFuncNavPrerequisite *prereq );

	virtual ActionResult< CNEOBot >	OnStart( CNEOBot *me, Action< CNEOBot > *priorAction );
	virtual ActionResult< CNEOBot >	Update( CNEOBot *me, float interval );
	virtual void					OnEnd( CNEOBot *me, Action< CNEOBot > *nextAction );

	virtual const char *GetName( void ) const	{ return "NavEntDestroyEntity"; };

private:
	CHandle< CFuncNavPrerequisite > m_prereq;
	PathFollower m_path;				// how we get to the target
	CountdownTimer m_repathTimer;
	bool m_wasIgnoringEnemies;

	//void DetonateStickiesWhenSet( CNEOBot *me, CWeapon_SLAM *stickyLauncher ) const; // NEO TODO (Adam) remotedet
	//bool m_isReadyToLaunchSticky; // NEO TODO (Adam) remotedet
};
