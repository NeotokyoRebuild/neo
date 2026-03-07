#include "cbase.h"
#include "neo_player.h"
#include "bot/neo_bot.h"
#include "bot/behavior/neo_bot_command_follow.h"
#include "bot/behavior/neo_bot_throw_weapon_at_player.h"
#include "nav_mesh.h"

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"


ConVar sv_neo_bot_cmdr_debug_pause_uncommanded("sv_neo_bot_cmdr_debug_pause_uncommanded", "0",
	FCVAR_CHEAT, "If true, uncommanded bots in behavior CNEOBotSeekAndDestroy will pause.", false, 0, false, 1);

// NEOTODO: Figure out a clean way to represent config distances as Hammer units, without needing to square on use
ConVar sv_neo_bot_cmdr_stop_distance_sq("sv_neo_bot_cmdr_stop_distance_sq", "5000",
	FCVAR_CHEAT, "Minimum distance gap between following bots", true, 3000, true, 100000);

ConVar sv_neo_bot_cmdr_look_weights_friendly_repulsion("sv_neo_bot_cmdr_look_weights_friendly_repulsion", "2",
	FCVAR_CHEAT, "Weight for friendly bot repulsion force", true, 1, true, 9999);
ConVar sv_neo_bot_cmdr_look_weights_wall_repulsion("sv_neo_bot_cmdr_look_weights_wall_repulsion", "3",
	FCVAR_CHEAT, "Weight for wall repulsion force", true, 1, true, 9999);
ConVar sv_neo_bot_cmdr_look_weights_explosives_repulsion("sv_neo_bot_cmdr_look_weights_explosives_repulsion", "4",
	FCVAR_CHEAT, "Weight for explosive repulsion force", true, 1, true, 9999);

ConVar sv_neo_bot_cmdr_look_weights_friendly_max_dist("sv_neo_bot_cmdr_look_weights_friendly_max_dist", "5000",
	FCVAR_CHEAT, "Distance to compare friendly repulsion forces", true, 1, true, MAX_TRACE_LENGTH);
ConVar sv_neo_bot_cmdr_look_weights_wall_repulsion_whisker_dist("sv_neo_bot_cmdr_look_weights_wall_repulsion_whisker_dist", "500",
	FCVAR_CHEAT, "Distance to extend whiskers", true, 1, true, MAX_TRACE_LENGTH);

//---------------------------------------------------------------------------------------------
CNEOBotCommandFollow::CNEOBotCommandFollow() : m_vGoalPos( CNEO_Player::VECTOR_INVALID_WAYPOINT )
{
}

//---------------------------------------------------------------------------------------------
ActionResult< CNEOBot >	CNEOBotCommandFollow::OnStart(CNEOBot *me, Action< CNEOBot > *priorAction)
{
	m_path.SetMinLookAheadDistance(me->GetDesiredPathLookAheadRange());
	m_commanderLookingAtMeTimer.Invalidate();
	m_bWasCommanderLookingAtMe = false;

	if (!FollowCommandChain(me))
	{
		return Done("No commander to follow");
	}

	return Continue();
}

//---------------------------------------------------------------------------------------------
ActionResult< CNEOBot >	CNEOBotCommandFollow::Update(CNEOBot *me, float interval)
{
	if (!FollowCommandChain(me))
	{
		return Done("Lost commander or released");
	}

	ActionResult<CNEOBot> weaponRequestResult = CheckCommanderWeaponRequest(me);
	if (weaponRequestResult.IsRequestingChange())
	{
		return weaponRequestResult;
	}

	m_path.Update(me);
	m_ghostEquipmentHandler.Update( me );

	return Continue();
}


