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

#ifdef GAME_DLL
	void SetActive(bool isActive);
	bool GetActive();
	void ResetCaptureState()
	{
		m_bGhostHasBeenCaptured = false;
		m_iSuccessfulCaptorClientIndex = 0;
	}

	int UpdateTransmitState() OVERRIDE;

	bool IsGhostCaptured(int &outTeamNumber, int &outCaptorClientIndex);

private:
	void InputEnable(inputdata_t &inputData);
	void InputDisable(inputdata_t &inputData);

	COutputEvent m_OnCap;

public:
	void Think_CheckMyRadius(void); // NEO FIXME (Rain): this should be private

	bool m_bStartDisabled = false;

private:
	CNetworkVar(int, m_iOwningTeam);
	CNetworkVar(float, m_flCapzoneRadius);
	CNetworkVar(bool, m_bIsActive);

	int m_iSuccessfulCaptorClientIndex = 0;
	bool m_bGhostHasBeenCaptured = false;
#else
	int m_iOwningTeam;
	float m_flCapzoneRadius;
	bool m_bIsActive;

	CNEOHud_GhostCapPoint *m_pHUDCapPoint = nullptr;
#endif
};

#endif // NEO_GHOST_CAP_POINT_H
