#include "cbase.h"
#include "neo_root.h"
#include "IOverrideInterface.h"

#include "vgui/ILocalize.h"
#include "vgui/IPanel.h"
#include "vgui/ISurface.h"
#include "vgui/ISystem.h"
#include "vgui/IVGui.h"
#include "ienginevgui.h"
#include <engine/IEngineSound.h>
#include "filesystem.h"
#include "neo_version_info.h"
#include "cdll_client_int.h"
#include <steam/steam_api.h>
#include <vgui_avatarimage.h>
#include <IGameUIFuncs.h>
#include "inputsystem/iinputsystem.h"
#include "characterset.h"
#include "materialsystem/materialsystem_config.h"
#include "neo_player_shared.h"
#include <ivoicetweak.h>

#include <vgui/IInput.h>
#include <vgui_controls/Controls.h>

extern ConVar neo_name;
extern ConVar cl_onlysteamnick;
extern ConVar sensitivity;
extern ConVar snd_victory_volume;

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

// TODO: Gamepad
//   Gamepad enable: joystick 0/1
//   Reverse up-down axis: joy_inverty 0/1
//   Swap sticks on dual-stick controllers: joy_movement_stick 0/1
//   Horizontal sens: joy_yawsensitivity: -0.5 to -7.0
//   Vertical sens: joy_pitchsensitivity: 0.5 to 7.0

using namespace vgui;

// See interface.h/.cpp for specifics:  basically this ensures that we actually Sys_UnloadModule
// the dll and that we don't call Sys_LoadModule over and over again.
static CDllDemandLoader g_GameUIDLL( "GameUI" );

CNeoRoot *g_pNeoRoot = nullptr;

namespace {

int g_iRowTall = 40;
int g_iMarginX = 10;
int g_iMarginY = 10;
int g_iAvatar = 64;
int g_iRootSubPanelWide = 600;
int g_iGSIX[GSIW__TOTAL] = {};
HFont g_neoFont;
#define COLOR_NEOPANELNORMALBG Color(40, 40, 40, 255)
#define COLOR_NEOPANELSELECTBG Color(40, 10, 10, 255)
#define COLOR_NEOPANELACCENTBG Color(0, 0, 0, 255)
#define COLOR_NEOPANELTEXTNORMAL Color(200, 200, 200, 255)
#define COLOR_NEOPANELTEXTBRIGHT Color(255, 255, 255, 255)
#define COLOR_NEOPANELPOPUPBG Color(0, 0, 0, 170)
#define COLOR_NEOPANELFRAMEBG Color(40, 40, 40, 150)
#define COLOR_NEOTITLE Color(128, 128, 128, 255)
#define COLOR_NEOPANELBAR Color(20, 20, 20, 255)
#define COLOR_NEOPANELMICTEST Color(30, 90, 30, 255)
static constexpr wchar_t WSZ_GAME_TITLE[] = L"neatbkyoc ue";

int LoopAroundInArray(const int iValue, const int iSize)
{
	if (iValue < 0)
	{
		return iSize - 1;
	}
	else if (iValue >= iSize)
	{
		return 0;
	}
	return iValue;
}

const wchar_t *QUALITY_LABELS[] = {
	L"Low",
	L"Medium",
	L"High",
	L"Very High",
};

enum QualityEnum
{
	QUALITY_LOW = 0,
	QUALITY_MEDIUM,
	QUALITY_HIGH,
	QUALITY_VERYHIGH,
};

const wchar_t *ENABLED_LABELS[] = {
	L"Disabled",
	L"Enabled",
};

}

void OverrideGameUI()
{
	if (!OverrideUI->GetPanel())
	{
		OverrideUI->Create(0U);
	}

	if (g_pNeoRoot->GetGameUI())
	{
		g_pNeoRoot->GetGameUI()->SetMainMenuOverride(g_pNeoRoot->GetVPanel());
	}
}

CNeoOverlay_KeyCapture::CNeoOverlay_KeyCapture(Panel *parent)
	: EditablePanel(parent, "neo_overlay_keycapture")
{
	// Required to make sure text entries are recognized
	MakePopup(true);
	SetKeyBoardInputEnabled(true);
	SetMouseInputEnabled(true);
	SetBgColor(COLOR_NEOPANELNORMALBG);
	SetFgColor(COLOR_NEOPANELNORMALBG);
	SetVisible(false);
	SetEnabled(false);
}

void CNeoOverlay_KeyCapture::PerformLayout()
{
	// Like the root panel, we'll want the whole screen
	int wide, tall;
	vgui::surface()->GetScreenSize(wide, tall);

	SetPos(0, 0);
	SetSize(wide, tall);
	SetFgColor(COLOR_NEOPANELPOPUPBG);
	SetBgColor(COLOR_NEOPANELPOPUPBG);
}

void CNeoOverlay_KeyCapture::Paint()
{
	BaseClass::Paint();

	int wide, tall;
	GetSize(wide, tall);

	const int tallSplit = tall / 3;
	surface()->DrawSetColor(COLOR_NEOPANELNORMALBG);
	surface()->DrawFilledRect(0, tallSplit, wide, tall - tallSplit);

	int yPos = 0;
	surface()->DrawSetTextColor(COLOR_NEOPANELTEXTBRIGHT);
	{
		int textWidth, textHeight;
		surface()->DrawSetTextFont(m_fontMain);
		surface()->GetTextSize(m_fontMain, m_wszBindingText, textWidth, textHeight);
		const int textYPos = tallSplit + (tallSplit / 2) - (textHeight / 2);
		surface()->DrawSetTextPos((wide - textWidth) / 2, textYPos);
		surface()->DrawPrintText(m_wszBindingText, V_wcslen(m_wszBindingText));
		yPos = textYPos + textHeight;
	}
	{
		static constexpr wchar_t SUB_INFO[] = L"Press ESC to cancel or DEL to remove keybind";
		int textWidth, textHeight;
		surface()->DrawSetTextFont(m_fontSub);
		surface()->GetTextSize(m_fontSub, SUB_INFO, textWidth, textHeight);
		surface()->DrawSetTextPos((wide - textWidth) / 2, yPos + (textHeight / 2));
		surface()->DrawPrintText(SUB_INFO, SZWSZ_LEN(SUB_INFO));
	}
}

void CNeoOverlay_KeyCapture::OnThink()
{
	if (ButtonCode_t code; engine->CheckDoneKeyTrapping(code))
	{
		const bool bUpdate = (code != KEY_ESCAPE);
		if (bUpdate)
		{
			m_iButtonCode = (code == KEY_DELETE) ? BUTTON_CODE_NONE : code;
		}
		SetVisible(false);
		SetEnabled(false);
		PostActionSignal(new KeyValues("KeybindUpdate", "UpdateKey", bUpdate));
	}
}

CNeoOverlay_Confirm::CNeoOverlay_Confirm(Panel *parent)
	: EditablePanel(parent, "cneooverlay_confirm")
{
	MakePopup(true);
	SetKeyBoardInputEnabled(true);
	SetMouseInputEnabled(true);
	SetBgColor(COLOR_NEOPANELNORMALBG);
	SetFgColor(COLOR_NEOPANELNORMALBG);
	SetVisible(false);
	SetEnabled(false);
}

void CNeoOverlay_Confirm::PerformLayout()
{
	int wide, tall;
	vgui::surface()->GetScreenSize(wide, tall);

	SetPos(0, 0);
	SetSize(wide, tall);
	SetFgColor(COLOR_NEOPANELPOPUPBG);
	SetBgColor(COLOR_NEOPANELPOPUPBG);
}

void CNeoOverlay_Confirm::Paint()
{
	BaseClass::Paint();

	int wide, tall;
	GetSize(wide, tall);

	const int tallSplit = tall / 3;
	const int btnWide = wide / 6;
	surface()->DrawSetColor(COLOR_NEOPANELNORMALBG);
	surface()->DrawFilledRect(0, tallSplit, wide, tall - tallSplit);

	surface()->DrawSetTextColor(COLOR_NEOPANELTEXTBRIGHT);
	surface()->DrawSetTextFont(m_fontMain);
	{
		static constexpr wchar_t CONFIRM_TEXT[] = L"Settings changed: Do you want to apply the settings?";

		int textWidth, textHeight;
		surface()->GetTextSize(m_fontMain, CONFIRM_TEXT, textWidth, textHeight);
		const int textYPos = tallSplit + (tallSplit / 3) - (textHeight / 2);
		surface()->DrawSetTextPos((wide - textWidth) / 2, textYPos);
		surface()->DrawPrintText(CONFIRM_TEXT, SZWSZ_LEN(CONFIRM_TEXT));
	}
	{
		const int buttonYPos = tallSplit + (tallSplit * 0.67f) - (g_iRowTall / 2);
		surface()->DrawSetTextFont(g_neoFont);

		{
			static constexpr wchar_t APPLY_TEXT[] = L"Apply (Enter)";
			int textWidth, textHeight;
			surface()->GetTextSize(g_neoFont, APPLY_TEXT, textWidth, textHeight);

			const int buttonXPos = (wide / 2) - btnWide;
			surface()->DrawSetColor((m_buttonHover == BUTTON_APPLY) ? COLOR_NEOPANELSELECTBG : COLOR_NEOPANELACCENTBG);
			surface()->DrawFilledRect(buttonXPos, buttonYPos, buttonXPos + btnWide, buttonYPos + g_iRowTall);
			surface()->DrawSetTextPos(buttonXPos + ((btnWide / 2) - (textWidth / 2)), buttonYPos + ((g_iRowTall / 2) - (textHeight / 2)));
			surface()->DrawPrintText(APPLY_TEXT, SZWSZ_LEN(APPLY_TEXT));
		}
		{
			static constexpr wchar_t DISCARD_TEXT[] = L"Discard (ESC)";
			int textWidth, textHeight;
			surface()->GetTextSize(g_neoFont, DISCARD_TEXT, textWidth, textHeight);

			const int buttonXPos = (wide / 2);
			surface()->DrawSetColor((m_buttonHover == BUTTON_DISCARD) ? COLOR_NEOPANELSELECTBG : COLOR_NEOPANELACCENTBG);
			surface()->DrawFilledRect(buttonXPos, buttonYPos, buttonXPos + btnWide, buttonYPos + g_iRowTall);
			surface()->DrawSetTextPos(buttonXPos + ((btnWide / 2) - (textWidth / 2)), buttonYPos + ((g_iRowTall / 2) - (textHeight / 2)));
			surface()->DrawPrintText(DISCARD_TEXT, SZWSZ_LEN(DISCARD_TEXT));
		}
	}
}

void CNeoOverlay_Confirm::OnKeyCodePressed(vgui::KeyCode code)
{
	if (code == KEY_ESCAPE || code == KEY_ENTER)
	{
		m_bChoice = (code == KEY_ENTER);

		SetVisible(false);
		SetEnabled(false);
		PostActionSignal(new KeyValues("ConfirmDialogUpdate", "Accepted", m_bChoice));
	}
}

void CNeoOverlay_Confirm::OnMousePressed(vgui::MouseCode code)
{
	if (code == MOUSE_LEFT && m_buttonHover != BUTTON_NONE)
	{
		m_bChoice = (m_buttonHover == BUTTON_APPLY);

		SetVisible(false);
		SetEnabled(false);
		PostActionSignal(new KeyValues("ConfirmDialogUpdate", "Accepted", m_bChoice));
	}
}

void CNeoOverlay_Confirm::OnCursorMoved(int x, int y)
{
	int wide, tall;
	GetSize(wide, tall);

	const int tallSplit = tall / 3;
	const int btnWide = wide / 6;
	const int buttonYPos = tallSplit + (tallSplit * 0.67f) - (g_iRowTall / 2);

	m_buttonHover = BUTTON_NONE;
	if (y >= buttonYPos && y < (buttonYPos + g_iRowTall))
	{
		if (x >= ((wide / 2) - btnWide) && x < (wide / 2))
		{
			m_buttonHover = BUTTON_APPLY;
		}
		else if (x >= (wide / 2) && x < ((wide / 2) + btnWide))
		{
			m_buttonHover = BUTTON_DISCARD;
		}
	}
}

void CNeoDataSlider::SetValue(const float value)
{
	iValCur = static_cast<int>(value * flMulti);
	ClampAndUpdate();
}

float CNeoDataSlider::GetValue() const
{
	return static_cast<float>(iValCur) * (1.0f / flMulti);
}

void CNeoDataSlider::ClampAndUpdate()
{
	iValCur = clamp(iValCur, iValMin, iValMax);
	if (flMulti == 1.0f)
	{
		iWszCacheLabelSize = V_swprintf_safe(wszCacheLabel, L"%d", iValCur);
	}
	else
	{
		iWszCacheLabelSize = V_swprintf_safe(wszCacheLabel, L"%.02f", GetValue());
	}
	if (iWszCacheLabelSize < 0) iWszCacheLabelSize = 0;
}

CNeoPanel_Base::CNeoPanel_Base(Panel *parent)
	: EditablePanel(parent, "CNeoPanel_Base")
{
	MakePopup(true);
	SetKeyBoardInputEnabled(true);
	SetMouseInputEnabled(true);
}

void CNeoPanel_Base::UserSettingsRestore()
{
	for (int i = 0; i < TabsListSize(); ++i)
	{
		TabsList()[i]->UserSettingsRestore();
	}
	m_bModified = false;
}

void CNeoPanel_Base::UserSettingsSave()
{
	for (int i = 0; i < TabsListSize(); ++i)
	{
		TabsList()[i]->UserSettingsSave();
	}
	m_bModified = false;
}

void CNeoPanel_Base::PerformLayout()
{
	int wide, tall;
	vgui::surface()->GetScreenSize(wide, tall);

	BaseClass::PerformLayout();
	SetSize(g_iRootSubPanelWide, g_iRowTall + (tall * 0.8f) + g_iRowTall);
	SetBgColor(COLOR_NEOPANELFRAMEBG);
	SetFgColor(COLOR_NEOPANELFRAMEBG);
}

