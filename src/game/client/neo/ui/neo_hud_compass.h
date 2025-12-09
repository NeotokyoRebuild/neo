#ifndef NEO_HUD_COMPASS_H
#define NEO_HUD_COMPASS_H
#ifdef _WIN32
#pragma once
#endif

#include "neo_hud_childelement.h"
#include "hudelement.h"
#include <vgui_controls/EditablePanel.h>

class CNEOHud_Compass : public CNEOHud_ChildElement, public CHudElement, public vgui::EditablePanel
{
	DECLARE_CLASS_SIMPLE(CNEOHud_Compass, EditablePanel);

public:
	CNEOHud_Compass(const char *pElementName, vgui::Panel *parent = nullptr);

	virtual void ApplySchemeSettings(vgui::IScheme *pScheme);
	virtual void Paint();

protected:
	virtual void UpdateStateForNeoHudElementDraw();
	virtual void DrawNeoHudElement();
	virtual ConVar* GetUpdateFrequencyConVar() const;

private:
	void DrawCompass() const;

private:
	vgui::HFont m_hFont;

	int m_resX, m_resY;
	float m_objAngle;

	wchar_t m_wszRangeFinder[11];

	CPanelAnimationVarAliasType(bool, m_showCompass, "visible", "1", "bool");
	CPanelAnimationVarAliasType(int, m_xPos, "xpos", "c-50", "proportional_xpos");
	CPanelAnimationVarAliasType(int, m_yPos, "ypos", "469", "proportional_ypos");
	CPanelAnimationVarAliasType(int, m_width, "wide", "100", "proportional_xpos");
	CPanelAnimationVarAliasType(int, m_height, "tall", "8", "proportional_ypos");
	CPanelAnimationVarAliasType(int, m_fov, "fov", "90", "int");
	CPanelAnimationVarAliasType(bool, m_separators, "separators", "1", "bool")
	CPanelAnimationVarAliasType(bool, m_objectiveVisible, "objective_visible", "1", "bool");
	CPanelAnimationVarAliasType(int, m_fadeExp, "text_fade_exp", "2", "int")
	CPanelAnimationVar(Color, m_textColor, "text_color", "255 255 255 150");
	CPanelAnimationVar(Color, m_boxColor, "box_color", "200 200 200 40");

private:
	CNEOHud_Compass(const CNEOHud_Compass &other);
};

#endif // NEO_HUD_COMPASS_H
