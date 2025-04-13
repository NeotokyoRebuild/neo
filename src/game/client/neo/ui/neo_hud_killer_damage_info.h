#pragma once

#include "neo_hud_childelement.h"
#include "c_neo_killer_damage_infos.h"
#include "neo_ui.h"

#include <hudelement.h>
#include <vgui_controls/Panel.h>

class CNEOHud_KillerDamageInfo : public CNEOHud_ChildElement, public CHudElement, public vgui::Panel
{
    DECLARE_CLASS_SIMPLE(CNEOHud_KillerDamageInfo, CNEOHud_ChildElement)

public:
    CNEOHud_KillerDamageInfo(const char *pszName, vgui::Panel *parent = nullptr);
	~CNEOHud_KillerDamageInfo();

    void ApplySchemeSettings(vgui::IScheme *pScheme) override;
    void Paint() override;
    void resetHUDState() override;

	bool m_bShowInfo = true;
	bool m_bPlayerShownHud = false;

protected:
    void UpdateStateForNeoHudElementDraw() override;
    void DrawNeoHudElement() override;
    ConVar *GetUpdateFrequencyConVar() const override;

    vgui::HFont m_hFont;
	NeoUI::Context m_uiCtx;

	wchar_t m_wszKillerTitle[256] = {};
	wchar_t m_wszBindBtnToggleMessage[128] = {};
};
