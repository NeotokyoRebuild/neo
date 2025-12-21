#ifndef NEO_NPC_TARGETSYSTEM_H
#define NEO_NPC_TARGETSYSTEM_H
#ifdef _WIN32
#pragma once
#endif

#include "cbase.h"
#include "baseentity.h"
#include "filters.h"

class CNEO_NPCTargetSystem : public CBaseEntity
{
public:
	DECLARE_CLASS(CNEO_NPCTargetSystem, CBaseEntity);
	DECLARE_DATADESC();

private:
	// KVs
	float m_flFOV = 90.0f;
	float m_flTopClip = 60.0f;
	float m_flBottomClip = -100.0f;
	float m_flMaxViewDistance = 800.0f;
	float m_flDeadzone = 100.0f;
	float m_flMiddleBoundsHalf = 20.0f;
	string_t m_iFilterName = NULL_STRING;
	bool m_bStartDisabled = false;
	bool m_bMotionVision = true;
	// Damage trace
	string_t m_strDamageSourceName = NULL_STRING;
	float m_flDamage = 10.0f;
	float m_flFireRate = 1.0f;

	CBaseFilter *m_pFilter = nullptr;
	EHANDLE m_hDamageSource = nullptr;

	COutputEvent m_OnSpotted;
	COutputEvent m_OnRight;
	COutputEvent m_OnLeft;
	COutputEvent m_OnMiddle;
	COutputEvent m_OnMiddleIgnore;
	COutputEvent m_OnExit;
	COutputEvent m_OnExitMiddle;

	void InputEnable(inputdata_t &inputData);
	void InputDisable(inputdata_t &inputData);

public:
	void Spawn();
	void Think();
	
	bool IsHostileTo( CBaseEntity *pEntity );

private:
	enum TargetZone_e
	{
		ZONE_NONE = 0,
		ZONE_LEFT,
		ZONE_MIDDLE,
		ZONE_RIGHT
	};

	TargetZone_e m_iLastZone = ZONE_NONE;
	bool m_bTargetAcquired = false;
	bool m_bMiddleIgnoreActive = false;
	float m_flNextFireTime = 0;
	CBasePlayer *m_pLastBestTarget = nullptr;
};

#endif // NEO_NPC_TARGETSYSTEM_H
