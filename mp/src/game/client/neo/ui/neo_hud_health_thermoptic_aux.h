#ifndef NEO_HUD_HEALTH_THERMOPTIC_AUX_H
#define NEO_HUD_HEALTH_THERMOPTIC_AUX_H
#ifdef _WIN32
#pragma once
#endif

#include "neo_hud_childelement.h"
#include "hudelement.h"
#include <vgui_controls/Panel.h>

class CNeoHudElements;

class CNEOHud_HTA : public CNEOHud_ChildElement, public CHudElement, public vgui::Panel
{
	DECLARE_CLASS_SIMPLE(CNEOHud_HTA, Panel);
public:
	CNEOHud_HTA(const char* pElementName, vgui::Panel* parent);

	virtual ~CNEOHud_HTA() { }

	virtual void ApplySchemeSettings(vgui::IScheme* pScheme);
	virtual void Paint();

protected:
	virtual void UpdateStateForNeoHudElementDraw();
	virtual void DrawNeoHudElement();
	virtual ConVar* GetUpdateFrequencyConVar() const;

private:
	void DrawHTA() const;

private:
	vgui::HFont m_hFont;

	int m_resX, m_resY;

private:
	CNEOHud_HTA(const CNEOHud_HTA& other);
};

#endif // NEO_HUD_HEALTH_THERMOPTIC_AUX_H