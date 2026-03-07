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
	void EnterDismountPhase( CNEOBot *me );
	void ResolveExitArea( CNEOBot *me );

	const CNavLadder *m_ladder;
	const CNavArea *m_pExitArea;

	bool m_bGoingUp;
	bool m_bDismountPhase;
	bool m_bJumpedOffLadder;

	float m_flLastZ;
	Vector m_exitAreaCenter;
	Vector m_ladderForward;

	CountdownTimer m_timeoutTimer;
	CountdownTimer m_stuckTimer;
	CountdownTimer m_dismountTimer;

	static constexpr float STUCK_CHECK_INTERVAL = 0.4f;
	static constexpr float STUCK_Z_TOLERANCE = 2.0f;
	static constexpr float DISMOUNT_TIMEOUT = 3.0f;	// Max time to walk toward exit area after leaving ladder
	static constexpr float SAFE_FALL_DIST = 200.0f;	// Max height to safely drop off a ladder
};
