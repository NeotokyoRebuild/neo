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

	struct PlayerInfo
	{
		int x;
		int y;
		float flTexScale;
		float flDistMeters;
	};

	// NEO TODO (Rain): we should move the entire textureScale logic to this class,
	// since it could be derived from the distance
	void SetVisibleGhostTargetPos(const int clientIndex,
			const int x, const int y,
			const float textureScale, const float distMeters);

	uint64_t m_bitsVisiblePlayers;
	PlayerInfo m_playersInfo[MAX_PLAYERS + 1];

protected:
	virtual void UpdateStateForNeoHudElementDraw();
	virtual void DrawNeoHudElement();
	virtual ConVar* GetUpdateFrequencyConVar() const;

private:
	int m_beaconTexWidth;
	int m_beaconTexHeight;
	vgui::HFont m_hFont;
	vgui::HTexture m_hTex;

private:
	CNEOHud_GhostBeacon(const CNEOHud_GhostBeacon &other);
};

#endif // NEO_HUD_GHOSTBEACON_H