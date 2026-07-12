#pragma once
#include "cbase.h"
#include "modelentities.h"
#include "neo_gamerules.h"

class CNEO_InfoKOTHZone;

class CNEO_KOTHBorder : public CFuncBrush
{
public:
	DECLARE_CLASS(CNEO_KOTHBorder, CFuncBrush);
	DECLARE_DATADESC();

	virtual void Spawn() override;
	virtual void Activate() override;

	// called by neo_info_koth_zone
	void SetZoneColor(KothControllingTeams team);
	void SetBorderVisible(bool bVisible);

	string_t m_iszZoneName;
private:
	CNEO_InfoKOTHZone *pZone = nullptr;
};

LINK_ENTITY_TO_CLASS(neo_func_koth_border, CNEO_KOTHBorder);
