#include "cbase.h"
#include "neo_teammenu.h"

#include <cdll_client_int.h>
#include <globalvars_base.h>
#include <cdll_util.h>
#include <KeyValues.h>

#include <vgui/IScheme.h>
#include <vgui/ILocalize.h>
#include <vgui/ISurface.h>
#include <vgui/IPanel.h>
#include <vgui/IInput.h>
#include <vgui_controls/ImageList.h>
#include <vgui_controls/MenuItem.h>
#include <vgui_controls/TextImage.h>
#include <vgui_controls/Divider.h>

#include <game_controls/IconPanel.h>

#include <vgui_int.h>

#include <stdio.h> // _snprintf define

#include <game/client/iviewport.h>
#include "commandmenu.h"
#include "hltvcamera.h"
#if defined( REPLAY_ENABLED )
#include "replay/replaycamera.h"
#endif

#include "c_neo_player.h"

//#include <game_controls/IconPanel.h>

#include <vgui_controls/TextEntry.h>
#include <vgui_controls/ImagePanel.h>
#include <vgui_controls/Panel.h>
#include <vgui_controls/Menu.h>
#include "IGameUIFuncs.h" // for key bindings
#include <imapoverview.h>
#include <shareddefs.h>
#include "neo_gamerules.h"
#include <igameresources.h>

#include <vgui/MouseCode.h>

#include "c_team.h"

#include "ienginevgui.h"

// NEO TODO (Rain): This file is based off Valve's SpectatorGUI.cpp,
// there's a good chance some of these includes aren't needed.
// Should clean up any unused ones.

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

using namespace vgui;

// These are defined in the .res file
#define CONTROL_JINRAI_BUTTON "neo_jinraibutton"
#define CONTROL_NSF_BUTTON "neo_nsfbutton"
#define CONTROL_SPEC_BUTTON "neo_specbutton"
#define CONTROL_AUTO_BUTTON "neo_autobutton"
#define CONTROL_CANCEL_BUTTON "neo_CancelButton"
#define CONTROL_JINRAI_IMAGE "neo_IconPanel1"
#define CONTROL_NSF_IMAGE "neo_IconPanel2"
#define CONTROL_BG_IMAGE "neo_IconPanel3"

#define CONTROL_TEAM_MENU_LABEL "neo_Label1"
#define CONTROL_JINRAI_PLAYERCOUNT_LABEL "neo_jplayercountlabel"
#define CONTROL_NSF_PLAYERCOUNT_LABEL "neo_nplayercountlabel"
#define CONTROL_JINRAI_SCORE_LABEL "neo_jscorelabel"
#define CONTROL_NSF_SCORE_LABEL "neo_nscorelabel"

#define CONTROL_DIVIDER "neo_Divider1"

Panel *NeoTeam_Factory()
{
	return new CNeoTeamMenu(gViewPortInterface);
}

DECLARE_BUILD_FACTORY_CUSTOM(CNeoTeamMenu, NeoTeam_Factory);

#ifndef _XBOX
extern IGameUIFuncs *gameuifuncs; // for key binding details
#endif

CNeoTeamMenu *g_pNeoTeamMenu = NULL;