//---------------------------------------------------------------------------------------------
ActionResult< CNEOBot > CNEOBotCommandFollow::CheckCommanderWeaponRequest(CNEOBot *me)
{
	// Can I even drop my primary? (e.g. Juggernaut's BALC)
	CNEOBaseCombatWeapon *pMyPrimary = assert_cast<CNEOBaseCombatWeapon*>(me->Weapon_GetSlot(0));
	if (!pMyPrimary || !pMyPrimary->CanDrop())
	{
		return Continue();
	}

	CNEO_Player* pCommander = me->m_hCommandingPlayer.Get();

	if (!pCommander || !pCommander->IsAlive())
	{
		return Continue();
	}

	// Check if commander has dropped primary
	if (pCommander->Weapon_GetSlot(0))
	{
		return Continue();
	}
	
	// Can the commander even pick my primary up? (e.g. Recon can't pick up PZ)
	if (!pMyPrimary->CanBePickedUpByClass(pCommander->GetClass()))
	{
		return Continue();
	}

	// Check if looking at me
	Vector vecToBot = me->EyePosition() - pCommander->EyePosition();
	vecToBot.NormalizeInPlace();
	Vector vecCmdrFacing;
	pCommander->EyeVectors(&vecCmdrFacing);

	if (vecCmdrFacing.Dot(vecToBot) <= 0.95f)
	{
		m_commanderLookingAtMeTimer.Invalidate();
		m_bWasCommanderLookingAtMe = false;
		return Continue();
	}

	if (!m_bWasCommanderLookingAtMe)
	{
		// Wait to see if it wasn't just a momentary glance
		m_commanderLookingAtMeTimer.Start();
		m_bWasCommanderLookingAtMe = true;
		return Continue();
	}

	if (m_commanderLookingAtMeTimer.GetElapsedTime() > 0.2f)
	{
		// The throw behavior triggers drop when the bot is looking at the commander, so practively aim at feet to avoid overthrowing
		me->GetBodyInterface()->AimHeadTowards(pCommander->GetAbsOrigin(), IBody::IMPORTANT, 0.2f, NULL, "Noticed commander looking at me without a primary");
	}

	if (m_commanderLookingAtMeTimer.GetElapsedTime() > 1.0f)
	{
		// Sanity check that commander is really looking at me with more expensive traceline
		// e.g. for edge case where I am behind the player the commander is looking at
		trace_t tr;
		const float estimatedReasonableThrowDistance = 300.0f;
		Vector traceEnd = pCommander->EyePosition() + vecCmdrFacing * estimatedReasonableThrowDistance;
		UTIL_TraceLine(pCommander->EyePosition(), traceEnd, MASK_SHOT_HULL, pCommander, COLLISION_GROUP_NONE, &tr);

		if (tr.DidHit() && tr.m_pEnt == me)
		{
			m_bWasCommanderLookingAtMe = false; // Reset state
			return SuspendFor(new CNEOBotThrowWeaponAtPlayer(pCommander), "Donating weapon to commander");
		}
		else
		{
			// Line of sight blocked
			m_commanderLookingAtMeTimer.Invalidate();
			m_bWasCommanderLookingAtMe = false;
		}
	}

	return Continue();
}

//---------------------------------------------------------------------------------------------
ActionResult< CNEOBot > CNEOBotCommandFollow::OnResume(CNEOBot *me, Action< CNEOBot > *interruptingAction)
{
	if (!FollowCommandChain(me))
	{
		return Done("No commander to follow on resume");
	}

	return Continue();
}

//---------------------------------------------------------------------------------------------
void CNEOBotCommandFollow::OnEnd(CNEOBot *me, Action< CNEOBot > *nextAction)
{
	me->m_hLeadingPlayer = nullptr;
	me->m_hCommandingPlayer = nullptr;
}


