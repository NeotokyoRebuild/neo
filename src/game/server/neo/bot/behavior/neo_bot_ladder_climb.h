#pragma once

#include "NextBotBehavior.h"
#include "bot/neo_bot.h"

class CNavLadder;

//----------------------------------------------------------------------------------------------------------------
/**
 * Implementation based on ladder climbing logic in https://github.com/Dragoteryx/drgbase/
 * Behavior that handles the actual ladder climbing.
 * DOES disable enemy awareness while climbing.
 * Checks if still on ladder - returns Done if knocked off to reevaluate situation.
 */
class CNEOBotLadderClimb : public Action<CNEOBot>
{
public:
	CNEOBotLadderClimb( const CNavLadder *ladder, bool goingUp );
	virtual ~CNEOBotLadderClimb() = default;

	virtual const char *GetName() const override { return "LadderClimb"; }

	virtual ActionResult<CNEOBot> OnStart( CNEOBot *me, Action<CNEOBot> *priorAction ) override;
	virtual ActionResult<CNEOBot> Update( CNEOBot *me, float interval ) override;
	virtual void OnEnd( CNEOBot *me, Action<CNEOBot> *nextAction ) override;
	virtual ActionResult<CNEOBot> OnSuspend( CNEOBot *me, Action<CNEOBot> *interruptingAction ) override;
	virtual ActionResult<CNEOBot> OnResume( CNEOBot *me, Action<CNEOBot> *interruptingAction ) override;

private:
	bool m_bGoingUp;
	bool m_bHasBeenOnLadder;
	bool m_bHasLeftGround;

	float m_flLastZ;
	float m_flHighestZ;

	const CNavLadder *m_ladder;
	CountdownTimer m_timeoutTimer;
	CountdownTimer m_stuckTimer;

	static constexpr float STUCK_CHECK_INTERVAL = 1.0f;
	static constexpr float STUCK_Z_TOLERANCE = 5.0f;
	static constexpr float DISMOUNT_PUSH_DISTANCE = 16.0f;	// How close to target Z before we start pushing toward exit
};