void CNeoPanel_Base::Paint()
{
	BaseClass::Paint();

	CNeoDataSettings_Base *tab = TabsList()[m_iNdsCurrent];
	surface()->DrawSetTextFont(g_neoFont);
	const int wgXPos = static_cast<int>(g_iRootSubPanelWide * 0.4f);
	const int widgetWide = g_iRootSubPanelWide - wgXPos;
	const int widgetTall = g_iRowTall;
	int fontWide, fontTall;
	surface()->GetTextSize(g_neoFont, L"<", fontWide, fontTall);
	const int fontStartXPos = (widgetTall / 2) - (fontWide / 2);
	const int fontStartYPos = (widgetTall / 2) - (fontTall / 2);
	const int panelTall = GetTall();
	const bool bEditBlinkShow = (static_cast<int>(gpGlobals->curtime * 1.5f) % 2 == 0);

	const int yOvershoot = m_iScrollOffset % g_iRowTall;
	for (int i = (m_iScrollOffset / g_iRowTall), yPos = TopAreaTall() - yOvershoot;
		 yPos < (panelTall - g_iRowTall) && i < tab->NdvListSize();
		 ++i, yPos += widgetTall)
	{
		CNeoDataVariant *ndv = &tab->NdvList()[i];
		const bool bThisHover = (m_eSectionActive == SECTION_MAIN && i == m_iNdvHover);
		const bool bThisFocus = (m_eSectionActive == SECTION_MAIN && i == m_iNdvFocus);
		const bool bThisHoverFocus = bThisHover || bThisFocus;

		surface()->DrawSetColor(bThisHoverFocus ? COLOR_NEOPANELSELECTBG : COLOR_NEOPANELACCENTBG);
		surface()->DrawSetTextColor(bThisFocus ? COLOR_NEOPANELTEXTBRIGHT : COLOR_NEOPANELTEXTNORMAL);

		if (ndv->type == CNeoDataVariant::GAMESERVER)
		{
			if (!bThisHoverFocus) surface()->DrawSetColor((i % 2 == 0) ? COLOR_NEOPANELNORMALBG : COLOR_NEOPANELACCENTBG);
			surface()->DrawFilledRect(0, yPos, g_iRootSubPanelWide, yPos + widgetTall);

			// TODO/TEMP (nullsystem): Probably should be more "custom" struct verse gameserveritem_t?
			const gameserveritem_t *gameserver = &ndv->gameServer.info;
			int xPos = 0;

			if (gameserver->m_bPassword)
			{
				surface()->DrawSetTextPos(xPos + g_iMarginX, yPos + fontStartYPos);
				surface()->DrawPrintText(L"P", 1);
			}
			xPos += g_iGSIX[GSIW_LOCKED];

			if (gameserver->m_bSecure)
			{
				surface()->DrawSetTextPos(xPos + g_iMarginX, yPos + fontStartYPos);
				surface()->DrawPrintText(L"S", 1);
			}
			xPos += g_iGSIX[GSIW_VAC];

			{
				wchar_t wszServerName[k_cbMaxGameServerName];
				const int iSize = g_pVGuiLocalize->ConvertANSIToUnicode(gameserver->GetName(), wszServerName, sizeof(wszServerName));
				surface()->DrawSetTextPos(xPos + g_iMarginX, yPos + fontStartYPos);
				surface()->DrawPrintText(wszServerName, iSize);
			}
			xPos += g_iGSIX[GSIW_NAME];

			{
				// In lower resolution, it may overlap from name, so paint a background here
				surface()->DrawFilledRect(xPos, yPos, g_iRootSubPanelWide, yPos + widgetTall);

				wchar_t wszMapName[k_cbMaxGameServerMapName];
				const int iSize = g_pVGuiLocalize->ConvertANSIToUnicode(gameserver->m_szMap, wszMapName, sizeof(wszMapName));
				surface()->DrawSetTextPos(xPos + g_iMarginX, yPos + fontStartYPos);
				surface()->DrawPrintText(wszMapName, iSize);
			}
			xPos += g_iGSIX[GSIW_MAP];

			{
				// In lower resolution, it may overlap from name, so paint a background here
				surface()->DrawFilledRect(xPos, yPos, g_iRootSubPanelWide, yPos + widgetTall);

				wchar_t wszPlayers[10];
				const int iSize = V_swprintf_safe(wszPlayers, L"%d/%d", gameserver->m_nPlayers, gameserver->m_nMaxPlayers);
				surface()->DrawSetTextPos(xPos + g_iMarginX, yPos + fontStartYPos);
				surface()->DrawPrintText(wszPlayers, iSize);
			}
			xPos += g_iGSIX[GSIW_PLAYERS];

			{
				wchar_t wszPing[10];
				const int iSize = V_swprintf_safe(wszPing, L"%d", gameserver->m_nPing);
				surface()->DrawSetTextPos(xPos + g_iMarginX, yPos + fontStartYPos);
				surface()->DrawPrintText(wszPing, iSize);
			}

			continue;
		}

		switch (ndv->type)
		{
		case CNeoDataVariant::RINGBOX:
		case CNeoDataVariant::SLIDER:
		{
			// Draw right-side widget's left-right "<" ">" buttons
			surface()->DrawFilledRect(wgXPos, yPos, wgXPos + widgetTall, yPos + widgetTall);
			surface()->DrawFilledRect(wgXPos + widgetWide - widgetTall, yPos, wgXPos + widgetWide, yPos + widgetTall);
			surface()->DrawSetTextPos(wgXPos + fontStartXPos, yPos + fontStartYPos);
			surface()->DrawPrintText(L"<", 1);
			surface()->DrawSetTextPos(wgXPos + widgetWide - widgetTall + fontStartXPos, yPos + fontStartYPos);
			surface()->DrawPrintText(L">", 1);
		}
		break;
		case CNeoDataVariant::TEXTENTRY:
		{
			surface()->DrawFilledRect(wgXPos, yPos, wgXPos + widgetWide, yPos + widgetTall);
		}
		break;
		case CNeoDataVariant::BINDENTRY:
		case CNeoDataVariant::BUTTON:
		case CNeoDataVariant::S_MICTESTER:
		{
			if (ndv->type != CNeoDataVariant::S_MICTESTER && !bThisHoverFocus)
			{
				surface()->DrawSetColor((i % 2 == 0) ? COLOR_NEOPANELNORMALBG : COLOR_NEOPANELACCENTBG);
			}
			if (ndv->labelSize > 0)	surface()->DrawFilledRect(wgXPos, yPos, wgXPos + widgetWide, yPos + widgetTall);
			else					surface()->DrawFilledRect(0, yPos, g_iRootSubPanelWide, yPos + widgetTall);
		}
		break;
		default: break;
		}

		// Draw the left-side label
		if (ndv->labelSize > 0)
		{
			const wchar_t *wszLeftLabel = ndv->label;
			if (ndv->type == CNeoDataVariant::TEXTLABEL)
			{
				wszLeftLabel = ndv->textLabel.wszLabel;

				int fontWide, fontTall;
				surface()->GetTextSize(g_neoFont, ndv->textLabel.wszLabel, fontWide, fontTall);
				surface()->DrawSetTextPos((g_iRootSubPanelWide / 2) - (fontWide / 2), yPos + fontStartYPos);
			}
			else
			{
				surface()->DrawSetTextPos(g_iMarginX, yPos + fontStartYPos);
			}
			surface()->DrawPrintText(wszLeftLabel, ndv->labelSize);
		}

		// Draw the right-side widget - widget specific
		switch (ndv->type)
		{
		case CNeoDataVariant::RINGBOX:
		{
			CNeoDataRingBox *rb = &ndv->ringBox;
			// Draw center text
			const wchar_t *wszLabel = rb->wszItems[rb->iCurIdx];
			int fontWide, fontTall;
			surface()->GetTextSize(g_neoFont, wszLabel, fontWide, fontTall);
			surface()->DrawSetTextPos(wgXPos + (widgetWide / 2) - (fontWide / 2), yPos + fontStartYPos);
			surface()->DrawPrintText(wszLabel, V_wcslen(wszLabel));
		}
		break;
		case CNeoDataVariant::SLIDER:
		{
			CNeoDataSlider *sl = &ndv->slider;

			// Draw the background percentage bar
			surface()->DrawSetColor(COLOR_NEOPANELBAR);
			const float perc = static_cast<float>(sl->iValCur - sl->iValMin) / static_cast<float>(sl->iValMax - sl->iValMin);
			surface()->DrawFilledRect(wgXPos + widgetTall,
									  yPos,
									  wgXPos + widgetTall + static_cast<int>(perc * static_cast<float>(widgetWide - (2 * widgetTall))),
									  yPos + widgetTall);

			// Draw center text - Generally size 0 never happens but sanity check
			if (sl->iWszCacheLabelSize > 0)
			{
				int fontWide, fontTall;
				surface()->GetTextSize(g_neoFont, sl->wszCacheLabel, fontWide, fontTall);
				surface()->DrawSetTextPos(wgXPos + (widgetWide / 2) - (fontWide / 2), yPos + fontStartYPos);
				surface()->DrawPrintText(sl->wszCacheLabel, sl->iWszCacheLabelSize);
				if (bThisFocus && bEditBlinkShow)
				{
					surface()->DrawSetTextPos(wgXPos + (widgetWide / 2) - (fontWide / 2) + fontWide,
											  yPos + fontStartYPos);
					surface()->DrawPrintText(L"_", 1);
				}
			}
		}
		break;
		case CNeoDataVariant::BUTTON:
		{
			CNeoDataButton *btn = &ndv->button;
			if (btn->iWszBtnLabelSize > 0)
			{
				int fontWide, fontTall;
				surface()->GetTextSize(g_neoFont, btn->wszBtnLabel, fontWide, fontTall);
				if (ndv->labelSize > 0)	surface()->DrawSetTextPos(wgXPos + (widgetWide / 2) - (fontWide / 2), yPos + fontStartYPos);
				else					surface()->DrawSetTextPos((g_iRootSubPanelWide / 2) - (fontWide / 2), yPos + fontStartYPos);
				surface()->DrawPrintText(btn->wszBtnLabel, btn->iWszBtnLabelSize);
			}
		}
		break;
		case CNeoDataVariant::BINDENTRY:
		{
			CNeoDataBindEntry *be = &ndv->bindEntry;
			wchar_t wszBindBtnName[64];
			const char *bindBtnName = g_pInputSystem->ButtonCodeToString(be->bcNext);
			g_pVGuiLocalize->ConvertANSIToUnicode(bindBtnName, wszBindBtnName, sizeof(wszBindBtnName));

			int fontWide, fontTall;
			surface()->GetTextSize(g_neoFont, wszBindBtnName, fontWide, fontTall);
			surface()->DrawSetTextPos(wgXPos + (widgetWide / 2) - (fontWide / 2), yPos + fontStartYPos);
			surface()->DrawPrintText(wszBindBtnName, V_wcslen(wszBindBtnName));
		}
		break;
		case CNeoDataVariant::TEXTENTRY:
		{
			CNeoDataTextEntry *te = &ndv->textEntry;
			surface()->DrawSetTextPos(wgXPos + g_iMarginX, yPos + fontStartYPos);
			surface()->DrawPrintText(te->wszEntry, V_wcslen(te->wszEntry));
			if (bThisFocus && bEditBlinkShow)
			{
				int textWide, textTall;
				surface()->GetTextSize(g_neoFont, te->wszEntry, textWide, textTall);
				surface()->DrawSetTextPos(wgXPos + g_iMarginX + textWide, yPos + fontStartYPos);
				surface()->DrawPrintText(L"_", 1);
			}
		}
		break;
		case CNeoDataVariant::S_DISPLAYNAME:
		{
			surface()->DrawSetTextPos(wgXPos + g_iMarginX, yPos + fontStartYPos);

			const auto *ndvMultiList = TabsList()[CNeoPanel_Settings::TAB_MULTI]->NdvList();
			const wchar_t *wszNeoName = ndvMultiList[CNeoDataSettings_Multiplayer::OPT_MULTI_NEONAME].textEntry.wszEntry;
			const bool bOnlySteamNick = static_cast<bool>(ndvMultiList[CNeoDataSettings_Multiplayer::OPT_MULTI_ONLYSTEAMNICK].ringBox.iCurIdx);
			if (bOnlySteamNick || !wszNeoName || wszNeoName[0] == '\0')
			{
				wchar_t wszSteamName[33];
				wszSteamName[0] = '\0';
				if (ISteamFriends *steamFriends = steamapicontext->SteamFriends())
				{
					g_pVGuiLocalize->ConvertANSIToUnicode(steamFriends->GetPersonaName(), wszSteamName, sizeof(wszSteamName));
				}
				surface()->DrawPrintText(wszSteamName, V_wcslen(wszSteamName));
			}
			else
			{
				surface()->DrawPrintText(wszNeoName, V_wcslen(wszNeoName));
			}
		}
		break;
		case CNeoDataVariant::S_MICTESTER:
		{
			CNeoDataMicTester *mt = &ndv->micTester;
			IVoiceTweak_s *voiceTweak = engine->GetVoiceTweakAPI();

			static constexpr wchar_t WSZ_MICTESTENTER[] = L"Start testing";
			static constexpr wchar_t WSZ_MICTESTEXIT[] = L"Stop testing";

			const bool bIsTweaking = voiceTweak->IsStillTweaking();
			if (bIsTweaking)
			{
				// Only fetch the value at interval as immediate is too quick/flickers, and give a longer delay when
				// it goes from sound to no sound.
				static constexpr float FL_FETCH_INTERVAL = 0.1f;
				static constexpr float FL_SILENCE_INTERVAL = 0.4f;
				const float flSpeaking = voiceTweak->GetControlFloat(SpeakingVolume);
				if ((flSpeaking > 0.0f && mt->flLastFetchInterval + FL_FETCH_INTERVAL < gpGlobals->curtime) ||
						(flSpeaking == 0.0f && mt->flSpeakingVol > 0.0f && mt->flLastFetchInterval + FL_SILENCE_INTERVAL < gpGlobals->curtime))
				{
					mt->flSpeakingVol = flSpeaking;
					mt->flLastFetchInterval = gpGlobals->curtime;
				}
				surface()->DrawSetColor(COLOR_NEOPANELMICTEST);
				surface()->DrawFilledRect(wgXPos,
										  yPos,
										  wgXPos + static_cast<int>(mt->flSpeakingVol * static_cast<float>(widgetWide)),
										  yPos + widgetTall);
			}

			int fontWide, fontTall;
			surface()->GetTextSize(g_neoFont, bIsTweaking ? WSZ_MICTESTEXIT : WSZ_MICTESTENTER, fontWide, fontTall);
			surface()->DrawSetTextPos(wgXPos + (widgetWide / 2) - (fontWide / 2), yPos + fontStartYPos);
			surface()->DrawPrintText(bIsTweaking ? WSZ_MICTESTEXIT : WSZ_MICTESTENTER,
									 bIsTweaking ? SZWSZ_LEN(WSZ_MICTESTEXIT) : SZWSZ_LEN(WSZ_MICTESTENTER));
		}
		break;
		}
	}

	if (TopAreaRows() > 0)
	{
		const int iTabWide = g_iRootSubPanelWide / TabsListSize();
		// Draw the top part
		surface()->DrawSetColor(COLOR_NEOPANELNORMALBG);
		surface()->DrawFilledRect(0, 0, g_iRootSubPanelWide, TopAreaTall());

		surface()->DrawSetColor(COLOR_NEOPANELSELECTBG);
		surface()->DrawSetTextColor(COLOR_NEOPANELTEXTNORMAL);

		// Draw the tabs buttons on top
		for (int i = 0, xPosTab = 0; i < TabsListSize(); ++i, xPosTab += iTabWide)
		{
			const bool bTabHover = (m_eSectionActive == SECTION_TOP && i == m_iNdvHover);
			if (i == m_iNdsCurrent || bTabHover)
			{
				surface()->DrawSetColor(bTabHover ? COLOR_NEOPANELSELECTBG : COLOR_NEOPANELACCENTBG);
				surface()->DrawFilledRect(xPosTab, 0, xPosTab + iTabWide, g_iRowTall);
			}
			surface()->DrawSetTextPos(xPosTab + g_iMarginX, fontStartYPos);
			const auto titleInfo = TabsList()[i]->Title();
			surface()->DrawPrintText(titleInfo.text, titleInfo.size);
		}
		TopAreaPaint();
	}

	const int bottomYStart = (panelTall - g_iRowTall);
	{
		const int iBtnWide = g_iRootSubPanelWide / BottomSectionListSize();

		// Draw the bottom part
		surface()->DrawSetColor(COLOR_NEOPANELNORMALBG);
		surface()->DrawFilledRect(0, bottomYStart, g_iRootSubPanelWide, panelTall);

		// Draw the buttons on bottom
		for (int i = 0, xPosTab = 0; i < BottomSectionListSize(); ++i, xPosTab += iBtnWide)
		{
			const auto bottomInfo = BottomSectionList()[i];
			const bool bBottomHover = (m_eSectionActive == SECTION_BOTTOM && i == m_iNdvHover);
			if (bottomInfo.text)
			{
				surface()->DrawSetColor(bBottomHover ? COLOR_NEOPANELSELECTBG : COLOR_NEOPANELNORMALBG);
				surface()->DrawFilledRect(xPosTab, bottomYStart, xPosTab + iBtnWide, panelTall);
				surface()->DrawSetTextPos(xPosTab + g_iMarginX, bottomYStart + fontStartYPos);
				surface()->DrawPrintText(bottomInfo.text, bottomInfo.size);
			}
		}
	}
}

void CNeoPanel_Base::OnMousePressed(vgui::MouseCode code)
{
	OnExitTextEditMode();
	switch (m_eSectionActive)
	{
	break; case SECTION_TOP:
	{
		m_iNdsCurrent = m_iNdvHover;
		m_iScrollOffset = 0;
		InvalidateLayout();
		PerformLayout();
		RequestFocus();
	}
	break; case SECTION_MAIN:
	{
		CNeoDataSettings_Base *tab = TabsList()[m_iNdsCurrent];
		if (code != MOUSE_LEFT || m_iNdvHover < 0 || m_iNdvHover >= tab->NdvListSize()) return;
		CNeoDataVariant *ndv = &tab->NdvList()[m_iNdvHover];
		switch (ndv->type)
		{
		case CNeoDataVariant::RINGBOX:
		{
			if (m_curMouse == WDG_NONE || m_curMouse == WDG_CENTER) return;

			CNeoDataRingBox *rb = &ndv->ringBox;
			rb->iCurIdx += (m_curMouse == WDG_LEFT) ? -1 : +1;
			rb->iCurIdx = LoopAroundInArray(rb->iCurIdx, rb->iItemsSize);

			m_bModified = true;
		}
		break;
		case CNeoDataVariant::SLIDER:
		{
			if (m_curMouse == WDG_NONE || m_curMouse == WDG_CENTER) return;

			CNeoDataSlider *sl = &ndv->slider;
			sl->iValCur += (m_curMouse == WDG_LEFT) ? -sl->iValStep : +sl->iValStep;
			sl->ClampAndUpdate();

			m_bModified = true;
		}
		break;
		case CNeoDataVariant::BUTTON:
			if (ndv->labelSize == 0 || (ndv->labelSize > 0 && m_curMouse == WDG_CENTER)) OnEnterButton(ndv);
			break;
		case CNeoDataVariant::BINDENTRY:
			if (m_curMouse == WDG_CENTER) OnEnterButton(ndv);
			break;
		case CNeoDataVariant::S_MICTESTER:
		{
			IVoiceTweak_s *voiceTweak = engine->GetVoiceTweakAPI();
			voiceTweak->IsStillTweaking() ? (void)voiceTweak->EndVoiceTweakMode() : (void)voiceTweak->StartVoiceTweakMode();
		}
		break;
		}
	}
	break; case SECTION_BOTTOM:
	{
		OnBottomAction(m_iNdvHover);
	}
	}
}

void CNeoPanel_Base::OnMouseDoublePressed(vgui::MouseCode code)
{
	OnExitTextEditMode();
	if (m_eSectionActive != SECTION_MAIN)
	{
		return;
	}
	CNeoDataSettings_Base *tab = TabsList()[m_iNdsCurrent];
	if (m_iNdvHover < 0 || m_iNdvHover >= tab->NdvListSize()) return;

	CNeoDataVariant *ndv = &tab->NdvList()[m_iNdvHover];
	switch (ndv->type)
	{
	case CNeoDataVariant::SLIDER:
	case CNeoDataVariant::TEXTENTRY:
	{
		if (m_curMouse == WDG_CENTER && code == MOUSE_LEFT)
		{
			m_iNdvFocus = m_iNdvHover;
			RequestFocus();
		}
		break;
	}
	case CNeoDataVariant::GAMESERVER:
		OnEnterServer(ndv->gameServer.info);
		break;
	}
}

