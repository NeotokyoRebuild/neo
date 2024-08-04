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
HFont g_neoFont;
static constexpr wchar_t WSZ_GAME_TITLE[] = L"neatbkyoc ue";

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
	SetBgColor(Color(40, 40, 40, 255));
	SetFgColor(Color(40, 40, 40, 255));
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
	SetFgColor(Color(0, 0, 0, 170));
	SetBgColor(Color(0, 0, 0, 170));
}

void CNeoOverlay_KeyCapture::Paint()
{
	BaseClass::Paint();

	int wide, tall;
	GetSize(wide, tall);

	const int tallSplit = tall / 3;
	surface()->DrawSetColor(Color(40, 40, 40, 255));
	surface()->DrawFilledRect(0, tallSplit, wide, tall - tallSplit);

	int yPos = 0;
	surface()->DrawSetTextColor(Color(255, 255, 255, 255));
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
		surface()->DrawPrintText(SUB_INFO, sizeof(SUB_INFO) / sizeof(wchar_t));
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
	SetBgColor(Color(40, 40, 40, 255));
	SetFgColor(Color(40, 40, 40, 255));
	SetVisible(false);
	SetEnabled(false);
}

void CNeoOverlay_Confirm::PerformLayout()
{
	int wide, tall;
	vgui::surface()->GetScreenSize(wide, tall);

	SetPos(0, 0);
	SetSize(wide, tall);
	SetFgColor(Color(0, 0, 0, 170));
	SetBgColor(Color(0, 0, 0, 170));
}