CNeoTeamMenu::CNeoTeamMenu(IViewPort *pViewPort)
	: BaseClass(NULL, PANEL_TEAM)
{
	Assert(pViewPort);

	// Quiet "parent not sized yet" spew
	SetSize(10, 10);

	m_pViewPort = pViewPort;

	m_bTeamMenu = false;

	// NEO TODO (Rain): It appears that original Neotokyo
	// hardcodes its scheme. We probably need to make our
	// own res definition file to mimic it.
	SetScheme("ClientScheme");

	LoadControlSettings(GetResFile());

	SetVisible(false);
	SetProportional(true);
	SetMouseInputEnabled(true);
	SetKeyBoardInputEnabled(true);

	SetTitleBarVisible(false);

	m_pTeamMenuLabel = FindControl<Label>(CONTROL_TEAM_MENU_LABEL);
	m_pJinrai_PlayercountLabel = FindControl<Label>(CONTROL_JINRAI_PLAYERCOUNT_LABEL);
	m_pNSF_PlayercountLabel = FindControl<Label>(CONTROL_NSF_PLAYERCOUNT_LABEL);
	m_pJinrai_ScoreLabel = FindControl<Label>(CONTROL_JINRAI_SCORE_LABEL);
	m_pNSF_ScoreLabel = FindControl<Label>(CONTROL_NSF_SCORE_LABEL);

	m_pJinrai_Button = FindControl<Button>(CONTROL_JINRAI_BUTTON);
	m_pNSF_Button = FindControl<Button>(CONTROL_NSF_BUTTON);
	m_pSpectator_Button = FindControl<Button>(CONTROL_SPEC_BUTTON);
	m_pAutoAssign_Button = FindControl<Button>(CONTROL_AUTO_BUTTON);
	m_pCancel_Button = FindControl<Button>(CONTROL_CANCEL_BUTTON);

	// We probably failed to mount original NT VGUI assets if this triggers.
#ifdef DEBUG
	Assert(m_pTeamMenuLabel);
	Assert(m_pJinrai_PlayercountLabel);
	Assert(m_pNSF_PlayercountLabel);
	Assert(m_pJinrai_ScoreLabel);
	Assert(m_pNSF_ScoreLabel);

	Assert(m_pJinrai_Button);
	Assert(m_pNSF_Button);
	Assert(m_pSpectator_Button);
	Assert(m_pAutoAssign_Button);
	Assert(m_pCancel_Button);
#else
	if (!(m_pTeamMenuLabel && m_pJinrai_PlayercountLabel && m_pNSF_PlayercountLabel &&
		m_pJinrai_ScoreLabel && m_pNSF_ScoreLabel && m_pJinrai_Button && m_pNSF_Button &&
		m_pSpectator_Button && m_pAutoAssign_Button && m_pCancel_Button))
	{
		// We should never be able to hit this line.
		Error("Failed to load original Neotokyo VGUI elements! This is a bug, please report it.\n");
	}
#endif

	m_pJinrai_PlayercountLabel->SetAutoResize(vgui::Panel::PinCorner_e::PIN_CENTER_LEFT, vgui::Panel::AutoResize_e::AUTORESIZE_RIGHT, 0, 0, 0, 0);
	m_pJinrai_ScoreLabel->SetAutoResize(vgui::Panel::PinCorner_e::PIN_CENTER_LEFT, vgui::Panel::AutoResize_e::AUTORESIZE_RIGHT, 0, 0, 0, 0);
	m_pNSF_PlayercountLabel->SetAutoResize(vgui::Panel::PinCorner_e::PIN_CENTER_RIGHT, vgui::Panel::AutoResize_e::AUTORESIZE_RIGHT, 0, 0, 0, 0);
	m_pNSF_ScoreLabel->SetAutoResize(vgui::Panel::PinCorner_e::PIN_CENTER_RIGHT, vgui::Panel::AutoResize_e::AUTORESIZE_RIGHT, 0, 0, 0, 0);

	m_pJinrai_TeamImage = FindControl<IconPanel>(CONTROL_JINRAI_IMAGE, true);
	m_pNSF_TeamImage = FindControl<IconPanel>(CONTROL_NSF_IMAGE, true);
	m_pBgDarkGrey = FindControl<IconPanel>(CONTROL_BG_IMAGE, true);

	m_pDivider = FindControl<Divider>(CONTROL_DIVIDER);

	if (!m_pJinrai_TeamImage)
	{
		m_pJinrai_TeamImage = new IconPanel(this, CONTROL_JINRAI_IMAGE);
		Assert(m_pJinrai_TeamImage);

		m_pJinrai_TeamImage->SetPos(10, 17);
		m_pJinrai_TeamImage->SetSize(110, 110);
		m_pJinrai_TeamImage->SetAutoResize(
			vgui::Panel::PinCorner_e::PIN_TOPLEFT,
			vgui::Panel::AutoResize_e::AUTORESIZE_NO,
			0, 0, 0, 0);
		m_pJinrai_TeamImage->SetEnabled(true);
		m_pJinrai_TeamImage->SetTabPosition(0);
		m_pJinrai_TeamImage->SetIcon("vgui/jinrai_128tm");
	}

	if (!m_pNSF_TeamImage)
	{
		m_pNSF_TeamImage = new IconPanel(this, CONTROL_NSF_IMAGE);
		Assert(m_pNSF_TeamImage);

		m_pNSF_TeamImage->SetPos(125, 18);
		m_pNSF_TeamImage->SetSize(110, 110);
		m_pNSF_TeamImage->SetAutoResize(
			vgui::Panel::PinCorner_e::PIN_TOPLEFT,
			vgui::Panel::AutoResize_e::AUTORESIZE_NO,
			0, 0, 0, 0);
		m_pNSF_TeamImage->SetEnabled(true);
		m_pNSF_TeamImage->SetTabPosition(0);
		m_pNSF_TeamImage->SetIcon("vgui/nsf_128tm");
	}

	if (!m_pBgDarkGrey)
	{
		m_pBgDarkGrey = new IconPanel(this, CONTROL_BG_IMAGE);
		Assert(m_pBgDarkGrey);

		m_pBgDarkGrey->SetPos(10, 166);
		m_pBgDarkGrey->SetSize(340, 40);
		m_pBgDarkGrey->SetAutoResize(
			vgui::Panel::PinCorner_e::PIN_TOPLEFT,
			vgui::Panel::AutoResize_e::AUTORESIZE_NO,
			0, 0, 0, 0);
		m_pBgDarkGrey->SetEnabled(true);
		m_pBgDarkGrey->SetTabPosition(0);
		m_pBgDarkGrey->SetIconColor(COLOR_BLACK);
		m_pBgDarkGrey->SetIcon("vgui/darkgrey_background");
	}

#ifdef DEBUG
	Assert(m_pJinrai_TeamImage);
	Assert(m_pNSF_TeamImage);
	Assert(m_pBgDarkGrey);

	Assert(m_pDivider);
#else
	if (!(m_pJinrai_TeamImage && m_pNSF_TeamImage && m_pBgDarkGrey && m_pDivider))
	{
		// We should never be able to hit this line.
		Error("Failed to load original Neotokyo VGUI elements! This is a bug, please report it.\n");
	}
#endif

	m_pJinrai_Button->AddActionSignalTarget(this);
	m_pNSF_Button->AddActionSignalTarget(this);
	m_pSpectator_Button->AddActionSignalTarget(this);
	m_pAutoAssign_Button->AddActionSignalTarget(this);
	m_pCancel_Button->AddActionSignalTarget(this);

	InvalidateLayout();

	g_pNeoTeamMenu = this;
}

