#ifndef NEO_HUD_GHOSTBEACON_H
#define NEO_HUD_GHOSTBEACON_H
#ifdef WIN_32
#pragma once
#endif

#include "hudelement.h"
#include <vgui_controls/Panel.h>

class CNEOHud_GhostBeacon : public CHudElement, public vgui::Panel
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

private:
	int m_posX, m_posY;
	int m_beaconTexWidth, m_beaconTexHeight;

	float m_flTexScale, m_flDistMeters;

	vgui::HFont m_hFont;

	vgui::HTexture m_hTex;

private:
	CNEOHud_GhostBeacon(const CNEOHud_GhostBeacon &other);
};

#endif // NEO_HUD_GHOSTBEACON_H