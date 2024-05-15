#include "cbase.h"
#include "neo_scoreboard.h"

#include "hud.h"
#include "cdll_util.h"
#include "c_team.h"
#include "c_playerresource.h"
#include "c_neo_player.h"
#include "neo_gamerules.h"

#include <KeyValues.h>
#include "ienginevgui.h"
#include "voice_status.h"

#include <vgui/IScheme.h>
#include <vgui/ILocalize.h>
#include <vgui/ISurface.h>
#include <vgui/IVGui.h>
#include <vgui_controls/SectionedListPanel.h>

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

using namespace vgui;

enum EScoreboardSections
{
	SCORESECTION_HEADER = 0,
	SCORESECTION_JINRAI,
	SCORESECTION_NSF,
	SCORESECTION_SPECTATOR,
};

CNEOScoreBoard::CNEOScoreBoard(IViewPort *pViewPort)
	: CHL2MPClientScoreBoardDialog(pViewPort)
{
	vgui::HScheme neoscheme = vgui::scheme()->LoadSchemeFromFileEx(
		enginevgui->GetPanel(PANEL_CLIENTDLL), "resource/ClientScheme.res", "ClientScheme");
	SetScheme(neoscheme);
}

CNEOScoreBoard::~CNEOScoreBoard()
{
}

void CNEOScoreBoard::InitScoreboardSections()
{
	m_pPlayerList->SetBgColor(Color(0, 0, 0, 0));
	m_pPlayerList->SetBorder(NULL);

	vgui::IScheme* scheme = vgui::scheme()->GetIScheme(GetScheme());
	Assert(scheme);
	m_pPlayerList->SetProportional(true);
	auto hFont = scheme->GetFont("DefaultBold", true);
	m_pPlayerList->SetHeaderFont(hFont);
	m_pPlayerList->SetRowFont(hFont);

	// fill out the structure of the scoreboard
	AddHeader();

	// add the team sections
	AddSection(TYPE_TEAM, TEAM_JINRAI);
	AddSection(TYPE_TEAM, TEAM_NSF);
	AddSection(TYPE_SPECTATORS, TEAM_SPECTATOR);
}

// Purpose: Update the scoreboard rows with currently connected players' info
void CNEOScoreBoard::UpdatePlayerInfo()
{
	if (!g_PR)
	{
		return;
	}

	CBasePlayer *player = C_BasePlayer::GetLocalPlayer();
	Assert(player);

	//const Color test = Color(255, 0, 0, 255);
	//surface()->DrawSetTextColor(test);
	int teamCountJinrai = 0;
	int teamCountNSF = 0;

	for (int i = 1; i <= gpGlobals->maxClients; i++)
	{
		int itemId = FindItemIDForPlayerIndex(i);

		if (g_PR->IsConnected(i))
		{
			KeyValues *playerData = new KeyValues("data");

			GetPlayerScoreInfo(i, playerData);
			const int playerTeam = g_PR->GetTeam(i);
			int sectionId = GetSectionFromTeamNumber(playerTeam);

			if (playerTeam == TEAM_JINRAI) ++teamCountJinrai;
			else if (playerTeam == TEAM_NSF) ++teamCountNSF;

			// We aren't in the scoreboard yet
			if (itemId == -1)
			{
				itemId = m_pPlayerList->AddItem(sectionId, playerData);
				m_pPlayerList->SetItemFgColor(itemId, GameResources()->GetTeamColor(playerTeam));
			}
			else
			{
				m_pPlayerList->ModifyItem(itemId, sectionId, playerData);
			}

			playerData->deleteThis();
		}
		// We have itemId for unconnected player, remove it
		else if (itemId != -1)
		{
			m_pPlayerList->RemoveItem(itemId);
		}
	}

	// Update team headers
	auto *teamJinrai = GetGlobalTeam(TEAM_JINRAI);
	auto *teamNSF = GetGlobalTeam(TEAM_NSF);
	if (teamJinrai && teamNSF)
	{
		char szTeamHeaderText[256];
		wchar_t wszTeamHeaderText[256];

		memset(szTeamHeaderText, 0, sizeof(szTeamHeaderText));
		V_snprintf(szTeamHeaderText, sizeof(szTeamHeaderText), "JINRAI:       Score: %d    Players: %d", teamJinrai->GetRoundsWon(), teamCountJinrai);
		g_pVGuiLocalize->ConvertANSIToUnicode(szTeamHeaderText, wszTeamHeaderText, sizeof(wszTeamHeaderText));
		m_pPlayerList->ModifyColumn(SCORESECTION_JINRAI, "name", wszTeamHeaderText);

		memset(szTeamHeaderText, 0, sizeof(szTeamHeaderText));
		V_snprintf(szTeamHeaderText, sizeof(szTeamHeaderText), "NSF:           Score: %d    Players: %d", teamNSF->GetRoundsWon(), teamCountNSF);
		g_pVGuiLocalize->ConvertANSIToUnicode(szTeamHeaderText, wszTeamHeaderText, sizeof(wszTeamHeaderText));
		m_pPlayerList->ModifyColumn(SCORESECTION_NSF, "name", wszTeamHeaderText);
	}

	m_pPlayerList->SetVisible(true);
}

