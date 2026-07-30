#ifndef NEO_HUD_COMPASS_H
#define NEO_HUD_COMPASS_H
#ifdef _WIN32
#pragma once
#endif

#include "neo_hud_childelement.h"
#include "hudelement.h"
#include "shareddefs.h"
#include "util_shared.h"
#include <vgui_controls/EditablePanel.h>

struct GhostCallout
{
	Vector worldPos;
	CountdownTimer timer;
};

class CNEOHud_Compass : public CNEOHud_ChildElement, public CHudElement, public vgui::EditablePanel
{
	DECLARE_CLASS_SIMPLE(CNEOHud_Compass, EditablePanel);

public:
	CNEOHud_Compass(const char *pElementName, vgui::Panel *parent = nullptr);

	virtual void Init() override;
	virtual void LevelShutdown() override;
	virtual void ApplySchemeSettings(vgui::IScheme *pScheme);
	virtual void Paint();

protected:
	virtual void UpdateStateForNeoHudElementDraw();
	virtual void DrawNeoHudElement();
	virtual ConVar* GetUpdateFrequencyConVar() const;
	virtual void FireGameEvent(IGameEvent* event) override;

private:
	void DrawCompass() const;
	void DrawCallouts() const;
	void HideAllGhostCallouts();

private:
	vgui::HFont m_hFont;
	vgui::HFont m_hFontSmall;

	int m_resX, m_resY;
	float m_objAngle;

	wchar_t m_wszRangeFinder[11];

	GhostCallout m_GhostCallouts[MAX_PLAYERS_ARRAY_SAFE];

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
