#pragma once

#include "NextBot/Path/NextBotPath.h"

class INextBot;
class PathFollower;

//----------------------------------------------------------------------------------------------------------------
// Steers a bot around props the nav mesh cannot know about, which physics or an animation moves,
// by searching a grid laid over the mesh on the path ahead for a way past the ones it sees
class CNEOBotPropDetour
{
public:
	CNEOBotPropDetour();

	void Reset();

	void Update( INextBot *bot, const PathFollower &path );

	bool IsDetouring() const { return m_waypoints.Count() > 0; }
	const Vector &GetMoveGoal() const { return m_waypoints[ 0 ]; }		// where to move next while detouring

	// The path goal the follower should take, if any: the one past the props while detouring,
	// and the one it had before the detour when the detour is given up
	const Path::Segment *GetPathGoal() const { return m_pathGoal; }

private:
	void Plan( INextBot *bot, const PathFollower &path );
	bool Replan( INextBot *bot );
	void NoteSearch( const Vector &feet );
	bool IsLastSearchValid( INextBot *bot ) const;

	CountdownTimer m_replanTimer;
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
