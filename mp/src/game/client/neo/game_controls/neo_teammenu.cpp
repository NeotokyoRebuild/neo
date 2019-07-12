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

#include <vgui_controls/TextEntry.h>
#include <vgui_controls/Panel.h>
#include <vgui_controls/ImagePanel.h>
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

#ifndef _XBOX
extern IGameUIFuncs *gameuifuncs; // for key binding details
#endif

CNeoTeamMenu *g_pNeoTeamMenu = NULL;

using namespace vgui;

static inline int GetTeamIntFromString(const char *pszTeamName)
{
	// Auto assign by default
	if (Q_stristr("default", pszTeamName) != 0) { return TEAM_ANY; }

	// Did client have a team preference?
	if (Q_stristr(TEAM_STR_JINRAI, pszTeamName) != 0) { return TEAM_JINRAI; }
	if (Q_stristr(TEAM_STR_NSF, pszTeamName) != 0) { return TEAM_NSF; }

	// Client didn't want auto assign nor a specific team; assign them to spec
	return TEAM_SPECTATOR;
}

CNeoTeamMenu::CNeoTeamMenu(IViewPort *pViewPort)
	: vgui::Frame(NULL, PANEL_TEAM)
{
	// Quiet "parent not sized yet" spew
	SetSize(10, 10);

	m_pViewPort = pViewPort;
	g_pNeoTeamMenu = this;

	m_bTeamMenu = false;

	// NEO TODO (Rain): It appears that original Neotokyo
	// hardcodes its scheme. We probably need to make our
	// own res definition file to mimic it.
	SetScheme("ClientScheme");

	SetVisible(false);
	SetProportional(true);
	SetMouseInputEnabled(false);
	SetKeyBoardInputEnabled(false);

	SetTitleBarVisible(false);
	SetProportional(true);

	m_pJinrai_TeamImage = new ImagePanel(this, "IconPanel1");
	m_pNSF_TeamImage = new ImagePanel(this, "IconPanel2");
	m_pBackgroundImage = new ImagePanel(this, "IconPanel3");
	m_pJinrai_TeamImage->SetImage("image");
	m_pNSF_TeamImage->SetImage("image");
	m_pBackgroundImage->SetImage("image");

	m_pTeamMenuLabel = new Label(this, "Label1", "labelText");
	m_pJinrai_PlayercountLabel = new Label(this, "jplayercountlabel", "labelText");
	m_pNSF_PlayercountLabel = new Label(this, "nplayercountlabel", "labelText");
	m_pJinrai_ScoreLabel = new Label(this, "jscorelabel", "labelText");
	m_pNSF_ScoreLabel = new Label(this, "nscorelabel", "labelText");

	m_pDivider = new Divider(this, "Divider1");

	m_pJinrai_Button = new Button(this, "jinraibutton", "labelText");
	m_pNSF_Button = new Button(this, "nsfbutton", "labelText");
	m_pSpectator_Button = new Button(this, "specbutton", "labelText");
	m_pAutoAssign_Button = new Button(this, "autobutton", "labelText");
	m_pCancel_Button = new Button(this, "CancelButton", "labelText");

	m_pJinrai_Button->AddActionSignalTarget(this);
	m_pNSF_Button->AddActionSignalTarget(this);
	m_pSpectator_Button->AddActionSignalTarget(this);
	m_pAutoAssign_Button->AddActionSignalTarget(this);
	m_pCancel_Button->AddActionSignalTarget(this);

	InvalidateLayout();
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
	SetMouseInputEnabled(false);
	SetKeyBoardInputEnabled(false);
	SetCursorAlwaysVisible(false);
	
	SetControlEnabled("jinraibutton", false);
	SetControlEnabled("nsfbutton", false);
	SetControlEnabled("specbutton", false);
	SetControlEnabled("autobutton", false);
	SetControlEnabled("CancelButton", false);

	SetVisible(false);
	SetEnabled(false);
}

void CNeoTeamMenu::OnCommand(const char *command)
{
	BaseClass::OnCommand(command);

	bool proceedToClassSelection = false;

	if (Q_stricmp(command, "PressButton"))
	{
		Button *pressedButton = GetPressedButton();
		if (pressedButton)
		{
			ShowPanel(false);
			SetMouseInputEnabled(false);
			SetKeyBoardInputEnabled(false);

			char buttonCmd[128];

			proceedToClassSelection = true;

			if (pressedButton == m_pJinrai_Button)
			{
				sprintf(buttonCmd, "jointeam %i", TEAM_JINRAI);
			}
			else if (pressedButton == m_pNSF_Button)
			{
				sprintf(buttonCmd, "jointeam %i", TEAM_NSF);
			}
			else if (pressedButton == m_pSpectator_Button)
			{
				sprintf(buttonCmd, "jointeam %i", TEAM_SPECTATOR);
			}
			else if (pressedButton == m_pAutoAssign_Button)
			{
				int team = RandomInt(TEAM_JINRAI, TEAM_NSF);
				sprintf(buttonCmd, "jointeam %i", team);
			}
			else
			{
				proceedToClassSelection = false;
			}

			engine->ExecuteClientCmd(buttonCmd);
		}
	}

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
	//Msg("Button pressed\n");
	KeyValuesDumpAsDevMsg(data);
}

