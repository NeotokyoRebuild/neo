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
	virtual void ApplySchemeSettings(vgui::IScheme* pScheme) override;
	virtual void Paint() override;

	void DrawPlayer(const Vector& playerPos) const;

protected:
	virtual void UpdateStateForNeoHudElementDraw() override;
	virtual void DrawNeoHudElement() override;
	virtual ConVar* GetUpdateFrequencyConVar() const override;

private:
	C_WeaponGhost* m_pGhost;
	float m_flNextAllowGhostShowTime;
	bool m_bHoldingGhost = false;

	vgui::HFont m_hFont;
	vgui::HTexture m_hTex;

private:
	CNEOHud_GhostBeacons(const CNEOHud_GhostBeacons &other);
};

#endif // NEO_HUD_GHOSTBEACON_H