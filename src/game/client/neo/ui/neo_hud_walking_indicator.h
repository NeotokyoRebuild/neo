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
	void DrawRangeFinder(C_NEO_Player* pPlayer, int &offset);
	void DrawWalkingIndicator(C_NEO_Player* pPlayer, int &offset);

	CPanelAnimationVarAliasType(bool, m_bVisible, "visible", "1", "bool");

	CPanelAnimationVarAliasType(int, m_iXpos, "xpos", "r203", "proportional_xpos");
	CPanelAnimationVarAliasType(int, m_iYpos, "ypos", "446", "proportional_ypos");

	CPanelAnimationVar(float, m_flAnimationSpeed, "animationSpeed", "2");

	CPanelAnimationVarAliasType(Color, m_color, "color", COLOR_NEO_WHITE_STR, "Color");
	CPanelAnimationVarAliasType(Color, m_color2, "color2", "255 150 150 255", "Color");
	CPanelAnimationVar(vgui::HFont, m_hFont, "font", "NHudPanelText");

private:
	CNEOHud_WalkingIndicator(const CNEOHud_WalkingIndicator&other);
};