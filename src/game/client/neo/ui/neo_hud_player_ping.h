#pragma once

#include "neo_hud_childelement.h"
#include "hudelement.h"
#include <vgui_controls/Panel.h>

struct playerPing
{
	Vector worldPos;
	float deathTime;
	int distance;
	int team;
	bool ghosterPing;
};

class CNEOHud_PlayerPing : public CNEOHud_ChildElement, public CHudElement, public vgui::Panel
{
	DECLARE_CLASS_SIMPLE(CNEOHud_PlayerPing, Panel)

public:
	CNEOHud_PlayerPing(const char *pElementName, vgui::Panel *parent = nullptr);

	void Init(void);
	virtual void ApplySchemeSettings(vgui::IScheme *pScheme) override;
	virtual void Paint() override;
	virtual void UpdateStateForNeoHudElementDraw() override;
	virtual void DrawNeoHudElement() override;
	int GetStringPixelWidth(wchar_t* pString, vgui::HFont hFont);
	virtual ConVar *GetUpdateFrequencyConVar() const override;

	void SetPos(const int index, Vector& pos, bool ghosterPing) {
		constexpr float PLAYER_PING_LIFETIME = 8;
		auto localPlayer = UTIL_PlayerByIndex(GetLocalPlayerIndex());
		if (!localPlayer) { return; }
		auto pingPlayer = UTIL_PlayerByIndex(index);
		if (!pingPlayer) { return; }

		m_iPlayerPings[index - 1].worldPos = pos;
		m_iPlayerPings[index - 1].deathTime = gpGlobals->curtime + PLAYER_PING_LIFETIME;
		m_iPlayerPings[index - 1].distance = METERS_PER_INCH * pos.DistTo(localPlayer->GetAbsOrigin());
		m_iPlayerPings[index - 1].team = pingPlayer->GetTeamNumber();
		m_iPlayerPings[index - 1].ghosterPing = ghosterPing;
	}

protected:
	virtual void FireGameEvent(IGameEvent* event);

private:
	playerPing m_iPlayerPings[MAX_PLAYERS] = {};
	
	int m_iPosX, m_iPosY;

	vgui::HFont m_hFont = 0UL;
	vgui::HFont m_hFontSmall = 0UL;
	int m_iFontTall;

	vgui::HTexture m_hTexture = 0UL;
	int m_iTexWidth, m_iTexHeight;
	int m_iTexTall;
};
