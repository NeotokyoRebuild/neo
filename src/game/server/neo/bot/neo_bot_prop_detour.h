#pragma once

#include "NextBot/Path/NextBotPath.h"

class INextBot;

//----------------------------------------------------------------------------------------------------------------
// What a look ahead hands a detour to plan: where past the props in the way it takes up the path again,
// the path goals there and before the props, the floor box from the bot past them, and whether only light props were in the way
struct PropDetourRequest_t
{
	Vector rejoin;
	const Path::Segment *rejoinGoal;
	const Path::Segment *resumeGoal;
	Vector2D regionLo;
	Vector2D regionHi;
	float floorLo;
	float floorHi;
	bool isPathPushable;
};

//----------------------------------------------------------------------------------------------------------------
// A way around props in the bot's way, found by searching a grid laid over the mesh,
// and kept while the props and the bot move until the bot is past them
class CNEOBotPropDetour
{
public:
	CNEOBotPropDetour();

	void Reset();

	// Search for a way around, and return how long to wait before looking again,
	// or 0 to look again at the look's own interval
	float Plan( INextBot *bot, const PropDetourRequest_t &request, float lookInterval );

	// Follow the detour, and search it again when the look is due:
	// return true if that search has to wait for a free tick
	bool Update( INextBot *bot, bool isSearchDue );

	bool IsDetouring() const { return m_waypoints.Count() > 0; }
	const Vector &GetMoveGoal() const { return m_waypoints[ 0 ]; }		// where to move next while detouring

	// The path goal the follower should take, if any: the one past the props while detouring,
	// and the one it had before the detour when the detour is given up
	const Path::Segment *GetPathGoal() const { return m_pathGoal; }

private:
	bool Replan( INextBot *bot );
	void NoteSearch( const Vector &feet );
	bool IsLastSearchValid( INextBot *bot ) const;

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
	// where the bot set out for its next waypoint, and how many props in the region rested (OBSTACLE_PROPS_MOVING if any was awake)
	Vector m_legStart;
	int m_restingPropCount;
	CountdownTimer m_searchAgeTimer;
};
