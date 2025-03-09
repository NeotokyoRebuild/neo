#ifndef NEO_HUD_TARGET_ID
#define NEO_HUD_TARGET_ID
#ifdef _WIN32
#pragma once
#endif
//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: HUD Target ID element
//
// $NoKeywords: $
//=============================================================================//
#include "cbase.h"
#include "hud.h"
#include "hudelement.h"
#include "c_hl2mp_player.h"
#include "c_playerresource.h"
#include "vgui_entitypanel.h"
#include "iclientmode.h"
#include "vgui/ILocalize.h"
#include "hl2mp_gamerules.h"

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

#define PLAYER_HINT_DISTANCE	150
#define PLAYER_HINT_DISTANCE_SQ	(PLAYER_HINT_DISTANCE*PLAYER_HINT_DISTANCE)

static ConVar hud_centerid("hud_centerid", "1");
static ConVar hud_showtargetid("hud_showtargetid", "1");

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
class CTargetID : public CHudElement, public vgui::Panel
{
	DECLARE_CLASS_SIMPLE(CTargetID, vgui::Panel);

public:
	CTargetID(const char* pElementName);
	void Init(void);
	virtual void	ApplySchemeSettings(vgui::IScheme* scheme);
	virtual void	Paint(void);
	void VidInit(void);

private:
	Color			GetColorForTargetTeam(int iTeamNumber);

	vgui::HFont		m_hFont;
	int				m_iLastEntIndex;
	float			m_flLastChangeTime;
};

#endif //NEO_HUD_TARGET_ID