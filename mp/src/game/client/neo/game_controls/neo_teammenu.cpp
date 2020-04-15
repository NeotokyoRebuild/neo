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

// NEO TODO (Rain): This file is based off Valve's SpectatorGUI.cpp,
// there's a good chance some of these includes aren't needed.
// Should clean up any unused ones.

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

using namespace vgui;

// These are defined in the .res file
#define CONTROL_JINRAI_BUTTON "jinraibutton"
#define CONTROL_NSF_BUTTON "ctbutton"
#define CONTROL_SPEC_BUTTON "specbutton"
#define CONTROL_AUTO_BUTTON "autobutton"
#define CONTROL_CANCEL_BUTTON "CancelButton"
#define CONTROL_JINRAI_IMAGE "ImagePanel1"
#define CONTROL_NSF_IMAGE "ImagePanel2"

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

	m_pJinrai_TeamImage = FindControl<ImagePanel>(CONTROL_JINRAI_IMAGE, false);
	m_pNSF_TeamImage = FindControl<ImagePanel>(CONTROL_NSF_IMAGE, false);

	m_pJinrai_Button = FindControl<Button>(CONTROL_JINRAI_BUTTON);
	m_pNSF_Button = FindControl<Button>(CONTROL_NSF_BUTTON);
	m_pSpectator_Button = FindControl<Button>(CONTROL_SPEC_BUTTON);
	m_pAutoAssign_Button = FindControl<Button>(CONTROL_AUTO_BUTTON);
	m_pCancel_Button = FindControl<Button>(CONTROL_CANCEL_BUTTON);

	// We probably failed to mount original NT VGUI assets if this triggers.
#ifdef DEBUG
	Assert(m_pJinrai_TeamImage);
	Assert(m_pNSF_TeamImage);
	Assert(m_pJinrai_Button);
	Assert(m_pNSF_Button);
	Assert(m_pSpectator_Button);
	Assert(m_pAutoAssign_Button);
	Assert(m_pCancel_Button);
#else
	if (!(m_pJinrai_TeamImage && m_pNSF_TeamImage && m_pJinrai_Button &&
		m_pNSF_Button && m_pSpectator_Button && m_pAutoAssign_Button &&
		m_pCancel_Button))
	{
		// We should never be able to hit this line.
		Error("Failed to load original Neotokyo VGUI elements! This is a bug, please report it.\n");
	}
#endif

#if(0)
	m_pBackgroundImage = new ImagePanel(this, "IconPanel3");
	m_pTeamMenuLabel = new Label(this, "Label1", "labelText");
	m_pJinrai_PlayercountLabel = new Label(this, "jplayercountlabel", "labelText");
	m_pNSF_PlayercountLabel = new Label(this, "nplayercountlabel", "labelText");
	m_pJinrai_ScoreLabel = new Label(this, "jscorelabel", "labelText");
	m_pNSF_ScoreLabel = new Label(this, "nscorelabel", "labelText");

	m_pDivider = new Divider(this, "Divider1");

	m_pJinrai_TeamImage->SetImage("image");
	m_pNSF_TeamImage->SetImage("image");
	m_pBackgroundImage->SetImage("image");
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

	itoa(numPlayersJinrai, textAscii, sizeof(numPlayersJinrai));
	g_pVGuiLocalize->ConvertANSIToUnicode(textAscii, textUnicode_Jinrai, sizeof(textUnicode_Jinrai));
	m_pJinrai_PlayercountLabel->SetText(textUnicode_Jinrai);

	itoa(numPlayersNSF, textAscii, sizeof(numPlayersNSF));
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

	const vgui::HFont font = pScheme->GetFont("Default", IsProportional());
	Assert(font > 0);

	Assert(m_pJinrai_Button);
	Assert(m_pNSF_Button);
	Assert(m_pSpectator_Button);
	Assert(m_pAutoAssign_Button);
	Assert(m_pCancel_Button);

	m_pJinrai_Button->SetFont(font);
	m_pNSF_Button->SetFont(font);
	m_pSpectator_Button->SetFont(font);
	m_pAutoAssign_Button->SetFont(font);
	// NEO FIXME (Rain): this line rarely throws; I have no idea why.
	// The assertions above are not hit when it occurs.
	m_pCancel_Button->SetFont(font);

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
