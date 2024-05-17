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

	CPanelAnimationVarAliasType(int, xpos, "xpos", "r203", "proportional_xpos");
	CPanelAnimationVarAliasType(int, ypos, "ypos", "446", "proportional_ypos");
	CPanelAnimationVarAliasType(int, wide, "wide", "203", "proportional_xpos");
	CPanelAnimationVarAliasType(int, tall, "tall", "32", "proportional_ypos");
	CPanelAnimationVarAliasType(int, visible, "visible", "1", "int");
	CPanelAnimationVarAliasType(int, enabled, "enabled", "1", "int");

	CPanelAnimationVarAliasType(int, text_xpos, "text_xpos", "194", "proportional_xpos");
	CPanelAnimationVarAliasType(int, text_ypos, "text_ypos", "2", "proportional_ypos");
	CPanelAnimationVarAliasType(int, digit_xpos, "digit_xpos", "24", "proportional_xpos");
	CPanelAnimationVarAliasType(int, digit_ypos, "digit_ypos", "6", "proportional_ypos");
	CPanelAnimationVarAliasType(int, digit_max_width, "digit_max_width", "150", "proportional_xpos");
	CPanelAnimationVarAliasType(int, digit2_xpos, "digit2_xpos", "194", "proportional_xpos");
	CPanelAnimationVarAliasType(int, digit2_ypos, "digit2_ypos", "16", "proportional_ypos");

	CPanelAnimationVarAliasType(int, icon_xpos, "icon_xpos", "3", "proportional_xpos");
	CPanelAnimationVarAliasType(int, icon_ypos, "icon_ypos", "5", "proportional_ypos");

private:
	CNEOHud_Ammo(const CNEOHud_Ammo& other);
};

#endif // NEO_HUD_AMMO_H