CNeoTeamMenu::~CNeoTeamMenu()
{
	//Msg("~CNeoTeamMenu\n");
}

void CNeoTeamMenu::OnMessage(const KeyValues *params, VPANEL fromPanel)
{
#if(0)
	if (Q_stricmp(params->GetName(), "OnMousePressed") == 0)
	{
		Msg("OnMousePressed\n");

		if (fromPanel == m_pJinrai_Button->GetVPanel())
		{
			Msg("We clicked Jinrai?\n");
		}
	}
#endif

	BaseClass::OnMessage(params, fromPanel);
}

void CNeoTeamMenu::OnMousePressed(vgui::MouseCode code)
{
#if(0)
    Msg("OnMousePressed\n");
#endif

    BaseClass::OnMousePressed(code);
}

void CNeoTeamMenu::ApplySchemeSettings(vgui::IScheme *pScheme)
{
    BaseClass::ApplySchemeSettings(pScheme);

    LoadControlSettings(GetResFile());

    SetBgColor(Color( 0,0,0,0 ) ); // make the background transparent

    const char *font = "Default";

    m_pJinrai_Button->SetFont(pScheme->GetFont(font, IsProportional()));
    m_pNSF_Button->SetFont(pScheme->GetFont(font, IsProportional()));
    m_pSpectator_Button->SetFont(pScheme->GetFont(font, IsProportional()));
    m_pAutoAssign_Button->SetFont(pScheme->GetFont(font, IsProportional()));
    m_pCancel_Button->SetFont(pScheme->GetFont(font, IsProportional()));

    m_pJinrai_Button->SetUseCaptureMouse(true);
    m_pJinrai_Button->SetSelectedColor(Color(255, 0, 0), Color(0, 0, 0));
    m_pJinrai_Button->SetArmedColor(Color(0, 255, 0), Color(0, 0, 0));
    m_pJinrai_Button->SetMouseInputEnabled(true);
    m_pJinrai_Button->InstallMouseHandler(this);

    m_pNSF_Button->SetUseCaptureMouse(true);
    m_pNSF_Button->SetSelectedColor(Color(255, 0, 0), Color(0, 0, 0));
    m_pNSF_Button->SetArmedColor(Color(0, 255, 0), Color(0, 0, 0));
    m_pNSF_Button->SetMouseInputEnabled(true);
    m_pNSF_Button->InstallMouseHandler(this);

    m_pSpectator_Button->SetUseCaptureMouse(true);
    m_pSpectator_Button->SetSelectedColor(Color(255, 0, 0), Color(0, 0, 0));
    m_pSpectator_Button->SetArmedColor(Color(0, 255, 0), Color(0, 0, 0));
    m_pSpectator_Button->SetMouseInputEnabled(true);
    m_pSpectator_Button->InstallMouseHandler(this);

    m_pAutoAssign_Button->SetUseCaptureMouse(true);
    m_pAutoAssign_Button->SetSelectedColor(Color(255, 0, 0), Color(0, 0, 0));
    m_pAutoAssign_Button->SetArmedColor(Color(0, 255, 0), Color(0, 0, 0));
    m_pAutoAssign_Button->SetMouseInputEnabled(true);
    m_pAutoAssign_Button->InstallMouseHandler(this);

    m_pCancel_Button->SetUseCaptureMouse(true);
    m_pCancel_Button->SetSelectedColor(Color(255, 0, 0), Color(0, 0, 0));
    m_pCancel_Button->SetArmedColor(Color(0, 255, 0), Color(0, 0, 0));
    m_pCancel_Button->SetMouseInputEnabled(true);
    m_pCancel_Button->InstallMouseHandler(this);

    //Msg("Jinraibutton is enabled: %i\n", (int)m_pJinrai_Button->IsEnabled());

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

void CNeoTeamMenu::OnThink()
{
	BaseClass::OnThink();

    /*
    if (m_pJinrai_Button->IsSelected())
    {
        Msg("m_pJinrai_Button selected\n");
    }
    else if (m_pNSF_Button->IsSelected())
    {
        Msg("m_pNSF_Button selected\n");
    }
    else if (m_pAutoAssign_Button->IsSelected())
    {
        Msg("m_pAutoAssign_Button selected\n");
    }
    else if (m_pSpectator_Button->IsSelected())
    {
        Msg("m_pSpectator_Button selected\n");
    }
    else if (m_pCancel_Button->IsSelected())
    {
        Msg("m_pCancel_Button selected\n");
    }*/
}