void CNeoPanel_Base::OnMouseWheeled(int delta)
{
	int iScreenWidth, iScreenHeight;
	engine->GetScreenSize(iScreenWidth, iScreenHeight);

	const float flScrollSpeed = static_cast<float>(iScreenHeight) * 0.05f;

	CNeoDataSettings_Base *tab = TabsList()[m_iNdsCurrent];
	const int iWidgetSectionTall = GetTall() - g_iRowTall - TopAreaTall();
	int iMaxScrollOffset = (tab->NdvListSize() * g_iRowTall) - iWidgetSectionTall;
	if (iMaxScrollOffset < 0) iMaxScrollOffset = 0;
	m_iScrollOffset += (int)((float)(-delta) * flScrollSpeed);
	m_iScrollOffset = clamp(m_iScrollOffset, 0, iMaxScrollOffset);
}

void CNeoPanel_Base::OnKeyCodeTyped(vgui::KeyCode code)
{
	CNeoDataSettings_Base *tab = TabsList()[m_iNdsCurrent];
	const int iNdvListSize = tab->NdvListSize();

	// "Universal" binds
	if (const int iBottomBtns = KeyCodeToBottomAction(code);
			iBottomBtns >= 0 && iBottomBtns < BottomSectionListSize())
	{
		OnBottomAction(iBottomBtns);
		return;
	}
	else if (code == KEY_F1 || code == KEY_F3)
	{
		// Change tab
		OnExitTextEditMode();
		const bool bBackward = (code == KEY_F1);
		const int iIncr = bBackward ? -1 : +1;
		m_iNdsCurrent += iIncr;
		m_iNdsCurrent = LoopAroundInArray(m_iNdsCurrent, TabsListSize());

		m_eSectionActive = SECTION_MAIN;
		m_iNdvHover = 0;
		return;
	}
	else if (code == KEY_TAB)
	{
		// Change section, top may active by key if it has more than just tabs
		OnExitTextEditMode();
		const bool bBackward = (input()->IsKeyDown(KEY_LSHIFT) || input()->IsKeyDown(KEY_RSHIFT));
		int iSectionActive = static_cast<int>(m_eSectionActive);
		iSectionActive += (bBackward) ? -1 : +1;
		iSectionActive = LoopAroundInArray(iSectionActive, SECTION__TOTAL);
		if (iSectionActive == SECTION_TOP && TopAreaRows() <= 1) iSectionActive = SECTION_MAIN;
		m_eSectionActive = static_cast<eSectionActive>(iSectionActive);
		m_iNdvHover = 0;
		return;
	}

	// Section limited binds
	switch (m_eSectionActive)
	{
	break; case SECTION_TOP:
	{
		// TODO: Deal with it via function?
	}
	break; case SECTION_MAIN:
	{
		if (code == KEY_DOWN || code == KEY_UP)
		{
			if (iNdvListSize == 0) return;

			OnExitTextEditMode();
			const int iIncr = (code == KEY_DOWN) ? +1 : -1;

			// Increment or decrement to the next valid NDV
			CNeoDataVariant::Type type;
			do
			{
				m_iNdvHover += iIncr;
				if (m_iNdvHover < 0) m_iNdvHover = (iNdvListSize - 1);
				else if (m_iNdvHover >= iNdvListSize) m_iNdvHover = 0;
				type = tab->NdvList()[m_iNdvHover].type;
			}
			while (type == CNeoDataVariant::TEXTLABEL || type == CNeoDataVariant::S_DISPLAYNAME);

			// Re-adjust scroll offset
			const int iWdgArea = GetTall() - g_iRowTall - TopAreaTall();
			const int iWdgYPos = g_iRowTall * m_iNdvHover;
			if (iWdgYPos < m_iScrollOffset)
			{
				m_iScrollOffset = iWdgYPos;
			}
			else if ((iWdgYPos + g_iRowTall) >= (m_iScrollOffset + iWdgArea))
			{
				m_iScrollOffset = (iWdgYPos - iWdgArea + g_iRowTall);
			}

			m_iNdvFocus = -1;
			return;
		}

		if (m_iNdvHover < 0 || m_iNdvHover >= iNdvListSize) return;
		CNeoDataVariant *ndv = &tab->NdvList()[m_iNdvHover];

		if (code == KEY_LEFT || code == KEY_RIGHT)
		{
			switch (ndv->type)
			{
			case CNeoDataVariant::RINGBOX:
			{
				CNeoDataRingBox *rb = &ndv->ringBox;
				rb->iCurIdx += (code == KEY_LEFT) ? -1 : +1;
				rb->iCurIdx = LoopAroundInArray(rb->iCurIdx, rb->iItemsSize);
				m_bModified = true;
			}
			break;
			case CNeoDataVariant::SLIDER:
			{
				CNeoDataSlider *sl = &ndv->slider;
				sl->iValCur += (code == KEY_LEFT) ? -sl->iValStep : +sl->iValStep;
				sl->ClampAndUpdate();
				m_bModified = true;
			}
			break;
			default: break;
			}

			OnExitTextEditMode();
			return;
		}
		else if (code == KEY_ENTER)
		{
			// Change editing mode
			switch (ndv->type)
			{
			case CNeoDataVariant::SLIDER:
			case CNeoDataVariant::TEXTENTRY:
			{
				if (m_iNdvFocus != -1 && ndv->type == CNeoDataVariant::SLIDER)
				{
					ndv->slider.ClampAndUpdate();
				}
				m_iNdvFocus = (m_iNdvFocus == -1) ? m_iNdvHover : -1;
				break;
			}
			case CNeoDataVariant::BUTTON:
			case CNeoDataVariant::BINDENTRY:
				OnEnterButton(ndv);
				break;
			case CNeoDataVariant::S_MICTESTER:
			{
				IVoiceTweak_s *voiceTweak = engine->GetVoiceTweakAPI();
				voiceTweak->IsStillTweaking() ? (void)voiceTweak->EndVoiceTweakMode() : (void)voiceTweak->StartVoiceTweakMode();
				break;
			}
			case CNeoDataVariant::GAMESERVER:
				OnEnterServer(ndv->gameServer.info);
				break;
			default:
				break;
			}
			return;
		}
		else
		{
			if (m_iNdvFocus < 0 || m_iNdvFocus >= iNdvListSize) return;
			CNeoDataVariant *focusNdv = &tab->NdvList()[m_iNdvFocus];
			switch (focusNdv->type)
			{
			case CNeoDataVariant::SLIDER:
			{
				CNeoDataSlider *sl = &focusNdv->slider;
				if (code == KEY_BACKSPACE && sl->iWszCacheLabelSize > 0)
				{
					sl->wszCacheLabel[--sl->iWszCacheLabelSize] = '\0';
					m_bModified = true;
				}
			}
			break;
			case CNeoDataVariant::TEXTENTRY:
			{
				CNeoDataTextEntry *te = &focusNdv->textEntry;
				int iWszSize = V_wcslen(te->wszEntry);
				if (code == KEY_BACKSPACE && iWszSize > 0)
				{
					te->wszEntry[--iWszSize] = '\0';
					m_bModified = true;
				}
			}
			break;
			default: break;
			}
		}
	}
	break; case SECTION_BOTTOM:
	{
		if (code == KEY_LEFT || code == KEY_RIGHT)
		{
			do
			{
				m_iNdvHover += (code == KEY_LEFT) ? -1 : +1;
				m_iNdvHover = LoopAroundInArray(m_iNdvHover, BottomSectionListSize());
			}
			while (BottomSectionList()[m_iNdvHover].text == nullptr);
		}
		else if (code == KEY_ENTER)
		{
			OnBottomAction(m_iNdvHover);
		}
	}
	}
}

void CNeoPanel_Base::OnKeyTyped(wchar_t unichar)
{
	CNeoDataSettings_Base *tab = TabsList()[m_iNdsCurrent];
	if (m_eSectionActive != SECTION_MAIN || m_iNdvFocus < 0 || m_iNdvFocus >= tab->NdvListSize()) return;
	CNeoDataVariant *ndv = &tab->NdvList()[m_iNdvFocus];
	switch (ndv->type)
	{
	case CNeoDataVariant::SLIDER:
	{
		CNeoDataSlider *sl = &ndv->slider;
		if (sl->iWszCacheLabelSize < sl->iChMax)
		{
			// Prevent minus usage if minimum is a positive
			// Prevent dot usage if flMulti = 1.0f, aka integer direct
			const bool bHasMinus = (sl->iValMin >= 0) || sl->wszCacheLabel[0] == L'-';
			const bool bHasDot = (sl->flMulti == 1.0f) || wmemchr(sl->wszCacheLabel, L'.', sl->iWszCacheLabelSize) != nullptr;
			if (iswdigit(unichar) || (!bHasDot && unichar == L'.') || (!bHasMinus && unichar == L'-'))
			{
				sl->wszCacheLabel[sl->iWszCacheLabelSize++] = unichar;
				sl->wszCacheLabel[sl->iWszCacheLabelSize] = '\0';
				char szCacheLabel[CNeoDataSlider::LABEL_MAX + 1] = {};
				g_pVGuiLocalize->ConvertUnicodeToANSI(sl->wszCacheLabel, szCacheLabel, sizeof(szCacheLabel));
				if (const float flVal = atof(szCacheLabel))
				{
					const int iVal = static_cast<int>(flVal * sl->flMulti);
					if (iVal >= sl->iValMin && iVal <= sl->iValMax)
					{
						sl->iValCur = iVal;
						m_bModified = true;
					}
				}
			}
		}
	}
	break;
	case CNeoDataVariant::TEXTENTRY:
	{
		CNeoDataTextEntry *te = &ndv->textEntry;
		int iWszSize = V_wcslen(te->wszEntry);
		if (iWszSize < CNeoDataTextEntry::ENTRY_MAX && iswprint(unichar))
		{
			te->wszEntry[iWszSize++] = unichar;
			te->wszEntry[iWszSize] = '\0';
			m_bModified = true;
		}
	}
	break;
	default: break;
	}
}

// Use the y position to get the y-axis partition, then
// use the x position to get the buttons
void CNeoPanel_Base::OnCursorMoved(int x, int y)
{
	const int iPrevNdsActive = m_iNdvHover;
	m_iPosX = clamp(x, 0, GetWide());
	m_eSectionActive = SECTION_MAIN;
	m_iNdvHover = -1;
	m_curMouse = WDG_NONE;
	m_bTopArea = false;

	if (x < 0 || x >= g_iRootSubPanelWide)
	{
		return;
	}

	CNeoDataSettings_Base *tab = TabsList()[m_iNdsCurrent];
	if (y < TopAreaTall())
	{
		m_eSectionActive = SECTION_TOP;
		if (y < g_iRowTall)
		{
			const int iTabWide = g_iRootSubPanelWide / TabsListSize();
			m_iNdvHover = x / iTabWide;
			OnExitTextEditMode(iPrevNdsActive);
		}
		else
		{
			m_bTopArea = true;
			OnCursorMovedTopArea(x, y);
		}
		return;
	}
	else if (y > (GetTall() - g_iRowTall))
	{
		m_eSectionActive = SECTION_BOTTOM;
		const int iTabWide = g_iRootSubPanelWide / BottomSectionListSize();
		m_iNdvHover = x / iTabWide;
		OnExitTextEditMode(iPrevNdsActive);
		return;
	}

	y -= TopAreaTall() - m_iScrollOffset;
	m_iNdvHover = y / g_iRowTall;
	if (m_iNdvHover != iPrevNdsActive)
	{
		OnExitTextEditMode(iPrevNdsActive);
	}
	if (m_iNdvHover < 0 || m_iNdvHover >= tab->NdvListSize())
	{
		return;
	}

	const int wgXPos = static_cast<int>(g_iRootSubPanelWide * 0.4f);
	const int widgetWide = g_iRootSubPanelWide - wgXPos;
	const int widgetTall = g_iRowTall;

	CNeoDataVariant *ndv = &tab->NdvList()[m_iNdvHover];
	switch (ndv->type)
	{
	case CNeoDataVariant::RINGBOX:
	case CNeoDataVariant::SLIDER:
		if (x < wgXPos)
		{
			m_curMouse = WDG_NONE;
		}
		else if (x < (wgXPos + widgetTall))
		{
			m_curMouse = WDG_LEFT;
		}
		else if (x > (g_iRootSubPanelWide - widgetTall))
		{
			m_curMouse = WDG_RIGHT;
		}
		else
		{
			m_curMouse = WDG_CENTER;
		}

		if (ndv->type == CNeoDataVariant::SLIDER && m_curMouse == WDG_CENTER && input()->IsMouseDown(MOUSE_LEFT))
		{
			const int wdgX = x - wgXPos;
			CNeoDataSlider *sl = &ndv->slider;

			const int iRange = sl->iValMax - sl->iValMin;
			const float flSliderWidth = static_cast<float>(widgetWide - (2 * widgetTall));
			const float perc = static_cast<float>(wdgX - widgetTall) / flSliderWidth;
			sl->iValCur = static_cast<int>(perc * static_cast<float>(iRange)) + sl->iValMin;
			sl->ClampAndUpdate();
			m_bModified = true;
		}
		break;
	case CNeoDataVariant::TEXTENTRY:
	case CNeoDataVariant::BUTTON:
	case CNeoDataVariant::BINDENTRY:
	case CNeoDataVariant::S_MICTESTER:
		if (wgXPos <= x && x < g_iRootSubPanelWide)
		{
			m_curMouse = WDG_CENTER;
		}
	default:
		break;
	}
}

void CNeoPanel_Base::OnExitTextEditMode(const int iOverrideNdsActive)
{
	if (m_iNdvFocus != -1)
	{
		const int iUneditNdv = (iOverrideNdsActive == -1) ? m_iNdvHover : iOverrideNdsActive;
		CNeoDataSettings_Base *tab = TabsList()[m_iNdsCurrent];
		if (iUneditNdv >= 0 && iUneditNdv < tab->NdvListSize())
		{
			CNeoDataVariant *ndv = &tab->NdvList()[iUneditNdv];
			if (ndv->type == CNeoDataVariant::SLIDER)
			{
				ndv->slider.ClampAndUpdate();
			}
		}
	}
	m_iNdvFocus = -1;
}

int CNeoPanel_Base::TopAreaTall() const
{
	return g_iRowTall * TopAreaRows();
}

CNeoPanel_Settings::CNeoPanel_Settings(Panel *parent)
	: CNeoPanel_Base(parent)
	, m_opConfirm(new CNeoOverlay_Confirm(this))
{
	m_opConfirm->AddActionSignalTarget(this);
}

CNeoPanel_Settings::~CNeoPanel_Settings()
{
	m_opConfirm->DeletePanel();
}

void CNeoPanel_Settings::ExitSettings()
{
	IVoiceTweak_s *voiceTweak = engine->GetVoiceTweakAPI();
	if (voiceTweak->IsStillTweaking())
	{
		voiceTweak->EndVoiceTweakMode();
	}

	if (m_bModified)
	{
		// Ask user if they want to discard or save changes
		m_opConfirm->m_bChoice = false;
		m_opConfirm->m_buttonHover = CNeoOverlay_Confirm::BUTTON_NONE;
		m_opConfirm->SetVisible(true);
		m_opConfirm->SetEnabled(true);
		m_opConfirm->MoveToFront();
		m_opConfirm->RequestFocus();
	}
	else
	{
		g_pNeoRoot->m_state = CNeoRoot::STATE_ROOT;
		g_pNeoRoot->UpdateControls();
	}
}

void CNeoPanel_Settings::OnKeybindUpdate(KeyValues *data)
{
	if (data->GetBool("UpdateKey"))
	{
		const ButtonCode_t button = m_opKeyCapture->m_iButtonCode;
		const int idx = m_opKeyCapture->m_iIndex;

		if (button >= KEY_NONE && idx >= 0 && idx < m_ndsKeys.m_ndvVec.Size())
		{
			m_ndsKeys.m_ndvVec[idx].bindEntry.bcNext = button;
		}

		m_bModified = true;
	}

	m_opKeyCapture->m_iButtonCode = BUTTON_CODE_INVALID;
	m_opKeyCapture->m_iIndex = -1;
	MoveToFront();
	RequestFocus();
}

void CNeoPanel_Settings::OnConfirmDialogUpdate(KeyValues *data)
{
	(m_opConfirm->m_bChoice) ? UserSettingsSave() : UserSettingsRestore();
	g_pNeoRoot->m_state = CNeoRoot::STATE_ROOT;
	g_pNeoRoot->UpdateControls();
}

