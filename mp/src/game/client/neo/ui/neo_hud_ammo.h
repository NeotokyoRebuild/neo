#ifndef NEO_HUD_AMMO_H
#define NEO_HUD_AMMO_H
#ifdef _WIN32
#pragma once
#endif

#include "neo_hud_childelement.h"
#include "hudelement.h"
#include <vgui_controls/Panel.h>

class CNeoHudElements;

class CNEOHud_Ammo : public CNEOHud_ChildElement, public CHudElement, public vgui::Panel
{
	DECLARE_CLASS_SIMPLE(CNEOHud_Ammo, Panel);

public:
	CNEOHud_Ammo(const char* pElementName, vgui::Panel* parent);

	virtual ~CNEOHud_Ammo() { }

	virtual void ApplySchemeSettings(vgui::IScheme* pScheme);
	virtual void Paint();

protected:
	virtual void UpdateStateForNeoHudElementDraw();
	virtual void DrawNeoHudElement();
	virtual ConVar* GetUpdateFrequencyConVar() const;

private:
	void DrawAmmo() const;

private:
	vgui::HFont m_hSmallTextFont;
	vgui::HFont m_hTextFont;
	vgui::HFont m_hBulletFont;

	int m_resX, m_resY;
	int m_smallFontWidth, m_smallFontHeight;
	int m_fontWidth;
	int m_bulletFontWidth, m_bulletFontHeight;

	Color box_color, ammo_color, transparent_ammo_color;

	CPanelAnimationVarAliasType(int, xpos, "xpos", "r203", "proportional_xpos");
	CPanelAnimationVarAliasType(int, ypos, "ypos", "446", "proportional_ypos");
	CPanelAnimationVarAliasType(int, wide, "wide", "203", "proportional_xpos");
	CPanelAnimationVarAliasType(int, tall, "tall", "32", "proportional_ypos");
	CPanelAnimationVarAliasType(int, visible, "visible", "1", "int");
	CPanelAnimationVarAliasType(int, enabled, "enabled", "1", "int");
	CPanelAnimationVarAliasType(int, box_color_r, "box_color_r", "116", "int");
	CPanelAnimationVarAliasType(int, box_color_g, "box_color_g", "116", "int");
	CPanelAnimationVarAliasType(int, box_color_b, "box_color_b", "116", "int");
	CPanelAnimationVarAliasType(int, box_color_a, "box_color_a", "178", "int");

	CPanelAnimationVarAliasType(bool, top_left_corner, "top_left_corner", "1", "bool");
	CPanelAnimationVarAliasType(bool, top_right_corner, "top_right_corner", "1", "bool");
	CPanelAnimationVarAliasType(bool, bottom_left_corner, "bottom_left_corner", "1", "bool");
	CPanelAnimationVarAliasType(bool, bottom_right_corner, "bottom_right_corner", "1", "bool");

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
	CPanelAnimationVarAliasType(int, ammo_color_r, "ammo_color_r", "255", "int");
	CPanelAnimationVarAliasType(int, ammo_color_g, "ammo_color_g", "255", "int");
	CPanelAnimationVarAliasType(int, ammo_color_b, "ammo_color_b", "255", "int");
	CPanelAnimationVarAliasType(int, ammo_color_a, "ammo_color_a", "178", "int");
private:
	CNEOHud_Ammo(const CNEOHud_Ammo& other);
};

#endif // NEO_HUD_AMMO_H
