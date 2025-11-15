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
	int victimUserID;
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
	void DrawPings(playerPing* pings, int localPlayerTeam, int spectateTargetTeam, int playerPingsInSpectate, bool isEnemyPings, bool isDeathPing);
	int GetStringPixelWidth(wchar_t* pString, vgui::HFont hFont);
	void UpdateDistanceToPlayer(C_BasePlayer* player, const int pingIndex, bool isShotPing, bool isDeathPing);
	void SetPos(const int index, const int playerTeam, const Vector& pos, bool ghosterPing, bool isShotPing, bool isDeathPing, int shotUserID);
	void NotifyPing(const int playerSlot, bool isShotPing, bool isDeathPing);

private:
	playerPing m_iPlayerPings[MAX_PLAYERS] = {};
	playerPing m_iEnemyHitPings[MAX_PLAYERS] = {};
	playerPing m_iDeathPings[MAX_PLAYERS] = {};

	int m_iPosX, m_iPosY;

	/*HSOUNDSCRIPTHANDLE pingSoundHandle;*/
	float m_flNextPingSoundTime = 0;

	vgui::HFont m_hFont = 0UL;
	vgui::HFont m_hFontSmall = 0UL;
	int m_iFontTall;

	vgui::HTexture m_hTexture = 0UL;
	vgui::HTexture m_hDeathPingTexture = 0UL;
	int m_iTexWidth, m_iTexHeight;
	int m_iTexTall;
};
