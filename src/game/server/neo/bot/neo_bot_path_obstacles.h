#pragma once

#include "NextBot/Path/NextBotPath.h"

class INextBot;
class PathFollower;

//----------------------------------------------------------------------------------------------------------------
// Looks along a bot's path for what the nav mesh cannot know about:
// props that physics or an animation moves, which it steers around by searching a grid laid over the mesh for a way past,
// and breakables in the way, which it reports for the bot to clear
class CNEOBotPathObstacles
{
public:
	CNEOBotPathObstacles();

	void Reset();

	void Update( INextBot *bot, const PathFollower &path );

	bool IsDetouring() const { return m_waypoints.Count() > 0; }
	const Vector &GetMoveGoal() const { return m_waypoints[ 0 ]; }		// where to move next while detouring

	// The path goal the follower should take, if any: the one past the props while detouring,
	// and the one it had before the detour when the detour is given up
	const Path::Segment *GetPathGoal() const { return m_pathGoal; }

	// The nearest breakable the bot's body would meet on the path ahead, as of the last look
	CBaseEntity *GetBreakableInWay() const { return m_breakable.Get(); }

private:
	void Plan( INextBot *bot, const PathFollower &path );
	void LookForBreakable( INextBot *bot, const PathFollower &path );
	bool Replan( INextBot *bot );
	void NoteSearch( const Vector &feet );
	bool IsLastSearchValid( INextBot *bot ) const;

	CountdownTimer m_lookTimer;				// the look ahead, or the detour's search again while detouring
	CountdownTimer m_breakableTimer;		// the look for breakables, when no plan this often looks for them too
	CHandle< CBaseEntity > m_breakable;
	CUtlVector< Vector > m_waypoints;
	const Path::Segment *m_pathGoal;

	// what a detour keeps while the props and the bot move: where it takes up the path again,
	// the path goal past that point and the one before the detour, and the floor it may use
	Vector m_rejoin;
	const Path::Segment *m_rejoinGoal;
	const Path::Segment *m_resumeGoal;
	Vector2D m_regionLo;
	Vector2D m_regionHi;
	float m_floorLo;
	float m_floorHi;
	bool m_isWide;		// found by the wide search, so its replans share the one wide search a tick
	bool m_isPathPushable;		// every prop in the way on the path was light, so a route through light props is no detour

	// what the last search saw, so the detour is searched again only when that changes or the search grows old:
	// where the bot set out for its next waypoint, and how many props in the region rested (PROP_DETOUR_PROPS_MOVING if any was awake)
	Vector m_legStart;
	int m_restingPropCount;
	CountdownTimer m_searchAgeTimer;
};
