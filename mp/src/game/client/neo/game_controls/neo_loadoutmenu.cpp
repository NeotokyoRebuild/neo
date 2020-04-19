#include "cbase.h"
#include "neo_loadoutmenu.h"

#include "vgui_controls/Label.h"
#include "vgui_controls/ImagePanel.h"
#include "vgui_controls/Button.h"

#include <vgui/ILocalize.h>
#include <vgui/ISurface.h>

#include "ienginevgui.h"

#include "c_neo_player.h"
#include "weapon_neobasecombatweapon.h"

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

static const char *szButtons[] = {
    CONTROL_SCOUT_BUTTON,
    CONTROL_MISC2_BUTTON,
    CONTROL_DONE_BUTTON,
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
    CONTROL_BUTTON13,
    CONTROL_BUTTON14,
};

const int iNumButtonStrings = ARRAYSIZE(szButtons);

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

	const char *fontName = "Default";
	auto fontHandle = pScheme->GetFont(fontName, IsProportional());

	const Color selectedBgColor(0, 0, 0), selectedFgColor(255, 0, 0),
		armedBgColor(0, 0, 0), armedFgColor(0, 255, 0);

    for (int i = 0; i < iNumButtonStrings; i++)
	{
        auto button = FindControl<Button>(szButtons[i]);

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
    for (int i = 0; i < iNumButtonStrings; i++)
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

extern ConVar neo_sv_ignore_wep_xp_limit;

static bool IsAllowedGun(const int loadoutId, const int currentXP)
{
	if (neo_sv_ignore_wep_xp_limit.GetBool())
	{
		return true;
	}

	// NEO TODO (Rain): set reasonably
	const int xpLimits[] = {
		-255,	// MPN
		0,		// SRM
		0,		// SRM-S
		0,		// Jitte
		0,		// Jittescoped
		0,		// ZR68C
		0,		// ZR68S
		0,		// ZR68L
		10,		// MX
		20,		// PZ
		10,		// Supa7
		0,		// M41
		0,		// M41L
	};

	bool allowedThisGun = false;

	if (loadoutId < 0 || loadoutId > ARRAYSIZE(xpLimits))
	{
		DevWarning("Weapon choice out of XP check range: %i\n", loadoutId);
	}
	else
	{
		allowedThisGun = (currentXP >= xpLimits[loadoutId]);
	}

	return allowedThisGun;
}

void CNeoLoadoutMenu::OnCommand(const char* command)
{
	BaseClass::OnCommand(command);

	if (*command == NULL)
	{
		return;
	}

	bool allowedThisGun = false;
	CUtlStringList loadoutArgs;
	V_SplitString(command, " ", loadoutArgs);

	if (loadoutArgs.Size() == 2)
	{
		Q_StripPrecedingAndTrailingWhitespace(loadoutArgs[1]);
		const int choiceNum = atoi(loadoutArgs[1]);

		auto localplayer = C_NEO_Player::GetLocalNEOPlayer();
		Assert(localplayer);
		if (localplayer)
		{
			const int myXp = localplayer->m_iXP;
			allowedThisGun = IsAllowedGun(choiceNum, myXp);

			if (!allowedThisGun)
			{
				const char* noxpmsg = "Not enough XP for equipping this loadout!\n";
				Msg(noxpmsg);
				engine->Con_NPrintf(0, noxpmsg);
			}
		}
	}

	if (allowedThisGun)
	{
		engine->ClientCmd(command);
	}
	else
	{
		engine->ClientCmd(loadoutmenu.GetName());
	}

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

    const char *schemeName = "ClientScheme";

    vgui::HScheme neoscheme = vgui::scheme()->LoadSchemeFromFileEx(
        enginevgui->GetPanel(VGuiPanel_t::PANEL_CLIENTDLL), GetResFile(), schemeName);
    if (!neoscheme)
    {
        Assert(false);
        Warning("Failed to ApplySchemeSettings for CNeoLoadoutMenu\n");
        return;
    }

    vgui::IScheme *scheme = vgui::scheme()->GetIScheme(neoscheme);
    if (!scheme)
    {
        Assert(false);
        Warning("Failed to ApplySchemeSettings for CNeoLoadoutMenu\n");
        return;
    }

	LoadControlSettings(GetResFile());

	SetBgColor(Color(0, 0, 0, 196));

	const Color selectedBgColor(75, 75, 75), selectedFgColor(255, 0, 0, 128),
		armedBgColor(50, 50, 50, 128), armedFgColor(0, 255, 0, 128);

	const char *font = "Default";

    for (int i = 0; i < iNumButtonStrings; i++)
	{
        auto button = FindControl<Button>(szButtons[i]);

		if (!button)
		{
			Assert(false);
			continue;
		}

		auto str = button->GetCommand()->GetString("Command");
		if (V_stristr(str, "loadout") > 0)
		{
			CUtlStringList loadoutArgs;
			V_SplitString(str, " ", loadoutArgs);
			if (loadoutArgs.Size() == 2)
			{
				Q_StripPrecedingAndTrailingWhitespace(loadoutArgs[1]);
				const int loadoutId = atoi(loadoutArgs[1]);
				if (loadoutId < NEO_WEP_LOADOUT_ID_COUNT)
				{
					auto localPlayer = C_NEO_Player::GetLocalNEOPlayer();
					if (localPlayer)
					{
						if (!IsAllowedGun(loadoutId, localPlayer->m_iXP))
						{
							const char noxp[] = " -- [insufficient XP!]";
							char wepname[48 + sizeof(noxp)];
							button->GetText(wepname, sizeof(wepname));
							V_strcat_safe(wepname, noxp);
							button->SetText(wepname);

							button->SetTextColorState(Label::EColorState::CS_DULL);
							button->SetBgColor(COLOR_RED);
						}
						else
						{
							button->SetTextColorState(Label::EColorState::CS_NORMAL);
							button->SetBgColor(COLOR_GREEN);
						}
					}

					button->SetPaintBackgroundEnabled(true);
				}
			}
			else
			{
				Assert(false);
			}
		}

        button->SetFont(scheme->GetFont(font, IsProportional()));
		button->SetUseCaptureMouse(true);
		button->SetSelectedColor(selectedFgColor, selectedBgColor);
		button->SetArmedColor(armedFgColor, armedBgColor);
		button->SetMouseInputEnabled(true);
		button->InstallMouseHandler(this);
	}

	SetPaintBorderEnabled(false);

	SetBorder(NULL);

	SetMinimumSize(1280, 1280);
}
