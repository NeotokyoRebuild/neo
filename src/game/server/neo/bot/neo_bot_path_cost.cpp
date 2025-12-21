#include "cbase.h"
#include "neo_bot.h"
#include "neo_bot_path_cost.h"
#include "neo_gamerules.h"
#include "neo_bot_locomotion.h"
#include "nav_mesh.h"
#include "neo_bot_path_reservation.h"
#include "neo_npc_targetsystem.h"

ConVar neo_bot_path_friendly_reservation_enable("neo_bot_path_friendly_reservation_enable", "1", FCVAR_NONE,
	"Enable friendly bot path dispersal", true, 0, false, 1);

ConVar neo_bot_path_around_friendly_cooldown("neo_bot_path_around_friendly_cooldown", "2.0", FCVAR_CHEAT,
	"How often to check for friendly path dispersion", false, 0, false, 60);

//-------------------------------------------------------------------------------------------------
CNEOBotPathCost::CNEOBotPathCost(CNEOBot* me, RouteType routeType)
{
	m_me = me;
	m_routeType = routeType;
	m_stepHeight = me->GetLocomotionInterface()->GetStepHeight();
	m_maxJumpHeight = me->GetLocomotionInterface()->GetMaxJumpHeight();
	m_maxDropHeight = me->GetLocomotionInterface()->GetDeathDropHeight();
	m_bIgnoreReservations = !neo_bot_path_friendly_reservation_enable.GetBool();
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
		}
		else if (length > 0.0)
		{
			dist = length;
		}
		else
		{
			dist = (area->GetCenter() - fromArea->GetCenter()).Length();
		}


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
			const float jumpPenalty = 2.0f;
			dist *= jumpPenalty;
		}
		else if (deltaZ < -m_maxDropHeight)
		{
			// too far to drop
			return -1.0f;
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
		if ( neo_bot_path_friendly_reservation_enable.GetBool()
			&& !m_bIgnoreReservations
			&& (m_routeType != FASTEST_ROUTE) )
		{
			cost += CNEOBotPathReservations()->GetPredictedFriendlyPathCount(area->GetID(), m_me->GetTeamNumber()) * neo_bot_path_reservation_penalty.GetFloat();
		}
		
		// Avoid the APC turret in ntre_rogue_ctg
		if ( V_strcmp( STRING( gpGlobals->mapname ), "ntre_rogue_ctg" ) == 0 )
		{
			CNEO_NPCTargetSystem *pTurret = NULL;
			while ( ( pTurret = (CNEO_NPCTargetSystem *)gEntList.FindEntityByClassname( pTurret, "neo_npc_targetsystem" ) ) != NULL )
			{
				if ( pTurret->IsHostileTo( m_me->GetEntity() ) )
				{
					// If the area is visible to the turret, apply a HUGE penalty
					// Optimize: Check distance first
					if ( ( area->GetCenter() - pTurret->GetAbsOrigin() ).LengthSqr() < Square( 1500.0f ) ) 
					{
						// We can't really do accurate LOS here without being too expensive
						// But CNEO_NPCTargetSystem uses IsPotentiallyVisible which uses PVS.
						// We can use the area's visibility to the turret.
						CNavArea *turretArea = TheNavMesh->GetNavArea( pTurret->GetAbsOrigin() );
						if ( turretArea && area->IsPotentiallyVisible( turretArea ) )
						{
							// Ideally we would do a trace, but that is too slow for pathfinding.
							// PVS is a good approximation.
							cost += 5000.0f;
						}
					}
				}
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
