#ifndef NEO_HUD_ROUND_STATE_H
#define NEO_HUD_ROUND_STATE_H
#ifdef _WIN32
#pragma once
#endif

#include "neo_hud_childelement.h"
#include "hudelement.h"
#include <vgui_controls/Panel.h>



class CNEOHud_RoundState : public CNEOHud_ChildElement, public CHudElement, public vgui::Panel
{
	DECLARE_CLASS_SIMPLE(CNEOHud_RoundState, Panel);

public:
	CNEOHud_RoundState(const char *pElementName, vgui::Panel *parent = NULL);

	virtual void ApplySchemeSettings(vgui::IScheme *pScheme);
	virtual void Paint();

protected:
	virtual void UpdateStateForNeoHudElementDraw() override;
	virtual void DrawNeoHudElement() override;
	virtual ConVar* GetUpdateFrequencyConVar() const override;

private:
	vgui::HFont m_hFont;

	int m_resX, m_resY;

	char m_szStatusANSI[6];
	wchar_t m_wszStatusUnicode[6];

private:
	CNEOHud_RoundState(const CNEOHud_RoundState &other);
};

#endif // NEO_HUD_ROUND_STATE_H