CNeoTeamMenu::~CNeoTeamMenu()
{
	m_pJinrai_Button->RemoveActionSignalTarget(this);
	m_pNSF_Button->RemoveActionSignalTarget(this);
	m_pSpectator_Button->RemoveActionSignalTarget(this);
	m_pAutoAssign_Button->RemoveActionSignalTarget(this);
	m_pCancel_Button->RemoveActionSignalTarget(this);

	// Make sure we deallocate all child elements
	m_pJinrai_TeamImage->SetAutoDelete(true);
	m_pNSF_TeamImage->SetAutoDelete(true);
	m_pBgDarkGrey->SetAutoDelete(true);

	m_pJinrai_Button->SetAutoDelete(true);
	m_pNSF_Button->SetAutoDelete(true);
	m_pSpectator_Button->SetAutoDelete(true);
	m_pAutoAssign_Button->SetAutoDelete(true);
	m_pCancel_Button->SetAutoDelete(true);

	g_pNeoTeamMenu = NULL;
}

void CNeoTeamMenu::Update()
{
#if(0)
	int numPlayersJinrai = 0, numPlayersNSF = 0;

	for (int i = 1; i <= gpGlobals->maxClients; i++)
	{
		auto player = UTIL_PlayerByIndex(i);

		if (player)
		{
			int team = player->GetTeamNumber();

			if (team == TEAM_JINRAI)
			{
				numPlayersJinrai++;
			}
			else if (team == TEAM_NSF)
			{
				numPlayersNSF++;
			}
		}
	}

	char textAscii[3]; // Support 2 chars + terminator
	wchar_t textUnicode_Jinrai[sizeof(wchar_t) * sizeof(textAscii)];
	wchar_t textUnicode_NSF[sizeof(wchar_t) * sizeof(textAscii)];

	inttostr(textAscii, 3, numPlayersJinrai);
	g_pVGuiLocalize->ConvertANSIToUnicode(textAscii, textUnicode_Jinrai, sizeof(textUnicode_Jinrai));
	m_pJinrai_PlayercountLabel->SetText(textUnicode_Jinrai);

	inttostr(textAscii, 3, numPlayersNSF);
	g_pVGuiLocalize->ConvertANSIToUnicode(textAscii, textUnicode_NSF, sizeof(textUnicode_NSF));
	m_pNSF_PlayercountLabel->SetText(textUnicode_NSF);
#endif
}

