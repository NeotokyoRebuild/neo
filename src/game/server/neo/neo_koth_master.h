#pragma once
#include "cbase.h"
#include "utlvector.h"

class CNEO_InfoKOTHZone;

class CNEO_KOTHMaster : public CLogicalEntity
{
public:
	DECLARE_CLASS( CNEO_KOTHMaster, CLogicalEntity );
	DECLARE_DATADESC();

	virtual void Spawn() override;
	// picks the next zone and makes it visible, but still locked (not capturable); schedules UnlockZoneThink
	void RevealNextZoneThink();
	// makes the revealed zone capturable (this is now the active zone); schedules CloseZoneThink
	void UnlockZoneThink();
	// closes the active zone (hides it, no longer capturable), then immediately reveals the next one
	void CloseZoneThink();

	// called by every neo_info_koth_zone on the map once it has found us
	void RegisterZone(CNEO_InfoKOTHZone *pZone);

	void ResetAllZones();

private:
	CNEO_InfoKOTHZone *PickNextZone(CNEO_InfoKOTHZone *pOldZone) const;

	CUtlVector<CHandle<CNEO_InfoKOTHZone>> m_Zones;
	CHandle<CNEO_InfoKOTHZone> m_hActiveZone;
	CHandle<CNEO_InfoKOTHZone> m_hPendingZone;
};
