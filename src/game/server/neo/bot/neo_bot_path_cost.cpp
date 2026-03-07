#include "cbase.h"
#include "neo_bot.h"
#include "neo_bot_path_cost.h"
#include "neo_gamerules.h"
#include "neo_bot_locomotion.h"
#include "nav_mesh.h"
#include "neo_bot_path_reservation.h"

extern ConVar neo_bot_path_reservation_enable;

ConVar neo_bot_path_around_friendly_cooldown("neo_bot_path_around_friendly_cooldown", "2.0", FCVAR_CHEAT,
	"How often to check for friendly path dispersion", false, 0, false, 60);

ConVar neo_bot_path_penalty_jump_multiplier("neo_bot_path_penalty_jump_multiplier", "100.0", FCVAR_CHEAT,
	"Maximum penalty multiplier for jump height changes in pathfinding", false, 0.01f, false, 1000.0f);

ConVar neo_bot_path_penalty_ladder_multiplier("neo_bot_path_penalty_ladder_multiplier", "3.0", FCVAR_CHEAT,
	"Penalty multiplier for ladder traversal in pathfinding", true, 0.1f, true, 100.0f);

//-------------------------------------------------------------------------------------------------
CNEOBotPathCost::CNEOBotPathCost(CNEOBot* me, RouteType routeType)
{
	m_me = me;
	m_routeType = routeType;
	m_stepHeight = me->GetLocomotionInterface()->GetStepHeight();
	m_maxJumpHeight = me->GetLocomotionInterface()->GetMaxJumpHeight();
	m_maxDropHeight = me->GetLocomotionInterface()->GetDeathDropHeight();
	m_bIgnoreReservations = !neo_bot_path_reservation_enable.GetBool();
}

//-------------------------------------------------------------------------------------------------
float CNEOBotPathCost::operator()(CNavArea* baseArea, CNavArea* fromArea, const CNavLadder* ladder, const CFuncElevator* elevator, float length) const
{
	VPROF_BUDGET("CNEOBotPathCost::operator()", "NextBot");

	CNavArea* area = (CNavArea*)baseArea;

	if (fromArea == NULL)
	{
		// first area in path, no cost
		return 0.0f;
	}
	else
	{
		if (!m_me->GetLocomotionInterface()->IsAreaTraversable(area))
		{
			return -1.0f;
		}

		// compute distance traveled along path so far
		float dist;

		if (ladder)
		{
			dist = ladder->m_length;

			// ladders leave bots exposed, but can be a shortcut
			const float ladderPenalty = neo_bot_path_penalty_ladder_multiplier.GetFloat();
			dist *= ladderPenalty;
		}
		else if (length > 0.0)
		{
			dist = length;
		}
		else
		{
			dist = (area->GetCenter() - fromArea->GetCenter()).Length();
		}

		// Only apply height restrictions for non-ladder jump paths
		if (!ladder)
		{
			// check height change
			float deltaZ = fromArea->ComputeAdjacentConnectionHeightChange(area);

			if (deltaZ >= m_stepHeight)
			{
				if (deltaZ >= m_maxJumpHeight)
				{
					// too high to reach
					return -1.0f;
				}

				// jumping is slower than flat ground
				const float jumpPenalty = neo_bot_path_penalty_jump_multiplier.GetFloat() * Square( deltaZ / m_maxJumpHeight );
				dist *= jumpPenalty;
			}
			else if (deltaZ < -m_maxDropHeight)
			{
				// too far to drop
				return -1.0f;
			}
		}

		// add a random penalty unique to this character so they choose different routes to the same place
		float preference = 1.0f;

		if (m_routeType == DEFAULT_ROUTE)
		{
			// this term causes the same bot to choose different routes over time,
			// but keep the same route for a period in case of repaths
			int timeMod = (int)(gpGlobals->curtime / 10.0f) + 1;
			preference = 1.0f + 50.0f * (1.0f + FastCos((float)(m_me->GetEntity()->entindex() * area->GetID() * timeMod)));
		}

		if (m_routeType == SAFEST_ROUTE)
		{
			// misyl: combat areas.
#if 0
			// avoid combat areas
			if (area->IsInCombat())
			{
				const float combatDangerCost = 4.0f;
				dist *= combatDangerCost * area->GetCombatIntensity();
			}
#endif
		}


		float cost = (dist * preference);

		// ------------------------------------------------------------------------------------------------
		// New path reservation related cost adjustments
		if ( !m_bIgnoreReservations && (m_routeType != FASTEST_ROUTE) )
		{
			cost += CNEOBotPathReservations()->GetPredictedFriendlyPathCount(area->GetID(), m_me->GetTeamNumber()) * neo_bot_path_reservation_penalty.GetFloat();
			cost += CNEOBotPathReservations()->GetAreaAvoidPenalty(area->GetID());

			if (m_routeType == SAFEST_ROUTE)
			{
				// NEO Jank Cheat: Incorporate enemy bot paths so that we don't run directly into their line of fire
				// Intended for use by ghost carrier team, to emulate a team that knows where enemies are likely to ambush
				// Compensates for bots' lack of meta knowledge by making them prefer routes not reserved by enemies
				// Adheres to cheat against bots but not against humans philosophy by not considering human players' positions
				cost += CNEOBotPathReservations()->GetPredictedFriendlyPathCount(area->GetID(), GetEnemyTeam(m_me->GetTeamNumber())) * neo_bot_path_reservation_penalty.GetFloat() * 2;
			}
		}
		// ------------------------------------------------------------------------------------------------

		if (area->HasAttributes(NAV_MESH_FUNC_COST))
		{
			cost *= area->ComputeFuncNavCost(m_me);
			DebuggerBreakOnNaN_StagingOnly(cost);
		}

		return cost + fromArea->GetCostSoFar();
	}
}