void CNeoOverlay_Confirm::Paint()
{
	BaseClass::Paint();

	int wide, tall;
	GetSize(wide, tall);

	const int tallSplit = tall / 3;
	const int btnWide = wide / 6;
	surface()->DrawSetColor(Color(40, 40, 40, 255));
	surface()->DrawFilledRect(0, tallSplit, wide, tall - tallSplit);

	surface()->DrawSetTextColor(Color(255, 255, 255, 255));
	surface()->DrawSetTextFont(m_fontMain);
	{
		static constexpr wchar_t CONFIRM_TEXT[] = L"Settings changed: Do you want to apply the settings?";

		int textWidth, textHeight;
		surface()->GetTextSize(m_fontMain, CONFIRM_TEXT, textWidth, textHeight);
		const int textYPos = tallSplit + (tallSplit / 3) - (textHeight / 2);
		surface()->DrawSetTextPos((wide - textWidth) / 2, textYPos);
		surface()->DrawPrintText(CONFIRM_TEXT, (sizeof(CONFIRM_TEXT) / sizeof(wchar_t)) - 1);
	}
	{
		const int buttonYPos = tallSplit + (tallSplit * 0.67f) - (g_iRowTall / 2);
		surface()->DrawSetTextFont(g_neoFont);

		{
			static constexpr wchar_t APPLY_TEXT[] = L"Apply (Enter)";
			int textWidth, textHeight;
			surface()->GetTextSize(g_neoFont, APPLY_TEXT, textWidth, textHeight);

			const int buttonXPos = (wide / 2) - btnWide;
			surface()->DrawSetColor((m_buttonHover == BUTTON_APPLY) ? Color(40, 10, 10, 255) : Color(0, 0, 0, 255));
			surface()->DrawFilledRect(buttonXPos, buttonYPos, buttonXPos + btnWide, buttonYPos + g_iRowTall);
			surface()->DrawSetTextPos(buttonXPos + ((btnWide / 2) - (textWidth / 2)), buttonYPos + ((g_iRowTall / 2) - (textHeight / 2)));
			surface()->DrawPrintText(APPLY_TEXT, (sizeof(APPLY_TEXT) / sizeof(wchar_t)) - 1);
		}
		{
			static constexpr wchar_t DISCARD_TEXT[] = L"Discard (ESC)";
			int textWidth, textHeight;
			surface()->GetTextSize(g_neoFont, DISCARD_TEXT, textWidth, textHeight);

			const int buttonXPos = (wide / 2);
			surface()->DrawSetColor((m_buttonHover == BUTTON_DISCARD) ? Color(40, 10, 10, 255) : Color(0, 0, 0, 255));
			surface()->DrawFilledRect(buttonXPos, buttonYPos, buttonXPos + btnWide, buttonYPos + g_iRowTall);
			surface()->DrawSetTextPos(buttonXPos + ((btnWide / 2) - (textWidth / 2)), buttonYPos + ((g_iRowTall / 2) - (textHeight / 2)));
			surface()->DrawPrintText(DISCARD_TEXT, (sizeof(DISCARD_TEXT) / sizeof(wchar_t)) - 1);
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

CNeoSettings_Dynamic::CNeoSettings_Dynamic(Panel *parent)
	: EditablePanel(parent, "CNeoSettings_Dynamic")
	, m_opConfirm(new CNeoOverlay_Confirm(this))
{
	m_opConfirm->AddActionSignalTarget(this);

	MakePopup(true);
	SetKeyBoardInputEnabled(true);
	SetMouseInputEnabled(true);
}

CNeoSettings_Dynamic::~CNeoSettings_Dynamic()
{
	m_opConfirm->DeletePanel();
}

void CNeoSettings_Dynamic::UserSettingsRestore()
{
	for (CNeoDataSettings_Base *nds : m_pNdsBases)
	{
		nds->UserSettingsRestore();
	}
	m_bModified = false;
}

void CNeoSettings_Dynamic::UserSettingsSave()
{
	for (CNeoDataSettings_Base *nds : m_pNdsBases)
	{
		nds->UserSettingsSave();
	}
	m_bModified = false;
}

void CNeoSettings_Dynamic::PerformLayout()
{
	int wide, tall;
	vgui::surface()->GetScreenSize(wide, tall);

	BaseClass::PerformLayout();
	SetSize(g_iRootSubPanelWide, g_iRowTall + (tall * 0.8f) + g_iRowTall);
	SetBgColor(Color(40, 40, 40, 150));
	SetFgColor(Color(40, 40, 40, 150));
}

void CNeoSettings_Dynamic::Paint()
{
	BaseClass::Paint();

	CNeoDataSettings_Base *tab = m_pNdsBases[m_iNdsCurrent];
	surface()->DrawSetTextFont(g_neoFont);
	const int wgXPos = static_cast<int>(g_iRootSubPanelWide * 0.4f);
	const int widgetWide = g_iRootSubPanelWide - wgXPos;
	const int widgetTall = g_iRowTall;
	int fontWide, fontTall;
	surface()->GetTextSize(g_neoFont, L"<", fontWide, fontTall);
	const int fontStartXPos = (widgetTall / 2) - (fontWide / 2);
	const int fontStartYPos = (widgetTall / 2) - (fontTall / 2);
	const int tabFullYSize = (m_pNdsBases[m_iNdsCurrent]->NdvListSize() * g_iRowTall);
	const int panelTall = GetTall();
	const bool bEditBlinkShow = (static_cast<int>(gpGlobals->curtime * 1.5f) % 2 == 0);

	const int yOvershoot = m_iScrollOffset % g_iRowTall;
	int yPos = g_iRowTall - yOvershoot;
	for (int i = (m_iScrollOffset / g_iRowTall);
		 yPos < (panelTall - g_iRowTall) && i < tab->NdvListSize();
		 ++i)
	{
		CNeoDataVariant *ndv = &tab->NdvList()[i];
		const bool bThisActive = (i == m_iNdvActive);

		surface()->DrawSetColor(bThisActive ? Color(40, 10, 10, 255) : Color(0, 0, 0, 255));
		surface()->DrawSetTextColor(Color(200, 200, 200, 255));

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
		case CNeoDataVariant::S_MICTESTER:
		{
			if (ndv->type == CNeoDataVariant::BINDENTRY && i != m_iNdvActive)
			{
				surface()->DrawSetColor((i % 2 == 0) ? Color(40, 40, 40, 255) : Color(0, 0, 0, 255));
			}
			surface()->DrawFilledRect(wgXPos, yPos, wgXPos + widgetWide, yPos + widgetTall);
		}
		break;
		default: break;
		}

		// Draw the left-side label
		if (ndv->type == CNeoDataVariant::TEXTLABEL)
		{
			int fontWide, fontTall;
			surface()->GetTextSize(g_neoFont, ndv->label, fontWide, fontTall);
			surface()->DrawSetTextPos((g_iRootSubPanelWide / 2) - (fontWide / 2), yPos + fontStartYPos);
		}
		else
		{
			surface()->DrawSetTextPos(g_iMarginX, yPos + fontStartYPos);
		}
		surface()->DrawPrintText(ndv->label, ndv->labelSize);

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
			surface()->DrawSetColor(Color(20, 20, 20, 255));
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
				if (m_bTextEditMode && bThisActive && bEditBlinkShow)
				{
					surface()->DrawSetTextPos(wgXPos + (widgetWide / 2) - (fontWide / 2) + fontWide,
											  yPos + fontStartYPos);
					surface()->DrawPrintText(L"_", 1);
				}
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
			if (m_bTextEditMode && bThisActive && bEditBlinkShow)
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

			const wchar_t *wszNeoName = m_ndsMulti.m_ndvList[CNeoDataSettings_Multiplayer::OPT_MULTI_NEONAME].textEntry.wszEntry;
			const bool bOnlySteamNick = static_cast<bool>(m_ndsMulti.m_ndvList[CNeoDataSettings_Multiplayer::OPT_MULTI_ONLYSTEAMNICK].ringBox.iCurIdx);
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

#define WSZ_LEN(wlabel) ((sizeof(wlabel) / sizeof(wchar_t)) - 1)
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
				surface()->DrawSetColor(Color(30, 90, 30, 255));
				surface()->DrawFilledRect(wgXPos,
										  yPos,
										  wgXPos + static_cast<int>(mt->flSpeakingVol * static_cast<float>(widgetWide)),
										  yPos + widgetTall);
			}

			int fontWide, fontTall;
			surface()->GetTextSize(g_neoFont, bIsTweaking ? WSZ_MICTESTEXIT : WSZ_MICTESTENTER, fontWide, fontTall);
			surface()->DrawSetTextPos(wgXPos + (widgetWide / 2) - (fontWide / 2), yPos + fontStartYPos);
			surface()->DrawPrintText(bIsTweaking ? WSZ_MICTESTEXIT : WSZ_MICTESTENTER,
									 bIsTweaking ? WSZ_LEN(WSZ_MICTESTEXIT) : WSZ_LEN(WSZ_MICTESTENTER));
		}
		break;
		}

		yPos += widgetTall;
	}

	const int iTabWide = g_iRootSubPanelWide / TAB__TOTAL;
	struct WLabelWSize
	{
		const wchar_t *text;
		int size;
	};
