#pragma once
#include "cbase.h"
#include "triggers.h"

class CNEO_TriggerWeapon : public CBaseTrigger
{
public:
	DECLARE_CLASS(CNEO_TriggerWeapon, CBaseTrigger);
	DECLARE_DATADESC();

	virtual void Spawn() override;
	virtual void Think() override;

private:
	void CheckForWeapon();
	bool IsEntityInside(CBaseEntity* pEnt);
};

class CNEO_GhostBoundary : public CNEO_TriggerWeapon
{
public:
	DECLARE_CLASS(CNEO_GhostBoundary, CNEO_TriggerWeapon);
	DECLARE_DATADESC();

	virtual void Think() override;
	virtual void StartTouch(CBaseEntity *pOther) override;

private:
	Vector		m_vecLastObjectivePos = vec3_origin;
};
