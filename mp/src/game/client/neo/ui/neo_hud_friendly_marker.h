#ifndef NEO_HUD_FRIENDLY_MARKER_H
#define NEO_HUD_FRIENDLY_MARKER_H
#ifdef _WIN32
#pragma once
#endif

#include "neo_hud_childelement.h"
#include "hudelement.h"
#include <vgui_controls/Panel.h>

#include "c_neo_player.h"
#include "neo_gamerules.h"

class CNEOHud_FriendlyMarker : public CNEOHud_ChildElement, public CHudElement, public vgui::Panel
{
	DECLARE_CLASS_SIMPLE(CNEOHud_FriendlyMarker, Panel);

public:
	CNEOHud_FriendlyMarker(const char *pElemName, vgui::Panel *parent = NULL);

	virtual void Paint();

protected:
	virtual void UpdateStateForNeoHudElementDraw() override;
	virtual void DrawNeoHudElement() override;
	virtual ConVar* GetUpdateFrequencyConVar() const override;

private:
	int m_iMarkerTexWidth, m_iMarkerTexHeight;
	int m_iPosX, m_iPosY;

	int m_x0[MAX_PLAYERS];
	int m_x1[MAX_PLAYERS];
	int m_y0[MAX_PLAYERS];
	int m_y1[MAX_PLAYERS];

	vgui::HTexture m_hTex;
	vgui::HFont m_hFont;
};

#endif // NEO_HUD_FRIENDLY_MARKER_H