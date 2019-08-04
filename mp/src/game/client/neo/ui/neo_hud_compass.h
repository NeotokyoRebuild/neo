#ifndef NEO_HUD_COMPASS_H
#define NEO_HUD_COMPASS_H
#ifdef _WIN32
#pragma once
#endif

#include "hudelement.h"
#include <vgui_controls/Panel.h>

class C_NEO_Player;

class CNEOHud_Compass : public CHudElement, public vgui::Panel
{
	DECLARE_CLASS_SIMPLE(CNEOHud_Compass, Panel);

public:
	CNEOHud_Compass(const char *pElementName, vgui::Panel *parent = NULL);

	void SetOwner(C_NEO_Player *owner);

	virtual void ApplySchemeSettings(vgui::IScheme *pScheme);
	virtual void Paint();

	inline void DrawCompass(void);
	inline void DrawDebugCompass(void);

private:
	vgui::HFont m_hFont;

	int m_resX, m_resY;

	C_NEO_Player *m_pOwner;

private:
	CNEOHud_Compass(const CNEOHud_Compass &other);
};

#endif // NEO_HUD_COMPASS_H