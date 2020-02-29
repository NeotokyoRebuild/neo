#ifndef NEO_HUD_GHOSTBEACON_H
#define NEO_HUD_GHOSTBEACON_H
#ifdef _WIN32
#pragma once
#endif

#include "neo_hud_childelement.h"
#include "hudelement.h"
#include <vgui_controls/Panel.h>

class CNEOHud_GhostBeacon : public CNEOHud_ChildElement, public CHudElement, public vgui::Panel
{
	DECLARE_CLASS_SIMPLE(CNEOHud_GhostBeacon, Panel);

public:
	CNEOHud_GhostBeacon(const char *pElementName, vgui::Panel *parent = NULL);

	virtual void Paint();

	// NEO TODO (Rain): we should move the entire textureScale logic to this class,
	// since it could be derived from the distance
	void SetGhostTargetPos(int x, int y, float textureScale, float distMeters)
	{
		m_posX = x;
		m_posY = y;
		m_flTexScale = textureScale;
		m_flDistMeters = distMeters;
	}

protected:
	virtual void UpdateStateForNeoHudElementDraw() override;
	virtual void DrawNeoHudElement() override;
	virtual ConVar* GetUpdateFrequencyConVar() const override;

private:
	int m_posX, m_posY;
	int m_beaconTexWidth, m_beaconTexHeight;

	float m_flTexScale, m_flDistMeters;

	vgui::HFont m_hFont;

	vgui::HTexture m_hTex;

	char m_szBeaconTextANSI[4 + 1];
	wchar_t m_wszBeaconTextUnicode[4 + 1];

private:
	CNEOHud_GhostBeacon(const CNEOHud_GhostBeacon &other);
};

#endif // NEO_HUD_GHOSTBEACON_H