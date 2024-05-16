#ifndef NEO_HUD_GHOSTBEACON_H
#define NEO_HUD_GHOSTBEACON_H
#ifdef _WIN32
#pragma once
#endif

#include "neo_hud_childelement.h"
#include "hudelement.h"
#include <vgui_controls/Panel.h>

#include "weapon_ghost.h"

class CNEOHud_GhostBeacons : public CNEOHud_ChildElement, public CHudElement, public vgui::Panel
{
	DECLARE_CLASS_SIMPLE(CNEOHud_GhostBeacons, Panel);

public:
	CNEOHud_GhostBeacons(const char *pElementName, vgui::Panel *parent = NULL);

	virtual void Paint();
	void DrawPlayer(const Vector& playerPos) const;

protected:
	virtual void UpdateStateForNeoHudElementDraw();
	virtual void DrawNeoHudElement();
	virtual ConVar* GetUpdateFrequencyConVar() const;

private:
	int m_beaconTexWidth, m_beaconTexHeight;
	C_WeaponGhost* m_pGhost;

	vgui::HFont m_hFont;

	vgui::HTexture m_hTex;

private:
	CNEOHud_GhostBeacons(const CNEOHud_GhostBeacons &other);
};

#endif // NEO_HUD_GHOSTBEACON_H