void CNEOScoreBoard::UpdateTeamInfo()
{
	BaseClass::UpdateTeamInfo();
}

bool CNEOScoreBoard::GetPlayerScoreInfo(int playerIndex, KeyValues *outPlayerInfo)
{
	return BaseClass::GetPlayerScoreInfo(playerIndex, outPlayerInfo);
}

void CNEOScoreBoard::PaintBackground()
{
	BaseClass::PaintBackground();
}

void CNEOScoreBoard::PaintBorder()
{
	BaseClass::PaintBorder();
}

void CNEOScoreBoard::ApplySchemeSettings(vgui::IScheme *pScheme)
{
	BaseClass::ApplySchemeSettings(pScheme);
}

#define ADD_COL(sectionId, columnName, columnText, columnFlags, width, fallbackFont) \
	m_pPlayerList->AddColumnToSection(sectionId, columnName, columnText, columnFlags, \
	scheme()->GetProportionalScaledValueEx(GetScheme(), width), fallbackFont)
#define COL_IMAGE SectionedListPanel::COLUMN_IMAGE

void CNEOScoreBoard::AddHeader()
{
	HFont hFallbackFont = scheme()->GetIScheme(GetScheme())->
		GetFont("DefaultVerySmallFallBack", false);

	m_pPlayerList->AddSection(0, "");
	m_pPlayerList->SetSectionAlwaysVisible(0);
	ADD_COL(0, "name", "", 0, NEO_NAME_WIDTH, hFallbackFont);
	ADD_COL(0, "rank", "Rank", 0, NEO_NAME_WIDTH / 4, hFallbackFont);
	ADD_COL(0, "class", "Class", 0, NEO_CLASS_WIDTH, hFallbackFont);
	ADD_COL(0, "xp", "XP", 0, NEO_SCORE_WIDTH, hFallbackFont);
	ADD_COL(0, "deaths", "#PlayerDeath", 0, NEO_DEATH_WIDTH, hFallbackFont);
	ADD_COL(0, "ping", "Ping", 0, NEO_PING_WIDTH, hFallbackFont);
	ADD_COL(0, "status", "Status", 0, NEO_STATUS_WIDTH, hFallbackFont);
	//ADD_COL(0, "voice", "#PlayerVoice", COL_IMAGE, NEO_VOICE_WIDTH, hFallbackFont);
	//ADD_COL(0, "tracker", "#PlayerTracker", COL_IMAGE, NEO_FRIENDS_WIDTH, hFallbackFont);
}

void CNEOScoreBoard::AddSection(int teamType, int teamNumber)
{
	HFont hFallbackFont = scheme()->GetIScheme(GetScheme())->
		GetFont("DefaultVerySmallFallBack", false);

	const int sectionID = GetSectionFromTeamNumber(teamNumber);
	m_pPlayerList->AddSection(sectionID, "", (teamNumber == TEAM_SPECTATOR) ? NULL : StaticPlayerSortFunc);
	m_pPlayerList->SetSectionAlwaysVisible(sectionID);

	// setup the columns
	if (teamType == TYPE_TEAM)
	{
		ADD_COL(sectionID, "name", "", 0, NEO_NAME_WIDTH, hFallbackFont);
		ADD_COL(sectionID, "rank", "", 0, NEO_NAME_WIDTH / 4, hFallbackFont);
		ADD_COL(sectionID, "class", "", 0, NEO_CLASS_WIDTH, hFallbackFont);
		ADD_COL(sectionID, "xp", "", 0, NEO_SCORE_WIDTH, hFallbackFont);
		ADD_COL(sectionID, "deaths", "", 0, NEO_DEATH_WIDTH, hFallbackFont);
		ADD_COL(sectionID, "ping", "", 0, NEO_PING_WIDTH, hFallbackFont);
		ADD_COL(sectionID, "status", "", 0, NEO_STATUS_WIDTH, hFallbackFont);
	}
	else if (teamType == TYPE_SPECTATORS)
	{
		ADD_COL(sectionID, "name", "Spectator", 0, NEO_NAME_WIDTH, hFallbackFont);
	}

	// set the section to have the team color
	if (GameResources())
	{
		m_pPlayerList->SetSectionFgColor(sectionID, (teamNumber == TEAM_SPECTATOR) ? COLOR_NEO_WHITE : GameResources()->GetTeamColor(teamNumber));
	}
}

int CNEOScoreBoard::GetSectionFromTeamNumber(int teamNumber)
{
	if (teamNumber == TEAM_JINRAI) { return SCORESECTION_JINRAI; }
	else if (teamNumber == TEAM_NSF) { return SCORESECTION_NSF; }
	else if (teamNumber == TEAM_SPECTATOR) { return SCORESECTION_SPECTATOR; }
	
	// We shouldn't ever fall through
	Assert(false);

	return SCORESECTION_SPECTATOR;
}

void CNEOScoreBoard::SetVisible(bool state)
{
	// Temp fix to show team scores etc when opening scoreboard.
	if (C_NEO_Player::GetLocalNEOPlayer()->IsAlive())
	{
		gViewPortInterface->ShowPanel(gViewPortInterface->FindPanelByName(PANEL_SPECGUI), state);
	}

	BaseClass::SetVisible(state);
}
