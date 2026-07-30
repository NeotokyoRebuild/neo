#pragma once

#include "neo_hud_childelement.h"
#include "neo_player_shared.h"

#include <hudelement.h>
#include <vgui_controls/Panel.h>
#include <vgui_avatarimage.h>

class CNEOHud_KillerInfo : public CNEOHud_ChildElement, public CHudElement, public vgui::Panel
{
	DECLARE_CLASS_SIMPLE(CNEOHud_KillerInfo, CNEOHud_ChildElement)

public:
	CNEOHud_KillerInfo(const char *pszName, vgui::Panel *parent = nullptr);

	void ApplySchemeSettings(vgui::IScheme *pScheme) override;
	void Paint() override;
	void resetHUDState() override;

protected:
	void UpdateStateForNeoHudElementDraw() override;
	void DrawNeoHudElement() override;
	ConVar *GetUpdateFrequencyConVar() const override;

private:
	void CommonSetupHUD();

	vgui::HFont m_hFontNormal;
	vgui::HFont m_hFontTitle;
	CAvatarImage m_avatar;
	bool m_bPlayerShownHud = false;
};