// ---------------------------------------------------------------------------------------------
// Process commander ping waypoint commands for bots
// true - order has been processed, continue following
// false - no order, stop following
bool CNEOBotCommandFollow::FollowCommandChain(CNEOBot* me)
{
	Assert(me);
	CNEO_Player* pCommander = me->m_hCommandingPlayer.Get();

	if (!pCommander)
	{
		return false;
	}

	// Mirror behavior of leader if we have one
	if ( CNEO_Player *pPlayerToMirror = me->m_hLeadingPlayer.Get() )
	{
		if (pPlayerToMirror->GetInThermOpticCamo())
		{
			me->EnableCloak(4.0f);
		}
		else
		{
			me->DisableCloak();
		}

		if (pPlayerToMirror->IsDucking())
		{
			me->PressCrouchButton(0.5f);
		}
		else
		{
			me->ReleaseCrouchButton();
		}
	}

	// Calibrate dynamic follow distance
	float follow_stop_distance_sq = sv_neo_bot_cmdr_stop_distance_sq.GetFloat();
	if (pCommander->m_flBotDynamicFollowDistanceSq > follow_stop_distance_sq)
	{
		follow_stop_distance_sq = pCommander->m_flBotDynamicFollowDistanceSq;
	}

	if (!pCommander->IsAlive())
	{
		// Commander died
		return false; // This triggers OnEnd which clears vars
	}

	// Follow commander if they are close enough to collect you (and ping cooldown elapsed)
	if (pCommander->m_tBotPlayerPingCooldown.IsElapsed()
		&& (me->GetStar() == pCommander->GetStar()) // only follow when commander is focused on your squad
		&& (me->GetAbsOrigin().DistToSqr(pCommander->GetAbsOrigin()) < sv_neo_bot_cmdr_stop_distance_sq.GetFloat()))
	{
		// Use sv_neo_bot_cmdr_stop_distance_sq for consistent bot collection range
		// follow_stop_distance_sq would be confusing if player doesn't know about distance tuning
		me->m_hLeadingPlayer = pCommander;
		m_vGoalPos = CNEO_Player::VECTOR_INVALID_WAYPOINT;
		pCommander->m_vLastPingByStar.GetForModify(me->GetStar()) = CNEO_Player::VECTOR_INVALID_WAYPOINT;
	}
	// Go to commander's ping
	else if (pCommander->m_vLastPingByStar.Get(me->GetStar()) != CNEO_Player::VECTOR_INVALID_WAYPOINT)
	{
		// Check if there's been an update for this star's ping waypoint
		if (pCommander->m_vLastPingByStar.Get(me->GetStar()) != me->m_vLastPingByStar.Get(me->GetStar()))
		{
			me->m_hLeadingPlayer = nullptr; // Stop following and start travelling to ping
			m_vGoalPos = pCommander->m_vLastPingByStar.Get(me->GetStar());
			me->m_vLastPingByStar.GetForModify(me->GetStar()) = pCommander->m_vLastPingByStar.Get(me->GetStar());

			// Force a repath to allow for fine tuned positioning
			CNEOBotPathCost cost(me, DEFAULT_ROUTE);
			if (m_path.Compute(me, m_vGoalPos, cost, 0.0f, true, true) && m_path.IsValid() && m_path.GetResult() == Path::COMPLETE_PATH)
			{
				return true;
			}
			else
			{
				me->m_hLeadingPlayer = pCommander; // fallback to following commander
				// continue with leader following logic below
			}
		}

		if (FanOutAndCover(me, m_vGoalPos))
		{
			// FanOutAndCover true: arrived at destination and settled, so don't recompute path
			return true;
		}

		CNEOBotPathCost cost(me, DEFAULT_ROUTE);
		if (m_path.Compute(me, m_vGoalPos, cost, 0.0f, true, true) && m_path.IsValid() && m_path.GetResult() == Path::COMPLETE_PATH)
		{
			return true;
		}
		else
		{
			me->m_hLeadingPlayer = pCommander; // fallback to following commander
			// continue with leader following logic below
		}
	}

	// Didn't have order from commander, so follow in snake formation
	CNEO_Player* pLeader = me->m_hLeadingPlayer.Get();
	if (!pLeader || !pLeader->IsAlive())
	{
		// Leader is no longer valid or alive
		me->m_hLeadingPlayer = nullptr;
		return true;
	}

	// Commander can swap and order around different squads while having followers
	// but otherwise bots only follow squadmates in the same star or their commander
	if ((pLeader != pCommander) && (pLeader->GetStar() != me->GetStar()))
	{
		me->m_hLeadingPlayer = nullptr;
		return true; // restart logic next tick
	}
	else if (me->GetAbsOrigin().DistToSqr(pLeader->GetAbsOrigin()) < follow_stop_distance_sq)
	{
		// Anti-collision: follow neighbor in snake chain
		for (int idx = 1; idx <= gpGlobals->maxClients; ++idx)
		{
			CNEO_Player* pOther = ToNEOPlayer(UTIL_PlayerByIndex(idx));
			if (!pOther || !pOther->IsBot() || pOther == me
				|| (pOther->m_hLeadingPlayer.Get() != me->m_hLeadingPlayer.Get()))
			{
				// Not another bot in the same snake formation
				continue;
			}

			if (me->GetAbsOrigin().DistToSqr(pOther->GetAbsOrigin()) < follow_stop_distance_sq / 2)
			{
				// Follow person I bumped into and link up to snake chain of followers
				me->m_hLeadingPlayer = pOther;
				break;
			}
		}

		m_hTargetEntity = NULL;
		m_bGoingToTargetEntity = false;
		m_path.Invalidate();
		Vector tempLeaderOrigin = pLeader->GetAbsOrigin();  // don't want to override m_vGoalPos
		FanOutAndCover(me, tempLeaderOrigin, false, follow_stop_distance_sq);
		return true;
	}

	// Set the bot's goal to the leader's position.
	m_vGoalPos = pLeader->GetAbsOrigin();
	CNEOBotPathCost cost(me, DEFAULT_ROUTE);
	if (m_path.Compute(me, m_vGoalPos, cost, 0.0f, true, true) && m_path.IsValid() && m_path.GetResult() == Path::COMPLETE_PATH)
	{
		// Prioritize following leader.
		return true;
	}
	else if (me->m_hLeadingPlayer.Get() == me->m_hCommandingPlayer.Get())
	{
		// Invalid path to leader who is also the commander, so reset both
		// Returning false will trigger OnEnd which resets vars
		return false;
	}
	else
	{
		// check command chain on next tick
		me->m_hLeadingPlayer = nullptr;
		return true;
	}
}

