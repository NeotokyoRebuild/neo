#ifndef NEO_HUD_COMPASS_H
#define NEO_HUD_COMPASS_H
#ifdef _WIN32
#pragma once
#endif

#include "neo_hud_childelement.h"
#include "hudelement.h"
#include <vgui_controls/EditablePanel.h>

static constexpr size_t UNICODE_NEO_COMPASS_VIS_AROUND = 33; // How many characters should be visible around each side of the needle position
static constexpr size_t UNICODE_NEO_COMPASS_STR_LENGTH = ((UNICODE_NEO_COMPASS_VIS_AROUND * 2) + 2);

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
	void DrawDebugCompass() const;
	void GetCompassUnicodeString(const float angle, wchar_t* outUnicodeStr) const;

private:
	vgui::HFont m_hFont;

	int m_resX, m_resY;
	mutable int m_savedXBoxWidth = 0;

	wchar_t m_wszCompassUnicode[UNICODE_NEO_COMPASS_STR_LENGTH];
	wchar_t m_wszRangeFinder[11];

	CPanelAnimationVarAliasType(bool, m_showCompass, "visible", "1", "bool");
	CPanelAnimationVarAliasType(int, m_yFromBottomPos, "y_bottom_pos", "3", "proportional_ypos");
	CPanelAnimationVarAliasType(bool, m_needleVisible, "needle_visible", "0", "bool");
	CPanelAnimationVarAliasType(bool, m_needleColored, "needle_colored", "0", "bool");
	CPanelAnimationVarAliasType(bool, m_objectiveVisible, "objective_visible", "1", "bool");
	CPanelAnimationVar(Color, m_boxColor, "box_color", "200 200 200 40");

private:
	CNEOHud_Compass(const CNEOHud_Compass &other);
};

#endif // NEO_HUD_COMPASS_H