#define LWSNULL { nullptr, 0 }
#define LWS(wlabel) { wlabel, (sizeof(wlabel) / sizeof(wchar_t)) - 1}
	{
		// Draw the top part
		surface()->DrawSetColor(Color(40, 40, 40, 255));
		surface()->DrawFilledRect(0, 0, g_iRootSubPanelWide, g_iRowTall);

		surface()->DrawSetColor(Color(40, 10, 10, 255));
		surface()->DrawSetTextColor(Color(200, 200, 200, 255));

		// Draw the tabs buttons on top
		static constexpr WLabelWSize TAB_NAMES[TAB__TOTAL] = {
			LWS(L"Multiplayer"), LWS(L"Keybinds"), LWS(L"Mouse"), LWS(L"Sound"), LWS(L"Video")
		};
		for (int i = 0, xPosTab = 0; i < TAB__TOTAL; ++i, xPosTab += iTabWide)
		{
			if (i == m_iNdsCurrent) surface()->DrawFilledRect(xPosTab, 0, xPosTab + iTabWide, g_iRowTall);
			surface()->DrawSetTextPos(xPosTab + g_iMarginX, fontStartYPos);
			surface()->DrawPrintText(TAB_NAMES[i].text, TAB_NAMES[i].size);
		}
	}

	const int bottomYStart = (panelTall - g_iRowTall);
	{
		// Draw the bottom part
		surface()->DrawSetColor(Color(40, 40, 40, 255));
		surface()->DrawFilledRect(0, bottomYStart, g_iRootSubPanelWide, panelTall);

		// Draw the buttons on bottom
		static constexpr WLabelWSize BBTN_NAMES[BBTN__TOTAL] = {
			LWS(L"Back (ESC)"), LWS(L"Legacy (Shift+F3)"), LWS(L"Reset (Ctrl+Z)"), LWSNULL, LWS(L"Apply (Ctrl+S)")
		};
		for (int i = 0, xPosTab = 0; i < BBTN__TOTAL; ++i, xPosTab += iTabWide)
		{
			if (!m_bModified && (i == BBTN_RESET || i == BBTN_APPLY)) continue;

			if (BBTN_NAMES[i].text)
			{
				surface()->DrawSetColor((m_iBottomHover == i) ? Color(40, 10, 10, 255) : Color(40, 40, 40, 255));
				surface()->DrawFilledRect(xPosTab, bottomYStart, xPosTab + iTabWide, panelTall);
				surface()->DrawSetTextPos(xPosTab + g_iMarginX, bottomYStart + fontStartYPos);
				surface()->DrawPrintText(BBTN_NAMES[i].text, BBTN_NAMES[i].size);
			}
		}
	}
