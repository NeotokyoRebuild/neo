#pragma once
#include "cbase.h"


class CNEO_InfoKOTHZone : public CPointEntity
{
public:
	DECLARE_CLASS(CNEO_InfoKOTHZone, CPointEntity);
	DECLARE_DATADESC();

	void UpdateState();
};

LINK_ENTITY_TO_CLASS(neo_info_koth_zone, CNEO_InfoKOTHZone);