#ifndef NEO_HUD_AMMO_H
#define NEO_HUD_AMMO_H
#ifdef _WIN32
#pragma once
#endif

#include "neo_hud_childelement.h"
#include "hudelement.h"
#include <vgui_controls/Panel.h>

class CNeoHudElements;

class CNEOHud_Ammo : public CNEOHud_ChildElement, public CHudElement, public vgui::Panel
{
	DECLARE_CLASS_SIMPLE(CNEOHud_Ammo, Panel);

public:
	CNEOHud_Ammo(const char* pElementName, vgui::Panel* parent);

	virtual ~CNEOHud_Ammo() { }

	virtual void ApplySchemeSettings(vgui::IScheme* pScheme);
	virtual void Paint();

protected:
	virtual void UpdateStateForNeoHudElementDraw();
	virtual void DrawNeoHudElement();
	virtual ConVar* GetUpdateFrequencyConVar() const;

private:
	void DrawAmmo() const;

private:
	vgui::HFont m_hSmallTextFont;
	vgui::HFont m_hTextFont;
	vgui::HFont m_hBulletFont;

	int m_resX, m_resY;
	int m_smallFontWidth, m_smallFontHeight;
	int m_fontWidth;
	int m_bulletFontWidth, m_bulletFontHeight;

private:
	CNEOHud_Ammo(const CNEOHud_Ammo& other);
};

#endif // NEO_HUD_AMMO_H