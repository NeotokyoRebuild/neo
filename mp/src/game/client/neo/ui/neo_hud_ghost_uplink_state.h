#pragma once

#include "neo_hud_childelement.h"
#include "hudelement.h"
#include <vgui_controls/Panel.h>

class CNEOHud_GhostUplinkState : public CNEOHud_ChildElement, public CHudElement, public vgui::Panel
{
	DECLARE_CLASS_SIMPLE(CNEOHud_GhostUplinkState, Panel);

public:
	CNEOHud_GhostUplinkState(const char *pElementName, vgui::Panel *parent = NULL);
	void ApplySchemeSettings(vgui::IScheme* pScheme) override;

	virtual void Paint();

protected:
	virtual void UpdateStateForNeoHudElementDraw() override;
	virtual void DrawNeoHudElement() override;
	virtual ConVar* GetUpdateFrequencyConVar() const;

private:
	float m_flTimeGhostEquip = 0.f;

	static constexpr int NUM_UPLINK_TEXTURES = 4;
	static constexpr int NUM_TEXTURE_START_TIMES = 11;
	static constexpr int textureOrder[NUM_TEXTURE_START_TIMES] = { 0, 1, 2, 3, 2, 0, 1, 2, 3, 0, 1 };
	static constexpr float textureStartTimes[NUM_TEXTURE_START_TIMES] = { 0, 0.2, 0.5, 0.6, 1.0, 1.1, 1.4, 1.8, 2.5, 2.7, 3.0 };

	vgui::HTexture m_pUplinkTextures[NUM_UPLINK_TEXTURES];

	int m_iCurrentTextureIndex = 0;
	int m_iUplinkTextureWidth, m_iUplinkTextureHeight;

private:
	CNEOHud_GhostUplinkState(const CNEOHud_GhostUplinkState&other);
};