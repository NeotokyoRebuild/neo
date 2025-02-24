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
	void DrawPlayer(int playerIndex, int teamIndex, const TeamLogoColor &teamLogoColor,
					const int xOffset, const bool drawHealthClass);
	void SetTextureToAvatar(int playerIndex);

private:
	vgui::HFont m_hOCRSmallFont = 0UL;
	vgui::HFont m_hOCRFont = 0UL;

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
	wchar_t m_wszGameTypeDescription[MAX_GAME_TYPE_OBJECTIVE_LENGTH] = {};
	char szGameTypeDescription[MAX_GAME_TYPE_OBJECTIVE_LENGTH] = {};

	// Game Description
	short m_iGameTypeDescriptionState = 0;

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

	int m_iGraphicID[NEO_CLASS__ENUM_COUNT] = {};
	TeamLogoColor m_teamLogoColors[TEAM__TOTAL] = {};


	CPanelAnimationVar(Color, box_color, "box_color", "200 200 200 40");
	CPanelAnimationVarAliasType(bool, health_monochrome, "health_monochrome", "1", "bool");

private:
	CNEOHud_RoundState(const CNEOHud_RoundState &other);
};

#endif // NEO_HUD_ROUND_STATE_H
