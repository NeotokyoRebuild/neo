#include "cbase.h"
#include "neo_classmenu.h"

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

#include "neo_teammenu.h"

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

#ifndef _XBOX
extern IGameUIFuncs *gameuifuncs; // for key binding details
#endif

CNeoClassMenu *g_pNeoClassMenu = NULL;

using namespace vgui;

const char* playerModels[]{
	"cm/jinrai_scout01",
	"cm/jinrai_scout02",
	"cm/jinrai_scout03",
	"cm/jinrai_assault01",
	"cm/jinrai_assault02",
	"cm/jinrai_assault03",
	"cm/jinrai_heavy01",
	"cm/jinrai_heavy02",
	"cm/jinrai_heavy03",
	"cm/nsf_scout02",
	"cm/nsf_scout03",
	"cm/nsf_scout01",
	"cm/nsf_assault01",
	"cm/nsf_assault02",
	"cm/nsf_assault03",
	"cm/nsf_heavy01",
	"cm/nsf_heavy02",
	"cm/nsf_heavy03",
};

CNeoClassMenu::CNeoClassMenu(IViewPort *pViewPort)
	: vgui::Frame(NULL, PANEL_CLASS)
{
	Assert(pViewPort);
	// Quiet "parent not sized yet" spew
	SetSize(10, 10);

	g_pNeoClassMenu = this;

	m_bClassMenu = false;

	LoadControlSettings(GetResFile());

	SetPaintBorderEnabled(false);
	SetPaintBackgroundEnabled(false);
	SetBorder(NULL);

	SetVisible(false);
	SetProportional(false);
	SetMouseInputEnabled(true);
	//SetKeyBoardInputEnabled(true); // Leaving here to highlight menu navigation with keyboard is possible atm
	SetTitleBarVisible(false);

	FindButtons();
}

CNeoClassMenu::~CNeoClassMenu()
{
	m_pSkinPanel1->SetAutoDelete(true);
	m_pSkinPanel2->SetAutoDelete(true);
	m_pSkinPanel3->SetAutoDelete(true);
	m_pSkin1_Button->SetAutoDelete(true);
	m_pSkin2_Button->SetAutoDelete(true);
	m_pSkin3_Button->SetAutoDelete(true);
	m_pRecon_Button->SetAutoDelete(true);
	m_pAssault_Button->SetAutoDelete(true);
	m_pSupport_Button->SetAutoDelete(true);
	m_pBack_Button->SetAutoDelete(true);
}

void CNeoClassMenu::FindButtons()
{
	m_pSkinPanel1 = FindControl<ImagePanel>("Model1_ImagePanel");
	m_pSkinPanel2 = FindControl<ImagePanel>("Model2_ImagePanel");
	m_pSkinPanel3 = FindControl<ImagePanel>("Model3_ImagePanel");
	m_pSkin1_Button = FindControl<Button>("Skin1_Button");
	m_pSkin2_Button = FindControl<Button>("Skin2_Button");
	m_pSkin3_Button = FindControl<Button>("Skin3_Button");
	m_pRecon_Button = FindControl<Button>("Scout_Button");
	m_pAssault_Button = FindControl<Button>("Assault_Button");
	m_pSupport_Button = FindControl<Button>("Heavy_Button");
	m_pBack_Button = FindControl<Button>("Back_Button");

	UpdateSkinImages(-1);
}

void CNeoClassMenu::CommandCompletion()
{	
	SetControlEnabled("Scout_Button", false);
	SetControlEnabled("Assault_Button", false);
	SetControlEnabled("Heavy_Button", false);
	SetControlEnabled("Skin1_Button", false);
	SetControlEnabled("Skin2_Button", false);
	SetControlEnabled("Skin3_Button", false);
	SetControlEnabled("Back_Button", false);

	SetVisible(false);
	SetEnabled(false);

	SetMouseInputEnabled(false);
	//SetKeyBoardInputEnabled(false);
	SetCursorAlwaysVisible(false);
}

void CNeoClassMenu::OnCommand(const char *command)
{
	BaseClass::OnCommand(command);

	if (*command == NULL)
	{ // No command
		return;
	}

	char commandBuffer[20]; // Needs to be large enough to hold longest command sent from this menu
	V_strcpy_safe(commandBuffer, command);

	if (Q_stristr(commandBuffer, "setclass") != 0)
	{ // Picking class, stay on this screen
		int classNumber = commandBuffer[9] - '1'; // Needed for skin images, 0 indexed, recon class is 1
		UpdateSkinImages(classNumber);
		engine->ClientCmd(commandBuffer);
		return;
	}

	if (Q_stristr(commandBuffer, "SetVariant") != 0)
	{ // Picking variant, move onto loadout selection screen
		engine->ClientCmd(command);
		ChangeMenu("loadoutmenu");
		engine->ClientCmd("loadoutmenu");
		return;
	}

	if (Q_stristr(commandBuffer, "playerstate_reverse") != 0)
	{ // Going back
		engine->ClientCmd(command);
		ChangeMenu("teammenu");
		engine->ClientCmd("teammenu");
		return;
	}
}

void CNeoClassMenu::ChangeMenu(const char* menuName = NULL)
{
	CommandCompletion();
	ShowPanel(false);
	C_NEO_Player* player = C_NEO_Player::GetLocalNEOPlayer();
	if (player)
	{
		player->m_bShowClassMenu = false;
		if (menuName == NULL)
		{
			return;
		}
		if (Q_stricmp(menuName, "teammenu") == 0)
		{
			player->m_bShowTeamMenu = true;
		}
	}
	else
	{
		Assert(false);
	}
}

