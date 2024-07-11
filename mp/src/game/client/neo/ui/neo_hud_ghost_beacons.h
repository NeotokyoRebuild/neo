#ifndef NEO_HUD_GHOSTBEACON_H
#define NEO_HUD_GHOSTBEACON_H
#ifdef _WIN32
#pragma once
#endif

#include "neo_hud_childelement.h"
#include "hudelement.h"
#include <vgui_controls/EditablePanel.h>

#include "weapon_ghost.h"

class CNEOHud_GhostBeacons : public CNEOHud_ChildElement, public CHudElement, public vgui::EditablePanel
{
	DECLARE_CLASS_SIMPLE(CNEOHud_GhostBeacons, EditablePanel);

public:
	CNEOHud_GhostBeacons(const char *pElementName, vgui::Panel *parent = NULL);
	virtual void ApplySchemeSettings(vgui::IScheme* pScheme);

	virtual void Paint();
	void DrawPlayer(const Vector& playerPos) const;

	Vector ghostBeaconOffset = Vector(0, 0, 0);
	Vector* m_pGhostBeaconOffset = &ghostBeaconOffset;

protected:
	virtual void UpdateStateForNeoHudElementDraw();
	virtual void DrawNeoHudElement();
	virtual ConVar* GetUpdateFrequencyConVar() const;

private:
	int m_beaconTexWidth, m_beaconTexHeight;
	C_WeaponGhost* m_pGhost;
	float m_flNextAllowGhostShowTime;
	bool m_curGhostHolding;

	vgui::HFont m_hFont;
	Color fontColor = COLOR_WHITE;

	vgui::HTexture m_hTex;

private:
	CNEOHud_GhostBeacons(const CNEOHud_GhostBeacons &other);
};

#endif // NEO_HUD_GHOSTBEACON_H