// ---------------------------------------------------------------------------------------------
// Process commander ping waypoint commands for bots
// The movementTarget is mutable to accomodate spreading out at goal zone 
bool CNEOBotCommandFollow::FanOutAndCover(CNEOBot* me, Vector& movementTarget, bool bMoveToSeparate /*= true*/, float flArrivalZoneSizeSq /*= -1.0f*/)
{
	if (flArrivalZoneSizeSq == -1.0f)
	{
		flArrivalZoneSizeSq = sv_neo_bot_cmdr_stop_distance_sq.GetFloat();
	}
	Vector vBotRepulsion = vec3_origin;
	Vector vWallRepulsion = vec3_origin;
	Vector vFinalForce = vec3_origin; // Initialize to vec3_origin
	bool bTooClose = false;

	if (gpGlobals->curtime > m_flNextFanOutLookCalcTime)
	{
		// Combined loop for player forces
		for (int idx = 1; idx <= gpGlobals->maxClients; ++idx)
		{
			CBasePlayer* pPlayer = UTIL_PlayerByIndex(idx);
			if (!pPlayer || pPlayer == me || !pPlayer->IsAlive())
				continue;

			// Only consider friendly players
			if (pPlayer->GetTeamNumber() != me->GetTeamNumber())
				continue;

			CNEO_Player* pOther = ToNEOPlayer(pPlayer);
			if (!pOther)
				continue;

			// Determine if we are too close to any friendly bot
			if (me->GetAbsOrigin().DistToSqr(pOther->GetAbsOrigin()) < sv_neo_bot_cmdr_stop_distance_sq.GetFloat() / 2)
			{
				bTooClose = true;
			}

			// Check for line of sight to other player for repulsion
			trace_t tr;
			UTIL_TraceLine(me->EyePosition(), pOther->EyePosition(), MASK_PLAYERSOLID, me, COLLISION_GROUP_NONE, &tr);

			if (tr.fraction == 1.0f || tr.m_pEnt == pOther)
			{
				Vector vToOther = pOther->GetAbsOrigin() - me->GetAbsOrigin();
				float flDistSqr = vToOther.LengthSqr();
				if (flDistSqr > 0.0f)
				{
					const float flMaxRepulsionDist = sv_neo_bot_cmdr_look_weights_friendly_max_dist.GetFloat();
					float flDist = FastSqrt(flDistSqr);
					float flRepulsionScale = 1.0f - (flDist / flMaxRepulsionDist);
					flRepulsionScale = Clamp(flRepulsionScale, 0.0f, 1.0f);
					float flRepulsion = flRepulsionScale * flRepulsionScale;

					if (flRepulsion > 0.0f)
					{
						vBotRepulsion -= vToOther.Normalized() * flRepulsion;
					}
				}
			}
		}

		// Next look time
		if (bTooClose)
		{
			m_flNextFanOutLookCalcTime = gpGlobals->curtime + 0.1; // 100 ms
		}
		else
		{
			float waitSec = RandomFloat(0.2f, 2.0);
			m_flNextFanOutLookCalcTime = gpGlobals->curtime + waitSec;
		}

		// Wall Repulsion
		trace_t tr;
		const int numWhiskers = 12;
		for (int i = 0; i < numWhiskers; ++i)
		{
			QAngle ang = me->GetLocalAngles();
			ang.y += (360.0f / numWhiskers) * i;
			Vector vWhiskerDir;
			AngleVectors(ang, &vWhiskerDir);
			float whiskerDist = sv_neo_bot_cmdr_look_weights_wall_repulsion_whisker_dist.GetFloat();
			vWhiskerDir.z = 0; // look at eye level for windows
			UTIL_TraceLine(me->GetBodyInterface()->GetEyePosition(), me->GetBodyInterface()->GetEyePosition() + vWhiskerDir * whiskerDist, MASK_SHOT, me, COLLISION_GROUP_NONE, &tr);
			if (tr.DidHit())
			{
				float flRepulsion = 1.0f - (tr.fraction);
				vWallRepulsion += tr.plane.normal * flRepulsion;
			}
		}

		// Combine forces and look at final direction
		float friendlyRepulsionWeight = sv_neo_bot_cmdr_look_weights_friendly_repulsion.GetFloat();
		float wallRepulsionWeight = sv_neo_bot_cmdr_look_weights_wall_repulsion.GetFloat();
		vFinalForce = (vBotRepulsion * friendlyRepulsionWeight) + (vWallRepulsion * wallRepulsionWeight);
		vFinalForce.z = 0; // avoid tilting awkwardly up or down
		vFinalForce.NormalizeInPlace();
		if (!vFinalForce.IsZero())
		{
			me->GetBodyInterface()->AimHeadTowards(me->GetBodyInterface()->GetEyePosition() + vFinalForce * 500.0f);
		}
	}

	// Fudge factor to reduce teammates running into each other trying to reach the same point
	if (me->GetAbsOrigin().DistToSqr(movementTarget) < flArrivalZoneSizeSq)
	{
		if (bMoveToSeparate)
		{
			if (bTooClose)
			{
				me->PressForwardButton(0.1);
			}
		}

		movementTarget = me->GetAbsOrigin();
		m_path.Invalidate();
		return true; // Is already at destination
	}

	// Still moving to destination, path will be recomputed by the calling context
	return false;
}
