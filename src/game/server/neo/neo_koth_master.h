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
	void Think();

	// called by every neo_info_koth_zone on the map once it has found us
	void RegisterZone(CNEO_InfoKOTHZone *pZone);

	// switcch zone, always new if possible
	void SwitchZone();
	// runs on round restart
	void ResetAllZones();

private:
	CUtlVector<CHandle<CNEO_InfoKOTHZone>> m_Zones;
	CHandle<CNEO_InfoKOTHZone> m_hActiveZone;
};

LINK_ENTITY_TO_CLASS(neo_koth_master, CNEO_KOTHMaster);
