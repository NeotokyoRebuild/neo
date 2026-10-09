#include "cbase.h"
#include "bot/neo_bot.h"
#include "neo_bot_path_compute.h"
#include "bot/neo_bot_path_reservation.h"
#include "NextBot/Path/NextBotPathFollow.h"
#include "NextBot/Path/NextBotChasePath.h"

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

static void CNEOBotReservePath(CNEOBot* me, PathFollower& path)
{
	if (!neo_bot_path_reservation_enable.GetBool() || !path.IsValid())
	{
		return;
	}

	CNEOBotPathReservations()->ReleaseAllAreas(me);

	const float reservation_distance = neo_bot_path_reservation_distance.GetFloat();
	const float reservation_duration = neo_bot_path_reservation_duration.GetFloat();

	for (const Path::Segment* seg = path.FirstSegment(); seg; seg = path.NextSegment(seg))
	{
		if (seg->distanceFromStart > reservation_distance)
		{
			break;
		}

		if (seg->area)
		{
			CNEOBotPathReservations()->ReserveArea(seg->area, me, reservation_duration);
		}
	}
}

// A path that stops short of the goal is only wanted if the caller asked for partial paths
static bool IsPathUsable(const PathFollower& path, bool reachedGoal, bool includeGoalIfPathFails)
{
	return path.IsValid() && (reachedGoal || includeGoalIfPathFails);
}

// By default, assumes that we would prefer to have partial paths even if we can't make a full path
// to accomodate scenarios like getting as close as possible to a hazard area but not into it
// Set includeGoalIfPathFails to false if you need to get the full path (or nothing on failure)
bool CNEOBotPathCompute(CNEOBot* bot, PathFollower& path, const Vector& goal, RouteType route, float maxPathLength, bool includeGoalIfPathFails, bool requireGoalArea)
{
	Assert(goal.IsValid());

	CNEOBotPathCost cost_with_reservations(bot, route);
	bool reachedGoal = path.Compute(bot, goal, cost_with_reservations, maxPathLength, includeGoalIfPathFails, requireGoalArea);
	if (IsPathUsable(path, reachedGoal, includeGoalIfPathFails))
	{
		CNEOBotReservePath(bot, path);
		return true;
	}

	CNEOBotPathCost cost_without_reservations(bot, FASTEST_ROUTE);
	cost_without_reservations.m_bIgnoreReservations = true;
	reachedGoal = path.Compute(bot, goal, cost_without_reservations, maxPathLength, includeGoalIfPathFails, requireGoalArea);
	if (IsPathUsable(path, reachedGoal, includeGoalIfPathFails))
	{
		CNEOBotReservePath(bot, path);
		return true;
	}

	// Path::Compute leaves a partial path valid even when told not to append the goal
	path.Invalidate();
	return false;
}

// When no route reaches the goal and the search got no nearer to it than the bot's own area,
// Path::Compute falls back to a straight line labeled COMPLETE_PATH: two segments in two areas
bool CNEOBotPathReachesGoal(const PathFollower& path)
{
	if (!path.IsValid() || path.GetResult() != Path::COMPLETE_PATH)
	{
		return false;
	}

	const Path::Segment* pFirst = path.FirstSegment();
	const Path::Segment* pLast = path.LastSegment();
	const bool bStraightLineFallback = (path.NextSegment(pFirst) == pLast) && (pFirst->area != pLast->area);
	return !bStraightLineFallback;
}

bool CNEOBotPathUpdateChase(CNEOBot* bot, ChasePath& path, CBaseEntity* subject, RouteType route, Vector* pPredictedSubjectPos)
{
	CNEOBotPathCost cost_with_reservations(bot, route);
	path.Update(bot, subject, cost_with_reservations, pPredictedSubjectPos);
	if (path.IsValid())
	{
		CNEOBotReservePath(bot, path);
		return true;
	}

	CNEOBotPathCost cost_without_reservations(bot, route);
	cost_without_reservations.m_bIgnoreReservations = true;
	path.Update(bot, subject, cost_without_reservations, pPredictedSubjectPos);
	if (path.IsValid())
	{
		CNEOBotReservePath(bot, path);
		return true;
	}

	return false;
}
