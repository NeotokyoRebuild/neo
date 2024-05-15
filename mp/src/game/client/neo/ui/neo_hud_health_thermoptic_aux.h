#ifndef NEO_HUD_HEALTH_THERMOPTIC_AUX_H
#define NEO_HUD_HEALTH_THERMOPTIC_AUX_H
#ifdef _WIN32
#pragma once
#endif

#include "neo_hud_childelement.h"
#include "hudelement.h"
#include <vgui_controls/Panel.h>

class CNeoHudElements;

class CNEOHud_HTA : public CNEOHud_ChildElement, public CHudElement, public vgui::Panel
{
	DECLARE_CLASS_SIMPLE(CNEOHud_HTA, Panel);
public:
	CNEOHud_HTA(const char* pElementName, vgui::Panel* parent);

	virtual ~CNEOHud_HTA() { }

	virtual void ApplySchemeSettings(vgui::IScheme* pScheme);
	virtual void Paint();

protected:
	virtual void UpdateStateForNeoHudElementDraw();
	virtual void DrawNeoHudElement();
	virtual ConVar* GetUpdateFrequencyConVar() const;

private:
	void DrawHTA() const;

private:
	vgui::HFont m_hFont;

	int m_resX, m_resY;

	CPanelAnimationVarAliasType(int, xpos, "xpos", "0", "proportional_xpos");
	CPanelAnimationVarAliasType(int, ypos, "ypos", "446", "proportional_ypos");
	CPanelAnimationVarAliasType(float, wide, "wide", "203", "proportional_float");
	CPanelAnimationVarAliasType(float, tall, "tall", "32", "proportional_float");
	CPanelAnimationVarAliasType(float, visible, "visible", "1", "proportional_float");
	CPanelAnimationVarAliasType(float, enabled, "enabled", "1", "proportional_float");

	CPanelAnimationVarAliasType(int, healthtext_xpos, "healthtext_xpos", "6", "proportional_xpos");
	CPanelAnimationVarAliasType(int, healthtext_ypos, "healthtext_ypos", "2", "proportional_ypos");
	CPanelAnimationVarAliasType(int, healthbar_xpos, "healthbar_xpos", "86", "proportional_xpos");
	CPanelAnimationVarAliasType(int, healthbar_ypos, "healthbar_ypos", "4", "proportional_ypos");
	CPanelAnimationVarAliasType(float, healthbar_w, "healthbar_w", "93", "proportional_float");
	CPanelAnimationVarAliasType(float, healthbar_h, "healthbar_h", "6", "proportional_float");
	CPanelAnimationVarAliasType(int, healthnum_xpos, "healthnum_xpos", "198", "proportional_xpos");
	CPanelAnimationVarAliasType(int, healthnum_ypos, "healthnum_ypos", "2", "proportional_ypos");

	CPanelAnimationVarAliasType(int, camotext_xpos, "camotext_xpos", "6", "proportional_xpos");
	CPanelAnimationVarAliasType(int, camotext_ypos, "camotext_ypos", "12", "proportional_ypos");
	CPanelAnimationVarAliasType(int, camobar_xpos, "camobar_xpos", "86", "proportional_xpos");
	CPanelAnimationVarAliasType(int, camobar_ypos, "camobar_ypos", "14", "proportional_ypos");
	CPanelAnimationVarAliasType(float, camobar_w, "camobar_w", "93", "proportional_float");
	CPanelAnimationVarAliasType(float, camobar_h, "camobar_h", "6", "proportional_float");
	CPanelAnimationVarAliasType(int, camonum_xpos, "camonum_xpos", "198", "proportional_xpos");
	CPanelAnimationVarAliasType(int, camonum_ypos, "camonum_ypos", "12", "proportional_ypos");

	CPanelAnimationVarAliasType(int, sprinttext_xpos, "sprinttext_xpos", "6", "proportional_xpos");
	CPanelAnimationVarAliasType(int, sprinttext_ypos, "sprinttext_ypos", "22", "proportional_ypos");
	CPanelAnimationVarAliasType(int, sprintbar_xpos, "sprintbar_xpos", "86", "proportional_xpos");
	CPanelAnimationVarAliasType(int, sprintbar_ypos, "sprintbar_ypos", "24", "proportional_ypos");
	CPanelAnimationVarAliasType(float, sprintbar_w, "sprintbar_w", "93", "proportional_float");
	CPanelAnimationVarAliasType(float, sprintbar_h, "sprintbar_h", "6", "proportional_float");
	CPanelAnimationVarAliasType(int, sprintnum_xpos, "sprintnum_xpos", "198", "proportional_xpos");
	CPanelAnimationVarAliasType(int, sprintnum_ypos, "sprintnum_ypos", "22", "proportional_ypos");

private:
	CNEOHud_HTA(const CNEOHud_HTA& other);
};

#endif // NEO_HUD_HEALTH_THERMOPTIC_AUX_H