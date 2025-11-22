#ifndef NEO_BOT_COMMAND_FOLLOW_H
#define NEO_BOT_COMMAND_FOLLOW_H
#ifdef _WIN32
#pragma once
#endif

#include "Path/NextBotChasePath.h"

class CNEOBotCommandFollow : public Action< CNEOBot >
{
public:
	CNEOBotCommandFollow();

	virtual ActionResult< CNEOBot >	OnStart( CNEOBot *me, Action< CNEOBot > *priorAction );
	virtual ActionResult< CNEOBot >	Update( CNEOBot *me, float interval );
	virtual ActionResult< CNEOBot >	OnResume( CNEOBot *me, Action< CNEOBot > *interruptingAction );
	virtual void					OnEnd( CNEOBot *me, Action< CNEOBot > *nextAction );

	virtual const char *GetName( void ) const	{ return "CommandFollow"; };

private:
	bool FollowCommandChain( CNEOBot *me );
	bool FanOutAndCover( CNEOBot *me, Vector &movementTarget, bool bMoveToSeparate = true, float flArrivalZoneSizeSq = -1.0f );

	PathFollower m_path;
	CountdownTimer m_repathTimer;

	EHANDLE m_hTargetEntity;
	bool m_bGoingToTargetEntity = false;
	Vector m_vGoalPos = vec3_origin;
	float m_flNextFanOutLookCalcTime = 0.0f;
};

#endif // NEO_BOT_COMMAND_FOLLOW_H
