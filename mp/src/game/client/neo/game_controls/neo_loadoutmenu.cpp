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

CLoadoutWeaponClass s_DevLoadoutWeapons[]
{
	{ "MPN45", -255, "loadout/loadout_mpn", "loadout/loadout_mpn_no", "weapon_mpn", "AMMO_PRI" },
	{ "SRM", -255, "/loadout/loadout_srm", "loadout/loadout_srm_no", "weapon_srm", "AMMO_PRI" },
	{ "SRM (silenced)", -255, "loadout/loadout_srms", "loadout/loadout_srms_no", "weapon_srm_s", "AMMO_PRI" },
	{ "Jitte", -255, "/loadout/loadout_jitte", "loadout/loadout_jitte_no", "weapon_jitte", "AMMO_PRI" },
	{ "Jitte (with scope)", -255, "/loadout/loadout_jittes", "loadout/loadout_jittes_no", "weapon_jittescoped", "AMMO_PRI" },
	{ "ZR68C", -255, "/loadout/loadout_zr68c", "weapon_zr68c", "loadout/loadout_zr68c_no", "AMMO_PRI" },
	{ "ZR68-S (silenced)", -255, "loadout/loadout_zr68s", "loadout/loadout_zr68s_no", "weapon_zr68s", "AMMO_PRI" },
	{ "ZR68-L (accurized)", -255, "/loadout/loadout_zr68l", "loadout/loadout_zr68l_no", "weapon_zr68l", "AMMO_PRI" },
	{ "MX", -255, "loadout/loadout_mx", "loadout/loadout_mx_no", "weapon_mx", "AMMO_PRI" },
	{ "PZ252", -255, "loadout/loadout_pz", "loadout/loadout_pz_no", "weapon_pz", "AMMO_PRI" },
	{ "Murata Supa-7", -255, "loadout/loadout_supa7", "loadout/loadout_supa7_no", "weapon_supa7", "AMMO_PRI" },
	{ "Mosok", -255, "loadout/loadout_mosok", "loadout/loadout_mosok_no", "weapon_m41", "AMMO_PRI" },
	{ "Mosok (with scope)", -255, "loadout/loadout_mosokl", "loadout/loadout_mosokl_no", "weapon_m41l", "AMMO_PRI" }
};
static const int i_DevLoadoutWeaponsSize = ARRAYSIZE(s_DevLoadoutWeapons);

CLoadoutWeaponClass s_ReconLoadoutWeapons[]
{
	{ "MPN45", -255, "loadout/loadout_mpn", "loadout/loadout_mpn_no", "weapon_mpn", "AMMO_PRI" },
	{ "SRM", 0, "/loadout/loadout_srm", "loadout/loadout_srm_no", "weapon_srm", "AMMO_PRI" },
	{ "Jitte", 0, "/loadout/loadout_jitte", "loadout/loadout_jitte_no", "weapon_jitte", "AMMO_PRI" },
	{ "SRM (silenced)", 4, "/loadout/loadout_srms", "loadout/loadout_srms_no", "weapon_srm_s", "AMMO_PRI" },
	{ "Jitte (with scope)", 4, "/loadout/loadout_jittes", "loadout/loadout_jittes_no", "weapon_jittescoped", "AMMO_PRI" },
	{ "ZR68-L (accurized)", 4, "/loadout/loadout_zr68l", "loadout/loadout_zr68l_no", "weapon_zr68l", "AMMO_PRI" },
	{ "ZR68C", 10, "/loadout/loadout_zr68c", "loadout/loadout_zr68c_no", "weapon_zr68c", "AMMO_PRI" },
	{ "Murata Supa-7", 20, "/loadout/loadout_supa7", "loadout/loadout_supa7_no", "weapon_supa7", "AMMO_10G_SHELL" },
	{ "Mosok Silenced", 20, "/loadout/loadout_mosokl", "loadout/loadout_mosokl_no", "weapon_m41s", "AMMO_PRI" }
};
static const int i_ReconLoadoutWeaponsSize = ARRAYSIZE(s_ReconLoadoutWeapons);

