#ifndef NEO_HUD_TEAM_MENU_H
#define NEO_HUD_TEAM_MENU_H
#ifdef _WIN32
#pragma once
#endif

#include "hudelement.h"
#include <vgui_controls/Panel.h>
#include <vgui_controls/Button.h>

class CNEOHud_TeamMenu : public CHudElement, public vgui::Panel
{
	DECLARE_CLASS_SIMPLE(CNEOHud_TeamMenu, Panel);

public:
	CNEOHud_TeamMenu(const char *pElemName, vgui::Panel *parent = NULL);

	virtual ~CNEOHud_TeamMenu();

	virtual void Paint() override;
	virtual void SetVisible(bool state) override;

	void OnMouseReleased(vgui::MouseCode code) override;

	bool ShouldShow() const { return m_bShouldShow; }

private:
	int m_iPosX, m_iPosY;

	vgui::HFont m_hFont;

	vgui::HTexture m_hTex_Jinrai, m_hTex_NSF;

	vgui::Button *m_Button_Jinrai, *m_Button_NSF;

	bool m_bShouldShow;

private:
	CNEOHud_TeamMenu(const CNEOHud_TeamMenu &other);
};

#endif // NEO_HUD_TEAM_MENU_H