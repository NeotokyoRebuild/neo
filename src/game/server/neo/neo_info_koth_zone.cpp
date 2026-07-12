#include "neo_info_koth_zone.h"
#include "neo_trigger_koth_zone.h"
#include "neo_koth_master.h"
#include "neo_koth_border.h"
#include "neo_player.h"
#include "neo_player_shared.h"

#define KOTHZONE_PRUNE_INTERVAL 0.5f
#define KOTHZONE_SCORE_CONTEXT "KothZoneScoreThink"
#define KOTHZONE_SCORE_INTERVAL 0.05f

#define KOTHZONE_SPRITE_NONE   "vgui/hud/cp/cp_none.vmt"
#define KOTHZONE_SPRITE_JINRAI "vgui/hud/cp/cp_jinrai.vmt"
#define KOTHZONE_SPRITE_NSF    "vgui/hud/cp/cp_nsf.vmt"
// neo TODO: no dedicated "contested" sprite yet - falls back to cp_none

static const char *KothZoneSpriteForState(KothControllingTeams team)
{
	switch (team)
	{
	case KOTH_JINRAI:
		return KOTHZONE_SPRITE_JINRAI;
	case KOTH_NSF:
		return KOTHZONE_SPRITE_NSF;
	case KOTH_NONE:
	case KOTH_BOTH:
	default:
		return KOTHZONE_SPRITE_NONE;
	}
}

BEGIN_DATADESC(CNEO_InfoKOTHZone)
	DEFINE_THINKFUNC(Think),
	DEFINE_THINKFUNC(ScoreThink),
END_DATADESC()

void CNEO_InfoKOTHZone::Spawn()
{
	SetModelName(MAKE_STRING(KothZoneSpriteForState(m_State)));

	BaseClass::Spawn();

	SetSpriteScale(0.5f);

	SetThink(&CNEO_InfoKOTHZone::Think);
	SetNextThink(gpGlobals->curtime + KOTHZONE_PRUNE_INTERVAL);

	SetContextThink(&CNEO_InfoKOTHZone::ScoreThink, gpGlobals->curtime + KOTHZONE_SCORE_INTERVAL, KOTHZONE_SCORE_CONTEXT);
}

void CNEO_InfoKOTHZone::Precache()
{
	BaseClass::Precache();

	PrecacheModel(KOTHZONE_SPRITE_NONE);
	PrecacheModel(KOTHZONE_SPRITE_JINRAI);
	PrecacheModel(KOTHZONE_SPRITE_NSF);
}

void CNEO_InfoKOTHZone::Activate()
{
	BaseClass::Activate();

	CNEO_KOTHMaster *pMaster = static_cast<CNEO_KOTHMaster *>( // NOLINT
		gEntList.FindEntityByClassname(nullptr, "neo_koth_master")
	);

	// neo TODO: do something when we see that there are several masters
	if (pMaster == nullptr)
		Warning("neo_info_koth_zone found with no neo_koth_master on the map!\n");
	else
		pMaster->RegisterZone(this);
}

void CNEO_InfoKOTHZone::Think()
{
	PruneStaleCaptors();
	SetNextThink(gpGlobals->curtime + KOTHZONE_PRUNE_INTERVAL);
}

void CNEO_InfoKOTHZone::ScoreThink()
{
	if (!NEORules()->IsRoundLive())
	{
		SetNextThink(gpGlobals->curtime + KOTHZONE_SCORE_INTERVAL, KOTHZONE_SCORE_CONTEXT);
		return;
	}

	switch (m_State)
	{
	case KOTH_JINRAI:
		m_flAccumulatorJinrai += KOTHZONE_SCORE_INTERVAL;
		m_flAccumulatorNSF = 0.0f;
		break;
	case KOTH_NSF:
		m_flAccumulatorNSF += KOTHZONE_SCORE_INTERVAL;
		m_flAccumulatorJinrai = 0.0f;
		break;
	default:
		// KOTH_NONE (empty) or KOTH_BOTH (contested) - nobody accumulates
		m_flAccumulatorNSF = 0.0f;
		m_flAccumulatorJinrai = 0.0f;
		break;
	}

	const float flSecondsPerPoint = sv_neo_koth_seconds_per_point.GetFloat();
	if (m_flAccumulatorJinrai >= flSecondsPerPoint)
	{
		const int points = int(m_flAccumulatorJinrai / flSecondsPerPoint);
		m_flAccumulatorJinrai -= flSecondsPerPoint * points;
		NEORules()->AddKothScore(TEAM_JINRAI, points);
	}
	if (m_flAccumulatorNSF >= flSecondsPerPoint)
	{
		const int points = int(m_flAccumulatorNSF / flSecondsPerPoint);
		m_flAccumulatorNSF -= flSecondsPerPoint * points;
		NEORules()->AddKothScore(TEAM_NSF, points);
	}

	SetNextThink(gpGlobals->curtime + KOTHZONE_SCORE_INTERVAL, KOTHZONE_SCORE_CONTEXT);
}

