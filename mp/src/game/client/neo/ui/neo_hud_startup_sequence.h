#pragma once

#include "neo_hud_childelement.h"
#include "hudelement.h"
#include <vgui_controls/Panel.h>

class CNEOHud_StartupSequence : public CNEOHud_ChildElement, public CHudElement, public vgui::Panel
{
	DECLARE_CLASS_SIMPLE(CNEOHud_StartupSequence, Panel);

public:
	CNEOHud_StartupSequence(const char *pElementName, vgui::Panel *parent = NULL);
	void ApplySchemeSettings(vgui::IScheme* pScheme) override;

	virtual void Paint() override;

protected:
	virtual void UpdateStateForNeoHudElementDraw() override;
	virtual void DrawNeoHudElement() override;
	virtual ConVar* GetUpdateFrequencyConVar() const override;

private:
	vgui::HFont m_hFont = 0UL;

	int m_iSelectedText = 0;
	float m_flNextTimeChangeText = 0.f;

	CPanelAnimationVarAliasType(int, xpos, "xpos", "10", "proportional_xpos");
	CPanelAnimationVarAliasType(int, ypos, "ypos", "426", "proportional_ypos");
	CPanelAnimationVarAliasType(int, wide, "wide", "640", "proportional_int");

private:
	CNEOHud_StartupSequence(const CNEOHud_StartupSequence&other);
};