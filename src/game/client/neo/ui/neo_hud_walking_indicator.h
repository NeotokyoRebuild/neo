#pragma once

#include "neo_hud_childelement.h"
#include "hudelement.h"
#include <vgui_controls/Panel.h>

class CNEOHud_WalkingIndicator : public CNEOHud_ChildElement, public CHudElement, public vgui::Panel
{
	DECLARE_CLASS_SIMPLE(CNEOHud_WalkingIndicator, Panel);

public:
	CNEOHud_WalkingIndicator(const char *pElementName, vgui::Panel *parent = NULL);
	void ApplySchemeSettings(vgui::IScheme* pScheme) override;

	virtual void Paint() override;

protected:
	virtual void UpdateStateForNeoHudElementDraw() override;
	virtual void DrawNeoHudElement() override;
	virtual ConVar* GetUpdateFrequencyConVar() const override;

private:
	vgui::HTexture m_hWalkingIndicatorTexture = 0;
	
	CPanelAnimationVarAliasType(int, xpos, "xpos", "r203", "proportional_xpos");
	CPanelAnimationVarAliasType(int, ypos, "ypos", "446", "proportional_ypos");
	CPanelAnimationVarAliasType(int, wide, "wide", "203", "proportional_xpos");
	CPanelAnimationVarAliasType(int, tall, "tall", "32", "proportional_ypos");
	CPanelAnimationVarAliasType(int, visible, "visible", "1", "int");
	CPanelAnimationVarAliasType(int, enabled, "enabled", "1", "int");

private:
	CNEOHud_WalkingIndicator(const CNEOHud_WalkingIndicator&other);
};