void CNEO_InfoKOTHZone::OnPlayerEnter(CNEO_Player *pPlayer)
{
	if (!pPlayer)
		return;

	EHANDLE hPlayer = pPlayer;
	for (int i = 0; i < m_Captors.Count(); ++i)
	{
		if (m_Captors[i].hPlayer == hPlayer)
		{
			// already standing in another trigger of this same zone
			m_Captors[i].touchCount++;
			return;
		}
	}

	ZoneCaptor captor;
	captor.hPlayer = hPlayer;
	// if they're somehow dead on entry, don't credit either team
	captor.team = pPlayer->IsAlive() ? pPlayer->GetTeamNumber() : TEAM_UNASSIGNED;
	captor.touchCount = 1;
	m_Captors.AddToTail(captor);

	if (captor.team == TEAM_JINRAI)
		m_iJinraiCount++;
	else if (captor.team == TEAM_NSF)
		m_iNSFCount++;

	UpdateState();
}

void CNEO_InfoKOTHZone::OnPlayerLeave(CNEO_Player *pPlayer)
{
	if (!pPlayer)
		return;

	EHANDLE hPlayer = pPlayer;
	for (int i = 0; i < m_Captors.Count(); ++i)
	{
		if (m_Captors[i].hPlayer != hPlayer)
			continue;

		// still touching another trigger belonging to this zone
		if (--m_Captors[i].touchCount > 0)
			return;

		if (m_Captors[i].team == TEAM_JINRAI)
			m_iJinraiCount--;
		else if (m_Captors[i].team == TEAM_NSF)
			m_iNSFCount--;

		m_Captors.Remove(i);
		UpdateState();
		return;
	}
}

void CNEO_InfoKOTHZone::AddChildTrigger(CNEO_TriggerKOTHZone *pTrigger)
{
	if (!pTrigger)
		return;

	// avoid double activation
	if (m_ChildTriggers.Find(pTrigger) != m_ChildTriggers.InvalidIndex())
		return;

	m_ChildTriggers.AddToTail(pTrigger);
}

void CNEO_InfoKOTHZone::AddChildBorder(CNEO_KOTHBorder *pBorder)
{
	if (!pBorder)
		return;

	// avoid double activation
	if (m_ChildBorders.Find(pBorder) != m_ChildBorders.InvalidIndex())
		return;

	m_ChildBorders.AddToTail(pBorder);
	pBorder->SetZoneColor(m_State);
}

void CNEO_InfoKOTHZone::SetActivity(bool bActive)
{
	if (bActive == m_bActive)
		return;

	m_bActive = bActive;

	if (m_bActive)
		TurnOn();
	else
		TurnOff();

	for (int i = 0; i < m_ChildTriggers.Count(); ++i)
	{
		CNEO_TriggerKOTHZone *pTrigger = m_ChildTriggers[i].Get();
		if (!pTrigger)
			continue;

		if (m_bActive)
			pTrigger->Enable();
		else
			pTrigger->Disable();
	}

	for (int i = 0; i < m_ChildBorders.Count(); ++i)
	{
		CNEO_KOTHBorder *pBorder = m_ChildBorders[i].Get();
		if (pBorder)
			pBorder->SetBorderVisible(bActive);
	}
}

void CNEO_InfoKOTHZone::ResetCapture()
{
	m_Captors.Purge();
	m_iJinraiCount = 0;
	m_iNSFCount = 0;
	// neo TODO: it could lead to an incorrect result if players somehow spawn right inside the trigger, but who cares?
	m_State = KOTH_NONE;
	m_flAccumulatorNSF = 0.0f;
	m_flAccumulatorJinrai = 0.0f;
}

void CNEO_InfoKOTHZone::PruneStaleCaptors()
{
	bool bChanged = false;

	// EndTouch won't reliably fire for players who disconnected or died without
	// physically leaving the trigger volume (e.g. respawning elsewhere) - catch those here
	for (int i = m_Captors.Count() - 1; i >= 0; --i)
	{
		CNEO_Player *pPlayer = static_cast<CNEO_Player *>(m_Captors[i].hPlayer.Get());
		if (pPlayer && pPlayer->IsAlive())
			continue;

		if (m_Captors[i].team == TEAM_JINRAI)
			m_iJinraiCount--;
		else if (m_Captors[i].team == TEAM_NSF)
			m_iNSFCount--;

		m_Captors.Remove(i);
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

	for (int i = 0; i < m_ChildBorders.Count(); ++i)
	{
		CNEO_KOTHBorder *pBorder = m_ChildBorders[i].Get();
		if (pBorder)
			pBorder->SetZoneColor(newState);
	}

	SetModel(KothZoneSpriteForState(newState));

	m_State = newState;
}
