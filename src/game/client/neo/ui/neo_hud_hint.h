#pragma once

#include "neo_hud_childelement.h"
#include "hudelement.h"
#include <vgui_controls/Panel.h>

class CNEOHud_Hint : public CNEOHud_ChildElement, public CHudElement, public vgui::Panel
{
	DECLARE_CLASS_SIMPLE(CNEOHud_Hint, Panel);

public:
	CNEOHud_Hint(const char* pElementName, vgui::Panel* parent = NULL);
	void ApplySchemeSettings(vgui::IScheme* pScheme) override;

	virtual void Paint() override;

protected:
	virtual void UpdateStateForNeoHudElementDraw() override;
	virtual void DrawNeoHudElement() override;
	virtual ConVar* GetUpdateFrequencyConVar() const override;

private:
	vgui::HTexture m_pHintTexture[23];
	int m_iTextureWidth, m_iTextureHeight;

	int m_iSelectedTexture = -1;
	bool m_bIsVisible = false;

	CPanelAnimationVarAliasType(int, xpos, "xpos", "466", "proportional_xpos");
	CPanelAnimationVarAliasType(int, ypos, "ypos", "59", "proportional_ypos");
	CPanelAnimationVarAliasType(int, wide, "wide", "142", "proportional_xpos");
	CPanelAnimationVarAliasType(int, tall, "tall", "284", "proportional_ypos");
};