const WLabelWSize *CNeoPanel_Settings::BottomSectionList() const
{
	if (m_bModified)
	{
		static constexpr WLabelWSize BBTN_NAMES[BBTN__TOTAL] = {
			LWS(L"Back (ESC)"), LWS(L"Legacy (Shift+F3)"), LWS(L"Reset (Ctrl+Z)"), LWSNULL, LWS(L"Apply (Ctrl+S)")
		};
		return BBTN_NAMES;
	}

	static constexpr WLabelWSize BBTN_NAMES[BBTN__TOTAL] = {
		LWS(L"Back (ESC)"), LWS(L"Legacy (Shift+F3)"), LWSNULL, LWSNULL, LWSNULL
	};
	return BBTN_NAMES;
}


int CNeoPanel_Settings::KeyCodeToBottomAction(vgui::KeyCode code) const
{
	const bool bCtrlMod = (input()->IsKeyDown(KEY_LCONTROL) || input()->IsKeyDown(KEY_RCONTROL));
	if (code == KEY_ESCAPE)
	{
		return BBTN_BACK;
	}
	else if (code == KEY_F3 && (input()->IsKeyDown(KEY_LSHIFT) || input()->IsKeyDown(KEY_RSHIFT)))
	{
		return BBTN_LEGACY;
	}
	else if (code == KEY_S && bCtrlMod)
	{
		return BBTN_APPLY;
	}
	else if (code == KEY_Z && bCtrlMod)
	{
		return BBTN_RESET;
	}
	return -1;
}

void CNeoPanel_Settings::OnBottomAction(const int btn)
{
	OnExitTextEditMode();
	switch (btn)
	{
	case BBTN_BACK:
		ExitSettings();
		break;
	case BBTN_LEGACY:
		g_pNeoRoot->GetGameUI()->SendMainMenuCommand("OpenOptionsDialog");
		break;
	case BBTN_RESET:
		if (m_bModified) UserSettingsRestore();
		break;
	case BBTN_APPLY:
		if (m_bModified) UserSettingsSave();
		break;
	default:
		break;
	}
}

void CNeoPanel_Settings::OnEnterButton(CNeoDataVariant *ndv)
{
	if (ndv->type == CNeoDataVariant::BINDENTRY && m_opKeyCapture)
	{
		CNeoDataBindEntry *be = &ndv->bindEntry;
		engine->StartKeyTrapMode();
		m_opKeyCapture->m_iButtonCode = be->bcNext;
		m_opKeyCapture->m_iIndex = m_iNdvHover;
		V_swprintf_safe(m_opKeyCapture->m_wszBindingText, L"Change binding for: %ls", be->wszDisplayText);
		m_opKeyCapture->SetVisible(true);
		m_opKeyCapture->SetEnabled(true);
		m_opKeyCapture->PerformLayout();
		m_opKeyCapture->MoveToFront();
		m_opKeyCapture->RequestFocus();
	}
}

extern ConVar neo_fov;
extern ConVar neo_viewmodel_fov_offset;
extern ConVar neo_aim_hold;
extern ConVar cl_autoreload_when_empty;
extern ConVar cl_righthand;
extern ConVar cl_showpos;
extern ConVar cl_showfps;

static const wchar_t *DLFILTER_LABELS[] = {
	L"Allow all custom files from server",
	L"Do not download custom sounds",
	L"Only allow map files",
	L"Do not download any custom files",
};

static const char *DLFILTER_STRMAP[] = {
	"all", "nosounds", "mapsonly", "none"
};

static constexpr int DLFILTER_SIZE = ARRAYSIZE(DLFILTER_LABELS);

static const wchar_t *SHOWFPS_LABELS[] = {
	L"Disabled",
	L"Enabled (FPS)",
	L"Enabled (Smooth FPS)",
};

CNeoDataSettings_Multiplayer::CNeoDataSettings_Multiplayer()
	: m_ndvList{
			NDV_INIT_TEXTENTRY(L"Name"),
			NDV_INIT_RINGBOX_ONOFF(L"Show only steam name"),
			NDV_INIT_S_DISPLAYNAME(L"Display name"),
			NDV_INIT_SLIDER(L"FOV", 75, 110, 1, 1.0f, 3),
			NDV_INIT_SLIDER(L"Viewmodel FOV Offset", -20, 40, 1, 1.0f, 3),
			NDV_INIT_RINGBOX_ONOFF(L"Aim hold"),
			NDV_INIT_RINGBOX_ONOFF(L"Reload empty"),
			NDV_INIT_RINGBOX_ONOFF(L"Right hand viewmodel"),
			NDV_INIT_RINGBOX_ONOFF(L"Show player spray"),
			NDV_INIT_RINGBOX_ONOFF(L"Show position"),
			NDV_INIT_RINGBOX(L"Show FPS", SHOWFPS_LABELS, ARRAYSIZE(SHOWFPS_LABELS)),
			NDV_INIT_RINGBOX(L"Download filter", DLFILTER_LABELS, DLFILTER_SIZE),
		}
	, m_cvrClPlayerSprayDisable("cl_playerspraydisable")
	, m_cvrClDownloadFilter("cl_downloadfilter")
{
}

void CNeoDataSettings_Multiplayer::UserSettingsRestore()
{
	g_pVGuiLocalize->ConvertANSIToUnicode(neo_name.GetString(), m_ndvList[OPT_MULTI_NEONAME].textEntry.wszEntry,
										  CNeoDataTextEntry::ENTRY_MAX * sizeof(wchar_t));
	m_ndvList[OPT_MULTI_ONLYSTEAMNICK].ringBox.iCurIdx = static_cast<int>(cl_onlysteamnick.GetBool());
	m_ndvList[OPT_MULTI_FOV].slider.iValCur = neo_fov.GetInt();
	m_ndvList[OPT_MULTI_VMFOV].slider.iValCur = neo_viewmodel_fov_offset.GetInt();
	m_ndvList[OPT_MULTI_AIMHOLD].ringBox.iCurIdx = neo_aim_hold.GetBool();
	m_ndvList[OPT_MULTI_RELOADEMPTY].ringBox.iCurIdx = cl_autoreload_when_empty.GetBool();
	m_ndvList[OPT_MULTI_VMRIGHTHAND].ringBox.iCurIdx = cl_righthand.GetBool();
	m_ndvList[OPT_MULTI_PLAYERSPRAYS].ringBox.iCurIdx = !(m_cvrClPlayerSprayDisable.GetBool()); // Inverse
	m_ndvList[OPT_MULTI_SHOWPOS].ringBox.iCurIdx = cl_showpos.GetBool();
	m_ndvList[OPT_MULTI_SHOWFPS].ringBox.iCurIdx = cl_showfps.GetInt();
	{
		const char *szDlFilter = m_cvrClDownloadFilter.GetString();
		int iDlFilter = 0;
		for (int i = 0; i < DLFILTER_SIZE; ++i)
		{
			if (V_strcmp(szDlFilter, DLFILTER_STRMAP[i]) == 0)
			{
				iDlFilter = i;
				break;
			}
		}
		m_ndvList[OPT_MULTI_DLFILTER].ringBox.iCurIdx = iDlFilter;
	}

	m_ndvList[OPT_MULTI_FOV].slider.ClampAndUpdate();
	m_ndvList[OPT_MULTI_VMFOV].slider.ClampAndUpdate();
}

void CNeoDataSettings_Multiplayer::UserSettingsSave()
{
	char neoNameText[64];
	g_pVGuiLocalize->ConvertUnicodeToANSI(m_ndvList[OPT_MULTI_NEONAME].textEntry.wszEntry, neoNameText, sizeof(neoNameText));
	neo_name.SetValue(neoNameText);
	cl_onlysteamnick.SetValue(static_cast<bool>(m_ndvList[OPT_MULTI_ONLYSTEAMNICK].ringBox.iCurIdx));
	neo_fov.SetValue(m_ndvList[OPT_MULTI_FOV].slider.iValCur);
	neo_viewmodel_fov_offset.SetValue(m_ndvList[OPT_MULTI_VMFOV].slider.iValCur);
	neo_aim_hold.SetValue(static_cast<bool>(m_ndvList[OPT_MULTI_AIMHOLD].ringBox.iCurIdx));
	cl_autoreload_when_empty.SetValue(static_cast<bool>(m_ndvList[OPT_MULTI_RELOADEMPTY].ringBox.iCurIdx));
	cl_righthand.SetValue(static_cast<bool>(m_ndvList[OPT_MULTI_VMRIGHTHAND].ringBox.iCurIdx));
	m_cvrClPlayerSprayDisable.SetValue(!(static_cast<bool>(m_ndvList[OPT_MULTI_PLAYERSPRAYS].ringBox.iCurIdx))); // Inverse
	cl_showpos.SetValue(static_cast<bool>(m_ndvList[OPT_MULTI_SHOWPOS].ringBox.iCurIdx));
	cl_showfps.SetValue(m_ndvList[OPT_MULTI_SHOWFPS].ringBox.iCurIdx);
	m_cvrClDownloadFilter.SetValue(DLFILTER_STRMAP[m_ndvList[OPT_MULTI_DLFILTER].ringBox.iCurIdx]);
}

extern ConVar hud_fastswitch;

CNeoDataSettings_Keys::CNeoDataSettings_Keys()
{
	characterset_t breakSet = {};
	breakSet.set[0] = '"';

	CNeoDataVariant ndvFs = NDV_INIT_RINGBOX_ONOFF(L"Weapon fastswitch");
	CNeoDataVariant ndvConsole = NDV_INIT_RINGBOX_ONOFF(L"Developer console");
	m_ndvVec.AddToTail(ndvFs);
	m_ndvVec.AddToTail(ndvConsole);

	// TODO: Alt/secondary keybind, different way on finding keybind
	CUtlBuffer buf(0, 0, CUtlBuffer::TEXT_BUFFER | CUtlBuffer::READ_ONLY);
	if (filesystem->ReadFile("scripts/kb_act.lst", nullptr, buf))
	{
		while (buf.IsValid())
		{
			char szFirstCol[64];
			char szRawDispText[64];

			bool bIsOk = false;
			bIsOk = buf.ParseToken(&breakSet, szFirstCol, sizeof(szFirstCol));
			if (!bIsOk) break;
			bIsOk = buf.ParseToken(&breakSet, szRawDispText, sizeof(szRawDispText));
			if (!bIsOk) break;

			if (szFirstCol[0] == '\0') continue;

			wchar_t wszDispText[64];
			if (wchar_t *localizedWszStr = g_pVGuiLocalize->Find(szRawDispText))
			{
				V_wcscpy_safe(wszDispText, localizedWszStr);
			}
			else
			{
				g_pVGuiLocalize->ConvertANSIToUnicode(szRawDispText, wszDispText, sizeof(wszDispText));
			}

			const bool bIsBlank = V_strcmp(szFirstCol, "blank") == 0;
			if (bIsBlank && szRawDispText[0] != '=')
			{
				// This is category label
				CNeoDataVariant ndv = { .type = CNeoDataVariant::TEXTLABEL, };
				V_wcscpy_safe(ndv.textLabel.wszLabel, wszDispText);
				m_ndvVec.AddToTail(ndv);
			}
			else if (!bIsBlank && V_strcmp(szFirstCol, "blank") != 0)
			{
				// This is a keybind
				CNeoDataVariant ndv = { .type = CNeoDataVariant::BINDENTRY, };
				CNeoDataBindEntry *be = &ndv.bindEntry;
				V_strcpy_safe(be->szBindingCmd, szFirstCol);
				V_wcscpy_safe(be->wszDisplayText, wszDispText);
				m_ndvVec.AddToTail(ndv);
			}
		}
	}

	// After all CNeoDataVariant items are created, then this is when it's suitable
	// to assign label pointer to point to their respective display text as the memory
	// shouldn't change from here on
	for (int i = OPT_KEYS_FIRST_NONFIXED; i < m_ndvVec.Size(); ++i)
	{
		CNeoDataVariant &ndv = m_ndvVec[i];
		if (ndv.type == CNeoDataVariant::BINDENTRY)
		{
			ndv.label = ndv.bindEntry.wszDisplayText;
			ndv.labelSize = V_wcslen(ndv.bindEntry.wszDisplayText);
		}
		else if (ndv.type == CNeoDataVariant::TEXTLABEL)
		{
			ndv.label = ndv.textLabel.wszLabel;
			ndv.labelSize = V_wcslen(ndv.label);
		}
	}
}

void CNeoDataSettings_Keys::UserSettingsRestore()
{
	m_ndvVec[OPT_KEYS_FIXED_WEPFASTSWITCH].ringBox.iCurIdx = hud_fastswitch.GetBool();
	m_ndvVec[OPT_KEYS_FIXED_DEVCONSOLE].ringBox.iCurIdx = (gameuifuncs->GetButtonCodeForBind("toggleconsole") > KEY_NONE);
	for (int i = OPT_KEYS_FIRST_NONFIXED; i < m_ndvVec.Size(); ++i)
	{
		CNeoDataVariant &ndv = m_ndvVec[i];
		if (ndv.type == CNeoDataVariant::BINDENTRY)
		{
			CNeoDataBindEntry *be = &ndv.bindEntry;
			be->bcCurrent = gameuifuncs->GetButtonCodeForBind(be->szBindingCmd);
			be->bcNext = be->bcCurrent;
		}
	}
}

void CNeoDataSettings_Keys::UserSettingsSave()
{
	hud_fastswitch.SetValue(m_ndvVec[OPT_KEYS_FIXED_WEPFASTSWITCH].ringBox.iCurIdx);
	{
		char cmdStr[128];
		V_sprintf_safe(cmdStr, "unbind \"`\"\n");
		engine->ClientCmd_Unrestricted(cmdStr);

		const bool bEnableConsole = m_ndvVec[OPT_KEYS_FIXED_DEVCONSOLE].ringBox.iCurIdx;
		if (bEnableConsole)
		{
			V_sprintf_safe(cmdStr, "bind \"`\" \"toggleconsole\"\n");
			engine->ClientCmd_Unrestricted(cmdStr);
		}
	}
	for (int i = OPT_KEYS_FIRST_NONFIXED; i < m_ndvVec.Size(); ++i)
	{
		CNeoDataVariant &ndv = m_ndvVec[i];
		if (ndv.type == CNeoDataVariant::BINDENTRY && ndv.bindEntry.bcCurrent > KEY_NONE)
		{
			char cmdStr[128];
			const char *bindBtnName = g_pInputSystem->ButtonCodeToString(ndv.bindEntry.bcCurrent);
			V_sprintf_safe(cmdStr, "unbind \"%s\"\n", bindBtnName);
			engine->ClientCmd_Unrestricted(cmdStr);
		}
	}
	for (int i = OPT_KEYS_FIRST_NONFIXED; i < m_ndvVec.Size(); ++i)
	{
		CNeoDataVariant &ndv = m_ndvVec[i];
		if (ndv.type == CNeoDataVariant::BINDENTRY && ndv.bindEntry.bcNext > KEY_NONE)
		{
			char cmdStr[128];
			const char *bindBtnName = g_pInputSystem->ButtonCodeToString(ndv.bindEntry.bcNext);
			V_sprintf_safe(cmdStr, "bind \"%s\" \"%s\"\n", bindBtnName, ndv.bindEntry.szBindingCmd);
			engine->ClientCmd_Unrestricted(cmdStr);
		}
	}
}

CNeoDataSettings_Mouse::CNeoDataSettings_Mouse()
	: m_ndvList{
			NDV_INIT_SLIDER(L"Sensitivity", 10, 1000, 10, 100.0f, 4),
			NDV_INIT_RINGBOX_ONOFF(L"Raw input"),
			NDV_INIT_RINGBOX_ONOFF(L"Mouse Filter"),
			NDV_INIT_RINGBOX_ONOFF(L"Mouse Reverse"),
			NDV_INIT_RINGBOX_ONOFF(L"Custom Acceleration"),
			NDV_INIT_SLIDER(L"Exponent", 100, 140, 1, 100.0f, 4),
		}
	, m_cvrMFilter("m_filter")
	, m_cvrPitch("m_pitch")
	, m_cvrCustomAccel("m_customaccel")
	, m_cvrCustomAccelExponent("m_customaccel_exponent")
	, m_cvrMRawInput("m_rawinput")
{
}

void CNeoDataSettings_Mouse::UserSettingsRestore()
{
	m_ndvList[OPT_MOUSE_SENSITIVITY].slider.SetValue(sensitivity.GetFloat());
	m_ndvList[OPT_MOUSE_RAWINPUT].ringBox.iCurIdx = static_cast<int>(m_cvrMRawInput.GetBool());
	m_ndvList[OPT_MOUSE_FILTER].ringBox.iCurIdx = static_cast<int>(m_cvrMFilter.GetBool());
	m_ndvList[OPT_MOUSE_REVERSE].ringBox.iCurIdx = static_cast<int>(m_cvrPitch.GetFloat() < 0.0f);
	m_ndvList[OPT_MOUSE_CUSTOMACCEL].ringBox.iCurIdx = static_cast<int>(m_cvrCustomAccel.GetInt() == 3);
	m_ndvList[OPT_MOUSE_EXPONENT].slider.SetValue(m_cvrCustomAccelExponent.GetFloat());
}

