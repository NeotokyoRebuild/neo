#include "neo_koth_master.h"
#include "neo_info_koth_zone.h"

BEGIN_DATADESC(CNEO_KOTHMaster)
	DEFINE_THINKFUNC(Think),
END_DATADESC()

void CNEO_KOTHMaster::Spawn()
{
	BaseClass::Spawn();

	SetThink(&CNEO_KOTHMaster::Think);
	// preparation time including freeze time
	SetNextThink(gpGlobals->curtime + NEORules()->GetRemainingPreRoundFreezeTime(true) + sv_neo_koth_zone_prep_time.GetFloat());
}

void CNEO_KOTHMaster::Think()
{
	SwitchZone();
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

void CNEO_KOTHMaster::SwitchZone()
{
	if (m_Zones.IsEmpty())
		return;

	CNEO_InfoKOTHZone *pOldZone = m_hActiveZone.Get();

	int idx = RandomInt(0, m_Zones.Count() - 1);
	if (m_Zones.Count() > 1 && m_Zones[idx].Get() == pOldZone)
		idx = (idx + 1) % m_Zones.Count();

	CNEO_InfoKOTHZone *pNewZone = m_Zones[idx].Get();

	if (pOldZone && pOldZone != pNewZone)
		pOldZone->SetActivity(false);

	if (pNewZone)
		pNewZone->SetActivity(true);

	m_hActiveZone = pNewZone;
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
