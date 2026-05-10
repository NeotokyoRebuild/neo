#pragma once
#include "cbase.h"


class CNEO_KOTHMaster : public CLogicalEntity
{
public:
	DECLARE_CLASS( CNEO_KOTHMaster, CLogicalEntity );
	DECLARE_DATADESC();

	virtual void Spawn() override;
	void SwitchZone();
	void UpdateState();
};