void CNeoDataSettings_Mouse::UserSettingsSave()
{
	sensitivity.SetValue(m_ndvList[OPT_MOUSE_SENSITIVITY].slider.GetValue());
	m_cvrMRawInput.SetValue(static_cast<bool>(m_ndvList[OPT_MOUSE_RAWINPUT].ringBox.iCurIdx));
	m_cvrMFilter.SetValue(static_cast<bool>(m_ndvList[OPT_MOUSE_FILTER].ringBox.iCurIdx));
	{
		const float absPitch = abs(m_cvrPitch.GetFloat());
		m_cvrPitch.SetValue(m_ndvList[OPT_MOUSE_REVERSE].ringBox.iCurIdx ? -absPitch : absPitch);
	}
	m_cvrCustomAccel.SetValue(m_ndvList[OPT_MOUSE_CUSTOMACCEL].ringBox.iCurIdx ? 3 : 0);
	m_cvrCustomAccelExponent.SetValue(m_ndvList[OPT_MOUSE_EXPONENT].slider.GetValue());
}

static constexpr float SLT_VOL = 100.0f;
static const wchar_t *SPEAKER_CFG_LABELS[] = {
	L"Auto",
	L"Headphones",
	L"2 Speakers (Stereo)",
#ifndef LINUX
	L"4 Speakers (Quadraphonic)",
	L"5.1 Surround",
	L"7.1 Surround",
#endif
};

CNeoDataSettings_Audio::CNeoDataSettings_Audio()
	: m_ndvList{
			NDV_INIT_SLIDER(L"Main Volume", 0, 100, 5, SLT_VOL, 4),
			NDV_INIT_SLIDER(L"Music Volume", 0, 100, 5, SLT_VOL, 4),
			NDV_INIT_SLIDER(L"Victory Volume", 0, 100, 5, SLT_VOL, 4),
			NDV_INIT_RINGBOX(L"Sound Setup", SPEAKER_CFG_LABELS, ARRAYSIZE(SPEAKER_CFG_LABELS)),
			NDV_INIT_RINGBOX(L"Sound Quality", QUALITY_LABELS, 3),
			NDV_INIT_RINGBOX_ONOFF(L"Mute Audio on un-focus"),
			NDV_INIT_RINGBOX_ONOFF(L"Voice Enabled"),
			NDV_INIT_SLIDER(L"Voice Receive", 0, 100, 5, SLT_VOL, 4),
			//NDV_INIT_SLIDER(L"Voice Send", 0, 100, 1, SLT_VOL, 4),
			NDV_INIT_RINGBOX_ONOFF(L"Microphone Boost"),
			NDV_INIT_S_MICTESTER(L"Microphone Tester"),
		}
	, m_cvrVolume("volume")
	, m_cvrSndMusicVolume("snd_musicvolume")
	, m_cvrSndSurroundSpeakers("snd_surround_speakers")
	, m_cvrVoiceEnabled("voice_enable")
	, m_cvrVoiceScale("voice_scale")
	, m_cvrSndMuteLoseFocus("snd_mute_losefocus")
	, m_cvrSndPitchquality("snd_pitchquality")
	, m_cvrDspSlowCpu("dsp_slow_cpu")
{
}

void CNeoDataSettings_Audio::UserSettingsRestore()
{
	IVoiceTweak_s *voiceTweak = engine->GetVoiceTweakAPI();

	// Output
	m_ndvList[OPT_AUDIO_INP_VOLMAIN].slider.SetValue(m_cvrVolume.GetFloat());
	m_ndvList[OPT_AUDIO_INP_VOLMUSIC].slider.SetValue(m_cvrSndMusicVolume.GetFloat());
	m_ndvList[OPT_AUDIO_INP_VOLVICTORY].slider.SetValue(snd_victory_volume.GetFloat());
	int surroundRBIdx = 0;
	switch (m_cvrSndSurroundSpeakers.GetInt())
	{
	break; case 0: surroundRBIdx = 1;
	break; case 2: surroundRBIdx = 2;
#ifndef LINUX
	break; case 4: surroundRBIdx = 3;
	break; case 5: surroundRBIdx = 4;
	break; case 7: surroundRBIdx = 5;
#endif
	break; default: break;
	}
	m_ndvList[OPT_AUDIO_INP_SURROUND].ringBox.iCurIdx = surroundRBIdx;
	m_ndvList[OPT_AUDIO_INP_MUTELOSEFOCUS].ringBox.iCurIdx = static_cast<int>(m_cvrSndMuteLoseFocus.GetBool());

	// Sound quality:
	// High: snd_pitchquality 1 | dsp_slow_cpu 0
	// Med: snd_pitchquality 0 | dsp_slow_cpu 0
	// Low: snd_pitchquality 0 | dsp_slow_cpu 1
	const int iSoundQuality = (m_cvrSndPitchquality.GetBool()) ? QUALITY_HIGH :
									(m_cvrDspSlowCpu.GetBool()) ? QUALITY_LOW :
																  QUALITY_MEDIUM;
	m_ndvList[OPT_AUDIO_INP_QUALITY].ringBox.iCurIdx = iSoundQuality;

	// TODO: Voice receive, voice_loopback quick test toggle
	// Input
	m_ndvList[OPT_AUDIO_OUT_VOICEENABLED].ringBox.iCurIdx = static_cast<int>(m_cvrVoiceEnabled.GetBool());
	m_ndvList[OPT_AUDIO_OUT_VOICERECV].slider.SetValue(m_cvrVoiceScale.GetFloat());
	const bool bMicBoost = (voiceTweak->GetControlFloat(MicBoost) != 0.0f);
	m_ndvList[OPT_AUDIO_OUT_MICBOOST].ringBox.iCurIdx = static_cast<int>(bMicBoost);
	//m_ndvList[OPT_AUDIO_OUT_VOICESEND].slider.SetValue(voiceTweak->GetControlFloat(MicrophoneVolume));
}

void CNeoDataSettings_Audio::UserSettingsSave()
{
	static constexpr int SURROUND_RE_MAP[] = {
		-1, 0, 2,
#ifndef LINUX
		4, 5, 7
#endif
	};

	IVoiceTweak_s *voiceTweak = engine->GetVoiceTweakAPI();

	// Output
	m_cvrVolume.SetValue(m_ndvList[OPT_AUDIO_INP_VOLMAIN].slider.GetValue());
	m_cvrSndMusicVolume.SetValue(m_ndvList[OPT_AUDIO_INP_VOLMUSIC].slider.GetValue());
	snd_victory_volume.SetValue(m_ndvList[OPT_AUDIO_INP_VOLVICTORY].slider.GetValue());
	m_cvrSndSurroundSpeakers.SetValue(SURROUND_RE_MAP[m_ndvList[OPT_AUDIO_INP_SURROUND].ringBox.iCurIdx]);
	m_cvrSndMuteLoseFocus.SetValue(static_cast<bool>(m_ndvList[OPT_AUDIO_INP_MUTELOSEFOCUS].ringBox.iCurIdx));
	const int iQuality = m_ndvList[OPT_AUDIO_INP_QUALITY].ringBox.iCurIdx;
	m_cvrSndPitchquality.SetValue(iQuality == QUALITY_HIGH);
	m_cvrDspSlowCpu.SetValue(iQuality == QUALITY_LOW);

	// Input
	m_cvrVoiceEnabled.SetValue(static_cast<bool>(m_ndvList[OPT_AUDIO_OUT_VOICEENABLED].ringBox.iCurIdx));
	m_cvrVoiceScale.SetValue(m_ndvList[OPT_AUDIO_OUT_VOICERECV].slider.GetValue());
	voiceTweak->SetControlFloat(MicBoost, static_cast<float>(m_ndvList[OPT_AUDIO_OUT_MICBOOST].ringBox.iCurIdx));
	//voiceTweak->SetControlFloat(MicrophoneVolume, m_ndvList[OPT_AUDIO_OUT_VOICESEND].slider.GetValue());
}

enum MultiThreadIdx
{
	THREAD_SINGLE = 0,
	THREAD_MULTI,
};

enum FilteringEnum
{
	FILTERING_BILINEAR = 0,
	FILTERING_TRILINEAR,
	FILTERING_ANISO2X,
	FILTERING_ANISO4X,
	FILTERING_ANISO8X,
	FILTERING_ANISO16X,

	FILTERING__TOTAL,
};

static const wchar_t *WINDOW_MODE[] = {
	L"Fullscreen",
	L"Windowed",
};

static const wchar_t *QUEUE_MODE[] = {
	L"Single",
	L"Multi",
};

static const wchar_t *QUALITY2_LABELS[] = {
	L"Low",
	L"High",
};

static const wchar_t *WATER_LABELS[] = {
	L"Simple Reflect",
	L"Reflect World",
	L"Reflect All",
};

static const wchar_t *MSAA_LABELS[] = {
	L"None",
	L"2x MSAA",
	L"4x MSAA",
	L"6x MSAA",
	L"8x MSAA",
};

static const wchar_t *FILTERING_LABELS[FILTERING__TOTAL] = {
	L"Bilinear",
	L"Trilinear",
	L"Anisotropic 2X",
	L"Anisotropic 4X",
	L"Anisotropic 8X",
	L"Anisotropic 16X",
};

static const wchar_t *HDR_LABELS[] = {
	L"None",
	L"Bloom",
	L"Full",
};

CNeoDataSettings_Video::CNeoDataSettings_Video()
	: m_ndvList{
			NDV_INIT_RINGBOX(L"Resolution", nullptr, 0),
			NDV_INIT_RINGBOX(L"Window", WINDOW_MODE, ARRAYSIZE(WINDOW_MODE)),
			NDV_INIT_RINGBOX(L"Core Rendering", QUEUE_MODE, ARRAYSIZE(QUEUE_MODE)),
			NDV_INIT_RINGBOX(L"Model detail", QUALITY_LABELS, 3),
			NDV_INIT_RINGBOX(L"Texture detail", QUALITY_LABELS, 4),
			NDV_INIT_RINGBOX(L"Shader detail", QUALITY2_LABELS, 2),
			NDV_INIT_RINGBOX(L"Water detail", WATER_LABELS, ARRAYSIZE(WATER_LABELS)),
			NDV_INIT_RINGBOX(L"Shadow detail", QUALITY_LABELS, 3),
			NDV_INIT_RINGBOX(L"Color correction", ENABLED_LABELS, 2),
			NDV_INIT_RINGBOX(L"Anti-aliasing", MSAA_LABELS, ARRAYSIZE(MSAA_LABELS)),
			NDV_INIT_RINGBOX(L"Filtering mode", FILTERING_LABELS, FILTERING__TOTAL),
			NDV_INIT_RINGBOX(L"V-Sync", ENABLED_LABELS, 2),
			NDV_INIT_RINGBOX(L"Motion blur", ENABLED_LABELS, 2),
			NDV_INIT_RINGBOX(L"HDR", HDR_LABELS, ARRAYSIZE(HDR_LABELS)),
			NDV_INIT_SLIDER(L"Gamma", 160, 260, 5, 100.0f, 4),
		}
	, m_cvrMatQueueMode("mat_queue_mode")
	, m_cvrRRootLod("r_rootlod")
	, m_cvrMatPicmip("mat_picmip")
	, m_cvrMatReducefillrate("mat_reducefillrate")
	, m_cvrRWaterForceExpensive("r_waterforceexpensive")
	, m_cvrRWaterForceReflectEntities("r_waterforcereflectentities")
	, m_cvrRFlashlightdepthtexture("r_flashlightdepthtexture")
	, m_cvrRShadowrendertotexture("r_shadowrendertotexture")
	, m_cvrMatColorcorrection("mat_colorcorrection")
	, m_cvrMatAntialias("mat_antialias")
	, m_cvrMatTrilinear("mat_trilinear")
	, m_cvrMatForceaniso("mat_forceaniso")
	, m_cvrMatVsync("mat_vsync")
	, m_cvrMatMotionBlurEnabled("mat_motion_blur_enabled")
	, m_cvrMatHdrLevel("mat_hdr_level")
	, m_cvrMatMonitorGamma("mat_monitorgamma")
{
	gameuifuncs->GetVideoModes(&m_vmList, &m_vmListSize);
	// NEO JANK (nullsystem): This is utter crap but it works out, one long array for storing the data, the other
	// just to point to those.
	//
	// Update m_reResolution display list
	static constexpr int DISP_SIZE = 64;
	m_wszVmDispMem = new wchar_t[m_vmListSize * DISP_SIZE];
	V_memset(m_wszVmDispMem, 0, m_vmListSize * DISP_SIZE);
	m_ndvList[OPT_VIDEO_RESOLUTION].ringBox.iCurIdx = m_vmListSize - 1;
	for (int i = 0, offset = 0; i < m_vmListSize; ++i, offset += DISP_SIZE)
	{
		vmode_t *vm = &m_vmList[i];
		swprintf(m_wszVmDispMem + offset, DISP_SIZE - 1, L"%d x %d", vm->width, vm->height);
	}
	m_wszVmDispList = new wchar_t *[m_vmListSize];
	for (int i = 0, offset = 0; i < m_vmListSize; ++i, offset += DISP_SIZE)
	{
		m_wszVmDispList[i] = m_wszVmDispMem + offset;
	}
	m_ndvList[OPT_VIDEO_RESOLUTION].ringBox.wszItems = const_cast<const wchar_t **>(m_wszVmDispList);
	m_ndvList[OPT_VIDEO_RESOLUTION].ringBox.iItemsSize = m_vmListSize;
}

CNeoDataSettings_Video::~CNeoDataSettings_Video()
{
	delete m_wszVmDispMem;
	delete m_wszVmDispList;
}

void CNeoDataSettings_Video::UserSettingsRestore()
{
	const int preVmListSize = m_vmListSize;
	gameuifuncs->GetDesktopResolution(m_iNativeWidth, m_iNativeHeight);
	int iScreenWidth, iScreenHeight;
	engine->GetScreenSize(iScreenWidth, iScreenHeight); // Or: g_pMaterialSystem->GetDisplayMode?
	m_ndvList[OPT_VIDEO_RESOLUTION].ringBox.iCurIdx = m_vmListSize - 1;
	for (int i = 0; i < m_vmListSize; ++i)
	{
		vmode_t *vm = &m_vmList[i];
		if (vm->width == iScreenWidth && vm->height == iScreenHeight)
		{
			m_ndvList[OPT_VIDEO_RESOLUTION].ringBox.iCurIdx = i;
			break;
		}
	}

	m_ndvList[OPT_VIDEO_WINDOWED].ringBox.iCurIdx = static_cast<int>(g_pMaterialSystem->GetCurrentConfigForVideoCard().Windowed());
	const int queueMode = m_cvrMatQueueMode.GetInt();
	m_ndvList[OPT_VIDEO_QUEUEMODE].ringBox.iCurIdx = (queueMode == -1 || queueMode == 2) ? THREAD_MULTI : THREAD_SINGLE;
	m_ndvList[OPT_VIDEO_ROOTLOD].ringBox.iCurIdx = 2 - m_cvrRRootLod.GetInt(); // Inverse, highest = 0, lowest = 2
	m_ndvList[OPT_VIDEO_MATPICMAP].ringBox.iCurIdx = 3 - (m_cvrMatPicmip.GetInt() + 1); // Inverse+1, highest = -1, lowest = 2
	m_ndvList[OPT_VIDEO_MATREDUCEFILLRATE].ringBox.iCurIdx = 1 - m_cvrMatReducefillrate.GetInt(); // Inverse, 1 = low, 0 = high
	// Water detail
	//                r_waterforceexpensive        r_waterforcereflectentities
	// Simple:                  0                              0
	// Reflect World:           1                              0
	// Reflect all:             1                              1
	const int waterDetail = (m_cvrRWaterForceReflectEntities.GetBool()) ? QUALITY_HIGH :
									(m_cvrRWaterForceExpensive.GetBool()) ? QUALITY_MEDIUM :
																			QUALITY_LOW;
	m_ndvList[OPT_VIDEO_WATERDETAIL].ringBox.iCurIdx = waterDetail;

	// Shadow detail
	//         r_flashlightdepthtexture     r_shadowrendertotexture
	// Low:              0                            0
	// Medium:           0                            1
	// High:             1                            1
	const int shadowDetail = (m_cvrRFlashlightdepthtexture.GetBool()) ? QUALITY_HIGH :
									(m_cvrRShadowrendertotexture.GetBool()) ? QUALITY_MEDIUM :
																			  QUALITY_LOW;
	m_ndvList[OPT_VIDEO_SHADOW].ringBox.iCurIdx = shadowDetail;

	m_ndvList[OPT_VIDEO_COLORCORRECTION].ringBox.iCurIdx = static_cast<int>(m_cvrMatColorcorrection.GetBool());
	m_ndvList[OPT_VIDEO_ANTIALIAS].ringBox.iCurIdx = (m_cvrMatAntialias.GetInt() / 2); // MSAA: Times by 2

	// Filtering mode
	// mat_trilinear: 0 = bilinear, 1 = trilinear (both: mat_forceaniso 1)
	// mat_forceaniso: Antisotropic 2x/4x/8x/16x (all aniso: mat_trilinear 0)
	int filterIdx = 0;
	if (m_cvrMatForceaniso.GetInt() < 2)
	{
		filterIdx = m_cvrMatTrilinear.GetBool() ? FILTERING_TRILINEAR : FILTERING_BILINEAR;
	}
	else
	{
		switch (m_cvrMatForceaniso.GetInt())
		{
		break; case 2: filterIdx = FILTERING_ANISO2X;
		break; case 4: filterIdx = FILTERING_ANISO4X;
		break; case 8: filterIdx = FILTERING_ANISO8X;
		break; case 16: filterIdx = FILTERING_ANISO16X;
		break; default: filterIdx = FILTERING_ANISO4X; // Some invalid number, just set to 4X (idx 3)
		}
	}
	m_ndvList[OPT_VIDEO_FILTERING].ringBox.iCurIdx = filterIdx;
	m_ndvList[OPT_VIDEO_VSYNC].ringBox.iCurIdx = static_cast<int>(m_cvrMatVsync.GetBool());
	m_ndvList[OPT_VIDEO_MOTIONBLUR].ringBox.iCurIdx = static_cast<int>(m_cvrMatMotionBlurEnabled.GetBool());
	m_ndvList[OPT_VIDEO_HDR].ringBox.iCurIdx = m_cvrMatHdrLevel.GetInt();
	m_ndvList[OPT_VIDEO_GAMMA].slider.SetValue(m_cvrMatMonitorGamma.GetFloat());
}

