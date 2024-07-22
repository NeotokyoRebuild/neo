#ifndef NEO_HUD_GHOST_MARKER_H
#define NEO_HUD_GHOST_MARKER_H
#ifdef _WIN32
#pragma once
#endif

#include "neo_hud_childelement.h"
#include "hudelement.h"
#include <vgui_controls/Panel.h>

#include "neo_hud_worldpos_marker.h"

class CNEOHud_GhostMarker : public CNEOHud_WorldPosMarker
{
	DECLARE_CLASS_SIMPLE(CNEOHud_GhostMarker, CNEOHud_WorldPosMarker)

public:
	CNEOHud_GhostMarker(const char *pElemName, vgui::Panel *parent = NULL);

	virtual void ApplySchemeSettings(vgui::IScheme *pScheme) override;
	virtual void Paint() override;

	void SetGhostingTeam(int team);
	void SetClientCurrentTeam(int team);
	void SetScreenPosition(int x, int y);
	void SetGhostDistance(float distance);

protected:
	virtual void UpdateStateForNeoHudElementDraw() override;
	virtual void DrawNeoHudElement() override;
	virtual ConVar* GetUpdateFrequencyConVar() const override;

private:
	float m_fMarkerScalesStart[4] = { 0.78f, 0.6f, 0.38f, 0.0f };
	float m_fMarkerScalesCurrent[4] = { 0.78f, 0.6f, 0.38f, 0.0f };
	int m_iMarkerTexWidth, m_iMarkerTexHeight;
	int m_iPosX, m_iPosY;
	int m_iGhostingTeam;
	int m_iClientTeam;

	char m_szMarkerText[32 + 1];
	wchar_t m_wszMarkerTextUnicode[32 + 1];

	float m_flDistMeters;

	vgui::HTexture m_hTex = 0U;

	vgui::HFont m_hFont = 0U;
};

#endif // NEO_HUD_GHOST_MARKER_H
