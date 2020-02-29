#ifndef NEO_HUD_COMPASS_H
#define NEO_HUD_COMPASS_H
#ifdef _WIN32
#pragma once
#endif

#include "neo_hud_childelement.h"
#include "hudelement.h"
#include <vgui_controls/Panel.h>

class CNeoHudElements;

#define UNICODE_NEO_COMPASS_STR_LENGTH 50

class CNEOHud_Compass : public CNEOHud_ChildElement, public CHudElement, public vgui::Panel
{
	DECLARE_CLASS_SIMPLE(CNEOHud_Compass, Panel);

public:
	CNEOHud_Compass(const char *pElementName, vgui::Panel *parent = NULL);

	virtual void ApplySchemeSettings(vgui::IScheme *pScheme);
	virtual void Paint();

protected:
	virtual void UpdateStateForNeoHudElementDraw() override;
	virtual void DrawNeoHudElement() override;
	virtual ConVar* GetUpdateFrequencyConVar() const override;

private:
	void DrawCompass();
	void DrawDebugCompass();
	void GetCompassUnicodeString(const float angle, wchar_t* outUnicodeStr) const;

private:
	vgui::HFont m_hFont;

	int m_resX, m_resY;

	float m_flCompassPulse;
	float m_flPulseStep;

	wchar_t m_wszCompassUnicode[UNICODE_NEO_COMPASS_STR_LENGTH];

private:
	CNEOHud_Compass(const CNEOHud_Compass &other);
};

#endif // NEO_HUD_COMPASS_H