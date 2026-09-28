#include "neo_npc_targetsystem.h"
#include "neo_player.h"
#include "ammodef.h"
#include "nav_mesh.h"
#include "NextBotManager.h"
#include "bot/neo_bot_path_reservation.h"

#include "tier0/memdbgon.h"

#define CLOAKED_VELOCITY_THRESHOLD 32400 // 180 horizontal velocity. Slightly under sprint/wigglerun speed

// Bot hazard timing
static constexpr float BOT_HAZARD_REACTION_TIME = 0.25f;
static constexpr float BOT_HAZARD_INTERVAL = BOT_HAZARD_REACTION_TIME;
static constexpr float BOT_HAZARD_DURATION = 2.0f;

LINK_ENTITY_TO_CLASS(neo_npc_targetsystem, CNEO_NPCTargetSystem);

BEGIN_DATADESC(CNEO_NPCTargetSystem)
	DEFINE_KEYFIELD(m_flFOV, FIELD_FLOAT, "fov"),
	DEFINE_KEYFIELD(m_flTopClip, FIELD_FLOAT, "topclip"),
	DEFINE_KEYFIELD(m_flBottomClip, FIELD_FLOAT, "bottomclip"),
	DEFINE_KEYFIELD(m_flMaxViewDistance, FIELD_FLOAT, "maxviewdistance"),
	DEFINE_KEYFIELD(m_flDeadzone, FIELD_FLOAT, "deadzone"),
	DEFINE_KEYFIELD(m_flMiddleBoundsHalf, FIELD_FLOAT, "middlebounds"),
	DEFINE_KEYFIELD(m_iFilterName, FIELD_STRING, "filtername"),
	DEFINE_KEYFIELD(m_bStartDisabled, FIELD_BOOLEAN, "StartDisabled"),
	DEFINE_KEYFIELD(m_bMotionVision, FIELD_BOOLEAN, "motionvision"),

	DEFINE_KEYFIELD(m_strDamageSourceName, FIELD_STRING, "damagesource"),
	DEFINE_KEYFIELD(m_flDamage, FIELD_FLOAT, "damage"),
	DEFINE_KEYFIELD(m_flFireRate, FIELD_FLOAT, "firerate"),

	DEFINE_INPUTFUNC(FIELD_VOID, "Enable", InputEnable),
	DEFINE_INPUTFUNC(FIELD_VOID, "Disable", InputDisable),

	DEFINE_OUTPUT(m_OnSpotted, "OnSpotted"),
	DEFINE_OUTPUT(m_OnRight, "OnRight"),
	DEFINE_OUTPUT(m_OnLeft, "OnLeft"),
	DEFINE_OUTPUT(m_OnMiddle, "OnMiddle"),
	DEFINE_OUTPUT(m_OnMiddleIgnore, "OnMiddleIgnore"),
	DEFINE_OUTPUT(m_OnExit, "OnExit"),
	DEFINE_OUTPUT(m_OnExitMiddle, "OnExitMiddle"),

	DEFINE_THINKFUNC(Think),
END_DATADESC()

// This entity is a built-for-purpose recreation of the HT tank's "vision"
// But it can be used for general machines in maps that require precise player tracking.
// The view is divided into left/middle/right for turning the head of the turret,
// where the middle zone is where it can attack the player, and stop turning

// The deadzone is a space where the turret shouldnt be able to hit the player
// but still see them (like behind the barrel). So OnMiddle is not fired when
// the target is in this zone. OnMiddleIgnore fires regardless of the deadzone.

// NOTE: Isnt actually an npc anymore

void CNEO_NPCTargetSystem::Spawn()
{
	if (m_iFilterName != NULL_STRING)
	{
		m_pFilter = dynamic_cast<CBaseFilter*>(gEntList.FindEntityByName(nullptr, m_iFilterName));
	}

	if (m_strDamageSourceName != NULL_STRING)
	{
		m_hDamageSource = gEntList.FindEntityByName(nullptr, m_strDamageSourceName);
	}

	SetThink(&CNEO_NPCTargetSystem::Think);
	if (m_bStartDisabled)
	{
		SetNextThink(TICK_NEVER_THINK);
	}
	else
	{
		SetNextThink(gpGlobals->curtime + 0.05f);
	}
}

