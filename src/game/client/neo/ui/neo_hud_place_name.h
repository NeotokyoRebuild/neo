#pragma once

#include "neo_hud_childelement.h"
#include "hudelement.h"
#include <vgui_controls/Panel.h>

class CNEOHud_PlaceName : public CNEOHud_ChildElement, public CHudElement, public vgui::Panel
{
	DECLARE_CLASS_SIMPLE(CNEOHud_PlaceName, Panel);

public:
	CNEOHud_PlaceName(const char *pElementName, vgui::Panel *parent = NULL);
	void ApplySchemeSettings(vgui::IScheme* pScheme) override;
	virtual void Paint() override;

protected:
	virtual void UpdateStateForNeoHudElementDraw() override;
	virtual void DrawNeoHudElement() override;
	virtual ConVar* GetUpdateFrequencyConVar() const override;

private:
	CNEOHud_PlaceName(const CNEOHud_PlaceName&other);
	
    wchar_t m_szPlaceName[MAX_PLACE_NAME_LENGTH];
	int textXOffset = 0;
	int wide = 0;

    CPanelAnimationVarAliasType(int, xpos, "xpos", "80", "proportional_xpos");
    CPanelAnimationVarAliasType(int, ypos, "ypos", "80", "proportional_ypos");

	CPanelAnimationVar(vgui::HFont, textFont, "textFont", "");
	CPanelAnimationVar(Color, textColor, "textColor", "255 255 255 255");
	CPanelAnimationVar(int, textXAlignment, "textXAlignment", "0");
	CPanelAnimationVar(int, textYAlignment, "textYAlignment", "0");
};