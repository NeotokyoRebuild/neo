#ifndef NEO_GHOST_CAP_POINT_H
#define NEO_GHOST_CAP_POINT_H
#ifdef _WIN32
#pragma once
#endif

#include "baseentity_shared.h"
#include "baseplayer_shared.h"
#include "neo_gamerules.h"

#ifdef GAME_DLL
#include "entityoutput.h"
#endif

#ifdef CLIENT_DLL
#include "iclientmode.h"
#include "hudelement.h"
#include <vgui/ISurface.h>
#include <vgui_controls/Controls.h>
#include <vgui_controls/Panel.h>

#include <vgui/ILocalize.h>
#include "tier3/tier3.h"
#include "vphysics_interface.h"
#include "c_neo_player.h"
#include "ienginevgui.h"
#endif

#ifdef CLIENT_DLL
#define CNEOGhostCapturePoint C_NEOGhostCapturePoint
class CNEOHud_GhostCapPoint;
#endif

class CNEOGhostCapturePoint : public CBaseEntity
{
	DECLARE_CLASS(CNEOGhostCapturePoint, CBaseEntity);
	//DECLARE_NETWORKCLASS();

public:
#ifdef CLIENT_DLL
	DECLARE_CLIENTCLASS();
	//DECLARE_PREDICTABLE();
#else
	DECLARE_SERVERCLASS();
#endif
	DECLARE_DATADESC();

	CNEOGhostCapturePoint();
	virtual ~CNEOGhostCapturePoint();
	
	virtual void Precache(void);
	virtual void Spawn(void);

	int owningTeamAlternate() const;

#ifdef CLIENT_DLL
	virtual void ClientThink(void);
#endif

	void SetActive(bool isActive);
	void ResetCaptureState()
	{
		m_bGhostHasBeenCaptured = false;
		m_iSuccessfulCaptorClientIndex = 0;
	}

#ifdef GAME_DLL
	int UpdateTransmitState() OVERRIDE;

	bool IsGhostCaptured(int &outTeamNumber, int &outCaptorClientIndex);

private:
	COutputEvent m_OnCap;
#endif

#ifdef GAME_DLL
public:
	void Think_CheckMyRadius(void); // NEO FIXME (Rain): this should be private
#endif

private:
	inline void UpdateVisibility(void);

private:
#ifdef GAME_DLL
	CNetworkVar(int, m_iOwningTeam);
	CNetworkVar(int, m_iSuccessfulCaptorClientIndex);

	CNetworkVar(float, m_flCapzoneRadius);

	CNetworkVar(bool, m_bGhostHasBeenCaptured);
	CNetworkVar(bool, m_bIsActive);
#else
	int m_iOwningTeam;
	int m_iSuccessfulCaptorClientIndex;

	float m_flCapzoneRadius;

	bool m_bGhostHasBeenCaptured;
	bool m_bIsActive;
#endif

#ifdef CLIENT_DLL
	CNEOHud_GhostCapPoint *m_pHUDCapPoint = nullptr;
#endif
};

#endif // NEO_GHOST_CAP_POINT_H