void CNEO_NPCTargetSystem::Think()
{
	float flBestLateral = FLT_MAX;
	float flBestForward = 0.0f;
	Vector vBestLateral;
	CBasePlayer *pBestTarget = nullptr;

	const float flMaxViewDistanceSqr = m_flMaxViewDistance * m_flMaxViewDistance;
	const float flTanFOV = tan(DEG2RAD(m_flFOV * 0.5f));

	Vector vecEye = EyePosition(); // Despite not using a model mappers are able to set their own eye offsets

	Vector vecForward, vecRight;
	AngleVectors(GetAbsAngles(), &vecForward, &vecRight, nullptr);

	for (int i = 1; i <= gpGlobals->maxClients; i++)
	{
		CBasePlayer* pPlayer = UTIL_PlayerByIndex(i);
		if (!pPlayer || !pPlayer->IsAlive())
		{
			continue;
		}

		if (m_pFilter && !m_pFilter->PassesFilter(this, pPlayer))
		{
			continue;
		}

		if (m_bMotionVision)
		{
			if (ToNEOPlayer(pPlayer)->m_bInThermOpticCamo)
			{
				if (pPlayer->GetAbsVelocity().Length2DSqr() < CLOAKED_VELOCITY_THRESHOLD)
				{
					continue;
				}
			}
		}

		// The player's eye position is used for detection.
		// The entity might be able to see the player's body up to their neck and they won't be detected
		Vector vecPlayerEye = pPlayer->EyePosition();
		Vector vecTargetPos = vecPlayerEye - vecEye;

		Vector vecTarget2DPos = vecTargetPos;
		vecTarget2DPos.z = 0;

		float flDistanceSqr = vecTarget2DPos.LengthSqr();
		if (flDistanceSqr > flMaxViewDistanceSqr) // Ignore if out of range
		{
			continue;
		}

		float flZDiff = vecPlayerEye.z - vecEye.z;
		if (flZDiff > m_flTopClip || flZDiff < m_flBottomClip) // Ignore if head is out of high/low bounds
		{
			continue;
		}

		if (!FVisible(pPlayer, MASK_BLOCKLOS, nullptr)) // Draw a trace to check if we can actually see the player
		{
			continue;
		}

		float flForwardDist = DotProduct(vecTarget2DPos, vecForward);
		if (flForwardDist <= 0) // Is the player behind?
		{
			continue;
		}

		// How close is this player to the center of our view
		Vector vLateral = vecTarget2DPos - (flForwardDist * vecForward);
		float flLateral = vLateral.Length();

		if (flLateral > flTanFOV * flForwardDist) // Ignore if they are outside our FOV
		{
			continue;
		}

		// The player is valid. But let's focus the one closest to us & our line of fire!!
		if (flLateral < flBestLateral)
		{
			flBestLateral = flLateral;
			flBestForward = flForwardDist;
			vBestLateral = vLateral;
			pBestTarget = pPlayer;
			m_pLastBestTarget = pPlayer;
		}
	}

	// Determine the zone this valid player is in
	enum TargetZone_e iNewZone = ZONE_NONE;
	if (pBestTarget)
	{
		if (flBestForward >= m_flDeadzone && flBestLateral <= m_flMiddleBoundsHalf) // If the player is outside the deadzone and inside the middle volume
		{
			iNewZone = ZONE_MIDDLE;
		}
		else if (flBestLateral > m_flMiddleBoundsHalf)
		{
			float flDotRight = DotProduct(vBestLateral, vecRight);
			iNewZone = (flDotRight > 0.0f) ? ZONE_RIGHT : ZONE_LEFT; // If the dot product is negative, the playa is on the left side
		}
	}

	bool bMiddleIgnore = pBestTarget && (flBestLateral <= m_flMiddleBoundsHalf); // Its just the middle zone ignoring the deadzone.

	// Fire outputs
	if (pBestTarget)
	{
		if (!m_bTargetAcquired)
		{
			m_OnSpotted.FireOutput(pBestTarget, this);
			m_bTargetAcquired = true;
			m_flNextHazardTime = gpGlobals->curtime + BOT_HAZARD_REACTION_TIME;
		}

		if (bMiddleIgnore && !m_bMiddleIgnoreActive)
		{
			m_OnMiddleIgnore.FireOutput(pBestTarget, this);
			m_bMiddleIgnoreActive = true;
		}
		else if (!bMiddleIgnore && m_bMiddleIgnoreActive)
		{
			m_bMiddleIgnoreActive = false;
		}

		if (iNewZone != m_iLastZone) // If the zone has changed
		{
			if (m_iLastZone == ZONE_MIDDLE && iNewZone != ZONE_MIDDLE)
			{
				m_OnExitMiddle.FireOutput(pBestTarget, this);
			}
			switch (iNewZone)
			{
			case ZONE_MIDDLE:
				m_OnMiddle.FireOutput(pBestTarget, this);
				break;
			case ZONE_LEFT:
				m_OnLeft.FireOutput(pBestTarget, this);
				break;
			case ZONE_RIGHT:
				m_OnRight.FireOutput(pBestTarget, this);
				break;
			default:
				break;
			}
		}
	}
	else
	{
		if (m_bTargetAcquired)
		{
			if (m_iLastZone == ZONE_MIDDLE)
			{
				m_OnExitMiddle.FireOutput(m_pLastBestTarget, this);
			}
			m_OnExit.FireOutput(m_pLastBestTarget, this);
			m_bTargetAcquired = false;
		}
		// Reset state when no target is found
		m_bMiddleIgnoreActive = false;
	}

	PublishBotHazards(pBestTarget);

	// Optional - damage the target
	if (m_hDamageSource && pBestTarget && (iNewZone == ZONE_MIDDLE))
	{
		if (gpGlobals->curtime >= m_flNextFireTime)
		{
			const Vector vecSrc = m_hDamageSource.Get()->GetAbsOrigin();
			const Vector vecTarget = pBestTarget->EyePosition();
			Vector vecDir = vecTarget - vecSrc;
			VectorNormalize(vecDir);

			// This intentionally should not create any tracer / decal effects being a server side only operation
			FireBulletsInfo_t info( 1, vecSrc, vecDir, vec3_origin, MAX_TRACE_LENGTH, GetAmmoDef()->Index("AMMO_PRI"), 28.0f, true, false );
			info.m_flDamage = m_flDamage;
			FireBullets(info);
			
			if (m_flFireRate > 0)
			{
				m_flNextFireTime = gpGlobals->curtime + (1.0f / m_flFireRate);
			}
		}
	}

	m_iLastZone = iNewZone;
	SetNextThink(gpGlobals->curtime + 0.05f);
}

