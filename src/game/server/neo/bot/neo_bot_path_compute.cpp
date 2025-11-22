#include "cbase.h"
#include "bot/neo_bot.h"
#include "neo_bot_path_compute.h"
#include "bot/neo_bot_path_reservation.h"
#include "NextBot/Path/NextBotPathFollow.h"
#include "NextBot/Path/NextBotChasePath.h"

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

extern ConVar neo_bot_path_reservation_enabled;
extern ConVar neo_bot_path_reservation_penalty;
extern ConVar neo_bot_path_reservation_duration;
extern ConVar neo_bot_path_reservation_distance;

static void CNEOBotReservePath(CNEOBot* me, PathFollower& path)
{
	if (!neo_bot_path_reservation_enabled.GetBool() || !path.IsValid())
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

bool CNEOBotPathUpdateChase(CNEOBot* bot, ChasePath& path, CBaseEntity* subject, RouteType route, Vector *pPredictedSubjectPos)
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
