#ifndef NEO_HUD_GHOSTBEACON_H
#define NEO_HUD_GHOSTBEACON_H
#ifdef WIN_32
#pragma once
#endif

#include "hudelement.h"
#include <vgui_controls/Panel.h>

#ifdef CLIENT_DLL
class CNEOHud_GhostBeacon : public CHudElement, public vgui::Panel
{
	DECLARE_CLASS_SIMPLE(CNEOHud_GhostBeacon, Panel);

public:
	CNEOHud_GhostBeacon(const char *pElementName, vgui::Panel *parent = NULL);

	virtual void Paint();

	void SetGhostTargetPos(int x, int y)
	{
		m_posX = x;
		m_posY = y;
	}

private:
	int m_posX, m_posY;

	vgui::HFont font;

private:
	CNEOHud_GhostBeacon(const CNEOHud_GhostBeacon &other);
};
#endif

#endif // NEO_HUD_GHOSTBEACON_H