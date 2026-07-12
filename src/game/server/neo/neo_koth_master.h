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
	// closes the current zonE
	void SwitchThink();
	// opens...
	void OpenPendingZoneThink();

	// called by every neo_info_koth_zone on the map once it has found us
	void RegisterZone(CNEO_InfoKOTHZone *pZone);

	void ResetAllZones();

private:
	CNEO_InfoKOTHZone *PickNextZone(CNEO_InfoKOTHZone *pOldZone) const;

	CUtlVector<CHandle<CNEO_InfoKOTHZone>> m_Zones;
	CHandle<CNEO_InfoKOTHZone> m_hActiveZone;
	CHandle<CNEO_InfoKOTHZone> m_hPendingZone;
};

LINK_ENTITY_TO_CLASS(neo_koth_master, CNEO_KOTHMaster);
