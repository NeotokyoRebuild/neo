#include "cbase.h"
#include "neo_ghost_cap_point.h"

#include "neo_gamerules.h"

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

LINK_ENTITY_TO_CLASS(neo_ghost_retrieval_point, CNEOGhostCapturePoint);

IMPLEMENT_NETWORKCLASS_ALIASED(NEOGhostCapturePoint, DT_NEOGhostCapturePoint);

BEGIN_NETWORK_TABLE_NOBASE(CNEOGhostCapturePoint, DT_NEOGhostCapturePoint)
#ifdef CLIENT_DLL
#else
#endif
END_NETWORK_TABLE()

BEGIN_DATADESC(CNEOGhostCapturePoint)
#ifdef GAME_DLL
	DEFINE_THINKFUNC(Think_CheckMyRadius),
#endif
END_DATADESC()

#if(0)
#ifdef CLIENT_DLL
BEGIN_PREDICTION_DATA(CNEOGhostCapturePoint)
END_PREDICTION_DATA()
#endif
#endif

CNEOGhostCapturePoint::CNEOGhostCapturePoint()
{
	m_iOwningTeam = TEAM_UNASSIGNED;
	m_iSuccessfulCaptorClientIndex = 0;

	m_flCapzoneRadius = 0;

	m_bIsActive = false;
	m_bGhostHasBeenCaptured = false;
}

void CNEOGhostCapturePoint::Spawn(void)
{
	BaseClass::Spawn();

	m_iOwningTeam = TEAM_JINRAI;
	m_flCapzoneRadius = 128.0f;
	SetActive(true);

	Precache();

#ifdef GAME_DLL
	RegisterThinkContext("CheckMyRadius");
	SetContextThink(&CNEOGhostCapturePoint::Think_CheckMyRadius,
		gpGlobals->curtime, "CheckMyRadius");
#else
	SetNextClientThink(gpGlobals->curtime);
#endif
}

#ifdef GAME_DLL
// Purpose: Checks if we have a valid ghoster inside our radius.
void CNEOGhostCapturePoint::Think_CheckMyRadius(void)
{
	if (m_bGhostHasBeenCaptured)
	{
		// We should have been reset after a cap before thinking!
		Assert(false);
		return;
	}

	const int checksPerSecond = 10;

	//DevMsg("CNEOGhostCapturePoint::Think_CheckMyRadius\n");

	for (int i = 1; i <= gpGlobals->maxClients; i++)
	{
		CBasePlayer *player = UTIL_PlayerByIndex(i);

		if (!player)
		{
			continue;
		}

		// Do we have a weapon?
		CBaseCombatWeapon *weapon = player->Weapon_GetSlot(WEAPON_PRIMARY_SLOT);
		if (!weapon)
		{
			continue;
		}

		// Is it a ghost?
		if (V_strcmp(weapon->GetName(), "weapon_ghost") != 0)
		{
			continue;
		}

		const int team = player->GetTeamNumber();

		Assert(team == TEAM_JINRAI || team == TEAM_NSF);

		// Is this our team's capzone?
		// NEO TODO (Rain): newbie UI helpers for attempting wrong team cap
		if (team != m_iOwningTeam)
		{
			continue;
		}

		const Vector dir = player->GetAbsOrigin() - GetAbsOrigin();
		const float distance = dir.Length2D();

		Assert(distance >= 0);

		// Has the ghost carrier reached inside our radius?
		// NEO TODO (Rain): newbie UI helpers for approaching wrong team cap
		if (distance >= m_flCapzoneRadius)
		{
			continue;
		}

		// We did it!
		m_bGhostHasBeenCaptured = true;
		m_iSuccessfulCaptorClientIndex = i;

		DevMsg("Player got ghost inside my radius\n");

		// Return early; we pass next think responsibility to gamerules,
		// whenever it sees fit to start capzone thinking again.
		return;
	}

	SetContextThink(&CNEOGhostCapturePoint::Think_CheckMyRadius,
		gpGlobals->curtime + (1.0f / checksPerSecond), "CheckMyRadius");
}
#else
// Purpose: Render capzone effects clientside.
void CNEOGhostCapturePoint::ClientThink(void)
{
	BaseClass::ClientThink();

	//DevMsg("CNEOGhostCapturePoint::ClientThink\n");

	SetNextClientThink(CLIENT_THINK_ALWAYS);
}
#endif

void CNEOGhostCapturePoint::Precache(void)
{
	BaseClass::Precache();

	AddEFlags(EFL_FORCE_CHECK_TRANSMIT);
}

void CNEOGhostCapturePoint::SetActive(bool isActive)
{
	m_bIsActive = isActive;
	UpdateVisibility();
}

inline void CNEOGhostCapturePoint::UpdateVisibility(void)
{
	if (m_bIsActive)
	{
		RemoveEFlags(EF_NODRAW);
	}
	else
	{
		AddEFlags(EF_NODRAW);
	}
}