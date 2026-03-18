#ifndef NEO_HUD_ROUND_STATE_H
#define NEO_HUD_ROUND_STATE_H
#ifdef _WIN32
#pragma once
#endif

#include "neo_gamerules.h"
#include "neo_hud_childelement.h"
#include "neo_player_shared.h"
#include "hudelement.h"

#include <vgui/ISurface.h>
#include <vgui_controls/Panel.h>

namespace vgui {
class ImagePanel;
class ImageList;
}

constexpr int MAX_GAME_TYPE_OBJECTIVE_LENGTH = 33;

class CNEOHud_RoundState : public CNEOHud_ChildElement, public CHudElement, public vgui::Panel
{
	DECLARE_CLASS_SIMPLE(CNEOHud_RoundState, Panel);

public:
	CNEOHud_RoundState(const char *pElementName, vgui::Panel *parent = NULL);

	virtual void ApplySchemeSettings(vgui::IScheme *pScheme);
	virtual void Paint();
	
	void UpdateAvatarSize();
	void UpdateStarSize();
	// sunk cost fallacy and all that, in hindsight I should have made all of this only work with two teams and split players into separate resizeable cutlvectors by team instead of doing this minus index stuff.
	// basically any minusindexed n index works such that negative values give the -nth player in the player list, and any positive values give the (n - leftTeamTotal)th player in the list
	int GetEntityIndexAtPositionInHud(int position, bool positionIsZeroIndexed = false);
	int GetMinusIndexedPositionOfPlayerInHud(int entIndex);
	int GetNextAlivePlayerInHud(int minusIndexedPosition = 0, bool reverse = false);
	void SelectNextAlivePlayerInHud();
	void SelectPreviousAlivePlayerInHud();
	int GetSelectedPlayerInHud();

protected:
	virtual void UpdateStateForNeoHudElementDraw();
	virtual void DrawNeoHudElement();
	virtual ConVar* GetUpdateFrequencyConVar() const;

private:
	struct TeamLogoColor
	{
		int logo;
		int totalLogo;
		Color color;
	};

	void CheckActiveStar();
	void DrawPlayerList();
	void DrawPlayerList_BotCmdr();
	int DrawPlayerRow(int playerIndex, int yOffset, bool small = false);
	int DrawPlayerRow_BotCmdr(int playerIndex, int yOffset, bool small = false, const Color* highlightColor = nullptr);
	void DrawPlayer(int playerIndex, int teamIndex, const TeamLogoColor &teamLogoColor,
					const int xOffset, const bool drawHealthClass, const bool isSelected = false);
	void SetTextureToAvatar(int playerIndex);

	virtual void LevelShutdown(void) override;

private:
	vgui::HFont m_hOCRLargeFont = 0UL;
	vgui::HFont m_hOCRFont = 0UL;
	vgui::HFont m_hOCRSmallFont = 0UL;
	vgui::HFont m_hOCRSmallerFont = 0UL;
	vgui::HFont m_hTinyText = 0UL;
	int m_iTinyTextHeight = 0;

	int m_iXpos = 0;

	// Center Box
	int m_iBoxYEnd = 0;
	int m_iSmallFontHeight = 0;

	// Center Box info
	wchar_t m_wszRoundUnicode[12] = {};
	int m_iWszRoundUCSize = 0;
	wchar_t m_wszTime[6] = {};
	wchar_t m_wszLeftTeamScore[3] = {};
	wchar_t m_wszRightTeamScore[3] = {};
	wchar_t m_wszPlayersAliveUnicode[16] = {};
	const wchar_t *m_pWszStatusUnicode = nullptr;
	int m_iStatusUnicodeSize = 0;

	// Totals info
	int m_iLeftPlayersAlive = 0;
	int m_iLeftPlayersTotal = 0;
	int m_iRightPlayersAlive = 0;
	int m_iRightPlayersTotal = 0;

	// Element Positions
	IntPos m_posLeftTeamScore = {};
	IntPos m_posRightTeamScore = {};
	int m_iLeftOffset = 0;
	int m_iRightOffset = 0;
	vgui::IntRect m_rectLeftTeamTotalLogo = {};
	vgui::IntRect m_rectRightTeamTotalLogo = {};
	int m_ilogoSize = 0;

	vgui::ImagePanel *m_ipStars[STAR__TOTAL] = {};
	int m_iPreviouslyActiveStar = -1;
	int m_iPreviouslyActiveTeam = -1;

	int m_iClassIcons = 0;
	TeamLogoColor m_teamLogoColors[TEAM__TOTAL] = {};

	int m_iBeepSecsTotal = 0;
	NeoRoundStatus m_ePrevRoundStatus = NeoRoundStatus::Idle;

	// Bot Commander Lists
	CUtlVector<int> m_commandedList;
	CUtlVector<int> m_nonCommandedList;
	CUtlVector<int> m_nonSquadList;

	// Top Squad List
	// Players who are connected, in a game team etc will have a positive value and sit at the front of the list. Order of negative value players (not connected, non-game team etc) not guaranteed
	struct playerIndexAndTheirValue
	{
		int playerIndex;
		int playerValue;
	};
	CUtlVector<playerIndexAndTheirValue> m_nPlayerList;

	// Used to index the list above. Index of 0 is invalid. Positive values correspond to team NSF, with index 1 being the first member of NSF. 
	// Negative values correspond to team Jinrai, with index -1 being the first member of Jinrai (Team Jinrai is drawn right to left, while moving left to right starting at index 0 in m_nPlayerList)
	int m_iSelectedPlayer = 0;		
	float m_flSelectedPlayerChangeTime = -1;

	CPanelAnimationVar(Color, box_color, "box_color", "200 200 200 40");
	CPanelAnimationVarAliasType(bool, health_monochrome, "health_monochrome", "1", "bool");
	CPanelAnimationVarAliasType(bool, top_left_corner, "top_left_corner", "0", "bool");
	CPanelAnimationVarAliasType(bool, top_right_corner, "top_right_corner", "0", "bool");
	CPanelAnimationVarAliasType(bool, bottom_left_corner, "bottom_left_corner", "1", "bool");
	CPanelAnimationVarAliasType(bool, bottom_right_corner, "bottom_right_corner", "1", "bool");

private:
	CNEOHud_RoundState(const CNEOHud_RoundState &other);
};

extern CNEOHud_RoundState *g_pNeoHudRoundState;
inline CNEOHud_RoundState* NeoHudRoundState()
{
	return g_pNeoHudRoundState;
}
#endif // NEO_HUD_ROUND_STATE_H
