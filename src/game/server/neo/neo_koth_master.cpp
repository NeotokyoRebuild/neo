#include "neo_koth_master.h"
#include "neo_info_koth_zone.h"

BEGIN_DATADESC(CNEO_KOTHMaster)
	DEFINE_THINKFUNC(SwitchThink),
	DEFINE_THINKFUNC(OpenPendingZoneThink),
END_DATADESC()

void CNEO_KOTHMaster::Spawn()
{
	BaseClass::Spawn();

	SetThink(&CNEO_KOTHMaster::SwitchThink);
	// preparation time including freeze time
	SetNextThink(gpGlobals->curtime + NEORules()->GetRemainingPreRoundFreezeTime(true) + sv_neo_koth_zone_prep_time.GetFloat());
}

void CNEO_KOTHMaster::SwitchThink()
{
	if (m_Zones.Size() <= 1) {
		return;
	}

	CNEO_InfoKOTHZone *pOldZone = m_hActiveZone.Get();
	if (pOldZone)
	{
		pOldZone->SetActivity(false);
		UTIL_CenterPrintAll("CONTROL POINT IS INACTIVE. AWAIT FURTHER INSTRUCTIONS.");
	}

	m_hPendingZone = PickNextZone(pOldZone);
	m_hActiveZone = nullptr;

	SetThink(&CNEO_KOTHMaster::OpenPendingZoneThink);
	SetNextThink(gpGlobals->curtime + sv_neo_koth_zone_pause_time.GetFloat());
}

void CNEO_KOTHMaster::OpenPendingZoneThink()
{
	CNEO_InfoKOTHZone *pNewZone = m_hPendingZone.Get();
	if (pNewZone)
	{
		pNewZone->SetActivity(true);
		UTIL_CenterPrintAll("CAPTURE NEW ZONE.");
	}
	m_hActiveZone = pNewZone;
	m_hPendingZone = nullptr;

	SetThink(&CNEO_KOTHMaster::SwitchThink);
	SetNextThink(gpGlobals->curtime + sv_neo_koth_zone_switch_time.GetFloat());
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
}