#undef LWS
}

void CNeoSettings_Dynamic::OnMousePressed(vgui::MouseCode code)
{
	OnExitTextEditMode();
	CNeoDataSettings_Base *tab = m_pNdsBases[m_iNdsCurrent];
	if (m_iNdsHover >= 0 && m_iNdsHover < TAB__TOTAL)
	{
		m_iNdsCurrent = m_iNdsHover;
		m_iScrollOffset = 0;
		InvalidateLayout();
		PerformLayout();
		RequestFocus();
		return;
	}
	if (m_iBottomHover >= 0 && m_iBottomHover < BBTN__TOTAL)
	{
		OnBottomAction(static_cast<BottomBtns>(m_iBottomHover));
		return;
	}

	if (m_iNdvActive < 0 || m_iNdvActive >= tab->NdvListSize() || code != MOUSE_LEFT) return;
	CNeoDataVariant *ndv = &tab->NdvList()[m_iNdvActive];
	switch (ndv->type)
	{
	case CNeoDataVariant::RINGBOX:
	{
		if (m_curMouse == WDG_NONE || m_curMouse == WDG_CENTER) return;

		CNeoDataRingBox *rb = &ndv->ringBox;
		rb->iCurIdx += (m_curMouse == WDG_LEFT) ? -1 : +1;
		if (rb->iCurIdx < 0) rb->iCurIdx = (rb->iItemsSize - 1);
		else if (rb->iCurIdx >= rb->iItemsSize) rb->iCurIdx = 0;

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
	case CNeoDataVariant::BINDENTRY:
		if (m_curMouse == WDG_CENTER) OnEnterBindEntry(ndv);
		break;
	case CNeoDataVariant::S_MICTESTER:
	{
		IVoiceTweak_s *voiceTweak = engine->GetVoiceTweakAPI();
		voiceTweak->IsStillTweaking() ? (void)voiceTweak->EndVoiceTweakMode() : (void)voiceTweak->StartVoiceTweakMode();
	}
	break;
	}
}

void CNeoSettings_Dynamic::OnMouseDoublePressed(vgui::MouseCode code)
{
	OnExitTextEditMode();

	CNeoDataSettings_Base *tab = m_pNdsBases[m_iNdsCurrent];
	if (m_iNdvActive < 0 || m_iNdvActive >= tab->NdvListSize()) return;
	CNeoDataVariant *ndv = &tab->NdvList()[m_iNdvActive];
	switch (ndv->type)
	{
	case CNeoDataVariant::SLIDER:
	case CNeoDataVariant::TEXTENTRY:
	{
		if (m_curMouse == WDG_CENTER && code == MOUSE_LEFT)
		{
			m_bTextEditMode = true;
			RequestFocus();
		}
	}
	break;
	}
}

void CNeoSettings_Dynamic::OnMouseWheeled(int delta)
{
	int iScreenWidth, iScreenHeight;
	engine->GetScreenSize(iScreenWidth, iScreenHeight);

	const float flScrollSpeed = static_cast<float>(iScreenHeight) * 0.05f;

	CNeoDataSettings_Base *tab = m_pNdsBases[m_iNdsCurrent];
	const int iWidgetSectionTall = GetTall() - g_iRowTall - g_iRowTall;
	int iMaxScrollOffset = (tab->NdvListSize() * g_iRowTall) - iWidgetSectionTall;
	if (iMaxScrollOffset < 0) iMaxScrollOffset = 0;
	m_iScrollOffset += (int)((float)(-delta) * flScrollSpeed);
	m_iScrollOffset = clamp(m_iScrollOffset, 0, iMaxScrollOffset);
}

void CNeoSettings_Dynamic::OnKeyCodeTyped(vgui::KeyCode code)
{
	const bool bCtrlMod = (input()->IsKeyDown(KEY_LCONTROL) || input()->IsKeyDown(KEY_RCONTROL));
	BottomBtns bbBottomBtns = BBTN__TOTAL;
	if (code == KEY_ESCAPE)
	{
		bbBottomBtns = BBTN_BACK;
	}
	else if (code == KEY_F3 && (input()->IsKeyDown(KEY_LSHIFT) || input()->IsKeyDown(KEY_RSHIFT)))
	{
		bbBottomBtns = BBTN_LEGACY;
	}
	else if (code == KEY_S && bCtrlMod)
	{
		bbBottomBtns = BBTN_APPLY;
	}
	else if (code == KEY_Z && bCtrlMod)
	{
		bbBottomBtns = BBTN_RESET;
	}

	if (bbBottomBtns != BBTN__TOTAL)
	{
		OnBottomAction(bbBottomBtns);
		return;
	}

	CNeoDataSettings_Base *tab = m_pNdsBases[m_iNdsCurrent];
	const int iNdvListSize = tab->NdvListSize();
	if (code == KEY_DOWN || code == KEY_UP)
	{
		OnExitTextEditMode();
		const int iIncr = (code == KEY_DOWN) ? +1 : -1;

		// Increment or decrement to the next valid NDV
		bool bGoNext = true;
		while (bGoNext)
		{
			m_iNdvActive += iIncr;
			if (m_iNdvActive < 0) m_iNdvActive = (iNdvListSize - 1);
			else if (m_iNdvActive >= iNdvListSize) m_iNdvActive = 0;
			const auto type = tab->NdvList()[m_iNdvActive].type;
			bGoNext = (type == CNeoDataVariant::TEXTLABEL || type == CNeoDataVariant::S_DISPLAYNAME);
		}

		// Re-adjust scroll offset
		const int iWdgArea = GetTall() - g_iRowTall - g_iRowTall;
		const int iWdgYPos = g_iRowTall * m_iNdvActive;
		if (iWdgYPos < m_iScrollOffset)
		{
			m_iScrollOffset = iWdgYPos;
		}
		else if ((iWdgYPos + g_iRowTall) >= (m_iScrollOffset + iWdgArea))
		{
			m_iScrollOffset = (iWdgYPos - iWdgArea + g_iRowTall);
		}
		return;
	}
	else if (code == KEY_TAB)
	{
		OnExitTextEditMode();
		const bool bShift = (input()->IsKeyDown(KEY_LSHIFT) || input()->IsKeyDown(KEY_RSHIFT));
		const int iIncr = (bShift) ? -1 : +1;
		m_iNdsCurrent += iIncr;
		if (m_iNdsCurrent >= TAB__TOTAL) m_iNdsCurrent = 0;
		else if (m_iNdsCurrent < 0) m_iNdsCurrent = (TAB__TOTAL - 1);
		m_iNdvActive = 0;
		return;
	}

	if (m_iNdvActive < 0 || m_iNdvActive >= iNdvListSize) return;
	CNeoDataVariant *ndv = &tab->NdvList()[m_iNdvActive];

	if (code == KEY_LEFT || code == KEY_RIGHT)
	{
		switch (ndv->type)
		{
		case CNeoDataVariant::RINGBOX:
		{
			CNeoDataRingBox *rb = &ndv->ringBox;
			rb->iCurIdx += (code == KEY_LEFT) ? -1 : +1;
			if (rb->iCurIdx < 0) rb->iCurIdx = (rb->iItemsSize - 1);
			else if (rb->iCurIdx >= rb->iItemsSize) rb->iCurIdx = 0;
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
		if (ndv->type == CNeoDataVariant::SLIDER || ndv->type == CNeoDataVariant::TEXTENTRY)
		{
			if (m_bTextEditMode && ndv->type == CNeoDataVariant::SLIDER)
			{
				ndv->slider.ClampAndUpdate();
			}
			m_bTextEditMode = !m_bTextEditMode;
		}
		else if (ndv->type == CNeoDataVariant::BINDENTRY)
		{
			OnEnterBindEntry(ndv);
		}
		else if (ndv->type == CNeoDataVariant::S_MICTESTER)
		{
			IVoiceTweak_s *voiceTweak = engine->GetVoiceTweakAPI();
			voiceTweak->IsStillTweaking() ? (void)voiceTweak->EndVoiceTweakMode() : (void)voiceTweak->StartVoiceTweakMode();
		}
		return;
	}
	else
	{
		switch (ndv->type)
		{
		case CNeoDataVariant::SLIDER:
		{
			CNeoDataSlider *sl = &ndv->slider;
			if (m_bTextEditMode && code == KEY_BACKSPACE && sl->iWszCacheLabelSize > 0)
			{
				sl->wszCacheLabel[--sl->iWszCacheLabelSize] = '\0';
				m_bModified = true;
			}
		}
		break;
		case CNeoDataVariant::TEXTENTRY:
		{
			CNeoDataTextEntry *te = &ndv->textEntry;
			int iWszSize = V_wcslen(te->wszEntry);
			if (m_bTextEditMode && code == KEY_BACKSPACE && iWszSize > 0)
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

void CNeoSettings_Dynamic::OnKeyTyped(wchar_t unichar)
{
	CNeoDataSettings_Base *tab = m_pNdsBases[m_iNdsCurrent];
	if (m_iNdvActive < 0 || m_iNdvActive >= tab->NdvListSize() || !m_bTextEditMode) return;
	CNeoDataVariant *ndv = &tab->NdvList()[m_iNdvActive];
	switch (ndv->type)
	{
	case CNeoDataVariant::SLIDER:
	{
		CNeoDataSlider *sl = &ndv->slider;
		if (sl->iWszCacheLabelSize < sl->iChMax)
		{
			// Prevent dot usage if flMulti = 1.0f, aka integer direct
			const bool bHasDot = (sl->flMulti == 1.0f) || wmemchr(sl->wszCacheLabel, L'.', sl->iWszCacheLabelSize) != nullptr;
			if (iswdigit(unichar) || (!bHasDot && unichar == L'.'))
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
void CNeoSettings_Dynamic::OnCursorMoved(int x, int y)
{
	const int iPrevNdsActive = m_iNdvActive;
	m_iPosX = clamp(x, 0, GetWide());
	m_iNdvActive = -1;
	m_iNdsHover = -1;
	m_iBottomHover = -1;
	m_curMouse = WDG_NONE;

	if (x < 0 || x >= g_iRootSubPanelWide)
	{
		return;
	}

	CNeoDataSettings_Base *tab = m_pNdsBases[m_iNdsCurrent];
	if (y < g_iRowTall)
	{
		const int iTabWide = g_iRootSubPanelWide / TAB__TOTAL;
		m_iNdsHover = x / iTabWide;
		OnExitTextEditMode(iPrevNdsActive);
		return;
	}
	else if (y > (GetTall() - g_iRowTall))
	{
		const int iTabWide = g_iRootSubPanelWide / BBTN__TOTAL;
		m_iBottomHover = x / iTabWide;
		OnExitTextEditMode(iPrevNdsActive);
		return;
	}

	y -= g_iRowTall - m_iScrollOffset;
	m_iNdvActive = y / g_iRowTall;
	if (m_iNdvActive != iPrevNdsActive)
	{
		OnExitTextEditMode(iPrevNdsActive);
	}
	if (m_iNdvActive < 0 || m_iNdvActive >= tab->NdvListSize())
	{
		return;
	}

	const int wgXPos = static_cast<int>(g_iRootSubPanelWide * 0.4f);
	const int widgetWide = g_iRootSubPanelWide - wgXPos;
	const int widgetTall = g_iRowTall;

	CNeoDataVariant *ndv = &tab->NdvList()[m_iNdvActive];
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

void CNeoSettings_Dynamic::OnEnterBindEntry(CNeoDataVariant *ndv)
{
	if (m_opKeyCapture)
	{
		CNeoDataBindEntry *be = &ndv->bindEntry;
		engine->StartKeyTrapMode();
		m_opKeyCapture->m_iButtonCode = be->bcNext;
		m_opKeyCapture->m_iIndex = m_iNdvActive;
		V_swprintf_safe(m_opKeyCapture->m_wszBindingText, L"Change binding for: %ls", be->wszDisplayText);
		m_opKeyCapture->SetVisible(true);
		m_opKeyCapture->SetEnabled(true);
		m_opKeyCapture->PerformLayout();
		m_opKeyCapture->MoveToFront();
		m_opKeyCapture->RequestFocus();
	}
}

void CNeoSettings_Dynamic::OnBottomAction(BottomBtns btn)
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

void CNeoSettings_Dynamic::OnExitTextEditMode(const int iOverrideNdsActive)
{
	if (m_bTextEditMode)
	{
		const int iUneditNdv = (iOverrideNdsActive == -1) ? m_iNdvActive : iOverrideNdsActive;
		CNeoDataSettings_Base *tab = m_pNdsBases[m_iNdsCurrent];
		if (iUneditNdv >= 0 && iUneditNdv < tab->NdvListSize())
		{
			CNeoDataVariant *ndv = &tab->NdvList()[iUneditNdv];
			if (ndv->type == CNeoDataVariant::SLIDER)
			{
				ndv->slider.ClampAndUpdate();
			}
		}
	}
	m_bTextEditMode = false;
}

void CNeoSettings_Dynamic::ExitSettings()
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

void CNeoSettings_Dynamic::OnKeybindUpdate(KeyValues *data)
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

void CNeoSettings_Dynamic::OnConfirmDialogUpdate(KeyValues *data)
{
	(m_opConfirm->m_bChoice) ? UserSettingsSave() : UserSettingsRestore();
	g_pNeoRoot->m_state = CNeoRoot::STATE_ROOT;
	g_pNeoRoot->UpdateControls();
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

static constexpr int DLFILTER_SIZE = sizeof(DLFILTER_LABELS) / sizeof(wchar_t *);

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
			NDV_INIT_SLIDER(L"Viewmodel FOV Offset", -20, 40, 1, 1.0f, 2),
			NDV_INIT_RINGBOX_ONOFF(L"Aim hold"),
			NDV_INIT_RINGBOX_ONOFF(L"Reload empty"),
			NDV_INIT_RINGBOX_ONOFF(L"Right hand viewmodel"),
			NDV_INIT_RINGBOX_ONOFF(L"Show player spray"),
			NDV_INIT_RINGBOX_ONOFF(L"Show position"),
			NDV_INIT_RINGBOX(L"Show FPS", SHOWFPS_LABELS, sizeof(SHOWFPS_LABELS) / sizeof(wchar_t *)),
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
			NDV_INIT_SLIDER(L"Sensitivity", 0, 100, 1, 10.0f, 4),
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
			NDV_INIT_RINGBOX(L"Sound Setup", SPEAKER_CFG_LABELS, sizeof(SPEAKER_CFG_LABELS) / sizeof(wchar_t *)),
			NDV_INIT_RINGBOX(L"Sound Quality", QUALITY_LABELS, 3),
			NDV_INIT_RINGBOX_ONOFF(L"Mute Audio on un-focus"),
			NDV_INIT_RINGBOX_ONOFF(L"Voice Enabled"),
			NDV_INIT_SLIDER(L"Voice Receive", 0, 100, 5, SLT_VOL, 4),
			//NDV_INIT_SLIDER(L"Voice Send", 0, 100, 5, SLT_VOL, 4),
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
			NDV_INIT_RINGBOX(L"Window", WINDOW_MODE, sizeof(WINDOW_MODE) / sizeof(wchar_t *)),
			NDV_INIT_RINGBOX(L"Core Rendering", QUEUE_MODE, sizeof(QUEUE_MODE) / sizeof(wchar_t *)),
			NDV_INIT_RINGBOX(L"Model detail", QUALITY_LABELS, 3),
			NDV_INIT_RINGBOX(L"Texture detail", QUALITY_LABELS, 4),
			NDV_INIT_RINGBOX(L"Shader detail", QUALITY2_LABELS, 2),
			NDV_INIT_RINGBOX(L"Water detail", WATER_LABELS, sizeof(WATER_LABELS) / sizeof(wchar_t *)),
			NDV_INIT_RINGBOX(L"Shadow detail", QUALITY_LABELS, 3),
			NDV_INIT_RINGBOX(L"Color correction", ENABLED_LABELS, 2),
			NDV_INIT_RINGBOX(L"Anti-aliasing", MSAA_LABELS, sizeof(MSAA_LABELS) / sizeof(wchar_t *)),
			NDV_INIT_RINGBOX(L"Filtering mode", FILTERING_LABELS, FILTERING__TOTAL),
			NDV_INIT_RINGBOX(L"V-Sync", ENABLED_LABELS, 2),
			NDV_INIT_RINGBOX(L"Motion blur", ENABLED_LABELS, 2),
			NDV_INIT_RINGBOX(L"HDR", HDR_LABELS, sizeof(HDR_LABELS) / sizeof(wchar_t *)),
			NDV_INIT_SLIDER(L"Gamma", 160, 260, 1, 100.0f, 3),
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
	{ "#GameUI_GameMenu_FindServers", "OpenServerBrowser", FLAG_SHOWINGAME | FLAG_SHOWINMAIN },
	{ "#GameUI_GameMenu_CreateServer", "OpenCreateMultiplayerGameDialog", FLAG_SHOWINGAME | FLAG_SHOWINMAIN },
	{ "#GameUI_GameMenu_Disconnect", "Disconnect", FLAG_SHOWINGAME },
	{ "#GameUI_GameMenu_PlayerList", "OpenPlayerListDialog", FLAG_SHOWINGAME },
	{ "#GameUI_GameMenu_Options", nullptr, FLAG_SHOWINGAME | FLAG_SHOWINMAIN },
	{ "#GameUI_GameMenu_Quit", "Quit", FLAG_SHOWINGAME | FLAG_SHOWINMAIN },
};

CNeoRoot::CNeoRoot(VPANEL parent)
	: EditablePanel(nullptr, "NeoRootPanel")
	, m_panelSettings(new CNeoSettings_Dynamic(this))
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

	const bool bInSettings = m_state == STATE_SETTINGS;
	m_panelSettings->SetVisible(bInSettings);
	m_panelSettings->SetEnabled(bInSettings);
	if (bInSettings)
	{
		m_panelSettings->m_iNdvActive = -1;
		m_panelSettings->m_curMouse = CNeoSettings_Dynamic::WDG_NONE;
		m_panelSettings->PerformLayout();
		m_panelSettings->SetPos((wide / 2) - (g_iRootSubPanelWide / 2),
								(tall / 2) - (m_panelSettings->GetTall() / 2));
		m_panelSettings->RequestFocus();
		m_panelSettings->MoveToFront();
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
	g_iRootSubPanelWide = static_cast<int>(static_cast<float>(wide) * 0.65f);

	g_neoFont = m_hTextFonts[FONT_NTSMALL];
	m_opKeyCapture->m_fontMain = m_hTextFonts[FONT_NTNORMAL];
	m_opKeyCapture->m_fontSub = m_hTextFonts[FONT_NTSMALL];
	m_panelSettings->m_opConfirm->m_fontMain = m_hTextFonts[FONT_NTNORMAL];
	UpdateControls();
}

void CNeoRoot::Paint()
{
	int wide, tall;
	GetSize(wide, tall);

	const bool bNextIsInGame = engine->IsInGame();
	const bool bInSettings = m_state == STATE_SETTINGS;
	if (!bInSettings)
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

		surface()->DrawSetTextColor(Color(128, 128, 128, 255));
		surface()->DrawSetTextPos(iBtnPlaceXMid - (iTitleWidth / 2), yTopPos - iTitleHeight);
		surface()->DrawPrintText(WSZ_GAME_TITLE, (sizeof(WSZ_GAME_TITLE) / sizeof(wchar_t)) - 1);

		surface()->DrawSetTextColor(Color(255, 255, 255, 255));
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
			surface()->DrawPrintText(WSZ_NEWS_TITLE, WSZ_LEN(WSZ_NEWS_TITLE));

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
				surface()->DrawSetColor((m_iHoverBtn == i) ? Color(0, 0, 0, 255) : Color(40, 40, 40, 255));
				surface()->DrawFilledRect(btnPos.x, btnPos.y, btnPos.x + m_iBtnWide, btnPos.y + g_iRowTall);

				surface()->DrawSetTextPos(btnPos.x + g_iMarginX, btnPos.y + fontStartYPos);
				surface()->DrawPrintText(m_wszDispBtnTexts[i], m_iWszDispBtnTextsSizes[i]);

				btnPos.y += g_iRowTall;
			}
		}
	}

	// Draw version info (bottom left corner) - Always
	surface()->DrawSetTextColor(Color(255, 255, 255, 255));
	int textWidth, textHeight;
	surface()->DrawSetTextFont(m_hTextFonts[FONT_NTSMALL]);
	surface()->GetTextSize(m_hTextFonts[FONT_NTSMALL], BUILD_DISPLAY, textWidth, textHeight);

	surface()->DrawSetTextPos(g_iMarginX, tall - textHeight - g_iMarginY);
	surface()->DrawPrintText(BUILD_DISPLAY, *BUILD_DISPLAY_SIZE);
}

void CNeoRoot::OnMousePressed(vgui::MouseCode code)
{
	if (code != MOUSE_LEFT || m_iHoverBtn < 0 || m_iHoverBtn >= BTN__TOTAL) return;

	const auto btnInfo = BTNS_INFO[m_iHoverBtn];
	if (m_iHoverBtn == BTN_SETTINGS)
	{
		m_state = STATE_SETTINGS;
		m_panelSettings->UserSettingsRestore();
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
	return (g_pNeoRoot &&
		(g_pNeoRoot->m_panelSettings->IsVisible() || g_pNeoRoot->m_panelSettings->m_opConfirm->IsVisible()));
}