void CNeoDataSettings_Video::UserSettingsSave()
{
	const int resIdx = m_ndvList[OPT_VIDEO_RESOLUTION].ringBox.iCurIdx;
	if (resIdx >= 0 && resIdx < m_vmListSize)
	{
		// mat_setvideomode [width] [height] [mode] | mode: 0 = fullscreen, 1 = windowed
		vmode_t *vm = &m_vmList[resIdx];
		char cmdStr[128];
		V_sprintf_safe(cmdStr, "mat_setvideomode %d %d %d",
					   vm->width, vm->height, m_ndvList[OPT_VIDEO_WINDOWED].ringBox.iCurIdx);
		engine->ClientCmd_Unrestricted(cmdStr);
	}
	m_cvrMatQueueMode.SetValue((m_ndvList[OPT_VIDEO_QUEUEMODE].ringBox.iCurIdx == THREAD_MULTI) ? 2 : 0);
	m_cvrRRootLod.SetValue(2 - m_ndvList[OPT_VIDEO_ROOTLOD].ringBox.iCurIdx);
	m_cvrMatPicmip.SetValue(2 - m_ndvList[OPT_VIDEO_MATPICMAP].ringBox.iCurIdx);
	m_cvrMatReducefillrate.SetValue(1 - m_ndvList[OPT_VIDEO_MATREDUCEFILLRATE].ringBox.iCurIdx);
	m_cvrRWaterForceExpensive.SetValue(m_ndvList[OPT_VIDEO_WATERDETAIL].ringBox.iCurIdx >= QUALITY_MEDIUM);
	m_cvrRWaterForceReflectEntities.SetValue(m_ndvList[OPT_VIDEO_WATERDETAIL].ringBox.iCurIdx == QUALITY_HIGH);
	m_cvrRShadowrendertotexture.SetValue(m_ndvList[OPT_VIDEO_SHADOW].ringBox.iCurIdx >= QUALITY_MEDIUM);
	m_cvrRFlashlightdepthtexture.SetValue(m_ndvList[OPT_VIDEO_SHADOW].ringBox.iCurIdx == QUALITY_HIGH);
	m_cvrMatColorcorrection.SetValue(static_cast<bool>(m_ndvList[OPT_VIDEO_COLORCORRECTION].ringBox.iCurIdx));
	m_cvrMatAntialias.SetValue(m_ndvList[OPT_VIDEO_ANTIALIAS].ringBox.iCurIdx * 2);
	const int filterIdx = m_ndvList[OPT_VIDEO_FILTERING].ringBox.iCurIdx;
	m_cvrMatTrilinear.SetValue(filterIdx == FILTERING_TRILINEAR);
	static constexpr int ANISO_MAP[FILTERING__TOTAL] = {
		1, 1, 2, 4, 8, 16
	};
	m_cvrMatForceaniso.SetValue(ANISO_MAP[filterIdx]);
	m_cvrMatVsync.SetValue(static_cast<bool>(m_ndvList[OPT_VIDEO_VSYNC].ringBox.iCurIdx));
	m_cvrMatMotionBlurEnabled.SetValue(static_cast<bool>(m_ndvList[OPT_VIDEO_MOTIONBLUR].ringBox.iCurIdx));
	m_cvrMatHdrLevel.SetValue(m_ndvList[OPT_VIDEO_HDR].ringBox.iCurIdx);
	m_cvrMatMonitorGamma.SetValue(m_ndvList[OPT_VIDEO_GAMMA].slider.GetValue());
}

// Map list
CNeoTab_MapList::CNeoTab_MapList()
{
	FileFindHandle_t findHdl;
	for (const char *pszFilename = filesystem->FindFirst("maps/*.bsp", &findHdl);
		 pszFilename;
		 pszFilename = filesystem->FindNext(findHdl))
	{
		// Sanity check: In-case somehow someone named a directory as *.bsp in here
		if (!filesystem->FindIsDirectory(findHdl))
		{
			CNeoDataVariant ndv = { .type = CNeoDataVariant::BUTTON, .labelSize = 0, };
			ndv.button.iWszBtnLabelSize = g_pVGuiLocalize->ConvertANSIToUnicode(
						pszFilename, ndv.button.wszBtnLabel, sizeof(ndv.button.wszBtnLabel));
			ndv.button.iWszBtnLabelSize -= sizeof(".bsp");
			ndv.button.wszBtnLabel[ndv.button.iWszBtnLabelSize] = '\0';
			m_ndvVec.AddToTail(ndv);
		}
	}
	filesystem->FindClose(findHdl);
}

CNeoOverlay_MapList::CNeoOverlay_MapList(vgui::Panel *parent)
	: CNeoPanel_Base(parent)
{
}

void CNeoOverlay_MapList::OnEnterButton(CNeoDataVariant *ndv)
{
	if (ndv->type != CNeoDataVariant::BUTTON) return;
	PostActionSignal(new KeyValues("MapListPicked", "Map", ndv->button.wszBtnLabel));
	SetVisible(false);
	SetEnabled(false);
}

void CNeoOverlay_MapList::OnBottomAction(const int btn)
{
	switch (btn)
	{
	case BBTN_BACK:
		SetVisible(false);
		SetEnabled(false);
		break;
	case BBTN_SELECT:
		// TODO: hover/active split
		break;
	}
}

const WLabelWSize *CNeoOverlay_MapList::BottomSectionList() const
{
	static constexpr WLabelWSize BBTN_NAMES[BBTN__TOTAL] = {
		LWS(L"Back (ESC)"), LWSNULL, LWSNULL, LWSNULL, LWS(L"Select"),
	};
	return BBTN_NAMES;
}

int CNeoOverlay_MapList::KeyCodeToBottomAction(vgui::KeyCode code) const
{
	return (code == KEY_ESCAPE) ? BBTN_BACK : -1;
}

///////////////
// PANEL: NEW GAME
///////////////
CNeoDataNewGame_General::CNeoDataNewGame_General()
	: m_ndvList{
		NDV_INIT_BUTTON(L"Map"),
		NDV_INIT_TEXTENTRY(L"Hostname"),
		NDV_INIT_SLIDER(L"Max players", 1, (MAX_PLAYERS - 1), 1, 1.0f, 2),
		NDV_INIT_TEXTENTRY(L"Password"),
		NDV_INIT_RINGBOX_ONOFF(L"Friendly fire"),
	}
{
	m_ndvList[OPT_NEW_MAPLIST].button.iWszBtnLabelSize = V_swprintf_safe(
				m_ndvList[OPT_NEW_MAPLIST].button.wszBtnLabel, L"nt_oilstain_ctg");
	V_swprintf_safe(m_ndvList[OPT_NEW_HOSTNAME].textEntry.wszEntry, L"NeoTokyo Rebuild");
	m_ndvList[OPT_NEW_MAXPLAYERS].slider.iValCur = 24;
	m_ndvList[OPT_NEW_MAXPLAYERS].slider.ClampAndUpdate();
	V_swprintf_safe(m_ndvList[OPT_NEW_SVPASSWORD].textEntry.wszEntry, L"neo");
	m_ndvList[OPT_NEW_FRIENDLYFIRE].ringBox.iCurIdx = 1;
}

CNeoPanel_NewGame::CNeoPanel_NewGame(vgui::Panel *parent)
	: CNeoPanel_Base(parent)
	, m_mapList(new CNeoOverlay_MapList(this))
{
	m_mapList->AddActionSignalTarget(this);
	m_mapList->SetVisible(false);
	m_mapList->SetEnabled(false);
}

CNeoPanel_NewGame::~CNeoPanel_NewGame()
{
	m_mapList->DeletePanel();
}

void CNeoPanel_NewGame::OnEnterButton(CNeoDataVariant *ndv)
{
	if (ndv->type == CNeoDataVariant::BUTTON)
	{
		int wide, tall;
		surface()->GetScreenSize(wide, tall);
		m_mapList->m_iNdvHover = -1;
		m_mapList->m_curMouse = CNeoPanel_Base::WDG_NONE;
		m_mapList->PerformLayout();
		m_mapList->SetPos((wide / 2) - (g_iRootSubPanelWide / 2),
								(tall / 2) - (m_mapList->GetTall() / 2));
		m_mapList->SetVisible(true);
		m_mapList->SetEnabled(true);
		m_mapList->MoveToFront();
		m_mapList->RequestFocus();
	}
}

void CNeoPanel_NewGame::OnMapListPicked(KeyValues *data)
{
	if (const wchar_t *pWszMapFilename = data->GetWString("Map"))
	{
		CNeoDataButton *btn = &m_ndsGeneral.m_ndvList[CNeoDataNewGame_General::OPT_NEW_MAPLIST].button;
		V_wcscpy_safe(btn->wszBtnLabel, pWszMapFilename);
		btn->iWszBtnLabelSize = V_wcslen(btn->wszBtnLabel);
	}

	MoveToFront();
	RequestFocus();
}

void CNeoPanel_NewGame::OnBottomAction(const int btn)
{
	switch (btn)
	{
	case BBTN_GO:
	{
		if (engine->IsInGame())
		{
			engine->ClientCmd_Unrestricted("disconnect");
		}

		using General = CNeoDataNewGame_General;
		static constexpr int ENTRY_MAX = CNeoDataTextEntry::ENTRY_MAX;

		char szMap[ENTRY_MAX + 1] = {};
		char szHostname[ENTRY_MAX + 1] = {};
		char szPassword[ENTRY_MAX + 1] = {};
		g_pVGuiLocalize->ConvertUnicodeToANSI(m_ndsGeneral.m_ndvList[General::OPT_NEW_MAPLIST].button.wszBtnLabel, szMap, sizeof(szMap));
		g_pVGuiLocalize->ConvertUnicodeToANSI(m_ndsGeneral.m_ndvList[General::OPT_NEW_HOSTNAME].textEntry.wszEntry, szHostname, sizeof(szHostname));
		g_pVGuiLocalize->ConvertUnicodeToANSI(m_ndsGeneral.m_ndvList[General::OPT_NEW_SVPASSWORD].textEntry.wszEntry, szPassword, sizeof(szPassword));
		const int iPlayers = m_ndsGeneral.m_ndvList[General::OPT_NEW_MAXPLAYERS].slider.iValCur;
		const bool bFriendlyFire = static_cast<bool>(m_ndsGeneral.m_ndvList[General::OPT_NEW_FRIENDLYFIRE].ringBox.iCurIdx);

		ConVarRef cvrHostname("hostname");
		ConVarRef cvrSvPassowrd("sv_password");
		ConVarRef cvrMpFriendlyFire("mp_friendlyfire");

		cvrHostname.SetValue(szHostname);
		cvrSvPassowrd.SetValue(szPassword);
		cvrMpFriendlyFire.SetValue(bFriendlyFire);

		char cmdStr[256];
		V_sprintf_safe(cmdStr, "maxplayers %d; progress_enable; map \"%s\"", iPlayers, szMap);
		engine->ClientCmd_Unrestricted(cmdStr);
	}
		[[fallthrough]]; // Also reset the main menu back to root state
	case BBTN_BACK:
		g_pNeoRoot->m_state = CNeoRoot::STATE_ROOT;
		g_pNeoRoot->UpdateControls();
		break;
	}
}

const WLabelWSize *CNeoPanel_NewGame::BottomSectionList() const
{
	static constexpr WLabelWSize BBTN_NAMES[BBTN__TOTAL] = {
		LWS(L"Back (ESC)"), LWSNULL, LWSNULL, LWSNULL, LWS(L"Start (F1)"),
	};
	return BBTN_NAMES;
}

int CNeoPanel_NewGame::KeyCodeToBottomAction(vgui::KeyCode code) const
{
	switch (code)
	{
	case KEY_ESCAPE: return BBTN_BACK;
	default: break;
	}
	return -1;
}

/////////////////
// SERVER BROWSER
/////////////////

enum AnitCheatMode
{
	ANTICHEAT_ANY,
	ANTICHEAT_ON,
	ANTICHEAT_OFF,

	ANTICHEAT__TOTAL,
};

static const wchar_t *ANTICHEAT_LABELS[ANTICHEAT__TOTAL] = {
	L"<Any>", L"On", L"Off"
};

static constexpr WLabelWSize GS_NAMES[GS__TOTAL] = {
	LWS(L"Internet"), LWS(L"LAN"), LWS(L"Friends"), LWS(L"Fav"), LWS(L"History"), LWS(L"Spec")
};

CNeoDataServerBrowser_General::CNeoDataServerBrowser_General()
{
}

void CNeoDataServerBrowser_General::UpdateFilteredList()
{
	m_ndvFilteredVec.RemoveAll();

	const auto ndvFilters = m_filters->m_ndvList;
	for (const CNeoDataVariant &ndv : m_ndvVec)
	{
		using SBFilter = CNeoDataServerBrowser_Filters;
		if (    (ndvFilters[SBFilter::OPT_FILTER_NOTFULL].ringBox.iCurIdx &&
					(ndv.gameServer.info.m_nPlayers == ndv.gameServer.info.m_nMaxPlayers))
				|| (ndvFilters[SBFilter::OPT_FILTER_HASPLAYERS].ringBox.iCurIdx &&
					(ndv.gameServer.info.m_nPlayers == 0))
				|| (ndvFilters[SBFilter::OPT_FILTER_NOTLOCKED].ringBox.iCurIdx &&
					(ndv.gameServer.info.m_bPassword))
				|| (ndvFilters[SBFilter::OPT_FILTER_VACMODE].ringBox.iCurIdx == ANTICHEAT_OFF &&
					(ndv.gameServer.info.m_bSecure))
				|| (ndvFilters[SBFilter::OPT_FILTER_VACMODE].ringBox.iCurIdx == ANTICHEAT_ON &&
					(!ndv.gameServer.info.m_bSecure))
				)
		{
			continue;
		}

		m_ndvFilteredVec.AddToTail(ndv);
	}

	if (m_ndvFilteredVec.IsEmpty())
	{
		CNeoDataVariant ndv = { .type = CNeoDataVariant::TEXTLABEL, };
		ndv.labelSize = V_swprintf_safe(ndv.textLabel.wszLabel, L"No %ls queries found.", GS_NAMES[m_iType].text);
		m_ndvFilteredVec.AddToTail(ndv);
	}
	else
	{
		// Can't use lamda capture for this, so pass through context
		V_qsort_s(m_ndvFilteredVec.Base(), m_ndvFilteredVec.Size(), sizeof(CNeoDataVariant),
				  [](void *vpCtx, const void *vpLeft, const void *vpRight) -> int {
			const GameServerSortContext gsCtx = *(static_cast<GameServerSortContext *>(vpCtx));
			auto *ndvLeft = static_cast<const CNeoDataVariant *>(vpLeft);
			auto *ndvRight = static_cast<const CNeoDataVariant *>(vpRight);

			// Always set szLeft/szRight to name as fallback
			const char *szLeft = ndvLeft->gameServer.info.GetName();
			const char *szRight = ndvRight->gameServer.info.GetName();
			int iLeft, iRight;
			bool bLeft, bRight;
			switch (gsCtx.col)
			{
			break; case GSIW_LOCKED:
				bLeft = ndvLeft->gameServer.info.m_bPassword;
				bRight = ndvRight->gameServer.info.m_bPassword;
			break; case GSIW_VAC:
				bLeft = ndvLeft->gameServer.info.m_bSecure;
				bRight = ndvRight->gameServer.info.m_bSecure;
			break; case GSIW_MAP:
			{
				const char *szMapLeft = ndvLeft->gameServer.info.m_szMap;
				const char *szMapRight = ndvRight->gameServer.info.m_szMap;
				if (V_strcmp(szMapLeft, szMapRight) != 0)
				{
					szLeft = szMapLeft;
					szRight = szMapRight;
				}
			}
			break; case GSIW_PLAYERS:
				iLeft = ndvLeft->gameServer.info.m_nPlayers;
				iRight = ndvRight->gameServer.info.m_nPlayers;
				if (iLeft == iRight)
				{
					iLeft = ndvLeft->gameServer.info.m_nMaxPlayers;
					iRight = ndvRight->gameServer.info.m_nMaxPlayers;
				}
			break; case GSIW_PING:
				iLeft = ndvLeft->gameServer.info.m_nPing;
				iRight = ndvRight->gameServer.info.m_nPing;
			break; case GSIW_NAME: default: break;
				// no-op, already assigned (default)
			}

			switch (gsCtx.col)
			{
			case GSIW_LOCKED:
			case GSIW_VAC:
				if (bLeft != bRight) return (gsCtx.bDescending) ? bLeft < bRight : bLeft > bRight;
				break;
			case GSIW_PLAYERS:
			case GSIW_PING:
				if (iLeft != iRight) return (gsCtx.bDescending) ? iLeft < iRight : iLeft > iRight;
				break;
			default:
				break;
			}

			return (gsCtx.bDescending) ? V_strcmp(szRight, szLeft) : V_strcmp(szLeft, szRight);
		}, m_pSortCtx);
	}
}

