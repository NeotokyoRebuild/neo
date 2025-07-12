#ifndef NEO_HUD_AMMO_H
#define NEO_HUD_AMMO_H
#ifdef _WIN32
#pragma once
#endif

#include "neo_hud_childelement.h"
#include "hudelement.h"
#include <vgui_controls/EditablePanel.h>

class CNEOHud_Ammo : public CNEOHud_ChildElement, public CHudElement, public vgui::EditablePanel
{
	DECLARE_CLASS_SIMPLE(CNEOHud_Ammo, EditablePanel);

public:
	CNEOHud_Ammo(const char *pElementName, vgui::Panel *parent = nullptr);

	virtual ~CNEOHud_Ammo() { }

	virtual void ApplySchemeSettings(vgui::IScheme* pScheme);
	virtual void Paint();

protected:
	virtual void UpdateStateForNeoHudElementDraw();
	virtual void DrawNeoHudElement();
	virtual ConVar* GetUpdateFrequencyConVar() const;

private:
	void DrawAmmo() const;
	void DrawHeatMeter(C_NEOBaseCombatWeapon* activeWep) const;

private:
	vgui::HFont m_hSmallTextFont = 0;
	vgui::HFont m_hTextFont = 0;
	vgui::HFont m_hBulletFont = 0;

	int m_resX, m_resY;
	int m_smallFontWidth, m_smallFontHeight;
	int m_fontWidth;
	int m_bulletFontWidth, m_bulletFontHeight;

	CPanelAnimationVarAliasType(int, xpos, "xpos", "r203", "proportional_xpos");
	CPanelAnimationVarAliasType(int, ypos, "ypos", "446", "proportional_ypos");
	CPanelAnimationVarAliasType(int, wide, "wide", "203", "proportional_xpos");
	CPanelAnimationVarAliasType(int, tall, "tall", "32", "proportional_ypos");
	CPanelAnimationVarAliasType(int, visible, "visible", "1", "int");
	CPanelAnimationVarAliasType(int, enabled, "enabled", "1", "int");
	CPanelAnimationVar(Color, box_color, "box_color", "200 200 200 40");

	CPanelAnimationVarAliasType(bool, top_left_corner, "top_left_corner", "1", "bool");
	CPanelAnimationVarAliasType(bool, top_right_corner, "top_right_corner", "0", "bool");
	CPanelAnimationVarAliasType(bool, bottom_left_corner, "bottom_left_corner", "1", "bool");
	CPanelAnimationVarAliasType(bool, bottom_right_corner, "bottom_right_corner", "0", "bool");

	CPanelAnimationVarAliasType(int, text_xpos, "text_xpos", "194", "proportional_xpos");
	CPanelAnimationVarAliasType(int, text_ypos, "text_ypos", "2", "proportional_ypos");
	CPanelAnimationVarAliasType(int, digit_xpos, "digit_xpos", "24", "proportional_xpos");
	CPanelAnimationVarAliasType(int, digit_ypos, "digit_ypos", "6", "proportional_ypos");
	CPanelAnimationVarAliasType(bool, digit_as_number, "digit_as_number", "0", "bool");
	CPanelAnimationVarAliasType(int, digit_max_width, "digit_max_width", "150", "proportional_xpos");
	CPanelAnimationVarAliasType(int, digit2_xpos, "digit2_xpos", "194", "proportional_xpos");
	CPanelAnimationVarAliasType(int, digit2_ypos, "digit2_ypos", "16", "proportional_ypos");
	CPanelAnimationVarAliasType(int, icon_xpos, "icon_xpos", "3", "proportional_xpos");
	CPanelAnimationVarAliasType(int, icon_ypos, "icon_ypos", "5", "proportional_ypos");
	CPanelAnimationVar(Color, ammo_text_color, "ammo_text_color", "255 255 255 100");
	CPanelAnimationVar(Color, ammo_color, "ammo_color", "255 255 255 150");
	CPanelAnimationVar(Color, emptied_ammo_color, "emptied_ammo_color", "255 255 255 50");

	CPanelAnimationVarAliasType(int, heatbar_xpos, "heatbar_xpos", "45", "proportional_xpos");
	CPanelAnimationVarAliasType(int, heatbar_ypos, "heatbar_ypos", "12", "proportional_ypos");
	CPanelAnimationVarAliasType(float, heatbar_w, "heatbar_w", "150", "proportional_float");
	CPanelAnimationVarAliasType(float, heatbar_h, "heatbar_h", "15", "proportional_float");
	CPanelAnimationVar(Color, heat_color, "heat_color", "255 50 50 200");

private:
	CNEOHud_Ammo(const CNEOHud_Ammo& other);
};

#endif // NEO_HUD_AMMO_H
