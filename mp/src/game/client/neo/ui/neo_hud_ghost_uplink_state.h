#ifndef NEO_HUD_GHOSTBEACON_H
#define NEO_HUD_GHOSTBEACON_H
#ifdef _WIN32
#pragma once
#endif

#include "neo_hud_childelement.h"
#include "hudelement.h"
#include <vgui_controls/Panel.h>

#include "weapon_ghost.h"

class CNEOHud_GhostUplinkState : public CNEOHud_ChildElement, public CHudElement, public vgui::Panel
{
	DECLARE_CLASS_SIMPLE(CNEOHud_GhostUplinkState, Panel);

public:
	CNEOHud_GhostUplinkState(const char *pElementName, vgui::Panel *parent = NULL);
	void ApplySchemeSettings(vgui::IScheme* pScheme) OVERRIDE;

	virtual void Paint();

protected:
	virtual void UpdateStateForNeoHudElementDraw();
	virtual void DrawNeoHudElement();
	virtual ConVar* GetUpdateFrequencyConVar() const;

private:
	float m_flTimeGhostEquip = 0.f;

	vgui::HTexture m_hTex0;
	vgui::HTexture m_hTex1;
	vgui::HTexture m_hTex2;
	vgui::HTexture m_hTex3;
	vgui::HTexture m_pUplinkTextures[4];

	static constexpr int numTextureStartTimes = 11;
	int textureOrder[numTextureStartTimes] = {0, 1, 2, 3, 2, 0, 1, 2, 3, 0, 1};
	float textureStartTimes[numTextureStartTimes] = {0, 0.2, 0.5, 0.6, 1.0, 1.1, 1.4, 1.8, 2.5, 2.7, 3.0};
	int currentTexture = 0;

	int textureXPos, textureYPos;
	int m_uplinkTexWidth, m_uplinkTexHeight;

private:
	CNEOHud_GhostUplinkState(const CNEOHud_GhostUplinkState&other);
};

#endif // NEO_HUD_GHOST_UPLINK_STATE_H