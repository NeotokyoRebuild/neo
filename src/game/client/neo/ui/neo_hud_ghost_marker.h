#ifndef NEO_HUD_GHOST_MARKER_H
#define NEO_HUD_GHOST_MARKER_H
#ifdef _WIN32
#pragma once
#endif

#include "neo_hud_childelement.h"
#include "hudelement.h"
#include <vgui_controls/Panel.h>

#include "neo_hud_worldpos_marker.h"

#include "neo_juggernaut.h"

class C_WeaponGhost;

class CNEOHud_GhostMarker : public CNEOHud_WorldPosMarker
{
	DECLARE_CLASS_SIMPLE(CNEOHud_GhostMarker, CNEOHud_WorldPosMarker)

public:
	CNEOHud_GhostMarker(const char *pElemName, vgui::Panel *parent = NULL);

	virtual void ApplySchemeSettings(vgui::IScheme *pScheme) override;
	virtual void Paint() override;
	virtual void resetHUDState() override;

protected:
	virtual void UpdateStateForNeoHudElementDraw() override;
	virtual void DrawNeoHudElement() override;
	virtual ConVar *GetUpdateFrequencyConVar() const override;

private:
	C_WeaponGhost *m_ghostInPVS = nullptr;
	CNEO_Juggernaut *m_jgrInPVS = nullptr;
	float m_fMarkerScalesStart[4] = { 0.78f, 0.6f, 0.38f, 0.0f };
	float m_fMarkerScalesCurrent[4] = { 0.78f, 0.6f, 0.38f, 0.0f };

	wchar_t m_wszMarkerTextUnicode[32 + 1];

	vgui::HTexture m_hTex = 0UL;
	vgui::HFont m_hFont = 0UL;
};

#endif // NEO_HUD_GHOST_MARKER_H
