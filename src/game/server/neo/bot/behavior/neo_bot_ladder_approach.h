#pragma once

#include "NextBotBehavior.h"
#include "bot/neo_bot.h"

class CNavLadder;

//----------------------------------------------------------------------------------------------------------------
/**
 * Implementation based on ladder climbing logic in https://github.com/Dragoteryx/drgbase/
 * Behavior that handles approaching a ladder and aligning with it.
 * Uses ChangeTo for climb transition so knocked-off bots reevaluate situation.
 * Recommended to use SuspendFor to transition to the behavior or else scenario behavior may be stopped.
 */
class CNEOBotLadderApproach : public Action<CNEOBot>
{
public:
	CNEOBotLadderApproach( const CNavLadder *ladder, bool goingUp );
	virtual ~CNEOBotLadderApproach() = default;

	virtual const char *GetName() const override { return "LadderApproach"; }

	virtual ActionResult<CNEOBot> OnStart( CNEOBot *me, Action<CNEOBot> *priorAction ) override;
	virtual ActionResult<CNEOBot> Update( CNEOBot *me, float interval ) override;

private:
	const CNavLadder *m_ladder;
	bool m_bGoingUp;
	CountdownTimer m_timeoutTimer;

	static constexpr float MOUNT_RANGE = 25.0f;         // Distance to start climbing
	static constexpr float ALIGN_RANGE = 50.0f;         // Distance to start alignment behavior
	static constexpr float ALIGN_DOT_THRESHOLD = -0.9f;	// cos(~25 degrees) alignment tolerance
};
