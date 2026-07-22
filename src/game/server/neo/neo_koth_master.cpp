#include "neo_koth_master.h"
#include "neo_info_koth_zone.h"

LINK_ENTITY_TO_CLASS(neo_koth_master, CNEO_KOTHMaster);

BEGIN_DATADESC(CNEO_KOTHMaster)
	DEFINE_THINKFUNC(RevealNextZoneThink),
	DEFINE_THINKFUNC(UnlockZoneThink),
	DEFINE_THINKFUNC(CloseZoneThink),
END_DATADESC()

void CNEO_KOTHMaster::Spawn()
{
	BaseClass::Spawn();

	// not called directly cuz we need to cancel our previous reveal/unlock/close calls
	SetThink(&CNEO_KOTHMaster::RevealNextZoneThink);
	SetNextThink(gpGlobals->curtime);
}

void CNEO_KOTHMaster::RevealNextZoneThink()
{
	// round isn't live (still in freeze/warmup, or intermission after a round already
	// ended) - the whole cycle is meaningless right now, just wait and check again
	if (!NEORules()->IsRoundLive())
	{
		SetNextThink(gpGlobals->curtime + 0.1f);
		return;
	}

	m_hPendingZone = PickNextZone(m_hActiveZone.Get());

	CNEO_InfoKOTHZone *pNextZone = m_hPendingZone.Get();
	if (pNextZone)
		pNextZone->SetVisible(true);

	SetThink(&CNEO_KOTHMaster::UnlockZoneThink);
	SetNextThink(gpGlobals->curtime + sv_neo_koth_zone_pause_time.GetFloat());
}

void CNEO_KOTHMaster::UnlockZoneThink()
{
	if (!NEORules()->IsRoundLive())
	{
		SetNextThink(gpGlobals->curtime + 0.1f);
		return;
	}

	CNEO_InfoKOTHZone *pZone = m_hPendingZone.Get();
	if (pZone)
	{
		pZone->SetCapturable(true);
		UTIL_CenterPrintAll("CAPTURE NEW OBJECTIVE.");
	}
	m_hActiveZone = pZone;
	m_hPendingZone = nullptr;
	// do not switch zones if we have only one
	if (m_Zones.Size() > 1) {
		SetThink(&CNEO_KOTHMaster::CloseZoneThink);
		SetNextThink(gpGlobals->curtime + sv_neo_koth_zone_switch_time.GetFloat());
	}
}

void CNEO_KOTHMaster::CloseZoneThink()
{
	if (!NEORules()->IsRoundLive())
	{
		SetNextThink(gpGlobals->curtime + 0.1f);
		return;
	}

	CNEO_InfoKOTHZone *pOldZone = m_hActiveZone.Get();
	if (pOldZone)
	{
		pOldZone->SetCapturable(false);
		pOldZone->SetVisible(false);
		UTIL_CenterPrintAll("NEW OBJECTIVE FOUND. GET READY.");
	}
	m_hActiveZone = nullptr;

	RevealNextZoneThink();
}

void CNEO_KOTHMaster::RegisterZone(CNEO_InfoKOTHZone *pZone)
{
	if (!pZone)
		return;

	// avoid double activation
	if (m_Zones.Find(pZone) != m_Zones.InvalidIndex())
		return;

	m_Zones.AddToTail(pZone);
}

CNEO_InfoKOTHZone *CNEO_KOTHMaster::PickNextZone(CNEO_InfoKOTHZone *pOldZone) const
{
	if (m_Zones.IsEmpty())
		return nullptr;

	int idx = RandomInt(0, m_Zones.Count() - 1);
	if (m_Zones.Count() > 1 && m_Zones[idx].Get() == pOldZone)
		idx = (idx + 1) % m_Zones.Count();

	return m_Zones[idx].Get();
}

void CNEO_KOTHMaster::ResetAllZones()
{
	for (int i = 0; i < m_Zones.Count(); ++i)
	{
		CNEO_InfoKOTHZone *pZone = m_Zones[i].Get();
		if (pZone)
			pZone->ResetCapture();
	}

	// force-close whatever was visible/active and restart the reveal cycle fresh for the new round
	CNEO_InfoKOTHZone *pActive = m_hActiveZone.Get();
	if (pActive)
	{
		pActive->SetCapturable(false);
		pActive->SetVisible(false);
	}

	CNEO_InfoKOTHZone *pPending = m_hPendingZone.Get();
	if (pPending && pPending != pActive)
	{
		pPending->SetCapturable(false);
		pPending->SetVisible(false);
	}

	m_hActiveZone = nullptr;
	m_hPendingZone = nullptr;

	SetThink(&CNEO_KOTHMaster::RevealNextZoneThink);
	SetNextThink(gpGlobals->curtime + NEORules()->GetRemainingPreRoundFreezeTime(true));
}
