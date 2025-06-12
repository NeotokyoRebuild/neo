#pragma once

#include "mathlib/vector.h"
#include "vgui/VGUI.h"
#include "cdll_util.h"

#include "neo_hud_worldpos_marker.h"

class CNEOHud_GhostCapPoint : public CNEOHud_WorldPosMarker
{
	DECLARE_CLASS_SIMPLE(CNEOHud_GhostCapPoint, CNEOHud_WorldPosMarker);

public:
	CNEOHud_GhostCapPoint(const char *pElementName, vgui::Panel *parent = nullptr);
	CNEOHud_GhostCapPoint(const CNEOHud_GhostCapPoint &other) = delete;

	virtual void ApplySchemeSettings(vgui::IScheme *pScheme) override;
	virtual void Paint() override;
	virtual void UpdateStateForNeoHudElementDraw() override;
	virtual void DrawNeoHudElement() override;
	virtual ConVar *GetUpdateFrequencyConVar() const override;

	void SetTeam(const int team) { m_iCapTeam = team; }
	void SetRadius(const float radius) { m_flMyRadius = radius; }
	void SetPos(const Vector &pos) { m_vecMyPos = pos; }

private:
	int m_iPosX = 0;
	int m_iPosY = 0;
	int m_flMyRadius = 0;
	int m_iCapTeam = TEAM_INVALID;
	float m_flDistance = 0.0f;
	float m_flCapTexScale = 1.0f;
	float m_fMarkerScalesStart[4] = { 0.78f, 0.6f, 0.38f, 0.0f };
	float m_fMarkerScalesCurrent[4] = { 0.78f, 0.6f, 0.38f, 0.0f };
	wchar_t m_wszMarkerTextUnicode[64 + 1] = {};
	Vector m_vecMyPos = Vector(0, 0, 0);
	vgui::HFont m_hFont = 0UL;
	vgui::HTexture m_hCapTex = 0UL;
};
