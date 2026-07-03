#pragma once
#include "cbase.h"
#include "triggers.h"
#include "neo_info_koth_zone.h"

class CNEO_TriggerKOTHZone : public CBaseTrigger
{
public:
	DECLARE_CLASS(CNEO_TriggerKOTHZone, CBaseTrigger);
	DECLARE_DATADESC();

	virtual void Spawn() override;
	virtual void Activate() override;
	virtual void StartTouch(CBaseEntity *pOther) override;
	virtual void EndTouch(CBaseEntity *pOther) override;

	string_t m_iszZoneName;
private:
	CNEO_InfoKOTHZone *pZone = nullptr;
};
