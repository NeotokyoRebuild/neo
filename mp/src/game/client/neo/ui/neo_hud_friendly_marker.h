#ifndef NEO_HUD_FRIENDLY_MARKER_H
#define NEO_HUD_FRIENDLY_MARKER_H
#ifdef _WIN32
#pragma once
#endif

#include "hudelement.h"
#include <vgui_controls/Panel.h>

#include "neo_gamerules.h"
#include "neo_hud_worldpos_marker.h"

class CNEOHud_FriendlyMarker : public CNEOHud_WorldPosMarker
{
	DECLARE_CLASS_SIMPLE(CNEOHud_FriendlyMarker, CNEOHud_WorldPosMarker)

public:
	CNEOHud_FriendlyMarker(const char *pElemName, vgui::Panel *parent = NULL);
	
	virtual void ApplySchemeSettings(vgui::IScheme *pScheme) override;
	virtual void Paint() override;

protected:
	virtual void DrawNeoHudElement() override;
	void DrawPlayerForTeam(C_Team *team, const C_NEO_Player *localPlayer, const C_NEO_Player *pTargetPlayer) const;
	virtual ConVar* GetUpdateFrequencyConVar() const override;
	virtual void UpdateStateForNeoHudElementDraw() override;

private:
	int m_iMarkerTexWidth, m_iMarkerTexHeight;
	int m_iMarkerWidth, m_iMarkerHeight;
	bool m_IsSpectator;

	int m_x0[MAX_PLAYERS];
	int m_x1[MAX_PLAYERS];
	int m_y0[MAX_PLAYERS];
	int m_y1[MAX_PLAYERS];

	vgui::HTexture m_hTex = 0UL;
	vgui::HFont m_hFont = 0UL;

	void DrawPlayer(Color teamColor, C_NEO_Player *player, const C_NEO_Player *localPlayer) const;
	static Color GetTeamColour(int team);

};

#endif // NEO_HUD_FRIENDLY_MARKER_H