void CNeoClassMenu::OnKeyCodeReleased(vgui::KeyCode code)
{
	switch (code) {
	case 93: // F2 - Close the menu
		ChangeMenu(NULL);
		break;
	case 2: // 1 - Pick Recon and go straight to loadout
		UpdateSkinImages(0);
		engine->ClientCmd("setclass 1");
		/*engine->ClientCmd("loadoutmenu");		NEO FIXME If player picking loadout with keyboard probably doesn't care about skin should go straight to loadout menu but loadout menu opens before player class and even player wanted class updates resulting in missmatch of showed loadout. Should edit loadoutmenu command to take class parameter? Can't think of case where player picked class isn't accepted unless using max players per class plugin
		ChangeMenu("loadoutmenu");*/
		break;
	case 3: // 2 - Pick Assault and go straight to loadout
		UpdateSkinImages(1);
		engine->ClientCmd("setclass 2");
		/*engine->ClientCmd("loadoutmenu");		NEO FIXME ==
		ChangeMenu("loadoutmenu");*/
		break;
	case 4: // 3 - Pick Support and go straight to loadout
		UpdateSkinImages(2);
		engine->ClientCmd("setclass 3");
		/*engine->ClientCmd("loadoutmenu");		NEO FIXME ==
		ChangeMenu("loadoutmenu");*/
		break;
	case 65: // Spacebar - Continue with currently selected class and skin (player is initialised with a default class and skin. If not need to set one here)
		engine->ClientCmd("loadoutmenu");
		ChangeMenu("loadoutmenu");
		break;
	}
	// Ignore other Key presses
	//BaseClass::OnKeyCodeReleased(code);
}

void CNeoClassMenu::UpdateSkinImages(int classNumber = -1)
{
	C_NEO_Player* player = C_NEO_Player::GetLocalNEOPlayer();
	if (!player)
	{
		return;
	}
	int teamNumber = player->GetTeamNumber() - 2;
	if (teamNumber < 0 || teamNumber>1)
	{ // player hasn't joined jinrai or nsf yet, do not update images
		return;
	}
	if (classNumber == -1)
	{ // player hasn't picked a class yet, work out what class he is by default
		classNumber = player->m_iNeoClass.Get();
	}

	m_pSkinPanel1->SetImage(playerModels[teamNumber * 9 + (classNumber * 3) + 0]);
	m_pSkinPanel2->SetImage(playerModels[teamNumber * 9 + (classNumber * 3) + 1]);
	m_pSkinPanel3->SetImage(playerModels[teamNumber * 9 + (classNumber * 3) + 2]);
}

void CNeoClassMenu::OnMessage(const KeyValues *params, VPANEL fromPanel)
{
	BaseClass::OnMessage(params, fromPanel);
}

void CNeoClassMenu::OnMousePressed(vgui::MouseCode code)
{
    BaseClass::OnMousePressed(code);
}

void CNeoClassMenu::ApplySchemeSettings(vgui::IScheme *pScheme)
{
    BaseClass::ApplySchemeSettings(pScheme);
    LoadControlSettings(GetResFile());
	SetPaintBorderEnabled(false);
	SetPaintBackgroundEnabled(false);
	SetBorder(NULL);

	FindButtons();

    m_pRecon_Button->SetUseCaptureMouse(true);
    m_pRecon_Button->SetMouseInputEnabled(true);
    m_pRecon_Button->InstallMouseHandler(this);

    m_pAssault_Button->SetUseCaptureMouse(true);
    m_pAssault_Button->SetMouseInputEnabled(true);
    m_pAssault_Button->InstallMouseHandler(this);

    m_pSupport_Button->SetUseCaptureMouse(true);
    m_pSupport_Button->SetMouseInputEnabled(true);
    m_pSupport_Button->InstallMouseHandler(this);

	m_pSkin1_Button->SetUseCaptureMouse(true);
	m_pSkin1_Button->SetMouseInputEnabled(true);
	m_pSkin1_Button->InstallMouseHandler(this);

	m_pSkin2_Button->SetUseCaptureMouse(true);
	m_pSkin2_Button->SetMouseInputEnabled(true);
	m_pSkin2_Button->InstallMouseHandler(this);

	m_pSkin3_Button->SetUseCaptureMouse(true);
	m_pSkin3_Button->SetMouseInputEnabled(true);
	m_pSkin3_Button->InstallMouseHandler(this);

    m_pBack_Button->SetUseCaptureMouse(true);
    m_pBack_Button->SetMouseInputEnabled(true);
    m_pBack_Button->InstallMouseHandler(this);

	UpdateSkinImages();
	InvalidateLayout();
}

void CNeoClassMenu::MoveLabelToFront(const char *textEntryName)
{
    Label *entry = dynamic_cast<Label *>(FindChildByName(textEntryName));
    if (entry)
    {
        entry->MoveToFront();
    }
}

void CNeoClassMenu::SetLabelText(const char *textEntryName, const char *text)
{
    Label *entry = dynamic_cast<Label *>(FindChildByName(textEntryName));
    if (entry)
    {
        entry->SetText(text);
    }
}

void CNeoClassMenu::SetLabelText(const char *textEntryName, wchar_t *text)
{
    Label *entry = dynamic_cast<Label *>(FindChildByName(textEntryName));
    if (entry)
    {
        entry->SetText(text);
    }
}

void CNeoClassMenu::ShowPanel( bool bShow )
{
    if (bShow && !IsVisible())
    {
        m_bClassMenu = false;
    }

    SetVisible(bShow);

    if (!bShow && m_bClassMenu)
    {
        gViewPortInterface->ShowPanel(PANEL_TEAM, false);
    }
}

void CNeoClassMenu::OnThink()
{
	BaseClass::OnThink();
}