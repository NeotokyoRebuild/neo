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

CNeoClassMenu::CNeoClassMenu(IViewPort *pViewPort)
	: vgui::Frame(NULL, PANEL_CLASS)
{
	// Quiet "parent not sized yet" spew
	SetSize(10, 10);

	m_pViewPort = pViewPort;
	g_pNeoClassMenu = this;

	m_bClassMenu = false;

	// NEO TODO (Rain): It appears that original Neotokyo
	// hardcodes its scheme. We probably need to make our
	// own res definition file to mimic it.
	SetScheme("ClientScheme");

	SetVisible(false);
	SetProportional(true);
	SetMouseInputEnabled(true);
	SetKeyBoardInputEnabled(true);

	SetTitleBarVisible(false);

	m_pImage_Jinrai_Recon_Skin1 = surface()->GetIconImageForFullPath("vgui/cm/jinrai_scout01");
	m_pImage_Jinrai_Recon_Skin2 = surface()->GetIconImageForFullPath("vgui/cm/jinrai_scout02");
	m_pImage_Jinrai_Recon_Skin3 = surface()->GetIconImageForFullPath("vgui/cm/jinrai_scout03");

	m_pSkinPanel1 = new ImagePanel(this, "Model_ImagePanel");
	m_pSkinPanel2 = new ImagePanel(this, "Model_ImagePanel2");
	m_pSkinPanel3 = new ImagePanel(this, "Model_ImagePanel3");
	m_pSkinPanel1->SetImage(m_pImage_Jinrai_Recon_Skin1);
	m_pSkinPanel2->SetImage(m_pImage_Jinrai_Recon_Skin2);
	m_pSkinPanel3->SetImage(m_pImage_Jinrai_Recon_Skin3);

	m_pClassMenuLabel = new Label(this, "TitleLabel", "labelText");

	m_pDivider = new Divider(this, "Divider1");

	m_pRecon_Button = new Button(this, "Scout_Button", "labelText");
	m_pAssault_Button = new Button(this, "Assault_Button", "labelText");
	m_pSupport_Button = new Button(this, "Heavy_Button", "labelText");
	m_pBack_Button = new Button(this, "Back_Button", "labelText");

	m_pRecon_Button->AddActionSignalTarget(this);
	m_pAssault_Button->AddActionSignalTarget(this);
	m_pSupport_Button->AddActionSignalTarget(this);
	m_pBack_Button->AddActionSignalTarget(this);

	InvalidateLayout();
}

inline Button *CNeoClassMenu::GetPressedButton()
{
	if (m_pRecon_Button->IsCursorOver()) { return m_pRecon_Button; }
	if (m_pAssault_Button->IsCursorOver()) { return m_pAssault_Button; }
	if (m_pSupport_Button->IsCursorOver()) { return m_pSupport_Button; }
	if (m_pBack_Button->IsCursorOver()) { return m_pBack_Button; }
	return NULL;
}

void CNeoClassMenu::CommandCompletion()
{	
	SetControlEnabled("Scout_Button", false);
	SetControlEnabled("Assault_Button", false);
	SetControlEnabled("Heavy_Button", false);
	SetControlEnabled("Back_Button", false);

	SetVisible(false);
	SetEnabled(false);

	SetMouseInputEnabled(false);
	SetKeyBoardInputEnabled(false);
	SetCursorAlwaysVisible(false);
}

void CNeoClassMenu::OnCommand(const char *command)
{
	BaseClass::OnCommand(command);

	if (Q_stricmp(command, "PressButton"))
	{
		bool hasCompletedClassSelection = false;

		Button *pressedButton = GetPressedButton();
		if (pressedButton)
		{
			hasCompletedClassSelection = true;

			ShowPanel(false);

			char buttonCmd[128];
			if (pressedButton == m_pRecon_Button)
			{
				sprintf(buttonCmd, "setclass %i", 1);
			}
			else if (pressedButton == m_pAssault_Button)
			{
				sprintf(buttonCmd, "setclass %i", 2);
			}
			else if (pressedButton == m_pSupport_Button)
			{
				sprintf(buttonCmd, "setclass %i", 3);
			}
			else
			{
				if (pressedButton != m_pBack_Button)
				{
					Assert(false);
				}

				sprintf(buttonCmd, "teammenu");

				hasCompletedClassSelection = false;
			}

			engine->ExecuteClientCmd(buttonCmd);
		}

		if (hasCompletedClassSelection)
		{
			C_NEO_Player *player = C_NEO_Player::GetLocalNEOPlayer();
			if (player)
			{
				player->m_bShowClassMenu = false;
			}
			else
			{
				Assert(false);
			}

			engine->ClientCmd("loadoutmenu");
		}
	}

	CommandCompletion();
}

void CNeoClassMenu::OnButtonPressed(KeyValues *data)
{
	Msg("Class button pressed\n");
	KeyValuesDumpAsDevMsg(data);
}

CNeoClassMenu::~CNeoClassMenu()
{
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

    SetBgColor(Color( 0,0,0,0 ) ); // make the background transparent

    const char *font = "Default";

    m_pRecon_Button->SetFont(pScheme->GetFont(font, IsProportional()));
    m_pAssault_Button->SetFont(pScheme->GetFont(font, IsProportional()));
    m_pSupport_Button->SetFont(pScheme->GetFont(font, IsProportional()));
    m_pBack_Button->SetFont(pScheme->GetFont(font, IsProportional()));

    m_pRecon_Button->SetUseCaptureMouse(true);
    m_pRecon_Button->SetSelectedColor(Color(255, 0, 0), Color(0, 0, 0));
    m_pRecon_Button->SetArmedColor(Color(0, 255, 0), Color(0, 0, 0));
    m_pRecon_Button->SetMouseInputEnabled(true);
    m_pRecon_Button->InstallMouseHandler(this);

    m_pAssault_Button->SetUseCaptureMouse(true);
    m_pAssault_Button->SetSelectedColor(Color(255, 0, 0), Color(0, 0, 0));
    m_pAssault_Button->SetArmedColor(Color(0, 255, 0), Color(0, 0, 0));
    m_pAssault_Button->SetMouseInputEnabled(true);
    m_pAssault_Button->InstallMouseHandler(this);

    m_pSupport_Button->SetUseCaptureMouse(true);
    m_pSupport_Button->SetSelectedColor(Color(255, 0, 0), Color(0, 0, 0));
    m_pSupport_Button->SetArmedColor(Color(0, 255, 0), Color(0, 0, 0));
    m_pSupport_Button->SetMouseInputEnabled(true);
    m_pSupport_Button->InstallMouseHandler(this);

    m_pBack_Button->SetUseCaptureMouse(true);
    m_pBack_Button->SetSelectedColor(Color(255, 0, 0), Color(0, 0, 0));
    m_pBack_Button->SetArmedColor(Color(0, 255, 0), Color(0, 0, 0));
    m_pBack_Button->SetMouseInputEnabled(true);
    m_pBack_Button->InstallMouseHandler(this);

    SetPaintBorderEnabled(false);

    SetBorder( NULL );
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