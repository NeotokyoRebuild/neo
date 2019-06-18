#ifndef NEO_GHOST_CAP_POINT_H
#define NEO_GHOST_CAP_POINT_H
#ifdef _WIN32
#pragma once
#endif

#include "baseentity_shared.h"
#include "baseplayer_shared.h"
#ifdef GAME_DLL
#include "entityoutput.h"
#endif

#ifdef CLIENT_DLL
#define CNEOGhostCapturePoint C_NEOGhostCapturePoint
#endif

class CNEOGhostCapturePoint : public CBaseEntity
{
	DECLARE_CLASS(CNEOGhostCapturePoint, CBaseEntity);

public:
	DECLARE_NETWORKCLASS();
#if(0)
	DECLARE_PREDICTABLE();
#endif
	DECLARE_DATADESC();

	CNEOGhostCapturePoint();

	virtual void Precache(void);
	virtual void Spawn(void);

#ifdef CLIENT_DLL
	virtual void ClientThink(void);
#endif

	void SetActive(bool isActive);

#ifdef GAME_DLL
	int ShouldTransmit(const CCheckTransmitInfo *pInfo) { return EF_BRIGHTLIGHT; }
#endif

private:
#ifdef GAME_DLL
	void Think_CheckMyRadius(void);
#endif

	inline void UpdateVisibility(void);

private:
	bool m_bIsActive;
	bool m_bGhostHasBeenCaptured;

	int m_iOwningTeam;
	int m_iSuccessfulCaptorClientIndex;

	float m_flCapzoneRadius;

};

#endif // NEO_GHOST_CAP_POINT_H