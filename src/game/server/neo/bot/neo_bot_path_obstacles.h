#pragma once

#include "NextBot/Path/NextBotPath.h"
#include "neo_bot_prop_detour.h"

class INextBot;
class PathFollower;

//----------------------------------------------------------------------------------------------------------------
// Looks along a bot's path for what the nav mesh cannot know about:
// props that move, which it detours around, and breakables in the way, which it reports for the bot to clear
class CNEOBotPathObstacles
{
public:
	CNEOBotPathObstacles();

	void Reset();

	void Update( INextBot *bot, const PathFollower &path );

	bool IsDetouring() const { return m_detour.IsDetouring(); }
	const Vector &GetMoveGoal() const { return m_detour.GetMoveGoal(); }		// where to move next while detouring

	// The path goal the follower should take, if any: the one past the props while detouring,
	// and the one it had before the detour when the detour is given up
	const Path::Segment *GetPathGoal() const { return m_detour.GetPathGoal(); }

	// The nearest breakable the bot's body would meet on the path ahead, as of the last look
	CBaseEntity *GetBreakableInWay() const { return m_breakable.Get(); }

private:
	void Plan( INextBot *bot, const PathFollower &path );
	void LookForBreakable( INextBot *bot, const PathFollower &path );

	CountdownTimer m_lookTimer;				// the look ahead, or the detour's search again while detouring
	CountdownTimer m_breakableTimer;		// the look for breakables alone, while no plan is looking for them
	CHandle< CBaseEntity > m_breakable;
	CNEOBotPropDetour m_detour;
};
