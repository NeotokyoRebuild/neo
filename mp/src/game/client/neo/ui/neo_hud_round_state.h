#ifndef NEO_HUD_ROUND_STATE_H
#define NEO_HUD_ROUND_STATE_H
#ifdef _WIN32
#pragma once
#endif

#include "neo_hud_childelement.h"
#include "hudelement.h"
#include <vgui_controls/Panel.h>

namespace vgui {
class ImagePanel;
}

class CNEOHud_RoundState : public CNEOHud_ChildElement, public CHudElement, public vgui::Panel
{
	DECLARE_CLASS_SIMPLE(CNEOHud_RoundState, Panel);

public:
	CNEOHud_RoundState(const char *pElementName, vgui::Panel *parent = NULL);

	virtual void ApplySchemeSettings(vgui::IScheme *pScheme);
	virtual void Paint();
	virtual void DeleteImageList() { if (m_pImageList) { delete m_pImageList; } };

protected:
	virtual void UpdateStateForNeoHudElementDraw();
	virtual void DrawNeoHudElement();
	virtual ConVar* GetUpdateFrequencyConVar() const;

private:
	void FireGameEvent(IGameEvent* event) OVERRIDE;
	void CheckActiveStar();
	void DrawFriend(int playerIndex, int teamIndex);
	void DrawEnemy(int playerIndex, int teamIndex);
	void UpdatePlayerAvatar(int playerIndex);
	void CNEOHud_RoundState::SetTextureToAvatar(int playerIndex);

private:
	vgui::HFont m_hOCRSmallFont;
	vgui::HFont m_hOCRFont;
	vgui::HFont m_hOCRLargeFont;

	int m_resX, m_resY;
	int m_iXpos, m_iYpos;

	// Center Box
	int m_iBoxWidth, m_iBoxHeight;
	int m_iBoxX0, m_iBoxY0, m_iBoxX1, m_iBoxY1;

	int m_iSmallFontWidth, m_iSmallFontHeight;
	int m_iFontWidth, m_iFontHeight;

	// Center Box info
	char m_szRoundANSI[9];
	wchar_t m_wszRoundUnicode[9];
	wchar_t m_wszTime[6];
	int m_iJinraiScore;
	int m_iNSFScore;
	wchar_t m_wszFriendlyScore[3];
	wchar_t m_wszEnemyScore[3];

	// Totals info
	int m_iFriendsAlive;
	int m_iFriendsTotal;
	int m_iEnemiesAlive;
	int m_iEnemiesTotal;
	char m_szPlayersAliveANSI[9];
	wchar_t m_wszPlayersAliveUnicode[9];

	char m_szStatusANSI[24];
	wchar_t m_wszStatusUnicode[24];

	// Element Positions
	int m_iTimeYpos; // time changes often, don't bother keeping track of time x pos
	int m_iFriendlyScoreXpos, m_iFriendlyScoreYpos;
	int m_iEnemyScoreXpos, m_iEnemyScoreYpos;
	int m_iRoundXpos, m_iRoundYpos;
	int m_iRoundStatusYpos;
	int m_iFriendlyTotalLogoX0, m_iFriendlyTotalLogoY0, m_iFriendlyTotalLogoX1, m_iFriendlyTotalLogoY1;
	int m_iEnemyTotalLogoX0, m_iEnemyTotalLogoY0, m_iEnemyTotalLogoX1, m_iEnemyTotalLogoY1;
	int m_iPlayersAliveNumXpos, m_iPlayersAliveNumYpos;
	int m_ilogoSize;
	int m_ilogoTotalSize;
	int m_iFriendlyLogoXOffset;
	int m_iEnemyLogoXOffset;

	vgui::ImagePanel *starNone, *starA, *starB, *starC, *starD, *starE, *starF;
	int m_iPreviouslyActiveStar;
	int m_iPreviouslyActiveTeam;

	// Graphic IDs
	int m_iJinraiID;
	int m_iJinraiTotalID;
	int m_iNSFID;
	int m_iNSFTotalID;
	int m_iSupportID;
	int m_iAssaultID;
	int m_iReconID;
	int m_iVIPID;
	
	// Graphic IDs used
	int m_iFriendlyLogo;
	int m_iFriendlyTotalLogo;
	int m_iEnemyLogo;
	int m_iEnemyTotalLogo;

	Color whiteColor = Color(255, 255, 255, 255);
	Color fadedWhiteColor = Color(255, 255, 255, 176);
	Color darkColor = Color(55, 55, 55, 255);
	Color fadedDarkColor = Color(55, 55, 55, 176);
	Color friendlyColor;
	Color enemyColor;
	Color deadColor = Color(155, 155, 155, 255);

	vgui::ImageList* m_pImageList;
	CUtlMap<CSteamID, int>		m_mapAvatarsToImageList;

	CPanelAnimationVarAliasType(int, box_color_r, "box_color_r", "116", "int");
	CPanelAnimationVarAliasType(int, box_color_g, "box_color_g", "116", "int");
	CPanelAnimationVarAliasType(int, box_color_b, "box_color_b", "116", "int");
	CPanelAnimationVarAliasType(int, box_color_a, "box_color_a", "178", "int");

private:
	CNEOHud_RoundState(const CNEOHud_RoundState &other);
};

#endif // NEO_HUD_ROUND_STATE_H