CNeoDataVariant *CNeoDataServerBrowser_General::NdvList()
{
	return m_ndvFilteredVec.Base();
}

WLabelWSize CNeoDataServerBrowser_General::Title()
{
	return GS_NAMES[m_iType];
}

void CNeoDataServerBrowser_General::RequestList(MatchMakingKeyValuePair_t **filters, const uint32 iFiltersSize)
{
	static constexpr HServerListRequest (ISteamMatchmakingServers::*pFnReq[GS__TOTAL])(
				AppId_t, MatchMakingKeyValuePair_t **, uint32, ISteamMatchmakingServerListResponse *) = {
		&ISteamMatchmakingServers::RequestInternetServerList,
		nullptr,
		&ISteamMatchmakingServers::RequestFriendsServerList,
		&ISteamMatchmakingServers::RequestFavoritesServerList,
		&ISteamMatchmakingServers::RequestHistoryServerList,
		&ISteamMatchmakingServers::RequestSpectatorServerList,
	};

	ISteamMatchmakingServers *steamMM = steamapicontext->SteamMatchmakingServers();
	m_hdlRequest = (m_iType == GS_LAN) ?
				steamMM->RequestLANServerList(engine->GetAppID(), this) :
				(steamMM->*pFnReq[m_iType])(engine->GetAppID(), filters, iFiltersSize, this);
}

// Server has responded ok with updated data
void CNeoDataServerBrowser_General::ServerResponded(HServerListRequest hRequest, int iServer)
{
	if (hRequest != m_hdlRequest) return;

	ISteamMatchmakingServers *steamMM = steamapicontext->SteamMatchmakingServers();
	gameserveritem_t *pServerDetails = steamMM->GetServerDetails(hRequest, iServer);
	if (pServerDetails)
	{
		CNeoDataVariant ndv = { .type = CNeoDataVariant::GAMESERVER, };
		ndv.gameServer.info = *pServerDetails;
		m_ndvVec.AddToTail(ndv);
		m_bModified = true;
	}
}

// Server has failed to respond
void CNeoDataServerBrowser_General::ServerFailedToRespond(HServerListRequest hRequest, [[maybe_unused]] int iServer)
{
	if (hRequest != m_hdlRequest) return;
}

// A list refresh you had initiated is now 100% completed
void CNeoDataServerBrowser_General::RefreshComplete(HServerListRequest hRequest, EMatchMakingServerResponse response)
{
	if (hRequest != m_hdlRequest) return;

	if (response == eNoServersListedOnMasterServer && m_ndvVec.IsEmpty())
	{
		m_bModified = true;
	}
}

CNeoDataServerBrowser_Filters::CNeoDataServerBrowser_Filters()
	: m_ndvList{
		NDV_INIT_RINGBOX_ONOFF(L"Server not full"),
		NDV_INIT_RINGBOX_ONOFF(L"Has users playing"),
		NDV_INIT_RINGBOX_ONOFF(L"Is not password protected"),
		NDV_INIT_RINGBOX(L"Anti-cheat", ANTICHEAT_LABELS, ARRAYSIZE(ANTICHEAT_LABELS)),
	}
{
}

CNeoPanel_ServerBrowser::CNeoPanel_ServerBrowser(vgui::Panel *parent)
	: CNeoPanel_Base(parent)
{
	for (int i = 0; i < GS__TOTAL; ++i)
	{
		m_ndsGeneral[i].m_iType = static_cast<GameServerType>(i);
		m_ndsGeneral[i].m_filters = &m_ndsFilters;
		m_ndsGeneral[i].m_pSortCtx = &m_sortCtx;
	}
	ivgui()->AddTickSignal(GetVPanel(), 200);
}

void CNeoPanel_ServerBrowser::OnBottomAction(const int btn)
{
	switch (btn)
	{
	case BBTN_LEGACY:
		g_pNeoRoot->GetGameUI()->SendMainMenuCommand("OpenServerBrowser");
		break;
	case BBTN_REFRESH:
	{
		if (m_iNdsCurrent < 0 || m_iNdsCurrent >= GS__TOTAL)
		{
			return;
		}

		ISteamMatchmakingServers *steamMM = steamapicontext->SteamMatchmakingServers();
		m_ndsGeneral[m_iNdsCurrent].m_ndvVec.RemoveAll();
		m_ndsGeneral[m_iNdsCurrent].m_ndvFilteredVec.RemoveAll();
		{
			CNeoDataVariant ndv = { .type = CNeoDataVariant::TEXTLABEL, };
			ndv.labelSize = V_swprintf_safe(ndv.textLabel.wszLabel, L"Searching %ls queries...", GS_NAMES[m_iNdsCurrent].text);
			m_ndsGeneral[m_iNdsCurrent].m_ndvFilteredVec.AddToTail(ndv);
		}

		if (m_ndsGeneral[m_iNdsCurrent].m_hdlRequest)
		{
			steamMM->CancelQuery(m_ndsGeneral[m_iNdsCurrent].m_hdlRequest);
			steamMM->ReleaseRequest(m_ndsGeneral[m_iNdsCurrent].m_hdlRequest);
			m_ndsGeneral[m_iNdsCurrent].m_hdlRequest = nullptr;
		}
		static MatchMakingKeyValuePair_t mmFilters[] = {
			{"gamedir", "neo"},
		};
		MatchMakingKeyValuePair_t *pMMFilters = mmFilters;
		m_ndsGeneral[m_iNdsCurrent].RequestList(&pMMFilters, 1);
		break;
	}
	case BBTN_GO:
	{
		//OnEnterServer(ndv->gameserver);
	}
		[[fallthrough]]; // Also reset the main menu back to root state
	case BBTN_BACK:
		g_pNeoRoot->m_state = CNeoRoot::STATE_ROOT;
		g_pNeoRoot->UpdateControls();
		break;
	}
}

void CNeoPanel_ServerBrowser::OnEnterServer(const gameserveritem_t gameserver)
{
	if (engine->IsInGame())
	{
		engine->ClientCmd_Unrestricted("disconnect");
	}

	// TODO (nullsystem): Deal with password protected server
	if (gameserver.m_bPassword)
	{
		// TODO
	}
	else
	{
		char connectCmd[256];
		const char *szAddress = gameserver.m_NetAdr.GetConnectionAddressString();
		V_sprintf_safe(connectCmd, "progress_enable; wait; connect %s", szAddress);
		engine->ClientCmd_Unrestricted(connectCmd);

		g_pNeoRoot->m_state = CNeoRoot::STATE_ROOT;
		g_pNeoRoot->UpdateControls();
	}
}

const WLabelWSize *CNeoPanel_ServerBrowser::BottomSectionList() const
{
	static constexpr WLabelWSize BBTN_NAMES[BBTN__TOTAL] = {
		LWS(L"Back (ESC)"), LWS(L"Legacy"), LWSNULL, LWS(L"Refresh"), LWS(L"Start (F1)"),
	};
	return BBTN_NAMES;
}

int CNeoPanel_ServerBrowser::KeyCodeToBottomAction(vgui::KeyCode code) const
{
	switch (code)
	{
	case KEY_ESCAPE: return BBTN_BACK;
	default: break;
	}
	return -1;
}

void CNeoPanel_ServerBrowser::TopAreaPaint()
{
	if (m_iNdsCurrent == TAB_FILTERS)
	{
		return;
	}

	const int iFontTall = surface()->GetFontTall(g_neoFont);
	const int iFontStartYPos = (g_iRowTall / 2) - (iFontTall / 2);

	static constexpr WLabelWSize SBLABEL_NAMES[GSIW__TOTAL] = {
		LWS(L"Lock"), LWS(L"VAC"), LWS(L"Name"), LWS(L"Map"), LWS(L"Players"), LWS(L"Ping"),
	};

	for (int i = 0, xPos = 0; i < GSIW__TOTAL; ++i)
	{
		if (m_sortCtx.col == i)
		{
			const int xPosEnd = xPos + g_iGSIX[i];
			const int yPosEnd = g_iRowTall + g_iRowTall;
			int iHintTall = g_iMarginY / 3;
			if (iHintTall <= 0) iHintTall = 1;

			// Background color
			surface()->DrawSetColor(COLOR_NEOPANELACCENTBG);
			surface()->DrawFilledRect(xPos, g_iRowTall, xPosEnd, yPosEnd);

			// Ascending/descending hint
			surface()->DrawSetColor(COLOR_NEOPANELTEXTNORMAL);
			if (!m_sortCtx.bDescending)	surface()->DrawFilledRect(xPos, g_iRowTall, xPosEnd, g_iRowTall + iHintTall);
			else						surface()->DrawFilledRect(xPos, yPosEnd - iHintTall, xPosEnd, yPosEnd);
		}
		surface()->DrawSetTextPos(xPos + g_iMarginX, g_iRowTall + iFontStartYPos);
		surface()->DrawPrintText(SBLABEL_NAMES[i].text, SBLABEL_NAMES[i].size);
		xPos += g_iGSIX[i];
	}
	surface()->DrawSetColor(COLOR_NEOPANELNORMALBG);
}

void CNeoPanel_ServerBrowser::OnMousePressed(vgui::MouseCode code)
{
	if (m_iNdsCurrent != TAB_FILTERS && m_bTopArea)
	{
		if (m_sortCtx.col == m_hoverGSCol)
		{
			m_sortCtx.bDescending = !m_sortCtx.bDescending;
		}
		m_sortCtx.col = m_hoverGSCol;
		m_bModified = true;
	}
	BaseClass::OnMousePressed(code);
}

void CNeoPanel_ServerBrowser::OnCursorMovedTopArea(int x, int y)
{
	if (m_iNdsCurrent == TAB_FILTERS)
	{
		return;
	}

	for (int i = 0, xPos = 0; i < GSIW__TOTAL; ++i)
	{
		if (x >= xPos && x < (xPos + g_iGSIX[i]))
		{
			m_hoverGSCol = static_cast<GameServerInfoW>(i);
			return;
		}
		xPos += g_iGSIX[i];
	}
}

void CNeoPanel_ServerBrowser::OnTick()
{
	if (m_bModified)
	{
		// Pass modified over to the tabs so it doesn't trigger
		// the filter refresh immeditely
		for (int i = 0; i < GS__TOTAL; ++i)
		{
			m_ndsGeneral[i].m_bModified = true;
		}
		m_bModified = false;
	}

	if (m_ndsGeneral[m_iNdsCurrent].m_bModified)
	{
		m_ndsGeneral[m_iNdsCurrent].UpdateFilteredList();
		m_ndsGeneral[m_iNdsCurrent].m_bModified = false;
	}
}

///////
// MAIN ROOT PANEL
///////

CNeoRootInput::CNeoRootInput(CNeoRoot *rootPanel)
	: Panel(rootPanel, "NeoRootPanelInput")
	, m_pNeoRoot(rootPanel)
{
	MakePopup(true);
	SetKeyBoardInputEnabled(true);
	SetMouseInputEnabled(false);
	SetVisible(true);
	SetEnabled(true);
	PerformLayout();
}

void CNeoRootInput::PerformLayout()
{
	SetPos(0, 0);
	SetSize(1, 1);
	SetBgColor(COLOR_TRANSPARENT);
	SetFgColor(COLOR_TRANSPARENT);
}

void CNeoRootInput::OnKeyCodeTyped(vgui::KeyCode code)
{
	m_pNeoRoot->OnRelayedKeyCodeTyped(code);
}

enum WidgetInfoFlags
{
	FLAG_NONE = 0,
	FLAG_SHOWINGAME = 1 << 0,
	FLAG_SHOWINMAIN = 1 << 1,
};

struct WidgetInfo
{
	const char *label;
	const char *gamemenucommand;
	int flags;
};

constexpr WidgetInfo BTNS_INFO[CNeoRoot::BTN__TOTAL] = {
	{ "#GameUI_GameMenu_ResumeGame", "ResumeGame", FLAG_SHOWINGAME },
	{ "#GameUI_GameMenu_FindServers", nullptr, FLAG_SHOWINGAME | FLAG_SHOWINMAIN },
	{ "#GameUI_GameMenu_CreateServer", nullptr, FLAG_SHOWINGAME | FLAG_SHOWINMAIN },
	{ "#GameUI_GameMenu_Disconnect", "Disconnect", FLAG_SHOWINGAME },
	{ "#GameUI_GameMenu_PlayerList", "OpenPlayerListDialog", FLAG_SHOWINGAME },
	{ "#GameUI_GameMenu_Options", nullptr, FLAG_SHOWINGAME | FLAG_SHOWINMAIN },
	{ "#GameUI_GameMenu_Quit", "Quit", FLAG_SHOWINGAME | FLAG_SHOWINMAIN },
};

CNeoRoot::CNeoRoot(VPANEL parent)
	: EditablePanel(nullptr, "NeoRootPanel")
	, m_panelSettings(new CNeoPanel_Settings(this))
	, m_panelNewGame(new CNeoPanel_NewGame(this))
	, m_panelServerBrowser(new CNeoPanel_ServerBrowser(this))
	, m_opKeyCapture(new CNeoOverlay_KeyCapture(this))
	, m_panelCaptureInput(new CNeoRootInput(this))
{
	SetParent(parent);
	g_pNeoRoot = this;
	LoadGameUI();
	SetVisible(true);
	SetProportional(false);

	vgui::HScheme neoscheme = vgui::scheme()->LoadSchemeFromFileEx(
		enginevgui->GetPanel(PANEL_CLIENTDLL), "resource/ClientScheme.res", "ClientScheme");
	SetScheme(neoscheme);

	for (int i = 0; i < BTN__TOTAL; ++i)
	{
		const char *label = BTNS_INFO[i].label;
		if (wchar_t *localizedWszStr = g_pVGuiLocalize->Find(label))
		{
			V_wcsncpy(m_wszDispBtnTexts[i], localizedWszStr, sizeof(m_wszDispBtnTexts[i]));
		}
		else
		{
			g_pVGuiLocalize->ConvertANSIToUnicode(label, m_wszDispBtnTexts[i], sizeof(m_wszDispBtnTexts[i]));
		}
		m_iWszDispBtnTextsSizes[i] = V_wcslen(m_wszDispBtnTexts[i]);
	}

	m_panelSettings->m_opKeyCapture = m_opKeyCapture;
	m_opKeyCapture->AddActionSignalTarget(m_panelSettings);

	SetKeyBoardInputEnabled(true);
	SetMouseInputEnabled(true);
	UpdateControls();

	vgui::IScheme *pScheme = vgui::scheme()->GetIScheme(neoscheme);
	ApplySchemeSettings(pScheme);
}

CNeoRoot::~CNeoRoot()
{
	m_panelCaptureInput->DeletePanel();
	m_opKeyCapture->DeletePanel();
	m_panelNewGame->DeletePanel();
	m_panelServerBrowser->DeletePanel();
	m_panelSettings->DeletePanel();
	if (m_avImage) delete m_avImage;

	m_gameui = nullptr;
	g_GameUIDLL.Unload();
}

IGameUI *CNeoRoot::GetGameUI()
{
	if (!m_gameui && !LoadGameUI()) return nullptr;
	return m_gameui;
}

void CNeoRoot::UpdateControls()
{
	int wide, tall;
	GetSize(wide, tall);

	CNeoPanel_Base *pPanel = nullptr;

	const bool bInSettings = m_state == STATE_SETTINGS;
	m_panelSettings->SetVisible(bInSettings);
	m_panelSettings->SetEnabled(bInSettings);
	if (bInSettings) pPanel = m_panelSettings;

	const bool bInNewGame = m_state == STATE_NEWGAME;
	m_panelNewGame->SetVisible(bInNewGame);
	m_panelNewGame->SetEnabled(bInNewGame);
	if (bInNewGame) pPanel = m_panelNewGame;

	const bool bInServerBrowser = m_state == STATE_SERVERBROWSER;
	m_panelServerBrowser->SetVisible(bInServerBrowser);
	m_panelServerBrowser->SetEnabled(bInServerBrowser);
	if (bInServerBrowser) pPanel = m_panelServerBrowser;

	if (pPanel)
	{
		pPanel->m_iNdvHover = -1;
		pPanel->m_curMouse = CNeoPanel_Base::WDG_NONE;
		pPanel->PerformLayout();
		pPanel->SetPos((wide / 2) - (g_iRootSubPanelWide / 2),
								(tall / 2) - (pPanel->GetTall() / 2));
		pPanel->RequestFocus();
		pPanel->MoveToFront();
	}
	else
	{
		RequestFocus();
		m_panelCaptureInput->RequestFocus();
	}
	InvalidateLayout();
}

