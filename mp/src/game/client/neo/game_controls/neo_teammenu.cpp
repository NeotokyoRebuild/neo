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

#define CONTROL_JINRAI_PLAYERCOUNT_LABEL "neo_jplayercountlabel"
#define CONTROL_NSF_PLAYERCOUNT_LABEL "neo_nplayercountlabel"
#define CONTROL_JINRAI_SCORE_LABEL "neo_jscorelabel"
#define CONTROL_NSF_SCORE_LABEL "neo_nscorelabel"

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

	g_pNeoTeamMenu = this;

	m_pViewPort = pViewPort;
	m_bTeamMenu = false;

	LoadControlSettings(GetResFile());

	SetPaintBorderEnabled(false);
	SetPaintBackgroundEnabled(false);
	SetBorder(NULL);

	SetVisible(false);
	SetProportional(false);
	SetMouseInputEnabled(true);
	SetKeyBoardInputEnabled(true);
	SetTitleBarVisible(false);

	FindButtons();
}

CNeoTeamMenu::~CNeoTeamMenu()
{
	m_pJinrai_Button->RemoveActionSignalTarget(this);
	m_pNSF_Button->RemoveActionSignalTarget(this);
	m_pSpectator_Button->RemoveActionSignalTarget(this);
	m_pAutoAssign_Button->RemoveActionSignalTarget(this);
	m_pCancel_Button->RemoveActionSignalTarget(this); 

	m_pJinrai_Button->SetAutoDelete(true);
	m_pNSF_Button->SetAutoDelete(true);
	m_pSpectator_Button->SetAutoDelete(true);
	m_pAutoAssign_Button->SetAutoDelete(true);
	m_pCancel_Button->SetAutoDelete(true);

	g_pNeoTeamMenu = NULL;
}

void CNeoTeamMenu::FindButtons()
{
	m_pJinrai_PlayercountLabel = FindControl<Label>(CONTROL_JINRAI_PLAYERCOUNT_LABEL);
	m_pNSF_PlayercountLabel = FindControl<Label>(CONTROL_NSF_PLAYERCOUNT_LABEL);
	m_pJinrai_ScoreLabel = FindControl<Label>(CONTROL_JINRAI_SCORE_LABEL);
	m_pNSF_ScoreLabel = FindControl<Label>(CONTROL_NSF_SCORE_LABEL);
	m_pJinrai_Button = FindControl<Button>(CONTROL_JINRAI_BUTTON);
	m_pNSF_Button = FindControl<Button>(CONTROL_NSF_BUTTON);
	m_pSpectator_Button = FindControl<Button>(CONTROL_SPEC_BUTTON);
	m_pAutoAssign_Button = FindControl<Button>(CONTROL_AUTO_BUTTON);
	m_pCancel_Button = FindControl<Button>(CONTROL_CANCEL_BUTTON);
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

	if (Q_strcmp(commandBuffer, "jointeam a") == 0)
	{ // pick a team automatically 
		int randomTeam = RandomInt(TEAM_JINRAI, TEAM_NSF);
		V_sprintf_safe(commandBuffer, "jointeam %i", randomTeam);
		ChangeMenu("classmenu");
		engine->ClientCmd(commandBuffer);
		return;
	}

	if (Q_strcmp(commandBuffer, "jointeam 1") == 0)
	{ // joining spectators
		ChangeMenu(NULL);
		engine->ClientCmd(commandBuffer);
		return;
	}

	if (Q_stristr(commandBuffer, "jointeam") != 0) // Note using stristr, not strcmp. Equates to true when jointeam in commandBuffer
	{ // joining jinrai or nsf
		ChangeMenu("classmenu");
		engine->ClientCmd(commandBuffer);
		return;
	}

	if (Q_stricmp(commandBuffer, "vguicancel") == 0)
	{ // cancel, close menu
		ChangeMenu(NULL);
	}
	
	// new command, should we sanitize and return without executing? (Players can edit button commands with ctrl-alt-shift-b) NEO FIXME
	engine->ClientCmd(command);
}

void CNeoTeamMenu::ChangeMenu(const char* menuName = NULL)
{
	CommandCompletion();
	ShowPanel(false);
	C_NEO_Player* player = C_NEO_Player::GetLocalNEOPlayer();
	if (player)
	{
		player->m_bShowTeamMenu = false;
		if (menuName == NULL)
		{
			return;
		}
		if (Q_stricmp(menuName, "classmenu") == 0)
		{
			player->m_bShowClassMenu = true;
		}
	}
	else
	{
		Assert(false);
	}
}

void CNeoTeamMenu::ApplySchemeSettings(vgui::IScheme *pScheme)
{
    BaseClass::ApplySchemeSettings(pScheme);
	LoadControlSettings(GetResFile());
	SetPaintBorderEnabled(false);
	SetPaintBackgroundEnabled(false);
	SetBorder(NULL);

	FindButtons();

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
	InvalidateLayout();
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
