#include "cbase.h"
#include "neo_loadoutmenu.h"

#include "vgui_controls/Label.h"
#include "vgui_controls/ImagePanel.h"
#include "vgui_controls/Button.h"

#include <vgui/ISurface.h>

#include "ienginevgui.h"

#include "c_neo_player.h"

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

using namespace vgui;

// These are defined in the .res file
#define CONTROL_SCOUT_BUTTON "Scout_Button"
#define CONTROL_MISC2_BUTTON "Misc2"
#define CONTROL_DONE_BUTTON "Done_Button"
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
#define CONTROL_BUTTON13 "Button13"
#define CONTROL_BUTTON14 "Button14"

Panel *NeoLoadout_Factory()
{
	return new CNeoLoadoutMenu(gViewPortInterface);
}

DECLARE_BUILD_FACTORY_CUSTOM(CNeoLoadoutMenu, NeoLoadout_Factory);

CNeoLoadoutMenu *g_pNeoLoadoutMenu = NULL;

CNeoLoadoutMenu::CNeoLoadoutMenu(IViewPort *pViewPort)
	: BaseClass(NULL, PANEL_NEO_LOADOUT)
{
	Assert(pViewPort);

	// Quiet "parent not sized yet" spew
	SetSize(10, 10);

	m_pViewPort = pViewPort;

	m_bLoadoutMenu = false;

	// NEO TODO (Rain): It appears that original Neotokyo
	// hardcodes its scheme. We probably need to make our
	// own res definition file to mimic it.
	const char *schemeName = "ClientScheme";

	vgui::HScheme neoscheme = vgui::scheme()->LoadSchemeFromFileEx(
		enginevgui->GetPanel(VGuiPanel_t::PANEL_CLIENTDLL), GetResFile(), schemeName);

	vgui::IScheme *pScheme = vgui::scheme()->GetIScheme(neoscheme);

	LoadControlSettings(GetResFile());

	SetVisible(false);
	SetProportional(true);
	SetMouseInputEnabled(true);
	SetKeyBoardInputEnabled(true);

	SetTitleBarVisible(false);

	m_pWeapon_ImagePanel = new ImagePanel(this, "Weapon_ImagePanel");
	m_pWeapon_ImagePanel->SetAutoDelete(true);

	m_pTitleLabel = new Label(this, "TitleLabel", "labelText");
	m_pTitleLabel->SetAutoDelete(true);

	m_pScout_Button = FindControl<Button>(CONTROL_SCOUT_BUTTON);
	m_pMisc2 = FindControl<Button>(CONTROL_MISC2_BUTTON);
	m_pDone_Button = FindControl<Button>(CONTROL_DONE_BUTTON);
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
	m_pButton13 = FindControl<Button>(CONTROL_BUTTON13);
	m_pButton14 = FindControl<Button>(CONTROL_BUTTON14);

	vgui::Button *buttons[] = {
		m_pScout_Button,
		m_pMisc2,
		m_pDone_Button,
		m_pButton1,
		m_pButton2,
		m_pButton3,
		m_pButton4,
		m_pButton5,
		m_pButton6,
		m_pButton7,
		m_pButton8,
		m_pButton9,
		m_pButton10,
		m_pButton11,
		m_pButton12,
		m_pButton13,
		m_pButton14,
	};

	const char *fontName = "Default";
	auto fontHandle = pScheme->GetFont(fontName, IsProportional());

	const Color selectedBgColor(0, 0, 0), selectedFgColor(255, 0, 0),
		armedBgColor(0, 0, 0), armedFgColor(0, 255, 0);

	for (vgui::Button *button : buttons)
	{
		if (!button)
		{
			Assert(false);
			Warning("Button was null on CNeoLoadoutMenu\n");
			continue;
		}

		button->AddActionSignalTarget(this);
		button->SetAutoDelete(true);

		button->SetFont(fontHandle);

		button->SetUseCaptureMouse(true);
		button->SetSelectedColor(selectedFgColor, selectedBgColor);
		button->SetArmedColor(armedFgColor, armedBgColor);
		button->SetMouseInputEnabled(true);
		button->InstallMouseHandler(this);
	}

	InvalidateLayout();

	g_pNeoLoadoutMenu = this;
}

CNeoLoadoutMenu::~CNeoLoadoutMenu()
{
}

void CNeoLoadoutMenu::CommandCompletion()
{
	vgui::Button *buttons[] = {
		m_pScout_Button,
		m_pMisc2,
		m_pDone_Button,
		m_pButton1,
		m_pButton2,
		m_pButton3,
		m_pButton4,
		m_pButton5,
		m_pButton6,
		m_pButton7,
		m_pButton8,
		m_pButton9,
		m_pButton10,
		m_pButton11,
		m_pButton12,
		m_pButton13,
		m_pButton14,
	};

	for (vgui::Button *button : buttons)
	{
		if (!button)
		{
			Assert(false);
			Warning("Button was null on CNeoLoadoutMenu\n");
			continue;
		}

		button->SetEnabled(false);
	}

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

void CNeoLoadoutMenu::OnCommand(const char* command)
{
	BaseClass::OnCommand(command);

	if (*command == NULL)
	{
		return;
	}

	engine->ClientCmd(command);

	CommandCompletion();
}

void CNeoLoadoutMenu::OnButtonPressed(KeyValues *data)
{
#if(0)
	DevMsg("Loadout button pressed\n");
	KeyValuesDumpAsDevMsg(data);
#endif
}

void CNeoLoadoutMenu::ApplySchemeSettings(vgui::IScheme *pScheme)
{
	BaseClass::ApplySchemeSettings(pScheme);

	if (!pScheme)
	{
		Assert(false);
		Warning("Failed to ApplySchemeSettings for CNeoLoadoutMenu\n");
		return;
	}

	LoadControlSettings(GetResFile());

	SetBgColor(Color(0, 0, 0, 0)); // make the background transparent

	vgui::Button *buttons[] = {
		m_pScout_Button,
		m_pMisc2,
		m_pDone_Button,
		m_pButton1,
		m_pButton2,
		m_pButton3,
		m_pButton4,
		m_pButton5,
		m_pButton6,
		m_pButton7,
		m_pButton8,
		m_pButton9,
		m_pButton10,
		m_pButton11,
		m_pButton12,
		m_pButton13,
		m_pButton14,
	};

	const Color selectedBgColor(0, 0, 0), selectedFgColor(255, 0, 0),
		armedBgColor(0, 0, 0), armedFgColor(0, 255, 0);

	const char *font = "Default";

	for (vgui::Button *button : buttons)
	{
		if (!button)
		{
			Assert(false);
			continue;
		}

		button->SetFont(pScheme->GetFont(font, IsProportional()));
		button->SetUseCaptureMouse(true);
		button->SetSelectedColor(selectedFgColor, selectedBgColor);
		button->SetArmedColor(armedFgColor, armedBgColor);
		button->SetMouseInputEnabled(true);
		button->InstallMouseHandler(this);
	}

	SetPaintBorderEnabled(false);

	SetBorder(NULL);
}