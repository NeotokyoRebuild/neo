#ifndef NEO_HUD_ROUND_STATE_H
#define NEO_HUD_ROUND_STATE_H
#ifdef _WIN32
#pragma once
#endif

#include "neo_hud_childelement.h"
#include "hudelement.h"
#include <vgui_controls/Panel.h>

class vgui::ImagePanel;

class CNEOHud_RoundState : public CNEOHud_ChildElement, public CHudElement, public vgui::Panel
{
	DECLARE_CLASS_SIMPLE(CNEOHud_RoundState, Panel);

public:
	CNEOHud_RoundState(const char *pElementName, vgui::Panel *parent = NULL);

	virtual void ApplySchemeSettings(vgui::IScheme *pScheme);
	virtual void Paint();

protected:
	virtual void UpdateStateForNeoHudElementDraw();
	virtual void DrawNeoHudElement();
	virtual ConVar* GetUpdateFrequencyConVar() const;

private:
	void CheckActiveStar();
	void DrawFriend(int playerIndex, int teamIndex, int xpos, int boxWidth);
	void DrawEnemy(int playerIndex, int teamIndex, int xpos, int boxWidth);

private:
	vgui::HFont m_hFont;

	int m_resX, m_resY;

	char m_szStatusANSI[24];
	wchar_t m_wszStatusUnicode[24];
	wchar_t m_wszJinraiScore[3];
	wchar_t m_wszNSFScore[3];
	wchar_t m_wszRound[3];

	vgui::ImagePanel *starNone, *starA, *starB, *starC, *starD, *starE, *starF;
	int m_iPreviouslyActiveStar;
	int m_iPreviouslyActiveTeam;

	int m_iJinraiID;
	int m_iJinraiTotalID;
	int m_iNSFID;
	int m_iNSFTotalID;
	int m_iSupportID;
	int m_iAssaultID;
	int m_iReconID;
	int m_iVIPID;
	int m_iFriendlyLogo;
	int m_iEnemyLogo;
	int m_iFriendlyTotalLogo;
	int m_iEnemyTotalLogo;

	int m_ilogoSize;
	int m_ilogoTotalSize;
	int m_iFriendsAlive;
	int m_iFriendsTotal;
	int m_iEnemiesAlive;
	int m_iEnemiesTotal;

	wchar_t m_wszFriendsAlive[4];
	wchar_t m_wszEnemiesAlive[4];

	int box_color_r, box_color_g, box_color_b, box_color_a;

private:
	CNEOHud_RoundState(const CNEOHud_RoundState &other);
};

#endif // NEO_HUD_ROUND_STATE_H
