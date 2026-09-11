#include "cbase.h"
#include "bot/neo_bot.h"
#include "neo_bot_path_compute.h"
#include "bot/neo_bot_path_reservation.h"
#include "NextBot/Path/NextBotPathFollow.h"
#include "NextBot/Path/NextBotChasePath.h"

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

ConVar sv_neo_bot_debug_emergence_area("sv_neo_bot_debug_emergence_area", "0", FCVAR_CHEAT,
	"Draw the hidden route and emergence area bots find when anticipating where a threat will come into view", true, 0, true, 1);

extern ConVar neo_bot_path_reservation_enable;
extern ConVar neo_bot_path_reservation_penalty;
extern ConVar neo_bot_path_reservation_duration;
extern ConVar neo_bot_path_reservation_distance;

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

bool CNEOBotPathCompute(CNEOBot* bot, PathFollower& path, const Vector& goal, RouteType route, float maxPathLength, bool includeGoalIfPathFails, bool requireGoalArea)
{
	Assert(goal.IsValid());

	CNEOBotPathCost cost_with_reservations(bot, route);
	if (path.Compute(bot, goal, cost_with_reservations, maxPathLength, includeGoalIfPathFails, requireGoalArea) && path.IsValid())
	{
		CNEOBotReservePath(bot, path);
		return true;
	}

	CNEOBotPathCost cost_without_reservations(bot, route);
	cost_without_reservations.m_bIgnoreReservations = true;
	if (path.Compute(bot, goal, cost_without_reservations, maxPathLength, includeGoalIfPathFails, requireGoalArea) && path.IsValid())
	{
		CNEOBotReservePath(bot, path);
		return true;
	}

	return false;
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

const Vector &CNEOBotFindPathEmergencePoint(const CNEOBot *bot, const Vector &familiarPos, const Vector &obscuredPos)
{
	CNavArea *myArea = bot->GetLastKnownArea();
	if (!myArea)
	{
		return vec3_invalid;
	}

	CNavArea *familiarArea = TheNavMesh->GetNavArea(familiarPos);
	if (!familiarArea)
	{
		return vec3_invalid;
	}

	CNavArea *obscuredArea = TheNavMesh->GetNavArea(obscuredPos);
	if (!obscuredArea)
	{
		return vec3_invalid;
	}

	ShortestPathCost cost;
	if (NavAreaBuildPath(familiarArea, obscuredArea, &obscuredPos, cost))
	{
		const bool debugDraw = sv_neo_bot_debug_emergence_area.GetBool();
		constexpr float debugDrawDuration = 2.0f;

		// search backwards from the obscured position for the first area visible to the bot
		for (CNavArea *area = obscuredArea; area; area = area->GetParent())
		{
			// the bot's own area always counts as visible, but its center is no place to aim
			if (area != myArea && myArea->IsPotentiallyVisible(area))
			{
				if (debugDraw)
				{
					area->DrawFilled(255, 255, 0, 64, debugDrawDuration);
				}
				return area->GetCenter();
			}

			if (area == familiarArea)
			{
				return vec3_invalid;
			}

			// Hidden stretch of the route leading out to the emergence area
			if (debugDraw && area->GetParent())
			{
				NDebugOverlay::HorzArrow(area->GetCenter(), area->GetParent()->GetCenter(),
					2.0f, 255, 255, 0, 255, true, debugDrawDuration);
			}
		}
	}

	return vec3_invalid;
}
