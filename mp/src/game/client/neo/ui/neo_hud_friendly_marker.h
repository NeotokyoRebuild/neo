#ifndef NEO_HUD_FRIENDLY_MARKER_H
#define NEO_HUD_FRIENDLY_MARKER_H
#ifdef _WIN32
#pragma once
#endif

#include "hudelement.h"
#include <vgui_controls/Panel.h>

class C_NEO_Player;

class CNEOHud_FriendlyMarker : public CHudElement, public vgui::Panel
{
	DECLARE_CLASS_SIMPLE(CNEOHud_FriendlyMarker, Panel);

public:
	CNEOHud_FriendlyMarker(const char *pElemName, vgui::Panel *parent = NULL);

	virtual void Paint();

	void SetOwner(C_NEO_Player *player) { m_pOwner = player; }
	C_NEO_Player *GetOwner() const { return m_pOwner; }

private:
	C_NEO_Player *m_pOwner;

	int m_iMarkerTexWidth, m_iMarkerTexHeight;
	int m_iPosX, m_iPosY;

	vgui::HTexture m_hTex;
	vgui::HFont m_hFont;
};

#endif // NEO_HUD_FRIENDLY_MARKER_H