bool CNeoRoot::LoadGameUI()
{
	if (!m_gameui)
	{
		CreateInterfaceFn gameUIFactory = g_GameUIDLL.GetFactory();
		if (!gameUIFactory) return false;

		m_gameui = reinterpret_cast<IGameUI *>(gameUIFactory(GAMEUI_INTERFACE_VERSION, nullptr));
		if (!m_gameui) return false;
	}
	return true;
}

void CNeoRoot::ApplySchemeSettings(IScheme *pScheme)
{
	BaseClass::ApplySchemeSettings(pScheme);
	// NEO TODO (nullsystem): If we're to provide color scheme controls:
	// LoadControlSettings("resource/NeoRootScheme.res");

	// Resize the panel to the screen size
	int wide, tall;
	surface()->GetScreenSize(wide, tall);
	SetSize(wide, tall);
	SetFgColor(COLOR_TRANSPARENT);
	SetBgColor(COLOR_TRANSPARENT);

	static constexpr const char *FONT_NAMES[FONT__TOTAL] = {
		"NHudOCR", "NHudOCRSmallNoAdditive", "ClientTitleFont"
	};
	for (int i = 0; i < FONT__TOTAL; ++i)
	{
		m_hTextFonts[i] = pScheme->GetFont(FONT_NAMES[i], true);
	}

	// In 1080p, g_iRowTall == 40, g_iMarginX = 10, g_iAvatar = 64,
	// other resolution scales up/down from it
	g_iRowTall = tall / 27;
	g_iMarginX = wide / 192;
	g_iMarginY = tall / 108;
	g_iAvatar = wide / 30;
	const float flWide = static_cast<float>(wide);
	float flWideAs43 = static_cast<float>(tall) * (4.0f / 3.0f);
	if (flWideAs43 > flWide) flWideAs43 = flWide;
	g_iRootSubPanelWide = static_cast<int>(flWideAs43 * 0.9f);

	g_neoFont = m_hTextFonts[FONT_NTSMALL];
	m_opKeyCapture->m_fontMain = m_hTextFonts[FONT_NTNORMAL];
	m_opKeyCapture->m_fontSub = m_hTextFonts[FONT_NTSMALL];
	m_panelSettings->m_opConfirm->m_fontMain = m_hTextFonts[FONT_NTNORMAL];

	constexpr int PARTITION = GSIW__TOTAL * 4;
	const int iSubDiv = g_iRootSubPanelWide / PARTITION;
	g_iGSIX[GSIW_LOCKED] = iSubDiv * 2;
	g_iGSIX[GSIW_VAC] = iSubDiv * 2;
	g_iGSIX[GSIW_NAME] = iSubDiv * 10;
	g_iGSIX[GSIW_MAP] = iSubDiv * 5;
	g_iGSIX[GSIW_PLAYERS] = iSubDiv * 3;
	g_iGSIX[GSIW_PING] = iSubDiv * 2;

	UpdateControls();
}

void CNeoRoot::Paint()
{
	int wide, tall;
	GetSize(wide, tall);

	const bool bNextIsInGame = engine->IsInGame();
	if (m_state != STATE_ROOT)
	{
		// Print the title
		static constexpr int STATE_TO_BTN_MAP[STATE__TOTAL] = {
			0, BTN_SETTINGS, BTN_SERVERCREATE, BTN_SERVERBROWSER,
		};
		const int iBtnIdx = STATE_TO_BTN_MAP[m_state];

		surface()->DrawSetTextFont(m_hTextFonts[FONT_NTNORMAL]);
		surface()->DrawSetTextColor(COLOR_NEOPANELTEXTBRIGHT);

		const int iPanelTall = g_iRowTall + (tall * 0.8f) + g_iRowTall;
		const int yTopPos = (tall - iPanelTall) / 2;
		int iTitleWidth, iTitleHeight;
		surface()->GetTextSize(m_hTextFonts[FONT_NTNORMAL], m_wszDispBtnTexts[iBtnIdx], iTitleWidth, iTitleHeight);
		surface()->DrawSetTextPos((wide / 2) - (g_iRootSubPanelWide / 2),
								  (yTopPos / 2) - (iTitleHeight / 2));
		surface()->DrawPrintText(m_wszDispBtnTexts[iBtnIdx], m_iWszDispBtnTextsSizes[iBtnIdx]);
	}
	else
	{
		const int iBtnPlaceXMid = (wide / 4);

		const int yTopPos = tall / 2 - ((g_iRowTall * BTN__TOTAL) / 2);
		const int iRightXPos = iBtnPlaceXMid + (m_iBtnWide / 2) + g_iMarginX;
		int iRightSideYStart = yTopPos;

		// Draw title
		int iTitleWidth, iTitleHeight;
		surface()->DrawSetTextFont(m_hTextFonts[FONT_LOGO]);
		surface()->GetTextSize(m_hTextFonts[FONT_LOGO], WSZ_GAME_TITLE, iTitleWidth, iTitleHeight);
		m_iBtnWide = iTitleWidth + (2 * g_iMarginX);

		surface()->DrawSetTextColor(COLOR_NEOTITLE);
		surface()->DrawSetTextPos(iBtnPlaceXMid - (iTitleWidth / 2), yTopPos - iTitleHeight);
		surface()->DrawPrintText(WSZ_GAME_TITLE, SZWSZ_LEN(WSZ_GAME_TITLE));

		surface()->DrawSetTextColor(COLOR_NEOPANELTEXTBRIGHT);
		ISteamUser *steamUser = steamapicontext->SteamUser();
		ISteamFriends *steamFriends = steamapicontext->SteamFriends();
		if (steamUser && steamFriends)
		{
			const int iSteamPlaceXStart = iRightXPos;

			// Draw player info (top left corner)
			const CSteamID steamID = steamUser->GetSteamID();
			if (!m_avImage)
			{
				m_avImage = new CAvatarImage;
				m_avImage->SetAvatarSteamID(steamID, k_EAvatarSize64x64);
			}
			m_avImage->SetPos(iSteamPlaceXStart + g_iMarginX, iRightSideYStart + g_iMarginY);
			m_avImage->SetSize(g_iAvatar, g_iAvatar);
			m_avImage->Paint();

			char szTextBuf[512] = {};
			const char *szSteamName = steamFriends->GetPersonaName();
			const char *szNeoName = neo_name.GetString();
			const bool bUseNeoName = (szNeoName && szNeoName[0] != '\0' && !cl_onlysteamnick.GetBool());
			V_sprintf_safe(szTextBuf, "%s", (bUseNeoName) ? szNeoName : szSteamName);

			wchar_t wszTextBuf[512] = {};
			g_pVGuiLocalize->ConvertANSIToUnicode(szTextBuf, wszTextBuf, sizeof(wszTextBuf));

			int iMainTextWidth, iMainTextHeight;
			surface()->DrawSetTextFont(m_hTextFonts[FONT_NTNORMAL]);
			surface()->GetTextSize(m_hTextFonts[FONT_NTNORMAL], wszTextBuf, iMainTextWidth, iMainTextHeight);

			const int iMainTextStartPosX = g_iMarginX + g_iAvatar + g_iMarginX;
			surface()->DrawSetTextPos(iSteamPlaceXStart + iMainTextStartPosX, iRightSideYStart + g_iMarginY);
			surface()->DrawPrintText(wszTextBuf, V_strlen(szTextBuf));

			if (bUseNeoName)
			{
				V_sprintf_safe(szTextBuf, "(Steam name: %s)", szSteamName);
				g_pVGuiLocalize->ConvertANSIToUnicode(szTextBuf, wszTextBuf, sizeof(wszTextBuf));

				int iSteamSubTextWidth, iSteamSubTextHeight;
				surface()->DrawSetTextFont(m_hTextFonts[FONT_NTSMALL]);
				surface()->GetTextSize(m_hTextFonts[FONT_NTSMALL], wszTextBuf, iSteamSubTextWidth, iSteamSubTextHeight);

				const int iRightOfNicknameXPos = iSteamPlaceXStart + iMainTextStartPosX + iMainTextWidth + g_iMarginX;
				// If we have space on the right, set it, otherwise on top of nickname
				if ((iRightOfNicknameXPos + iSteamSubTextWidth) < wide)
				{
					surface()->DrawSetTextPos(iRightOfNicknameXPos, iRightSideYStart + g_iMarginY);
				}
				else
				{
					surface()->DrawSetTextPos(iSteamPlaceXStart + iMainTextStartPosX,
											  iRightSideYStart - iSteamSubTextHeight);
				}
				surface()->DrawPrintText(wszTextBuf, V_strlen(szTextBuf));
			}

			static constexpr const wchar_t *WSZ_PERSONA_STATES[k_EPersonaStateMax] = {
				L"Offline", L"Online", L"Busy", L"Away", L"Snooze", L"Trading", L"Looking to play"
			};
			const auto eCurStatus = steamFriends->GetPersonaState();
			int iStatusTall = 0;
			if (eCurStatus != k_EPersonaStateMax)
			{
				const wchar_t *wszState = WSZ_PERSONA_STATES[static_cast<int>(eCurStatus)];
				const int iStatusTextStartPosY = g_iMarginY + iMainTextHeight + g_iMarginY;

				[[maybe_unused]] int iStatusWide;
				surface()->DrawSetTextFont(m_hTextFonts[FONT_NTSMALL]);
				surface()->GetTextSize(m_hTextFonts[FONT_NTSMALL], wszState, iStatusWide, iStatusTall);
				surface()->DrawSetTextPos(iSteamPlaceXStart + iMainTextStartPosX,
										  iRightSideYStart + iStatusTextStartPosY);
				surface()->DrawPrintText(wszState, V_wcslen(wszState));
			}

			// Put the news title in either from avatar or text end Y position
			const int iTextTotalTall = iMainTextHeight + iStatusTall;
			iRightSideYStart += (g_iMarginX * 2) + ((iTextTotalTall > g_iAvatar) ? iTextTotalTall : g_iAvatar);
		}

		{
			// Show the news
			static constexpr wchar_t WSZ_NEWS_TITLE[] = L"News";

			int iMainTextWidth, iMainTextHeight;
			surface()->DrawSetTextFont(m_hTextFonts[FONT_NTNORMAL]);
			surface()->GetTextSize(m_hTextFonts[FONT_NTNORMAL], WSZ_NEWS_TITLE, iMainTextWidth, iMainTextHeight);
			surface()->DrawSetTextPos(iRightXPos, iRightSideYStart + g_iMarginY);
			surface()->DrawPrintText(WSZ_NEWS_TITLE, SZWSZ_LEN(WSZ_NEWS_TITLE));

			// Write some headlines
			static constexpr const wchar_t *WSZ_NEWS_HEADLINES[] = {
				L"2024-08-03: NT;RE v7.1 Released",
			};

			int iHlYPos = iRightSideYStart + (2 * g_iMarginY) + iMainTextHeight;
			for (const wchar_t *wszHeadline : WSZ_NEWS_HEADLINES)
			{
				int iHlTextWidth, iHlTextHeight;
				surface()->DrawSetTextFont(m_hTextFonts[FONT_NTSMALL]);
				surface()->GetTextSize(m_hTextFonts[FONT_NTSMALL], wszHeadline, iHlTextWidth, iHlTextHeight);
				surface()->DrawSetTextPos(iRightXPos, iHlYPos);
				surface()->DrawPrintText(wszHeadline, V_wcslen(wszHeadline));

				iHlYPos += g_iMarginY + iHlTextHeight;
			}
		}

		// Draw buttons
		IntPos btnPos{
			.x = iBtnPlaceXMid - (m_iBtnWide / 2),
			.y = yTopPos,
		};
		const int flagToMatch = engine->IsInGame() ? FLAG_SHOWINGAME : FLAG_SHOWINMAIN;
		surface()->DrawSetTextFont(m_hTextFonts[FONT_NTSMALL]);
		const int fontTall = surface()->GetFontTall(m_hTextFonts[FONT_NTSMALL]);
		const int fontStartYPos = (g_iRowTall / 2) - (fontTall / 2);
		for (int i = 0; i < BTN__TOTAL; ++i)
		{
			if (BTNS_INFO[i].flags & flagToMatch)
			{
				surface()->DrawSetColor((m_iHoverBtn == i) ? COLOR_NEOPANELACCENTBG : COLOR_NEOPANELNORMALBG);
				surface()->DrawFilledRect(btnPos.x, btnPos.y, btnPos.x + m_iBtnWide, btnPos.y + g_iRowTall);

				surface()->DrawSetTextPos(btnPos.x + g_iMarginX, btnPos.y + fontStartYPos);
				surface()->DrawPrintText(m_wszDispBtnTexts[i], m_iWszDispBtnTextsSizes[i]);

				btnPos.y += g_iRowTall;
			}
		}
	}

	// Draw version info (bottom left corner) - Always
	surface()->DrawSetTextColor(COLOR_NEOPANELTEXTBRIGHT);
	int textWidth, textHeight;
	surface()->DrawSetTextFont(m_hTextFonts[FONT_NTSMALL]);
	surface()->GetTextSize(m_hTextFonts[FONT_NTSMALL], BUILD_DISPLAY, textWidth, textHeight);

	surface()->DrawSetTextPos(g_iMarginX, tall - textHeight - g_iMarginY);
	surface()->DrawPrintText(BUILD_DISPLAY, *BUILD_DISPLAY_SIZE);
}

void CNeoRoot::OnMousePressed(vgui::MouseCode code)
{
	if (code != MOUSE_LEFT || m_iHoverBtn < 0 || m_iHoverBtn >= BTN__TOTAL) return;

	surface()->PlaySound("ui/buttonclickrelease.wav");
	const auto btnInfo = BTNS_INFO[m_iHoverBtn];
	if (m_iHoverBtn == BTN_SETTINGS)
	{
		m_state = STATE_SETTINGS;
		m_panelSettings->UserSettingsRestore();
		UpdateControls();
	}
	else if (m_iHoverBtn == BTN_SERVERCREATE)
	{
		m_state = STATE_NEWGAME;
		UpdateControls();
	}
	else if (m_iHoverBtn == BTN_SERVERBROWSER)
	{
		m_state = STATE_SERVERBROWSER;
		UpdateControls();
	}
	else if (btnInfo.gamemenucommand)
	{
		m_state = STATE_ROOT;
		UpdateControls();
		GetGameUI()->SendMainMenuCommand(btnInfo.gamemenucommand);
	}
}

void CNeoRoot::OnCursorExited()
{
	m_iHoverBtn = -1;
}

void CNeoRoot::OnCursorMoved(int x, int y)
{
	const bool inSettings = m_state == STATE_SETTINGS;

	const int iPrevHoverBtn = m_iHoverBtn;
	m_iHoverBtn = -1;
	const int wide = GetWide();
	const int iStartX = (wide / 4) - (m_iBtnWide / 2);
	if (inSettings || x < iStartX || x >= (iStartX + m_iBtnWide)) return;

	const int flagToMatch = engine->IsInGame() ? FLAG_SHOWINGAME : FLAG_SHOWINMAIN;
	int iTotal = 0;
	int iMapIdx[BTN__TOTAL] = {};
	for (int i = 0; i < BTN__TOTAL; ++i)
	{
		if (BTNS_INFO[i].flags & flagToMatch)
		{
			iMapIdx[iTotal] = i;
			++iTotal;
		}
	}

	const int yTopPos = GetTall() / 2 - ((g_iRowTall * BTN__TOTAL) / 2);
	if (y >= yTopPos && y < yTopPos + (g_iRowTall * iTotal))
	{
		y -= yTopPos;
		const int num = y / g_iRowTall;
		if (num >= 0 && num < iTotal)
		{
			m_iHoverBtn = iMapIdx[num];
			if (iPrevHoverBtn != m_iHoverBtn)
			{
				surface()->PlaySound("ui/buttonrollover.wav");
			}
		}
	}
}

void CNeoRoot::OnRelayedKeyCodeTyped(vgui::KeyCode code)
{
	if (code == KEY_DOWN || code == KEY_UP)
	{
		const int iFlagToMatch = engine->IsInGame() ? FLAG_SHOWINGAME : FLAG_SHOWINMAIN;
		const int iIncr = (code == KEY_DOWN) ? +1 : -1;

		bool bGoNext = true;
		while (bGoNext)
		{
			m_iHoverBtn += iIncr;
			if (m_iHoverBtn < 0) m_iHoverBtn = (BTN__TOTAL - 1);
			else if (m_iHoverBtn >= BTN__TOTAL) m_iHoverBtn = 0;
			bGoNext = (!(BTNS_INFO[m_iHoverBtn].flags & iFlagToMatch));
		}

		m_iHoverBtn = clamp(m_iHoverBtn, 0, BTN__TOTAL - 1);
	}
	else if (code == KEY_ENTER)
	{
		OnMousePressed(MOUSE_LEFT);
	}
}

bool NeoRootCaptureESC()
{
	return (g_pNeoRoot && (
			(g_pNeoRoot->m_panelSettings->IsVisible() || g_pNeoRoot->m_panelSettings->m_opConfirm->IsVisible()) ||
			(g_pNeoRoot->m_panelNewGame->IsVisible() || g_pNeoRoot->m_panelServerBrowser->IsVisible()) ||
			(g_pNeoRoot->m_panelNewGame->m_mapList->IsVisible())
				));
}