inline Button *CNeoTeamMenu::GetPressedButton()
{
	if (m_pJinrai_Button->IsCursorOver()) { return m_pJinrai_Button; }
	if (m_pNSF_Button->IsCursorOver()) { return m_pNSF_Button; }
	if (m_pSpectator_Button->IsCursorOver()) { return m_pSpectator_Button; }
	if (m_pAutoAssign_Button->IsCursorOver()) { return m_pAutoAssign_Button; }
	if (m_pCancel_Button->IsCursorOver()) { return m_pCancel_Button; }
	return NULL;
}

void CNeoTeamMenu::CommandCompletion()
{	
	SetControlEnabled(CONTROL_JINRAI_BUTTON, false);
	SetControlEnabled(CONTROL_NSF_BUTTON, false);
	SetControlEnabled(CONTROL_SPEC_BUTTON, false);
	SetControlEnabled(CONTROL_AUTO_BUTTON, false);
	SetControlEnabled(CONTROL_CANCEL_BUTTON, false);

	SetVisible(false);
	SetEnabled(false);

	SetMouseInputEnabled(false);
	SetCursorAlwaysVisible(false);
}

void CNeoTeamMenu::OnCommand(const char *command)
{
	BaseClass::OnCommand(command);

	if (*command == NULL)
	{
		return;
	}

	char commandBuffer[11];
	V_strcpy_safe(commandBuffer, command);

	// As a bit of a hack, we assume "jointeam 0" means autoassign in this context,
	// because that's how Neotokyo's .res file treats it.
	if (Q_strcmp(commandBuffer, "jointeam 0") == 0)
	{
		int randomTeam = RandomInt(TEAM_JINRAI, TEAM_NSF);
		V_sprintf_safe(commandBuffer, "jointeam %i", randomTeam);
	}

	engine->ClientCmd(commandBuffer);

	bool proceedToClassSelection = (Q_stristr(commandBuffer, "jointeam") != 0);

	if (proceedToClassSelection)
	{
		C_NEO_Player *player = C_NEO_Player::GetLocalNEOPlayer();
		if (player)
		{
			player->m_bShowTeamMenu = false;
			player->m_bShowClassMenu = true;
		}
		else
		{
			Assert(false);
		}
	}

	CommandCompletion();
}

void CNeoTeamMenu::OnButtonPressed(KeyValues *data)
{
#if(0)
	KeyValuesDumpAsDevMsg(data);
#endif
}

