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

	virtual void Paint() override;

protected:
	virtual void UpdateStateForNeoHudElementDraw() override;
	virtual void DrawNeoHudElement() override;
	virtual ConVar* GetUpdateFrequencyConVar() const override;

private:
	float m_flTimeGhostEquip = 0.f;

	static constexpr int NUM_UPLINK_TEXTURES = 4;
	static constexpr int NUM_TEXTURE_START_TIMES = 11;

	vgui::HTexture m_pUplinkTextures[NUM_UPLINK_TEXTURES];

	int m_iCurrentTextureIndex = 0;
	int m_iUplinkTextureWidth, m_iUplinkTextureHeight;

	int m_iJGRTexture;

private:
	CNEOHud_GhostUplinkState(const CNEOHud_GhostUplinkState&other);
};