CLoadoutWeaponClass s_AssaultLoadoutWeapons[]
{
	{ "MPN45", -255, "loadout/loadout_mpn", "loadout/loadout_mpn_no", "weapon_mpn", "AMMO_PRI" },
	{ "SRM", 0, "/loadout/loadout_srm", "loadout/loadout_srm_no", "weapon_srm", "AMMO_PRI" },
	{ "Jitte", 0, "/loadout/loadout_jitte", "loadout/loadout_jitte_no", "weapon_jitte", "AMMO_PRI" },
	{ "ZR68C", 0, "/loadout/loadout_zr68c", "loadout/loadout_zr68c_no", "weapon_zr68c", "AMMO_PRI" },
	{ "ZR68-S (silenced)", 4, "loadout/loadout_zr68s", "loadout/loadout_zr68s_no", "weapon_zr68s", "AMMO_PRI" },
	{ "Murata Supa-7", 4, "/loadout/loadout_supa7", "loadout/loadout_supa7_no", "weapon_supa7", "AMMO_10G_SHELL" },
	{ "Mosok", 4, "/loadout/loadout_mosok", "/loadout/loadout_mosok_no", "weapon_m41", "AMMO_PRI" },
	{ "Mosok Silenced", 10, "/loadout/loadout_mosokl", "loadout/loadout_mosokl_no", "weapon_m41s", "AMMO_PRI" },
	{ "MX", 10, "loadout/loadout_mx", "loadout/loadout_mx_no", "weapon_mx", "AMMO_PRI" },
	{ "MX Silenced", 10, "/loadout/loadout_mxs", "/loadout/loadout_mxs_no", "weapon_mx_silenced", "AMMO_PRI" },
	{ "AA13", 20, "/loadout/loadout_aa13", "/loadout/loadout_aa13_no", "weapon_aa13", "AMMO_10G_SHELL" },
	{ "SRS", 20, "/loadout/loadout_srs", "/loadout/loadout_srs_no", "weapon_srs", "AMMO_PRI" }
};
static const int i_AssaultLoadoutWeaponsSize = ARRAYSIZE(s_AssaultLoadoutWeapons);

CLoadoutWeaponClass s_SupportLoadoutWeapons[]
{
	{ "MPN45", -255, "loadout/loadout_mpn", "loadout/loadout_mpn_no", "weapon_mpn", "AMMO_PRI" },
	{ "SRM", 0, "/loadout/loadout_srm", "loadout/loadout_srm_no", "weapon_srm", "AMMO_PRI" },
	{ "ZR68C", 0, "/loadout/loadout_zr68c", "loadout/loadout_zr68c_no", "weapon_zr68c", "AMMO_PRI" },
	{ "Mosok", 0, "/loadout/loadout_mosok", "/loadout/loadout_mosok_no", "weapon_m41", "AMMO_PRI" },
	{ "Murata Supa-7", 0, "/loadout/loadout_supa7", "loadout/loadout_supa7_no", "weapon_supa7", "AMMO_10G_SHELL" },
	{ "MX", 4, "loadout/loadout_mx", "loadout/loadout_mx_no", "weapon_mx", "AMMO_PRI" },
	{ "Mosok Silenced", 4, "/loadout/loadout_mosokl", "loadout/loadout_mosokl_no", "weapon_m41s", "AMMO_PRI" },
	{ "MX Silenced", 10, "/loadout/loadout_mxs", "/loadout/loadout_mxs_no", "weapon_mx_silenced", "AMMO_PRI" },
	{ "PZ252", 20, "loadout/loadout_pz", "loadout/loadout_pz_no", "weapon_pz", "AMMO_PRI" }
};
static const int i_SupportLoadoutWeaponsSize = ARRAYSIZE(s_SupportLoadoutWeapons);


CLoadoutWeaponClass::CLoadoutWeaponClass(const char* weaponName, int weaponPrice, const char* vguiImage, const char* vguiImageNo, const char* weaponEntityName, const char* ammoType)
{
	m_szWeaponName = weaponName;
	m_iWeaponPrice = weaponPrice;
	m_szVguiImage = vguiImage;
	m_szVguiImageNo = vguiImageNo;
	m_szWeaponEntityName = weaponEntityName;
	m_szAmmoType = ammoType;
}

int GetNumberOfLoadoutWeapons(int rank, int classType, CLoadoutWeaponClass* pLoadout, int selectedLoadoutSize)
{
	int ammount = 0;
	int i = 0;

	for (i; i < selectedLoadoutSize; i++)
	{
		if (pLoadout[i].m_iWeaponPrice > rank)
		{
			return ammount;
		}
		ammount++;
	}
	return ammount;
}

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

const int iNumButtonStrings = ARRAYSIZE(szButtons);

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
	SetKeyBoardInputEnabled(true);
	SetTitleBarVisible(false);
	
	FindButtons();
}