//-----------------------------------------------------------------------------
// Bot hazard publishing - Bots cannot fight this entity.
// Hazards: its own area and all adjacent areas for every team (run-over),
// and its targeted player's area once acquired.
//-----------------------------------------------------------------------------

void CNEO_NPCTargetSystem::PublishBotHazards(CBasePlayer *pTarget)
{
	if (!TheNavMesh->IsLoaded() || TheNextBots().GetNextBotCount() == 0)
	{
		return;
	}

	if (gpGlobals->curtime < m_flNextHazardTime)
	{
		return;
	}

	m_flNextHazardTime = gpGlobals->curtime + BOT_HAZARD_INTERVAL;
	const float flExpireTime = gpGlobals->curtime + BOT_HAZARD_DURATION;

	// A team is targeted if a living member passes the filter; on ntre_rogue_ctg
	// the filter swaps on ghost pickup so the carrier's team is left out
	bool bTeamTargeted[TEAM__TOTAL] = {};
	for (int i = 1; i <= gpGlobals->maxClients; i++)
	{
		CBasePlayer *pPlayer = UTIL_PlayerByIndex(i);
		if (!pPlayer || !pPlayer->IsAlive() || pPlayer->GetTeamNumber() < FIRST_GAME_TEAM)
		{
			continue;
		}

		if (!m_pFilter || m_pFilter->PassesFilter(this, pPlayer))
		{
			bTeamTargeted[pPlayer->GetTeamNumber()] = true;
		}
	}

	// Mark areas PVS to the tank as do not hang out in my view
	CNavArea *pOwnArea = TheNavMesh->GetNearestNavArea(GetAbsOrigin());
	if (pOwnArea)
	{
		for (int iTeam = FIRST_GAME_TEAM; iTeam < TEAM__TOTAL; ++iTeam)
		{
			CNEOBotPathReservations()->AddDeadlyHazard(pOwnArea->GetID(), flExpireTime, iTeam);

			if (bTeamTargeted[iTeam])
			{
				AddVisibleHazard(pOwnArea, iTeam, flExpireTime);
			}
		}
	}

	if (pTarget)
	{
		CNavArea *pTargetArea = TheNavMesh->GetNearestNavArea(pTarget->GetAbsOrigin());
		if (pTargetArea)
		{
			// Don't propagate the PVS for the targeted bot location to avoid marking the areas around the corner
			AddVisibleHazard(pTargetArea, pTarget->GetTeamNumber(), flExpireTime, false);
		}
	}
}

void CNEO_NPCTargetSystem::AddVisibleHazard(CNavArea *pArea, int iTeam, float flExpireTime, bool bPropagatePVS)
{
	const bool bMotionVision = m_bMotionVision;
	auto addHazard = [flExpireTime, iTeam, bMotionVision, bPropagatePVS](CNavArea *pVisible)
	{
		if (bMotionVision)
		{
			CNEOBotPathReservations()->AddNpcTurretHazard(pVisible->GetID(), flExpireTime, iTeam, bPropagatePVS);
		}
		else
		{
			CNEOBotPathReservations()->AddDeadlyHazard(pVisible->GetID(), flExpireTime, iTeam, bPropagatePVS);
		}
		return true;
	};
	addHazard(pArea);
}

bool CNEO_NPCTargetSystem::CanSee(CBaseEntity *pEntity)
{
	if (m_pFilter && m_pFilter->PassesFilter(this, pEntity))
	{
		return true;
	}
	return false;
}

void CNEO_NPCTargetSystem::InputEnable(inputdata_t &inputData)
{
	SetNextThink(gpGlobals->curtime + 0.05f);
}

void CNEO_NPCTargetSystem::InputDisable(inputdata_t &inputData)
{
	SetNextThink(TICK_NEVER_THINK);
}