void CNeoTeamMenu::ApplySchemeSettings(vgui::IScheme *pScheme)
{
    BaseClass::ApplySchemeSettings(pScheme);

	if (!pScheme)
	{
		Assert(false);
		Warning("Failed to ApplySchemeSettings for CNeoTeamMenu\n");
		return;
	}

    LoadControlSettings(GetResFile());

	SetBgColor(Color(0, 0, 0, 196));

	this->SetPos(332, 276);
	this->SetSize(360, 215);

	//vgui::HScheme neoScheme = vgui::scheme()->LoadSchemeFromFileEx(
	//	enginevgui->GetPanel(PANEL_CLIENTDLL), "resource/ClientScheme_Neo.res", "ClientScheme_Neo");
	//vgui::IScheme* neoIScheme = vgui::scheme()->GetIScheme(neoScheme);
	//Assert(neoIScheme);
	//
	//const vgui::HFont font = neoIScheme->GetFont("BOOT", false);// IsProportional());
	//Assert(font > 0);

	Assert(m_pJinrai_Button);
	Assert(m_pNSF_Button);
	Assert(m_pSpectator_Button);
	Assert(m_pAutoAssign_Button);
	Assert(m_pCancel_Button);

	//m_pJinrai_Button->SetFont(font);
	//m_pNSF_Button->SetFont(font);
	//m_pSpectator_Button->SetFont(font);
	//m_pAutoAssign_Button->SetFont(font);
	//// NEO FIXME (Rain): this line rarely throws; I have no idea why.
	//// The assertions above are not hit when it occurs.
	//m_pCancel_Button->SetFont(font);

	//if (g_hFontJoinMenus == vgui::INVALID_FONT)
	//{
	//	auto pNeoKillfeedOverrideScheme = vgui::scheme()->GetIScheme(vgui::scheme()->
	//		LoadSchemeFromFile("resource/ClientScheme_JoinMenus.res", "JoinMenus"));
	//	Assert(pNeoKillfeedOverrideScheme);
	//	if (pNeoKillfeedOverrideScheme)
	//	{
	//		g_hFontKillfeed = pNeoKillfeedOverrideScheme->GetFont("joinmenus");
	//		Assert(g_hFontKillfeed != vgui::INVALID_FONT);
	//	}
	//}
	//
	//m_pJinrai_Button->SetFont(g_hFontKillfeed);

	m_pJinrai_Button->SizeToContents();
	m_pNSF_Button->SizeToContents();
	m_pSpectator_Button->SizeToContents();
	m_pAutoAssign_Button->SizeToContents();
	m_pCancel_Button->SizeToContents();

	const Color selectedBgColor(0, 0, 0), selectedFgColor(255, 0, 0),
		armedBgColor(0, 0, 0), armedFgColor(0, 255, 0);

	const bool bUseCaptureMouse = true, bMouseInputEnabled = true;

	m_pJinrai_Button		->SetUseCaptureMouse(bUseCaptureMouse);
	m_pNSF_Button			->SetUseCaptureMouse(bUseCaptureMouse);
	m_pSpectator_Button		->SetUseCaptureMouse(bUseCaptureMouse);
	m_pAutoAssign_Button	->SetUseCaptureMouse(bUseCaptureMouse);
	m_pCancel_Button		->SetUseCaptureMouse(bUseCaptureMouse);

	m_pJinrai_Button		->SetMouseInputEnabled(bMouseInputEnabled);
	m_pNSF_Button			->SetMouseInputEnabled(bMouseInputEnabled);
	m_pSpectator_Button		->SetMouseInputEnabled(bMouseInputEnabled);
	m_pAutoAssign_Button	->SetMouseInputEnabled(bMouseInputEnabled);
	m_pCancel_Button		->SetMouseInputEnabled(bMouseInputEnabled);

	m_pJinrai_Button		->InstallMouseHandler(this);
	m_pNSF_Button			->InstallMouseHandler(this);
	m_pSpectator_Button		->InstallMouseHandler(this);
	m_pAutoAssign_Button	->InstallMouseHandler(this);
	m_pCancel_Button		->InstallMouseHandler(this);

	m_pJinrai_Button		->SetSelectedColor(selectedFgColor, selectedBgColor);
	m_pNSF_Button			->SetSelectedColor(selectedFgColor, selectedBgColor);
	m_pSpectator_Button		->SetSelectedColor(selectedFgColor, selectedBgColor);
	m_pAutoAssign_Button	->SetSelectedColor(selectedFgColor, selectedBgColor);
	m_pCancel_Button		->SetSelectedColor(selectedFgColor, selectedBgColor);

	m_pJinrai_Button		->SetArmedColor(armedFgColor, armedBgColor);
	m_pNSF_Button			->SetArmedColor(armedFgColor, armedBgColor);
	m_pSpectator_Button		->SetArmedColor(armedFgColor, armedBgColor);
	m_pAutoAssign_Button	->SetArmedColor(armedFgColor, armedBgColor);
	m_pCancel_Button		->SetArmedColor(armedFgColor, armedBgColor);

    SetPaintBorderEnabled(false);

    SetBorder( NULL );

	SetMinimumSize(900, 900);

	auto pJinrai = GetGlobalTeam(TEAM_JINRAI);
	auto pNsf = GetGlobalTeam(TEAM_NSF);

	const int jinScore = (pJinrai != NULL ? pJinrai->Get_Score() : 0);
	const int jinNumPlayers = (pJinrai != NULL ? pJinrai->Get_Number_Players() : 0);

	const int nsfScore = (pNsf != NULL ? pNsf->Get_Score() : 0);
	const int nsfNumPlayers = (pNsf != NULL ? pNsf->Get_Number_Players() : 0);

	Assert(m_pJinrai_ScoreLabel);
	Assert(m_pNSF_ScoreLabel);
	Assert(m_pJinrai_PlayercountLabel);
	Assert(m_pNSF_PlayercountLabel);

	char textBuff[13 + 1];
	V_sprintf_safe(textBuff, "SCORE:%d", jinScore);
	m_pJinrai_ScoreLabel->SetText(textBuff);

	V_sprintf_safe(textBuff, "SCORE:%d", nsfScore);
	m_pNSF_ScoreLabel->SetText(textBuff);

	V_sprintf_safe(textBuff, "PLAYERS: %d", jinNumPlayers);
	m_pJinrai_PlayercountLabel->SetText(textBuff);

	V_sprintf_safe(textBuff, "PLAYERS: %d", nsfNumPlayers);
	m_pNSF_PlayercountLabel->SetText(textBuff);

	m_pJinrai_ScoreLabel->SizeToContents();
	m_pNSF_ScoreLabel->SizeToContents();
	m_pJinrai_PlayercountLabel->SizeToContents();
	m_pNSF_PlayercountLabel->SizeToContents();
}

void CNeoTeamMenu::MoveLabelToFront(const char *textEntryName)
{
    Label *entry = dynamic_cast<Label *>(FindChildByName(textEntryName));
    if (entry)
    {
        entry->MoveToFront();
    }
}

void CNeoTeamMenu::SetLabelText(const char *textEntryName, const char *text)
{
    Label *entry = dynamic_cast<Label *>(FindChildByName(textEntryName));
    if (entry)
    {
        entry->SetText(text);
    }
}

void CNeoTeamMenu::SetLabelText(const char *textEntryName, wchar_t *text)
{
    Label *entry = dynamic_cast<Label *>(FindChildByName(textEntryName));
    if (entry)
    {
        entry->SetText(text);
    }
}

void CNeoTeamMenu::ShowPanel( bool bShow )
{
    if (bShow && !IsVisible())
    {
        m_bTeamMenu = false;
    }

    SetVisible(bShow);

    if (!bShow && m_bTeamMenu)
    {
        gViewPortInterface->ShowPanel(PANEL_TEAM, false);
    }
}