CNeoLoadoutMenu::~CNeoLoadoutMenu()
{
	m_pButton1->SetAutoDelete(true);
	m_pButton2->SetAutoDelete(true);
	m_pButton3->SetAutoDelete(true);
	m_pButton4->SetAutoDelete(true);
	m_pButton5->SetAutoDelete(true);
	m_pButton6->SetAutoDelete(true);
	m_pButton7->SetAutoDelete(true);
	m_pButton8->SetAutoDelete(true);
	m_pButton9->SetAutoDelete(true);
	m_pButton10->SetAutoDelete(true);
	m_pButton11->SetAutoDelete(true);
	m_pButton12->SetAutoDelete(true);
	returnButton->SetAutoDelete(true);
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
	returnButton = FindControl<Button>("ReturnButton");

	for (int i = 0; i < iNumButtonStrings; i++)
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

	returnButton->SetUseCaptureMouse(true);
	returnButton->SetMouseInputEnabled(true);
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

	if (Q_stristr(command, "classmenu"))
	{ // return to class selection 
		CommandCompletion();
		ShowPanel(false);
		m_bLoadoutMenu = false;
		engine->ClientCmd("classmenu");
		C_NEO_Player* player = C_NEO_Player::GetLocalNEOPlayer();
		if (player)
		{
			player->m_bShowClassMenu = true;
		}
		else
		{
			Assert(false);
		}
		return;
	}

	if (Q_stristr(command, "loadout "))
	{ // set player loadout
		CUtlStringList loadoutArgs;
		V_SplitString(command, " ", loadoutArgs);

		if (loadoutArgs.Size() != 2) {return;}

		Q_StripPrecedingAndTrailingWhitespace(loadoutArgs[1]);
		const int choiceNum = atoi(loadoutArgs[1]);

		bool isDev = false;
		auto localPlayer = C_NEO_Player::GetLocalNEOPlayer();
		if (!localPlayer) { return; }

		int currentXP = localPlayer->m_iXP.Get();
		int currentClass = localPlayer->m_iNextSpawnClassChoice != -1 ? localPlayer->m_iNextSpawnClassChoice : localPlayer->GetClass();
		CLoadoutWeaponClass* selectedLoadout = isDev ? s_DevLoadoutWeapons : (currentClass == NEO_CLASS_RECON ? s_ReconLoadoutWeapons : (currentClass == NEO_CLASS_ASSAULT ? s_AssaultLoadoutWeapons : s_SupportLoadoutWeapons)); // might need an empty loadout if can't find class
		int selectedLoadoutSize = isDev ? i_DevLoadoutWeaponsSize : (currentClass == NEO_CLASS_RECON ? i_ReconLoadoutWeaponsSize : (currentClass == NEO_CLASS_ASSAULT ? i_AssaultLoadoutWeaponsSize : i_SupportLoadoutWeaponsSize));

		int numWeapons = GetNumberOfLoadoutWeapons(currentXP, currentClass, selectedLoadout, selectedLoadoutSize);
			
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

void CNeoLoadoutMenu::OnButtonPressed(KeyValues *data)
{
}

void CNeoLoadoutMenu::ApplySchemeSettings(vgui::IScheme *pScheme)
{
	SetPaintBorderEnabled(false);
	SetPaintBackgroundEnabled(false);
	SetBorder(NULL);

	BaseClass::ApplySchemeSettings(pScheme);

	if (!pScheme)
	{
		Assert(false);
		Warning("Failed to ApplySchemeSettings for CNeoLoadoutMenu\n");
		return;
	}

	LoadControlSettings(GetResFile());

	bool isDev = false;
	auto localPlayer = C_NEO_Player::GetLocalNEOPlayer();
	if (!localPlayer) { return; }

	int currentXP = localPlayer->m_iXP.Get();
	int currentClass = localPlayer->m_iNextSpawnClassChoice != -1 ? localPlayer->m_iNextSpawnClassChoice : localPlayer->GetClass();
	CLoadoutWeaponClass* selectedLoadout = isDev ? s_DevLoadoutWeapons : (currentClass == NEO_CLASS_RECON ? s_ReconLoadoutWeapons : (currentClass == NEO_CLASS_ASSAULT ? s_AssaultLoadoutWeapons : s_SupportLoadoutWeapons)); // might need an empty loadout if can't find class
	int selectedLoadoutSize = isDev ? i_DevLoadoutWeaponsSize : (currentClass == NEO_CLASS_RECON ? i_ReconLoadoutWeaponsSize : (currentClass == NEO_CLASS_ASSAULT ? i_AssaultLoadoutWeaponsSize : i_SupportLoadoutWeaponsSize));
	
	int numWeapons = GetNumberOfLoadoutWeapons(currentXP, currentClass, selectedLoadout, selectedLoadoutSize);
	int i = 0;
	for (i; i < MIN(iNumButtonStrings,numWeapons); i++)
	{ // update all available weapons
		auto button = FindControl<Button>(szButtons[i]);
		/*char command[64]; 
		sprintf(command, "loadout %s", selectedLoadout[i].m_szWeaponEntityName);
		button->SetCommand(command);*/
		button->SetUseCaptureMouse(true);
		button->SetMouseInputEnabled(true);

		auto image = FindControl<ImagePanel>(szButtonImages[i]);
		image->SetImage(selectedLoadout[i].m_szVguiImage);
	}

	for (i; i < MIN(iNumButtonStrings,selectedLoadoutSize); i++)
	{ // update all locked weapons
		auto button = FindControl<Button>(szButtons[i]);
		const char* command = ("");
		button->SetCommand(command);

		auto image = FindControl<ImagePanel>(szButtonImages[i]);
		image->SetImage(selectedLoadout[i].m_szVguiImageNo);
	}
	for (i; i < iNumButtonStrings; i++)
	{ // fill rest with dummy locked weapon
		auto image = FindControl<ImagePanel>(szButtonImages[i]);
		image->SetImage("loadout/loadout_none");
	}

	returnButton->SetUseCaptureMouse(true);
	returnButton->SetMouseInputEnabled(true);
	InvalidateLayout();
}
