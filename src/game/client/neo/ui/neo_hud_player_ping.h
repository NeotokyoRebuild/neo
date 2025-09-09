#pragma once

#include "neo_hud_childelement.h"
#include "hudelement.h"
#include <vgui_controls/Panel.h>

struct playerPing
{
	Vector worldPos;
	float deathTime;
	float distance;
	int team;
	bool noLineOfSight;
	bool ghosterPing;
};

class CNEOHud_PlayerPing : public CNEOHud_ChildElement, public CHudElement, public vgui::Panel
{
	DECLARE_CLASS_SIMPLE(CNEOHud_PlayerPing, Panel)

public:
	CNEOHud_PlayerPing(const char *pElementName, vgui::Panel *parent = nullptr);

	virtual void Init(void) override;
	virtual void ApplySchemeSettings(vgui::IScheme *pScheme) override;
	virtual void Paint() override;
	virtual void UpdateStateForNeoHudElementDraw() override;
	virtual void DrawNeoHudElement() override;
	virtual ConVar *GetUpdateFrequencyConVar() const override;
	virtual void LevelShutdown(void) override;

	void HideAllPings();
protected:
	virtual void FireGameEvent(IGameEvent* event) override;

private:
	int GetStringPixelWidth(wchar_t* pString, vgui::HFont hFont);
	void UpdateDistanceToPlayer(C_BasePlayer* player, const int pingIndex);
	void SetPos(const int index, const int playerTeam, const Vector& pos, bool ghosterPing);
	void NotifyPing(const int playerSlot);

private:
	playerPing m_iPlayerPings[MAX_PLAYERS] = {};

	int m_iPosX, m_iPosY;

	/*HSOUNDSCRIPTHANDLE pingSoundHandle;*/
	float m_flNextPingSoundTime = 0;

	vgui::HFont m_hFont = 0UL;
	vgui::HFont m_hFontSmall = 0UL;
	int m_iFontTall;

	vgui::HTexture m_hTexture = 0UL;
	int m_iTexWidth, m_iTexHeight;
	int m_iTexTall;
};
