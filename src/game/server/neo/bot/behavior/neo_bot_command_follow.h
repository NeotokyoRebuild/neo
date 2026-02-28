#ifndef NEO_BOT_COMMAND_FOLLOW_H
#define NEO_BOT_COMMAND_FOLLOW_H
#ifdef _WIN32
#pragma once
#endif

#include "NextBotBehavior.h"
#include "bot/behavior/neo_bot_ctg_carrier.h"

class CNEOBotCommandFollow : public Action< CNEOBot >
{
public:
	CNEOBotCommandFollow();

	virtual ActionResult< CNEOBot >	OnStart( CNEOBot *me, Action< CNEOBot > *priorAction ) override;
	virtual ActionResult< CNEOBot >	Update( CNEOBot *me, float interval ) override;
	virtual ActionResult< CNEOBot >	OnResume( CNEOBot *me, Action< CNEOBot > *interruptingAction ) override;
	virtual void					OnEnd( CNEOBot *me, Action< CNEOBot > *nextAction ) override;

	virtual const char *GetName( void ) const override { return "CommandFollow"; };

private:
	bool FollowCommandChain( CNEOBot *me );
	bool FanOutAndCover( CNEOBot *me, Vector &movementTarget, bool bMoveToSeparate = true, float flArrivalZoneSizeSq = -1.0f );
	ActionResult< CNEOBot > CheckCommanderWeaponRequest( CNEOBot *me );

	PathFollower m_path;
	CountdownTimer m_repathTimer;
	CNEOBotGhostEquipmentHandler m_ghostEquipmentHandler;

	IntervalTimer m_commanderLookingAtMeTimer;
	bool m_bWasCommanderLookingAtMe = false;

	EHANDLE m_hTargetEntity;
	bool m_bGoingToTargetEntity = false;
	Vector m_vGoalPos;
	float m_flNextFanOutLookCalcTime = 0.0f;
};

#endif // NEO_BOT_COMMAND_FOLLOW_H
