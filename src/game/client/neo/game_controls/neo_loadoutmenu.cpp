#include "cbase.h"
#include "neo_loadoutmenu.h"

#include "vgui_controls/Label.h"
#include "vgui_controls/ImagePanel.h"
#include "vgui_controls/Button.h"
#include "neo/game_controls/neo_button.h"

#include <vgui/ILocalize.h>
#include <vgui/ISurface.h>

#include "ienginevgui.h"

#include "c_neo_player.h"
#include "weapon_neobasecombatweapon.h"
#include "neo_weapon_loadout.h"
#include "neo_gamerules.h"
#include "ui/neo_root.h"
#include "IGameUIFuncs.h" // for key bindings

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

using namespace vgui;

// These are defined in the .res file
#define CONTROL_BUTTON1 "Button1"
#define CONTROL_BUTTON2 "Button2"
#define CONTROL_BUTTON3 "Button3"
#define CONTROL_BUTTON4 "Button4"
#define CONTROL_BUTTON5 "Button5"
#define CONTROL_BUTTON6 "Button6"
#define CONTROL_BUTTON7 "Button7"
#define CONTROL_BUTTON8 "Button8"
#define CONTROL_BUTTON9 "Button9"
#define CONTROL_BUTTON10 "Button10"
#define CONTROL_BUTTON11 "Button11"
#define CONTROL_BUTTON12 "Button12"

#define IMAGE_BUTTON1 "Button1Image"
#define IMAGE_BUTTON2 "Button2Image"
#define IMAGE_BUTTON3 "Button3Image"
#define IMAGE_BUTTON4 "Button4Image"
#define IMAGE_BUTTON5 "Button5Image"
#define IMAGE_BUTTON6 "Button6Image"
#define IMAGE_BUTTON7 "Button7Image"
#define IMAGE_BUTTON8 "Button8Image"
#define IMAGE_BUTTON9 "Button9Image"
#define IMAGE_BUTTON10 "Button10Image"
#define IMAGE_BUTTON11 "Button11Image"
#define IMAGE_BUTTON12 "Button12Image"

#define concat(first, second) first second

static const char *szButtons[] = {
    CONTROL_BUTTON1,
    CONTROL_BUTTON2,
    CONTROL_BUTTON3,
    CONTROL_BUTTON4,
    CONTROL_BUTTON5,
    CONTROL_BUTTON6,
    CONTROL_BUTTON7,
    CONTROL_BUTTON8,
    CONTROL_BUTTON9,
    CONTROL_BUTTON10,
    CONTROL_BUTTON11,
    CONTROL_BUTTON12,
};
static const char* szButtonImages[] = {
	IMAGE_BUTTON1,
	IMAGE_BUTTON2,
	IMAGE_BUTTON3,
	IMAGE_BUTTON4,
	IMAGE_BUTTON5,
	IMAGE_BUTTON6,
	IMAGE_BUTTON7,
	IMAGE_BUTTON8,
	IMAGE_BUTTON9,
	IMAGE_BUTTON10,
	IMAGE_BUTTON11,
	IMAGE_BUTTON12,
};

Panel *NeoLoadout_Factory()
{
	return new CNeoLoadoutMenu(gViewPortInterface);
}

DECLARE_BUILD_FACTORY_CUSTOM(CNeoLoadoutMenu, NeoLoadout_Factory);

