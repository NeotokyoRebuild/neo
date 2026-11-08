#pragma once

#include "NextBotBehavior.h"
#include "bot/neo_bot.h"

class CNavLadder;
class PathFollower;

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
	const CNavArea *FindLanding( const PathFollower *path, const CNavArea *pathExit ) const;
	const CNavArea *LadderEndArea() const;
	bool IsNarrowLanding( CNEOBot *me ) const;
	Vector GetDismountPos( CNEOBot *me ) const;
	bool IsDismountBlocked( CNEOBot *me, const Vector &toExit ) const;
	Vector GetCrawlDirection( const Vector &toExit ) const;
	float GetCrawlFeetZ() const;
	bool IsCrawlOnlyExit( CNEOBot *me, const Vector &toExit ) const;
	void UpdateCrawlDismount( CNEOBot *me, const Vector &toExit, bool bOnLadder );
	void ClaimLadder( CNEOBot *me ) const;

	const CNavLadder *m_ladder;
	const CNavArea *m_pExitArea;

	bool m_bGoingUp;
	bool m_bDismountPhase;
	bool m_bJumpedOffLadder;
	bool m_bCrawlDismount;	// stepping off crouched, through a gap only a crouched body fits
	bool m_bCrawlPushed;	// set in the gap and pushed off the ladder

	float m_flLastZ;
	Vector m_dismountPos;
	Vector m_ladderForward;

	CountdownTimer m_timeoutTimer;
	CountdownTimer m_stuckTimer;
	CountdownTimer m_dismountTimer;
	CountdownTimer m_nudgeTimer;	// shimmying sideways past a snag
	int m_nNudges;

	static constexpr float STUCK_CHECK_INTERVAL = 0.4f;
	static constexpr float STUCK_Z_TOLERANCE = 2.0f;
	static constexpr float DISMOUNT_TIMEOUT = 3.0f;	// Max time to walk toward exit area after leaving ladder
	static constexpr float SAFE_FALL_DIST = 200.0f;	// Max height to safely drop off a ladder
	static constexpr float MAX_DEVIATION_DIST = 100.0f; // Distance from ladder before canceling
	static constexpr float DISMOUNT_CLEARANCE_HULLS = 1.5f; // Hull widths toward the exit that must be clear to step off
	static constexpr float NARROW_LANDING_HULLS = 0.5f; // Hull widths deep a landing must be to stand on (measured on dawn and dusk L1's 7 u wall cap)
	static constexpr float LANDING_INSET = 16.0f; // How far inside a landing's near edge the dismount aims: half a hull, so the whole hull lands
	static constexpr float CRAWL_FEET_ABOVE_TOP = 4.0f; // Crouched, the feet step off this far above the ladder's top
	static constexpr float CRAWL_DUCK_RANGE = 64.0f; // How far below that height the climb ducks for a crawl exit: a few bot updates' climb
	static constexpr float CRAWL_SNAP_DIST = 24.0f; // How far from that height the bot may be set there: a little over a bot update's climb
	static constexpr float CRAWL_PUSH_SPEED = 150.0f; // How fast the bot is pushed off the ladder into the gap,
	static constexpr float CRAWL_PUSH_RISE_SPEED = 40.0f; // and up, a unit's rise
	static constexpr float CRAWL_CROUCH_HOLD = 0.3f; // Seconds each crouch press lasts, outliving a bot update
	static constexpr int STALL_NUDGES = 3; // Sideways shimmies before a stalled climb jumps off (measured on subsurface L1 and saitama L2)
};
