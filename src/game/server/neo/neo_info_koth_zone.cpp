#include "neo_info_koth_zone.h"
#include "neo_player.h"
#include "neo_player_shared.h"

#define KOTHZONE_PRUNE_INTERVAL 0.5f

BEGIN_DATADESC(CNEO_InfoKOTHZone)
	DEFINE_THINKFUNC(Think),
END_DATADESC()

void CNEO_InfoKOTHZone::Spawn()
{
	BaseClass::Spawn();

	SetThink(&CNEO_InfoKOTHZone::Think);
	SetNextThink(gpGlobals->curtime + KOTHZONE_PRUNE_INTERVAL);
}

void CNEO_InfoKOTHZone::Think()
{
	PruneStaleOccupants();
	SetNextThink(gpGlobals->curtime + KOTHZONE_PRUNE_INTERVAL);
}

void CNEO_InfoKOTHZone::OnPlayerEnter(CNEO_Player *pPlayer)
{
	if (!pPlayer)
		return;

	EHANDLE hPlayer = pPlayer;
	for (int i = 0; i < m_Occupants.Count(); ++i)
	{
		if (m_Occupants[i].hPlayer == hPlayer)
		{
			// already standing in another trigger of this same zone
			m_Occupants[i].touchCount++;
			return;
		}
	}

	ZoneOccupant occupant;
	occupant.hPlayer = hPlayer;
	// if they're somehow dead on entry, don't credit either team
	occupant.team = pPlayer->IsAlive() ? pPlayer->GetTeamNumber() : TEAM_UNASSIGNED;
	occupant.touchCount = 1;
	m_Occupants.AddToTail(occupant);

	if (occupant.team == TEAM_JINRAI)
		m_iJinraiCount++;
	else if (occupant.team == TEAM_NSF)
		m_iNSFCount++;

	UpdateState();
}

void CNEO_InfoKOTHZone::OnPlayerLeave(CNEO_Player *pPlayer)
{
	if (!pPlayer)
		return;

	EHANDLE hPlayer = pPlayer;
	for (int i = 0; i < m_Occupants.Count(); ++i)
	{
		if (m_Occupants[i].hPlayer != hPlayer)
			continue;

		// still touching another trigger belonging to this zone
		if (--m_Occupants[i].touchCount > 0)
			return;

		if (m_Occupants[i].team == TEAM_JINRAI)
			m_iJinraiCount--;
		else if (m_Occupants[i].team == TEAM_NSF)
			m_iNSFCount--;

		m_Occupants.Remove(i);
		UpdateState();
		return;
	}
}

void CNEO_InfoKOTHZone::PruneStaleOccupants()
{
	bool bChanged = false;

	// EndTouch won't reliably fire for players who disconnected or died without
	// physically leaving the trigger volume (e.g. respawning elsewhere) - catch those here
	for (int i = m_Occupants.Count() - 1; i >= 0; --i)
	{
		CNEO_Player *pPlayer = static_cast<CNEO_Player *>(m_Occupants[i].hPlayer.Get());
		if (pPlayer && pPlayer->IsAlive())
			continue;

		if (m_Occupants[i].team == TEAM_JINRAI)
			m_iJinraiCount--;
		else if (m_Occupants[i].team == TEAM_NSF)
			m_iNSFCount--;

		m_Occupants.Remove(i);
		bChanged = true;
	}

	if (bChanged)
		UpdateState();
}

void CNEO_InfoKOTHZone::UpdateState()
{
	KothControllingTeams newState;
	if (m_iJinraiCount > 0 && m_iNSFCount > 0)
		newState = KOTH_BOTH;
	else if (m_iJinraiCount > 0)
		newState = KOTH_JINRAI;
	else if (m_iNSFCount > 0)
		newState = KOTH_NSF;
	else
		newState = KOTH_NONE;

	if (newState == m_State)
		return;

	m_State = newState;

	// neo TODO: notify neo_koth_master / CNEORules once that layer exists
}
