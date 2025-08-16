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
	void CheckForWeapon();

private:
	bool IsEntityInside(CBaseEntity* pEnt);
};

class CNEO_GhostBoundry : public CNEO_TriggerWeapon
{
public:
	DECLARE_CLASS(CNEO_GhostBoundry, CNEO_TriggerWeapon);
	DECLARE_DATADESC();

	virtual void Think() override;
	virtual void StartTouch(CBaseEntity *pOther) override;

	Vector		m_vecLastGhosterPos = vec3_origin;
};