CNeoLoadoutMenu::CNeoLoadoutMenu(IViewPort *pViewPort)
	: BaseClass(NULL, PANEL_NEO_LOADOUT)
{
	Assert(pViewPort);
	// Quiet "parent not sized yet" spew
	SetSize(10, 10);

	m_pViewPort = pViewPort;

	m_bLoadoutMenu = false;

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

CNeoLoadoutMenu::~CNeoLoadoutMenu()
{
}

void CNeoLoadoutMenu::FindButtons()
{
	m_pButton1 = FindControl<Button>(CONTROL_BUTTON1);
	m_pButton2 = FindControl<Button>(CONTROL_BUTTON2);
	m_pButton3 = FindControl<Button>(CONTROL_BUTTON3);
	m_pButton4 = FindControl<Button>(CONTROL_BUTTON4);
	m_pButton5 = FindControl<Button>(CONTROL_BUTTON5);
	m_pButton6 = FindControl<Button>(CONTROL_BUTTON6);
	m_pButton7 = FindControl<Button>(CONTROL_BUTTON7);
	m_pButton8 = FindControl<Button>(CONTROL_BUTTON8);
	m_pButton9 = FindControl<Button>(CONTROL_BUTTON9);
	m_pButton10 = FindControl<Button>(CONTROL_BUTTON10);
	m_pButton11 = FindControl<Button>(CONTROL_BUTTON11);
	m_pButton12 = FindControl<Button>(CONTROL_BUTTON12);

	for (int i = 0; i < MAX_WEAPON_LOADOUTS; i++)
	{
		auto button = FindControl<Button>(szButtons[i]); // Duplicate FindControl NEO FIXME

		if (!button)
		{
			Assert(false);
			Warning("Button was null on CNeoLoadoutMenu\n");
			continue;
		}

		button->SetUseCaptureMouse(true);
		button->SetMouseInputEnabled(true);
	}

	returnButton = FindControl<CNeoButton>("ReturnButton");
	returnButton->SetUseCaptureMouse(true);
	returnButton->SetMouseInputEnabled(true);
	if (!returnButton)
	{
		Assert(false);
	}
}

void CNeoLoadoutMenu::CommandCompletion()
{
    for (int i = 0; i < MAX_WEAPON_LOADOUTS; i++)
	{
        auto button = FindControl<Button>(szButtons[i]);

		if (!button)
		{
			Assert(false);
			Warning("Button was null on CNeoLoadoutMenu\n");
			continue;
		}

		button->SetEnabled(false);
	}
	
	returnButton->SetEnabled(false);

	SetVisible(false);
	SetEnabled(false);

	SetMouseInputEnabled(false);
	SetCursorAlwaysVisible(false);
}

void CNeoLoadoutMenu::ShowPanel(bool bShow)
{
	//gViewPortInterface->ShowPanel(PANEL_NEO_LOADOUT, bShow);

	if (bShow && !IsVisible())
	{
		m_bLoadoutMenu = false;
	}

	SetVisible(bShow);

	if (!bShow && m_bLoadoutMenu)
	{
		gViewPortInterface->ShowPanel(PANEL_NEO_LOADOUT, false);
	}
}

void CNeoLoadoutMenu::OnMessage(const KeyValues* params, vgui::VPANEL fromPanel)
{
	BaseClass::OnMessage(params, fromPanel);
}

void CNeoLoadoutMenu::OnThink()
{
	BaseClass::OnThink();
}

void CNeoLoadoutMenu::OnMousePressed(vgui::MouseCode code)
{
	BaseClass::OnMousePressed(code);
}

extern ConCommand loadoutmenu;

extern ConVar sv_neo_ignore_wep_xp_limit;
extern ConVar sv_neo_dev_loadout;

void CNeoLoadoutMenu::OnClose()
{
	CommandCompletion();
	BaseClass::OnClose();
}

void CNeoLoadoutMenu::OnCommand(const char* command)
{
	BaseClass::OnCommand(command);

	if (command == nullptr || *command == '\0')
	{
		return;
	}

	if (Q_stristr(command, "classmenu"))
	{ // return to class selection
		ChangeMenu("classmenu");
		return;
	}

	if (Q_stristr(command, "loadout "))
	{ // set player loadout
		CUtlStringList loadoutArgs;
		V_SplitString(command, " ", loadoutArgs);

		if (loadoutArgs.Size() != 2) {return;}

		Q_StripPrecedingAndTrailingWhitespace(loadoutArgs[1]);
		const int choiceNum = atoi(loadoutArgs[1]);

		auto localPlayer = C_NEO_Player::GetLocalNEOPlayer();
		if (!localPlayer) { return; }

		int currentXP = localPlayer->m_iXP.Get();
		int currentClass = localPlayer->m_iNextSpawnClassChoice.Get() != -1 ? localPlayer->m_iNextSpawnClassChoice.Get() : localPlayer->m_iNeoClass.Get();
		int numWeapons = CNEOWeaponLoadout::GetNumberOfLoadoutWeapons(currentXP,
				sv_neo_dev_loadout.GetBool() ? NEO_LOADOUT_DEV : currentClass);
			
		if (choiceNum+1 > numWeapons)
		{
#define INSUFFICIENT_LOADOUT_XP_MSG "Insufficient XP for equipping this loadout!\n"
			Msg(INSUFFICIENT_LOADOUT_XP_MSG);
			engine->Con_NPrintf(0, INSUFFICIENT_LOADOUT_XP_MSG);
			engine->ClientCmd(loadoutmenu.GetName());
		}else
		{
			engine->ClientCmd(command);
		}

	}
	CommandCompletion();
}


void CNeoLoadoutMenu::ChangeMenu(const char* menuName = NULL)
{
	CommandCompletion();
	ShowPanel(false);
	m_bLoadoutMenu = false;
	if (menuName == NULL)
	{
		return;
	}
	if (Q_stricmp(menuName, "classmenu") == 0 && NEORules()->GetForcedClass() < 0)
	{
		engine->ClientCmd(menuName);
	}
}

void CNeoLoadoutMenu::OnKeyCodeReleased(vgui::KeyCode code)
{
	if (code == gameuifuncs->GetButtonCodeForBind("loadoutmenu"))
	{
		ChangeMenu(NULL);
		return;
	}

	switch (code) {
	case KEY_SPACE: // Continue with the currently selected weapon
		ChangeMenu(NULL);
		break;
	default:
		// Ignore other key presses
		break;
	}
}

void CNeoLoadoutMenu::OnButtonPressed(KeyValues *data)
{
}

void CNeoLoadoutMenu::ApplySchemeSettings(vgui::IScheme *pScheme)
{
	BaseClass::ApplySchemeSettings(pScheme);
	LoadControlSettings(GetResFile());
	SetPaintBorderEnabled(false);
	SetPaintBackgroundEnabled(false);
	SetBorder(NULL);
	
	// FindButtons(); Doing this further down for all enabled buttons

	auto localPlayer = C_NEO_Player::GetLocalNEOPlayer();
	if (!localPlayer) { return; }

	const int currentXP = localPlayer->m_iXP.Get();
	const int currentClass = localPlayer->m_iNextSpawnClassChoice.Get() != -1 ? localPlayer->m_iNextSpawnClassChoice.Get() : localPlayer->m_iNeoClass.Get();

	int iLoadout = sv_neo_dev_loadout.GetBool() ? NEO_LOADOUT_DEV : currentClass;
	if (IN_BETWEEN_AR(0, iLoadout, NEO_LOADOUT__COUNT) == false)
	{
		iLoadout = NEO_LOADOUT_INVALID;
	}

	const auto &loadout = CNEOWeaponLoadout::s_LoadoutWeapons[iLoadout];
	for (int i = 0; i < MAX_WEAPON_LOADOUTS; ++i)
	{
		const int iWepPrice = loadout[i].m_iWeaponPrice;
		auto image = FindControl<ImagePanel>(szButtonImages[i]);

		if (iWepPrice <= currentXP)
		{
			// Available weapons
			auto button = FindControl<Button>(szButtons[i]);
			button->SetUseCaptureMouse(true);
			button->SetMouseInputEnabled(true);

			image->SetImage(loadout[i].info.m_szVguiImage);
		}
		else if (iWepPrice < XP_EMPTY)
		{
			// Locked weapons
			auto button = FindControl<Button>(szButtons[i]);
			const char *command = "";
			button->SetCommand(command);

			image->SetImage(loadout[i].info.m_szVguiImageNo);
		}
		else
		{
			// Dummy locked weapon slots
			image->SetImage("loadout/loadout_none");
		}
	}

	returnButton = FindControl<CNeoButton>("ReturnButton");
	returnButton->SetUseCaptureMouse(true);
	returnButton->SetMouseInputEnabled(true);
	InvalidateLayout();
}
