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
	virtual void OnEnd( CNEOBot *me, Action<CNEOBot> *nextAction ) override;
	virtual ActionResult<CNEOBot> OnSuspend( CNEOBot *me, Action<CNEOBot> *interruptingAction ) override;
	virtual ActionResult<CNEOBot> OnResume( CNEOBot *me, Action<CNEOBot> *interruptingAction ) override;

	static constexpr float ALIGN_RANGE = 100.0f;        // Distance to start alignment behavior

private:
	ActionResult<CNEOBot> UpdateOverTop( CNEOBot *me, const Vector &hangPos, float range );
	bool IsBarrierAhead( CNEOBot *me, const Vector &hangPos ) const;

	const CNavLadder *m_ladder;
	bool m_bGoingUp;
	bool m_bOverTop;	// going down from behind a barrier at the ladder's top (UpdateOverTop)
	Vector m_ladderCenter;
	CountdownTimer m_timeoutTimer;

	static constexpr float MOUNT_RANGE = 25.0f;         // Distance to start climbing
	static constexpr float HANG_CLEARANCE = 2.0f;       // Gap between the face and a descending bot's hull as it mounts
	static constexpr float ALIGN_DOT_THRESHOLD = -0.9f;	// cos(~25 degrees) alignment tolerance

	static constexpr float OVER_TOP_TIMEOUT = 5.0f;		// approach timeout going over the top: line up, hop, back out
	static constexpr float FACE_LOOK_RANGE = 1000.0f;	// how far into the face the view is pointed going over the top
	static constexpr float FACE_LOOK_DROP = 50.0f;		// and how far below eye level there (3 degrees)
	static constexpr float LINEUP_TOLERANCE = 6.0f;		// least distance to the side of the ladder's middle a hop may start
	static constexpr float LINEUP_DEPTH = 8.0f;			// the line-up point on the floor, behind the face beyond the hull's half width
	static constexpr float BARRIER_PROBE = 4.0f;		// how close a barrier on the way to the hang point is hopped onto
	static constexpr float EDGE_ZONE = 8.0f;			// beyond the hull's half width behind the face: over the ladder's own top
	static constexpr float CROUCH_HOLD = 0.3f;			// crouch held from each update while backing out on the top
};
