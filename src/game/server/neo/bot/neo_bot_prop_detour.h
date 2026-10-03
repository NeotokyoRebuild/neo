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
};
