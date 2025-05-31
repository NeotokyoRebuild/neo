#include "cbase.h"
#include "neo_root.h"
#include "IOverrideInterface.h"

#include "vgui/ILocalize.h"
#include "vgui/IPanel.h"
#include "vgui/ISurface.h"
#include "vgui/IVGui.h"
#include "ienginevgui.h"
#include <engine/IEngineSound.h>
#include "filesystem.h"
#include "neo_version_info.h"
#include "cdll_client_int.h"
#include <steam/steam_api.h>
#include <vgui_avatarimage.h>
#include <IGameUIFuncs.h>
#include <voice_status.h>
#include <c_playerresource.h>
#include <ivoicetweak.h>
#include "tier1/interface.h"
#include <ctime>
#include "ui/neo_loading.h"
#include "ui/neo_utils.h"
#include "neo_gamerules.h"
#include "neo_misc.h"

#include <vgui/IInput.h>
#include <vgui_controls/Controls.h>
#include <stb_image.h>

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

// TODO: Gamepad
//   Gamepad enable: joystick 0/1
//   Reverse up-down axis: joy_inverty 0/1
//   Swap sticks on dual-stick controllers: joy_movement_stick 0/1
//   Horizontal sens: joy_yawsensitivity: -0.5 to -7.0
//   Vertical sens: joy_pitchsensitivity: 0.5 to 7.0

using namespace vgui;

bool IsInGame()
{
	return (engine->IsInGame() && !engine->IsLevelMainMenuBackground());
}

// See interface.h/.cpp for specifics:  basically this ensures that we actually Sys_UnloadModule
// the dll and that we don't call Sys_LoadModule over and over again.
static CDllDemandLoader g_GameUIDLL("GameUI");

extern ConVar neo_name;
extern ConVar cl_onlysteamnick;
extern ConVar cl_neo_streamermode;
extern ConVar neo_clantag;

CNeoRoot *g_pNeoRoot = nullptr;
void NeoToggleconsole();
extern CNeoLoading *g_pNeoLoading;
inline NeoUI::Context g_uiCtx;
inline ConVar cl_neo_toggleconsole("cl_neo_toggleconsole", "1", FCVAR_ARCHIVE,
								   "If the console can be toggled with the ` keybind or not.", true, 0.0f, true, 1.0f);
inline int g_iRowsInScreen;

namespace {

int g_iAvatar = 64;
int g_iRootSubPanelWide = 600;
constexpr wchar_t WSZ_GAME_TITLE1[] = L"neAtBkyoC";
constexpr wchar_t WSZ_GAME_TITLE1_a[] = L"neAtBkyo";
constexpr wchar_t WSZ_GAME_TITLE1_b[] = L"C";
constexpr wchar_t WSZ_GAME_TITLE2[] = L"Hrebuild";
#define SZ_WEBSITE "https://neotokyorebuild.github.io"

ConCommand neo_toggleconsole("neo_toggleconsole", NeoToggleconsole, "toggle the console", FCVAR_DONTRECORD);

struct YMD
{
	YMD(const struct tm tm)
		: year(tm.tm_year + 1900)
		, month(tm.tm_mon + 1)
		, day(tm.tm_mday)
	{
	}

	bool operator==(const YMD &other) const
	{
		return year == other.year && month == other.month && day == other.day;
	}
	bool operator!=(const YMD &other) const
	{
		return !(*this == other);
	}

	int year;
	int month;
	int day;
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

CNeoRootInput::CNeoRootInput(CNeoRoot *rootPanel)
	: Panel(rootPanel, "NeoRootPanelInput")
	, m_pNeoRoot(rootPanel)
{
	MakePopup(true);
	SetKeyBoardInputEnabled(true);
	SetMouseInputEnabled(true);
	SetVisible(true);
	SetEnabled(true);
	PerformLayout();
}

void CNeoRootInput::PerformLayout()
{
	int iScrWide, iScrTall;
	vgui::surface()->GetScreenSize(iScrWide, iScrTall);
	SetPos(0, 0);
	SetSize(iScrWide, iScrTall);
	SetBgColor(COLOR_TRANSPARENT);
	SetFgColor(COLOR_TRANSPARENT);
}

void CNeoRootInput::OnKeyCodeTyped(vgui::KeyCode code)
{
	m_pNeoRoot->OnRelayedKeyCodeTyped(code);
}

void CNeoRootInput::OnKeyTyped(wchar_t unichar)
{
	m_pNeoRoot->OnRelayedKeyTyped(unichar);
}

void CNeoRootInput::OnMousePressed(vgui::MouseCode code)
{
	m_pNeoRoot->OnRelayedMousePressed(code);
}

void CNeoRootInput::OnMouseReleased(vgui::MouseCode code)
{
	m_pNeoRoot->OnRelayedMouseReleased(code);
}

void CNeoRootInput::OnMouseDoublePressed(vgui::MouseCode code)
{
	m_pNeoRoot->OnRelayedMouseDoublePressed(code);
}

void CNeoRootInput::OnMouseWheeled(int delta)
{
	m_pNeoRoot->OnRelayedMouseWheeled(delta);
}

void CNeoRootInput::OnCursorMoved(int x, int y)
{
	m_pNeoRoot->OnRelayedCursorMoved(x, y);
}

void CNeoRootInput::OnThink()
{
	ButtonCode_t code;
	if (engine->CheckDoneKeyTrapping(code))
	{
		if (code != KEY_ESCAPE)
		{
			if (code != KEY_DELETE)
			{
				// The keybind system used requires 1:1 so unbind any duplicates
				for (auto &bind : m_pNeoRoot->m_ns.keys.vBinds)
				{
					if (bind.bcNext == code)
					{
						bind.bcNext = BUTTON_CODE_NONE;
					}
				}
			}
			m_pNeoRoot->m_ns.keys.vBinds[m_pNeoRoot->m_iBindingIdx].bcNext =
					(code == KEY_DELETE) ? BUTTON_CODE_NONE : code;
			m_pNeoRoot->m_ns.bModified = true;
		}
		m_pNeoRoot->m_wszBindingText[0] = '\0';
		m_pNeoRoot->m_iBindingIdx = -1;
		m_pNeoRoot->m_state = STATE_SETTINGS;
		V_memcpy(g_uiCtx.iYOffset, m_pNeoRoot->m_iSavedYOffsets, NeoUI::SIZEOF_SECTIONS);
	}
}

constexpr WidgetInfo BTNS_INFO[BTNS_TOTAL] = {
	{ "#GameUI_GameMenu_ResumeGame", false, "ResumeGame", true, STATE__TOTAL, FLAG_SHOWINGAME },
	{ "#GameUI_GameMenu_FindServers", false, nullptr, true, STATE_SERVERBROWSER, FLAG_SHOWINGAME | FLAG_SHOWINMAIN },
	{ "#GameUI_GameMenu_CreateServer", false, nullptr, true, STATE_NEWGAME, FLAG_SHOWINGAME | FLAG_SHOWINMAIN },
	{ "#GameUI_GameMenu_Disconnect", false, "Disconnect", true, STATE__TOTAL, FLAG_SHOWINGAME },
	{ "#GameUI_GameMenu_PlayerList", false, nullptr, true, STATE_PLAYERLIST, FLAG_SHOWINGAME },
	{ "", true, nullptr, true, STATE__TOTAL, FLAG_SHOWINMAIN },
	{ "#GameUI_GameMenu_Tutorial", false, "sv_use_steam_networking 0; map ntre_class_tut", false, STATE__TOTAL, FLAG_SHOWINMAIN},
	{ "#GameUI_GameMenu_FiringRange", false, "sv_use_steam_networking 0; map ntre_shooting_tut", false, STATE__TOTAL, FLAG_SHOWINMAIN},
	{ "", true, nullptr, true, STATE__TOTAL, FLAG_SHOWINGAME | FLAG_SHOWINMAIN },
	{ "#GameUI_GameMenu_Options", false, nullptr, true, STATE_SETTINGS, FLAG_SHOWINGAME | FLAG_SHOWINMAIN },
	{ "#GameUI_GameMenu_Quit", false, nullptr, true, STATE_QUIT, FLAG_SHOWINGAME | FLAG_SHOWINMAIN },
};

CNeoRoot::CNeoRoot(VPANEL parent)
	: EditablePanel(nullptr, "NeoRootPanel")
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

	for (int i = 0; i < BTNS_TOTAL; ++i)
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

	NeoSettingsInit(&m_ns);
	{
		// Initialize map list
		FileFindHandle_t findHdl;
		for (const char *pszFilename = filesystem->FindFirst("maps/*.bsp", &findHdl);
			 pszFilename;
			 pszFilename = filesystem->FindNext(findHdl))
		{
			// Sanity check: In-case somehow someone named a directory as *.bsp in here
			if (!filesystem->FindIsDirectory(findHdl))
			{
				MapInfo mapInfo;
				int iSize = g_pVGuiLocalize->ConvertANSIToUnicode(pszFilename, mapInfo.wszName, sizeof(mapInfo.wszName));
				iSize -= sizeof(".bsp");
				mapInfo.wszName[iSize] = '\0';
				m_vWszMaps.AddToTail(mapInfo);
			}
		}
		filesystem->FindClose(findHdl);
	}
	for (int i = 0; i < GS__TOTAL; ++i)
	{
		m_serverBrowser[i].m_iType = static_cast<GameServerType>(i);
		m_serverBrowser[i].m_pSortCtx = &m_sortCtx;
	}

	// NEO TODO (nullsystem): What will happen in 2038? 64-bit Source 1 SDK when? Source 2 SDK when?
	// We could use GCC 64-bit compiled time_t or Win32 API direct to side-step IFileSystem "long" 32-bit
	// limitation for now. Although that could mess with the internal IFileSystem related API usages of time_t.
	// If _FILE_OFFSET_BITS=64 and _TIME_BITS=64 is set on Linux, time_t will be 64-bit even on 32-bit executable
	//
	// If news.txt doesn't exists, it'll just give 1970-01-01 which will always be different to ymdNow anyway
	const long lFileTime = filesystem->GetFileTime("news.txt");
	const time_t ttFileTime = lFileTime;
	struct tm tmFileStruct;
	const struct tm tmFile = *(Plat_localtime(&ttFileTime, &tmFileStruct));
	const YMD ymdFile{tmFile};

	struct tm tmNowStruct;
	const time_t tNow = time(nullptr);
	const struct tm tmNow = *(Plat_localtime(&tNow, &tmNowStruct));
	const YMD ymdNow{tmNow};

	// Read the cached file regardless of needing update or not
	if (filesystem->FileExists("news.txt"))
	{
		CUtlBuffer buf(0, 0, CUtlBuffer::TEXT_BUFFER | CUtlBuffer::READ_ONLY);
		if (filesystem->ReadFile("news.txt", nullptr, buf))
		{
			ReadNewsFile(buf);
		}
	}
	if (ymdFile != ymdNow)
	{
		ISteamHTTP *http = steamapicontext->SteamHTTP();
		HTTPRequestHandle httpReqHdl = http->CreateHTTPRequest(k_EHTTPMethodGET, SZ_WEBSITE "/news.txt");
		SteamAPICall_t httpReqCallback;
		http->SendHTTPRequest(httpReqHdl, &httpReqCallback);
		m_ccallbackHttp.Set(httpReqCallback, this, &CNeoRoot::HTTPCallbackRequest);
	}

	SetKeyBoardInputEnabled(true);
	SetMouseInputEnabled(true);
	UpdateControls();
	ivgui()->AddTickSignal(GetVPanel(), 200);
	ListenForGameEvent("server_spawn");
	ListenForGameEvent("game_newmap");

	vgui::IScheme *pScheme = vgui::scheme()->GetIScheme(neoscheme);
	ApplySchemeSettings(pScheme);
}

CNeoRoot::~CNeoRoot()
{
	if (g_pNeoRoot->m_pFileIODialog) g_pNeoRoot->m_pFileIODialog->DeletePanel();
	m_panelCaptureInput->DeletePanel();
	if (m_avImage) delete m_avImage;
	NeoSettingsDeinit(&m_ns);

	m_gameui = nullptr;
	g_GameUIDLL.Unload();

	NeoUI::FreeContext(&g_uiCtx);
}

IGameUI *CNeoRoot::GetGameUI()
{
	if (!m_gameui && !LoadGameUI()) return nullptr;
	return m_gameui;
}

void CNeoRoot::UpdateControls()
{
	if (m_state == STATE_ROOT)
	{
		auto hdlFont = g_uiCtx.fonts[NeoUI::FONT_LOGOSMALL].hdl;
		surface()->DrawSetTextFont(hdlFont);
		surface()->GetTextSize(hdlFont, WSZ_GAME_TITLE2, m_iTitleWidth, m_iTitleHeight);
	}
	g_uiCtx.iActive = NeoUI::FOCUSOFF_NUM;
	g_uiCtx.iActiveSection = -1;
	V_memset(g_uiCtx.iYOffset, 0, sizeof(g_uiCtx.iYOffset));
	m_ns.bBack = false;
	m_bShowBrowserLabel = false;
	RequestFocus();
	m_panelCaptureInput->RequestFocus();
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

	static constexpr const char *FONT_NAMES[NeoUI::FONT__TOTAL] = {
		"NeoUINormal", "NHudOCR", "NHudOCRSmallNoAdditive", "ClientTitleFont", "ClientTitleFontSmall",
		"NeoUILarge"
	};
	for (int i = 0; i < NeoUI::FONT__TOTAL; ++i)
	{
		g_uiCtx.fonts[i].hdl = pScheme->GetFont(FONT_NAMES[i], true);
	}

	// In 1080p, g_uiCtx.layout.iDefRowTall == 40, g_uiCtx.iMarginX = 10, g_iAvatar = 64,
	// other resolution scales up/down from it
	g_uiCtx.layout.iDefRowTall = tall / 27;
	g_iRowsInScreen = (tall * 0.85f) / g_uiCtx.layout.iDefRowTall;
	g_uiCtx.iMarginX = wide / 192;
	g_uiCtx.iMarginY = tall / 108;
	g_iAvatar = wide / 30;
	const float flWide = static_cast<float>(wide);
	m_flWideAs43 = static_cast<float>(tall) * (4.0f / 3.0f);
	if (m_flWideAs43 > flWide) m_flWideAs43 = flWide;
	g_iRootSubPanelWide = static_cast<int>(m_flWideAs43 * 0.9f);

	UpdateControls();
}

void CNeoRoot::Paint()
{
	OnMainLoop(NeoUI::MODE_PAINT);
}

void CNeoRoot::OnRelayedMousePressed(vgui::MouseCode code)
{
	g_uiCtx.eCode = code;
	OnMainLoop(NeoUI::MODE_MOUSEPRESSED);
}

void CNeoRoot::OnRelayedMouseReleased(vgui::MouseCode code)
{
	g_uiCtx.eCode = code;
	OnMainLoop(NeoUI::MODE_MOUSERELEASED);
}

void CNeoRoot::OnRelayedMouseDoublePressed(vgui::MouseCode code)
{
	g_uiCtx.eCode = code;
	OnMainLoop(NeoUI::MODE_MOUSEDOUBLEPRESSED);
}

void CNeoRoot::OnRelayedMouseWheeled(int delta)
{
	g_uiCtx.eCode = (delta > 0) ? MOUSE_WHEEL_UP : MOUSE_WHEEL_DOWN;
	OnMainLoop(NeoUI::MODE_MOUSEWHEELED);
}

void CNeoRoot::OnRelayedCursorMoved(int x, int y)
{
	g_uiCtx.iMouseAbsX = x;
	g_uiCtx.iMouseAbsY = y;
	OnMainLoop(NeoUI::MODE_MOUSEMOVED);
}

void CNeoRoot::OnTick()
{
	if (m_state == STATE_SERVERBROWSER)
	{
		if (m_bSBFiltModified)
		{
			// Pass modified over to the tabs so it doesn't trigger
			// the filter refresh immeditely
			for (int i = 0; i < GS__TOTAL; ++i)
			{
				m_serverBrowser[i].m_bModified = true;
			}
			m_bSBFiltModified = false;
		}

		auto *pSbTab = &m_serverBrowser[m_iServerBrowserTab];
		if (pSbTab->m_bModified)
		{
			pSbTab->UpdateFilteredList();
			pSbTab->m_bModified = false;
		}
	}
	else if (m_state == STATE_SERVERDETAILS)
	{
		if (m_bSPlayersSortModified)
		{
			m_serverPlayers.UpdateSortedList();
			m_bSPlayersSortModified = false;
		}
	}
}

void CNeoRoot::FireGameEvent(IGameEvent *event)
{
	const char *type = event->GetName();
	if (Q_strcmp(type, "server_spawn") == 0)
	{
		g_pVGuiLocalize->ConvertANSIToUnicode(event->GetString("hostname"), m_wszHostname, sizeof(m_wszHostname));
	}
	else if (Q_strcmp(type, "game_newmap") == 0)
	{
		g_pVGuiLocalize->ConvertANSIToUnicode(event->GetString("mapname"), m_wszMap, sizeof(m_wszMap));
	}
}

void CNeoRoot::OnRelayedKeyCodeTyped(vgui::KeyCode code)
{
	if (m_ns.keys.bcConsole <= KEY_NONE)
	{
		m_ns.keys.bcConsole = gameuifuncs->GetButtonCodeForBind("neo_toggleconsole");
		m_ns.keys.bcTeamMenu = gameuifuncs->GetButtonCodeForBind("teammenu");
		m_ns.keys.bcClassMenu = gameuifuncs->GetButtonCodeForBind("classmenu");
		m_ns.keys.bcLoadoutMenu = gameuifuncs->GetButtonCodeForBind("loadoutmenu");
	}

	if (code == m_ns.keys.bcConsole && code != KEY_BACKQUOTE)
	{
		// NEO JANK (nullsystem): Prevent toggle being handled twice causing it to not really open.
		// This can happen if using the default ` due to the engine enacting this all the time, however calling
		// NeoToggleconsole here is required for non-` keys that is toggled while the root panel is opened. The
		// engine will call non-` keys by itself it it's in game and the root panel is not opened, or the console is
		// opened which generally doesn't endup calling OnRelayedKeyCodeTyped anyway.
		NeoToggleconsole();
		return;
	}
	g_uiCtx.eCode = code;
	OnMainLoop(NeoUI::MODE_KEYPRESSED);
}

void CNeoRoot::OnRelayedKeyTyped(wchar_t unichar)
{
	g_uiCtx.unichar = unichar;
	OnMainLoop(NeoUI::MODE_KEYTYPED);
}

void CNeoRoot::OnMainLoop(const NeoUI::Mode eMode)
{
	int wide, tall;
	GetSize(wide, tall);

	const RootState ePrevState = m_state;

	// Laading screen just overlays over the root, so don't render anything else if so
	if (!m_bOnLoadingScreen)
	{
		static constexpr void (CNeoRoot::*P_FN_MAIN_LOOP[STATE__TOTAL])(const MainLoopParam param) = {
			&CNeoRoot::MainLoopRoot,			// STATE_ROOT
			&CNeoRoot::MainLoopSettings,		// STATE_SETTINGS
			&CNeoRoot::MainLoopNewGame,			// STATE_NEWGAME
			&CNeoRoot::MainLoopServerBrowser,	// STATE_SERVERBROWSER

			&CNeoRoot::MainLoopMapList,			// STATE_MAPLIST
			&CNeoRoot::MainLoopServerDetails,	// STATE_SERVERDETAILS
			&CNeoRoot::MainLoopPlayerList,		// STATE_PLAYERLIST
			&CNeoRoot::MainLoopSprayPicker,		// STATE_SPRAYPICKER
			&CNeoRoot::MainLoopSprayPicker,		// STATE_SPRAYDELETER

			&CNeoRoot::MainLoopPopup,			// STATE_KEYCAPTURE
			&CNeoRoot::MainLoopPopup,			// STATE_CONFIRMSETTINGS
			&CNeoRoot::MainLoopPopup,			// STATE_QUIT
			&CNeoRoot::MainLoopPopup,			// STATE_SERVERPASSWORD
			&CNeoRoot::MainLoopPopup,			// STATE_SETTINGSRESETDEFAULT
			&CNeoRoot::MainLoopPopup,			// STATE_SPRAYDELETERCONFIRM
		};
		(this->*P_FN_MAIN_LOOP[m_state])(MainLoopParam{.eMode = eMode, .wide = wide, .tall = tall});

		if (m_state != ePrevState)
		{
			if (ePrevState == STATE_SETTINGS)
			{
				V_memcpy(m_iSavedYOffsets, g_uiCtx.iYOffset, NeoUI::SIZEOF_SECTIONS);
			}
			UpdateControls();
			if (m_state == STATE_SETTINGS && ePrevState >= STATE__POPUPSTART && ePrevState < STATE__TOTAL)
			{
				V_memcpy(g_uiCtx.iYOffset, m_iSavedYOffsets, NeoUI::SIZEOF_SECTIONS);
			}
		}
	}

	if (eMode == NeoUI::MODE_PAINT)
	{
		// Draw version info (bottom left corner) - Always
		surface()->DrawSetTextColor(COLOR_NEOPANELTEXTBRIGHT);
		int textWidth, textHeight;
		surface()->DrawSetTextFont(g_uiCtx.fonts[NeoUI::FONT_NTNORMAL].hdl);
		surface()->GetTextSize(g_uiCtx.fonts[NeoUI::FONT_NTNORMAL].hdl, BUILD_DISPLAY, textWidth, textHeight);

		surface()->DrawSetTextPos(g_uiCtx.iMarginX, tall - textHeight - g_uiCtx.iMarginY);
		surface()->DrawPrintText(BUILD_DISPLAY, wcslen(BUILD_DISPLAY));
	}
}

void CNeoRoot::MainLoopRoot(const MainLoopParam param)
{
	int iTitleNWidth, iTitleNHeight;
	const int iBtnPlaceXMid = (param.wide / 6);
	const int iMargin = (6 * g_uiCtx.iMarginX);
	const int iMarginHalf = iMargin * 0.5;
	const int iTitleMarginTop = (param.tall * 0.2);
	surface()->GetTextSize(g_uiCtx.fonts[NeoUI::FONT_LOGO].hdl, L"n", iTitleNWidth, iTitleNHeight);
	g_uiCtx.dPanel.wide = (m_iTitleWidth) +iMargin;
	g_uiCtx.dPanel.tall = param.tall;
	g_uiCtx.dPanel.x = iBtnPlaceXMid - (m_iTitleWidth * 0.5) + (iTitleNWidth * 1.16) - iMarginHalf;
	g_uiCtx.dPanel.y = iTitleMarginTop + (2 * iTitleNHeight);
	g_uiCtx.bgColor = Color(0, 0, 0, 0);

	vgui::surface()->DrawSetColor(COLOR_NEOPANELNORMALBG);
	vgui::surface()->DrawFilledRect(g_uiCtx.dPanel.x, 0,
									g_uiCtx.dPanel.x + g_uiCtx.dPanel.wide, param.tall);

	NeoUI::BeginContext(&g_uiCtx, param.eMode, nullptr, "CtxRoot");
	NeoUI::BeginSection(true);
	{
		g_uiCtx.eButtonTextStyle = NeoUI::TEXTSTYLE_CENTER;
		const int iFlagToMatch = IsInGame() ? FLAG_SHOWINGAME : FLAG_SHOWINMAIN;
		for (int i = 0; i < BTNS_TOTAL; ++i)
		{
			const auto btnInfo = BTNS_INFO[i];
			if (btnInfo.flags & iFlagToMatch)
			{
				if (btnInfo.isFake)
				{
					NeoUI::Pad();
					continue;
				}
				const auto retBtn = NeoUI::Button(m_wszDispBtnTexts[i]);
				if (retBtn.bPressed || (i == MMBTN_QUIT && !IsInGame() && NeoUI::Bind(KEY_ESCAPE)))
				{
					surface()->PlaySound("ui/buttonclickrelease.wav");
					if (btnInfo.command)
					{
						m_state = STATE_ROOT;
						if (btnInfo.isMainMenuCommand)
						{
							GetGameUI()->SendMainMenuCommand(btnInfo.command);
						}
						else
						{
							engine->ClientCmd(btnInfo.command);
						}
					}
					else if (btnInfo.nextState < STATE__TOTAL)
					{
						m_state = btnInfo.nextState;
						if (m_state == STATE_SETTINGS)
						{
							NeoSettingsRestore(&m_ns);
						}
					}
				}
				if (retBtn.bMouseHover && i != m_iHoverBtn)
				{
					// Sound rollover feedback
					surface()->PlaySound("ui/buttonrollover.wav");
					m_iHoverBtn = i;
				}
			}
		}
	}
	NeoUI::EndSection();
	g_uiCtx.bgColor = COLOR_TRANSPARENT;

	const int iBtnWide = m_iTitleWidth + iMargin;
	const int iRightXPos = iBtnPlaceXMid + (iBtnWide / 2) + iMarginHalf;
	int iRightSideYStart = (iTitleMarginTop + (2 * iTitleNHeight));

	// Draw top steam section portion
	{
		// Draw title
		int iFirstWidth, iFirstHeight;
		int iDropShadowOffset = MIN(8, param.wide * 0.005);
		surface()->DrawSetTextFont(g_uiCtx.fonts[NeoUI::FONT_LOGO].hdl);
		surface()->GetTextSize(g_uiCtx.fonts[NeoUI::FONT_LOGO].hdl, WSZ_GAME_TITLE1_a, iFirstWidth, iFirstHeight);
		surface()->DrawSetTextColor(COLOR_BLACK);
		surface()->DrawSetTextPos(iBtnPlaceXMid - (m_iTitleWidth * 0.5) - iDropShadowOffset, iTitleMarginTop + iDropShadowOffset);
		surface()->DrawPrintText(WSZ_GAME_TITLE1, SZWSZ_LEN(WSZ_GAME_TITLE1));
		surface()->DrawSetTextColor(COLOR_NEOTITLE);
		surface()->DrawSetTextPos(iBtnPlaceXMid - (m_iTitleWidth * 0.5), iTitleMarginTop);
		surface()->DrawPrintText(WSZ_GAME_TITLE1_a, SZWSZ_LEN(WSZ_GAME_TITLE1_a));
		surface()->DrawSetTextColor(COLOR_RED);
		surface()->DrawPrintText(WSZ_GAME_TITLE1_b, SZWSZ_LEN(WSZ_GAME_TITLE1_b));

		surface()->DrawSetTextColor(COLOR_BLACK);
		surface()->DrawSetTextPos(iBtnPlaceXMid - (m_iTitleWidth * 0.5) + (iTitleNWidth * 1.16) - iDropShadowOffset, iTitleMarginTop + m_iTitleHeight + iDropShadowOffset);
		surface()->DrawSetTextFont(g_uiCtx.fonts[NeoUI::FONT_LOGOSMALL].hdl);
		surface()->DrawPrintText(WSZ_GAME_TITLE2, SZWSZ_LEN(WSZ_GAME_TITLE2));
		surface()->DrawSetTextPos(iBtnPlaceXMid - (m_iTitleWidth * 0.5) + (iTitleNWidth * 1.16) - iDropShadowOffset, iTitleMarginTop + m_iTitleHeight + iDropShadowOffset);
		surface()->DrawPrintText(L"G", SZWSZ_LEN(L"G"));

		surface()->DrawSetTextColor(COLOR_NEOTITLE);
		surface()->DrawSetTextPos(iBtnPlaceXMid - (m_iTitleWidth * 0.5) + (iTitleNWidth * 1.16), iTitleMarginTop + m_iTitleHeight);
		surface()->DrawSetTextFont(g_uiCtx.fonts[NeoUI::FONT_LOGOSMALL].hdl);
		surface()->DrawPrintText(WSZ_GAME_TITLE2, SZWSZ_LEN(WSZ_GAME_TITLE2));
		surface()->DrawSetTextColor(COLOR_RED);
		surface()->DrawSetTextPos(iBtnPlaceXMid - (m_iTitleWidth * 0.5) + (iTitleNWidth * 1.16), iTitleMarginTop + m_iTitleHeight);
		surface()->DrawPrintText(L"G", SZWSZ_LEN(L"G"));
		surface()->DrawSetTextFont(g_uiCtx.fonts[NeoUI::FONT_NTNORMAL].hdl);
	}

#if (0)	// NEO TODO (Adam) place the current player info in the top right corner maybe?
	{
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
			m_avImage->SetPos(iSteamPlaceXStart + g_uiCtx.iMarginX, iRightSideYStart + g_uiCtx.iMarginY);
			m_avImage->SetSize(g_iAvatar, g_iAvatar);
			m_avImage->Paint();

			wchar_t wszDisplayName[NEO_MAX_DISPLAYNAME];
			GetClNeoDisplayName(wszDisplayName, neo_name.GetString(), neo_clantag.GetString(), cl_onlysteamnick.GetBool());

			const char *szNeoName = neo_name.GetString();
			const bool bUseNeoName = (szNeoName && szNeoName[0] != '\0' && !cl_onlysteamnick.GetBool());

			int iMainTextWidth, iMainTextHeight;
			surface()->DrawSetTextFont(g_uiCtx.fonts[NeoUI::FONT_NTHEADING].hdl);
			surface()->GetTextSize(g_uiCtx.fonts[NeoUI::FONT_NTHEADING].hdl, wszDisplayName, iMainTextWidth, iMainTextHeight);

			const int iMainTextStartPosX = g_uiCtx.iMarginX + g_iAvatar + g_uiCtx.iMarginX;
			surface()->DrawSetTextPos(iSteamPlaceXStart + iMainTextStartPosX, iRightSideYStart + g_uiCtx.iMarginY);
			surface()->DrawPrintText(wszDisplayName, V_wcslen(wszDisplayName));

			if (bUseNeoName)
			{
				char szTextBuf[64];
				V_sprintf_safe(szTextBuf, "(Steam name: %s)", steamFriends->GetPersonaName());
				wchar_t wszTextBuf[64] = {};
				g_pVGuiLocalize->ConvertANSIToUnicode(szTextBuf, wszTextBuf, sizeof(wszTextBuf));

				int iSteamSubTextWidth, iSteamSubTextHeight;
				surface()->DrawSetTextFont(g_uiCtx.fonts[NeoUI::FONT_NTNORMAL].hdl);
				surface()->GetTextSize(g_uiCtx.fonts[NeoUI::FONT_NTNORMAL].hdl, wszTextBuf, iSteamSubTextWidth, iSteamSubTextHeight);

				const int iRightOfNicknameXPos = iSteamPlaceXStart + iMainTextStartPosX + iMainTextWidth + g_uiCtx.iMarginX;
				// If we have space on the right, set it, otherwise on top of nickname
				if ((iRightOfNicknameXPos + iSteamSubTextWidth) < param.wide)
				{
					surface()->DrawSetTextPos(iRightOfNicknameXPos, iRightSideYStart + g_uiCtx.iMarginY);
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
				wchar_t wszStatusTotal[48];
				const int iStatusTotalSize = V_swprintf_safe(wszStatusTotal, L"%ls%ls",
								WSZ_PERSONA_STATES[static_cast<int>(eCurStatus)],
								cl_neo_streamermode.GetBool() ? L" [Streamer mode on]" : L"");
				const int iStatusTextStartPosY = g_uiCtx.iMarginY + iMainTextHeight + g_uiCtx.iMarginY;

				[[maybe_unused]] int iStatusWide;
				surface()->DrawSetTextFont(g_uiCtx.fonts[NeoUI::FONT_NTNORMAL].hdl);
				surface()->GetTextSize(g_uiCtx.fonts[NeoUI::FONT_NTNORMAL].hdl, wszStatusTotal, iStatusWide, iStatusTall);
				surface()->DrawSetTextPos(iSteamPlaceXStart + iMainTextStartPosX,
										  iRightSideYStart + iStatusTextStartPosY);
				surface()->DrawPrintText(wszStatusTotal, iStatusTotalSize);
			}

			// Put the news title in either from avatar or text end Y position
			const int iTextTotalTall = iMainTextHeight + iStatusTall;
			iRightSideYStart += (g_uiCtx.iMarginX * 2) + ((iTextTotalTall > g_iAvatar) ? iTextTotalTall : g_iAvatar);
		}
	}
#endif

#if (0) // NEO TODO (Adam) some kind of drop down for the news section, better position the current server info etc.
	g_uiCtx.dPanel.x = iRightXPos;
	g_uiCtx.dPanel.y = iRightSideYStart;
	if (IsInGame())
	{
		g_uiCtx.dPanel.wide = m_flWideAs43 * 0.7f;
	}
	else
	{
		g_uiCtx.dPanel.wide = GetWide() - iRightXPos - (g_uiCtx.iMarginX * 2);
	}
	NeoUI::BeginSection();
	{
		if (IsInGame())
		{
			// Show the current server's information
			NeoUI::Label(L"Hostname:", m_wszHostname);
			NeoUI::Label(L"Map:", m_wszMap);
			NeoUI::Label(L"Game mode:", NEO_GAME_TYPE_DESC_STRS[NEORules()->GetGameType()].wszStr);
			// TODO: more info, g_PR, scoreboard stuff, etc...
		}
		else
		{
			// Show the news
			NeoUI::SwapFont(NeoUI::FONT_NTHEADING);
			NeoUI::Label(L"News");
			NeoUI::SwapFont(NeoUI::FONT_NTNORMAL);

			g_uiCtx.eButtonTextStyle = NeoUI::TEXTSTYLE_LEFT;
			NeoUI::SwapColorNormal(COLOR_TRANSPARENT);
			for (int i = 0; i < m_iNewsSize; ++i)
			{
				if (NeoUI::Button(m_news[i].wszTitle).bPressed)
				{
					NeoUI::OpenURL(SZ_WEBSITE, m_news[i].szSitePath);
					m_bShowBrowserLabel = true;
				}
			}

			if (m_bShowBrowserLabel)
			{
				surface()->DrawSetTextColor(Color(178, 178, 178, 178));
				NeoUI::Label(L"Link opened in your web browser");
				surface()->DrawSetTextColor(COLOR_NEOPANELTEXTNORMAL);
			}

			g_uiCtx.eButtonTextStyle = NeoUI::TEXTSTYLE_CENTER;
			NeoUI::SwapColorNormal(COLOR_NEOPANELACCENTBG);
		}
	}
	NeoUI::EndSection();
#endif
}

void CNeoRoot::MainLoopSettings(const MainLoopParam param)
{
	struct NeoSettingsFunc
	{
		void (*func)(NeoSettings *);
		bool bUISectionManaged;
	};
	static constexpr NeoSettingsFunc P_FN[] = {
		{NeoSettings_General, false},
		{NeoSettings_Keys, false},
		{NeoSettings_Mouse, false},
		{NeoSettings_Audio, false},
		{NeoSettings_Video, false},
		{NeoSettings_Crosshair, true},
	};
	static const wchar_t *WSZ_TABS_LABELS[ARRAYSIZE(P_FN)] = {
		L"Multiplayer", L"Keybinds", L"Mouse", L"Audio", L"Video", L"Crosshair"
	};

	m_ns.iNextBinding = -1;

	const int iTallTotal = g_uiCtx.layout.iRowTall * (g_iRowsInScreen + 2);

	g_uiCtx.dPanel.wide = g_iRootSubPanelWide;
	g_uiCtx.dPanel.x = (param.wide / 2) - (g_iRootSubPanelWide / 2);
	g_uiCtx.dPanel.y = (param.tall / 2) - (iTallTotal / 2);
	g_uiCtx.dPanel.tall = g_uiCtx.layout.iRowTall;
	g_uiCtx.bgColor = COLOR_NEOPANELFRAMEBG;
	NeoUI::BeginContext(&g_uiCtx, param.eMode, g_pNeoRoot->m_wszDispBtnTexts[MMBTN_OPTIONS], "CtxOptions");
	{
		NeoUI::BeginSection();
		{
			NeoUI::Tabs(WSZ_TABS_LABELS, ARRAYSIZE(WSZ_TABS_LABELS), &m_ns.iCurTab);
		}
		NeoUI::EndSection();
		if (!P_FN[m_ns.iCurTab].bUISectionManaged)
		{
			g_uiCtx.dPanel.y += g_uiCtx.dPanel.tall;
			g_uiCtx.dPanel.tall = g_uiCtx.layout.iRowTall * g_iRowsInScreen;
			NeoUI::BeginSection(true);
			NeoUI::SetPerRowLayout(2, NeoUI::ROWLAYOUT_TWOSPLIT);
		}
		{
			P_FN[m_ns.iCurTab].func(&m_ns);
		}
		if (!P_FN[m_ns.iCurTab].bUISectionManaged)
		{
			NeoUI::EndSection();
		}
		g_uiCtx.dPanel.y += g_uiCtx.dPanel.tall;
		g_uiCtx.dPanel.tall = g_uiCtx.layout.iDefRowTall;
		NeoUI::BeginSection();
		g_uiCtx.eButtonTextStyle = NeoUI::TEXTSTYLE_CENTER;
		NeoUI::SetPerRowLayout(5);
		{
			NeoUI::SwapFont(NeoUI::FONT_NTHORIZSIDES);
			if (NeoUI::Button(L"Back (ESC)").bPressed || NeoUI::Bind(KEY_ESCAPE))
			{
				m_ns.bBack = true;
			}
			if (NeoUI::Button(L"Legacy").bPressed)
			{
				g_pNeoRoot->GetGameUI()->SendMainMenuCommand("OpenOptionsDialog");
			}
			if (NeoUI::Button(L"Default").bPressed)
			{
				m_state = STATE_SETTINGSRESETDEFAULT;
				engine->GetVoiceTweakAPI()->EndVoiceTweakMode();
			}
			if (m_ns.bModified)
			{
				if (NeoUI::Button(L"Revert").bPressed)
				{
					NeoSettingsRestore(&m_ns);
				}
				if (NeoUI::Button(L"Accept").bPressed)
				{
					NeoSettingsSave(&m_ns);
				}
			}
			NeoUI::SwapFont(NeoUI::FONT_NTNORMAL);
		}
		NeoUI::EndSection();
	}
	NeoUI::EndContext();
	if (!m_ns.bModified && g_uiCtx.bValueEdited)
	{
		m_ns.bModified = true;
	}

	if (m_ns.bBack)
	{
		m_ns.bBack = false;
		m_state = (m_ns.bModified) ? STATE_CONFIRMSETTINGS : STATE_ROOT;
		engine->GetVoiceTweakAPI()->EndVoiceTweakMode();
	}
	else if (m_ns.iNextBinding >= 0)
	{
		m_iBindingIdx = m_ns.iNextBinding;
		m_ns.iNextBinding = -1;
		V_swprintf_safe(m_wszBindingText, L"Change binding for: %ls",
						m_ns.keys.vBinds[m_iBindingIdx].wszDisplayText);
		m_state = STATE_KEYCAPTURE;
		engine->StartKeyTrapMode();
	}
}

void CNeoRoot::MainLoopNewGame(const MainLoopParam param)
{
	const int iTallTotal = g_uiCtx.layout.iRowTall * (g_iRowsInScreen + 2);
	g_uiCtx.dPanel.wide = g_iRootSubPanelWide;
	g_uiCtx.dPanel.x = (param.wide / 2) - (g_iRootSubPanelWide / 2);
	g_uiCtx.dPanel.y = (param.tall / 2) - (iTallTotal / 2);
	g_uiCtx.dPanel.tall = g_uiCtx.layout.iRowTall * (g_iRowsInScreen + 1);
	g_uiCtx.bgColor = COLOR_NEOPANELFRAMEBG;
	NeoUI::BeginContext(&g_uiCtx, param.eMode, m_wszDispBtnTexts[MMBTN_CREATESERVER], "CtxNewGame");
	{
		NeoUI::BeginSection(true);
		{
			NeoUI::SetPerRowLayout(2, NeoUI::ROWLAYOUT_TWOSPLIT);
			if (NeoUI::Button(L"Map", m_newGame.wszMap).bPressed)
			{
				m_state = STATE_MAPLIST;
			}
			NeoUI::TextEdit(L"Hostname", m_newGame.wszHostname, SZWSZ_LEN(m_newGame.wszHostname));
			NeoUI::SliderInt(L"Max players", &m_newGame.iMaxPlayers, 1, 32);
			NeoUI::TextEdit(L"Password", m_newGame.wszPassword, SZWSZ_LEN(m_newGame.wszPassword));
			NeoUI::RingBoxBool(L"Friendly fire", &m_newGame.bFriendlyFire);
			NeoUI::RingBoxBool(L"Use Steam networking", &m_newGame.bUseSteamNetworking);
		}
		NeoUI::EndSection();
		g_uiCtx.dPanel.y += g_uiCtx.dPanel.tall;
		g_uiCtx.dPanel.tall = g_uiCtx.layout.iRowTall;
		NeoUI::BeginSection();
		{
			NeoUI::SwapFont(NeoUI::FONT_NTHORIZSIDES);
			g_uiCtx.eButtonTextStyle = NeoUI::TEXTSTYLE_CENTER;
			NeoUI::SetPerRowLayout(5);
			{
				if (NeoUI::Button(L"Back (ESC)").bPressed || NeoUI::Bind(KEY_ESCAPE))
				{
					m_state = STATE_ROOT;
				}
				NeoUI::Pad();
				NeoUI::Pad();
				NeoUI::Pad();
				if (NeoUI::Button(L"Start").bPressed)
				{
					if (IsInGame())
					{
						engine->ClientCmd_Unrestricted("disconnect");
					}

					static constexpr int ENTRY_MAX = 64;
					char szMap[ENTRY_MAX + 1] = {};
					char szHostname[ENTRY_MAX + 1] = {};
					char szPassword[ENTRY_MAX + 1] = {};
					g_pVGuiLocalize->ConvertUnicodeToANSI(m_newGame.wszMap, szMap, sizeof(szMap));
					g_pVGuiLocalize->ConvertUnicodeToANSI(m_newGame.wszHostname, szHostname, sizeof(szHostname));
					g_pVGuiLocalize->ConvertUnicodeToANSI(m_newGame.wszPassword, szPassword, sizeof(szPassword));

					ConVarRef("hostname").SetValue(szHostname);
					ConVarRef("sv_password").SetValue(szPassword);
					ConVarRef("mp_friendlyfire").SetValue(m_newGame.bFriendlyFire);
					ConVarRef("sv_use_steam_networking").SetValue(m_newGame.bUseSteamNetworking);

					char cmdStr[256];
					V_sprintf_safe(cmdStr, "maxplayers %d; progress_enable; map \"%s\"", m_newGame.iMaxPlayers, szMap);
					engine->ClientCmd_Unrestricted(cmdStr);

					if (g_pNeoLoading)
					{
						V_wcscpy_safe(g_pNeoLoading->m_wszLoadingMap, m_newGame.wszMap);
					}

					m_state = STATE_ROOT;
				}
			}
			NeoUI::SwapFont(NeoUI::FONT_NTNORMAL);
		}
		NeoUI::EndSection();
	}
	NeoUI::EndContext();
}

static constexpr const wchar_t *SBLABEL_NAMES[GSIW__TOTAL] = {
	L"Lock", L"VAC", L"Name", L"Map", L"Players", L"Ping",
};
static const int ROWLAYOUT_TABLESPLIT[GSIW__TOTAL] = {
	8, 8, 40, 20, 12, -1
};

static void ServerBrowserDrawRow(const gameserveritem_t &server)
{
	int xPos = 0;
	const int fontStartYPos = g_uiCtx.fonts[g_uiCtx.eFont].iYOffset;

	if (server.m_bPassword)
	{
		vgui::surface()->DrawSetTextPos(g_uiCtx.rWidgetArea.x0 + xPos + g_uiCtx.iMarginX, g_uiCtx.rWidgetArea.y0 + fontStartYPos);
		vgui::surface()->DrawPrintText(L"P", 1);
	}
	xPos += static_cast<int>(g_uiCtx.irWidgetWide * (ROWLAYOUT_TABLESPLIT[GSIW_LOCKED] / 100.0f));

	if (server.m_bSecure)
	{
		vgui::surface()->DrawSetTextPos(g_uiCtx.rWidgetArea.x0 + xPos + g_uiCtx.iMarginX, g_uiCtx.rWidgetArea.y0 + fontStartYPos);
		vgui::surface()->DrawPrintText(L"S", 1);
	}
	xPos += static_cast<int>(g_uiCtx.irWidgetWide * (ROWLAYOUT_TABLESPLIT[GSIW_VAC] / 100.0f));

	{
		wchar_t wszServerName[k_cbMaxGameServerName];
		const int iSize = g_pVGuiLocalize->ConvertANSIToUnicode(server.GetName(), wszServerName, sizeof(wszServerName));

		vgui::surface()->DrawSetTextPos(g_uiCtx.rWidgetArea.x0 + xPos + g_uiCtx.iMarginX, g_uiCtx.rWidgetArea.y0 + fontStartYPos);
		vgui::surface()->DrawPrintText(wszServerName, iSize - 1);
	}
	xPos += static_cast<int>(g_uiCtx.irWidgetWide * (ROWLAYOUT_TABLESPLIT[GSIW_NAME] / 100.0f));

	{
		// In lower resolution, it may overlap from name, so paint a background here
		vgui::surface()->DrawFilledRect(g_uiCtx.rWidgetArea.x0 + xPos, g_uiCtx.rWidgetArea.y0,
										g_uiCtx.rWidgetArea.x1, g_uiCtx.rWidgetArea.y1);

		wchar_t wszMapName[k_cbMaxGameServerMapName];
		const int iSize = g_pVGuiLocalize->ConvertANSIToUnicode(server.m_szMap, wszMapName, sizeof(wszMapName));

		vgui::surface()->DrawSetTextPos(g_uiCtx.rWidgetArea.x0 + xPos + g_uiCtx.iMarginX, g_uiCtx.rWidgetArea.y0 + fontStartYPos);
		vgui::surface()->DrawPrintText(wszMapName, iSize - 1);
	}
	xPos += static_cast<int>(g_uiCtx.irWidgetWide * (ROWLAYOUT_TABLESPLIT[GSIW_MAP] / 100.0f));

	{
		// In lower resolution, it may overlap from name, so paint a background here
		vgui::surface()->DrawFilledRect(g_uiCtx.rWidgetArea.x0 + xPos, g_uiCtx.rWidgetArea.y0,
										g_uiCtx.rWidgetArea.x1, g_uiCtx.rWidgetArea.y1);

		wchar_t wszPlayers[10];
		const int iSize = V_swprintf_safe(wszPlayers, L"%d/%d", server.m_nPlayers, server.m_nMaxPlayers);
		vgui::surface()->DrawSetTextPos(g_uiCtx.rWidgetArea.x0 + xPos + g_uiCtx.iMarginX, g_uiCtx.rWidgetArea.y0 + fontStartYPos);
		vgui::surface()->DrawPrintText(wszPlayers, iSize);
	}
	xPos += static_cast<int>(g_uiCtx.irWidgetWide * (ROWLAYOUT_TABLESPLIT[GSIW_PLAYERS] / 100.0f));

	{
		wchar_t wszPing[10];
		const int iSize = V_swprintf_safe(wszPing, L"%d", server.m_nPing);
		vgui::surface()->DrawSetTextPos(g_uiCtx.rWidgetArea.x0 + xPos + g_uiCtx.iMarginX, g_uiCtx.rWidgetArea.y0 + fontStartYPos);
		vgui::surface()->DrawPrintText(wszPing, iSize);
	}
}

static void DrawSortHint(const bool bDescending)
{
	if (g_uiCtx.eMode != NeoUI::MODE_PAINT || !IN_BETWEEN_AR(0, g_uiCtx.iLayoutY, g_uiCtx.dPanel.tall))
	{
		return;
	}
	int iHintTall = g_uiCtx.iMarginY / 3;
	vgui::surface()->DrawSetColor(COLOR_NEOPANELTEXTNORMAL);
	if (!bDescending)
	{
		vgui::surface()->DrawFilledRect(g_uiCtx.rWidgetArea.x0, g_uiCtx.rWidgetArea.y0,
										g_uiCtx.rWidgetArea.x1, g_uiCtx.rWidgetArea.y0 + iHintTall);
	}
	else
	{
		vgui::surface()->DrawFilledRect(g_uiCtx.rWidgetArea.x0, g_uiCtx.rWidgetArea.y1 - iHintTall,
										g_uiCtx.rWidgetArea.x1, g_uiCtx.rWidgetArea.y1);
	}
	vgui::surface()->DrawSetColor(COLOR_NEOPANELACCENTBG);
}

void CNeoRoot::MainLoopServerBrowser(const MainLoopParam param)
{
	static const wchar_t *GS_NAMES[GS__TOTAL] = {
		L"Internet", L"LAN", L"Friends", L"Fav", L"History", L"Spec"
	};
	static const wchar_t *ANTICHEAT_LABELS[ANTICHEAT__TOTAL] = {
		L"<Any>", L"On", L"Off"
	};

	bool bEnterServer = false;
	const int iTallTotal = g_uiCtx.layout.iRowTall * (g_iRowsInScreen + 2);
	g_uiCtx.dPanel.wide = g_iRootSubPanelWide;
	g_uiCtx.dPanel.x = (param.wide / 2) - (g_iRootSubPanelWide / 2);
	g_uiCtx.dPanel.y = (param.tall / 2) - (iTallTotal / 2);
	g_uiCtx.dPanel.tall = g_uiCtx.layout.iRowTall * 2;
	g_uiCtx.bgColor = COLOR_NEOPANELFRAMEBG;
	NeoUI::BeginContext(&g_uiCtx, param.eMode, m_wszDispBtnTexts[MMBTN_FINDSERVER], "CtxServerBrowser");
	{
		bool bForceRefresh = false;
		NeoUI::BeginSection();
		{
			const int iPrevTab = m_iServerBrowserTab;
			NeoUI::Tabs(GS_NAMES, ARRAYSIZE(GS_NAMES), &m_iServerBrowserTab);
			if (iPrevTab != m_iServerBrowserTab)
			{
				m_iSelectedServer = -1;
			}
			if (!m_serverBrowser[m_iServerBrowserTab].m_bReloadedAtLeastOnce)
			{
				bForceRefresh = true;
				m_serverBrowser[m_iServerBrowserTab].m_bReloadedAtLeastOnce = true;
			}
			g_uiCtx.eButtonTextStyle = NeoUI::TEXTSTYLE_LEFT;
			NeoUI::SetPerRowLayout(GSIW__TOTAL, ROWLAYOUT_TABLESPLIT);
			for (int i = 0; i < GS__TOTAL; ++i)
			{
				vgui::surface()->DrawSetColor((m_sortCtx.col == i) ? COLOR_NEOPANELACCENTBG : COLOR_NEOPANELNORMALBG);
				if (NeoUI::Button(SBLABEL_NAMES[i]).bPressed)
				{
					if (m_sortCtx.col == i)
					{
						m_sortCtx.bDescending = !m_sortCtx.bDescending;
					}
					else
					{
						m_sortCtx.col = static_cast<GameServerInfoW>(i);
					}
					m_bSBFiltModified = true;
				}

				if (m_sortCtx.col == i)
				{
					DrawSortHint(m_sortCtx.bDescending);
				}
			}

			// TODO: Should give proper controls over colors through NeoUI
			vgui::surface()->DrawSetColor(COLOR_NEOPANELACCENTBG);
			vgui::surface()->DrawSetTextColor(COLOR_NEOPANELTEXTNORMAL);
			g_uiCtx.eButtonTextStyle = NeoUI::TEXTSTYLE_CENTER;
		}
		NeoUI::EndSection();
		static constexpr int FILTER_ROWS = 5;
		g_uiCtx.dPanel.y += g_uiCtx.dPanel.tall;
		g_uiCtx.dPanel.tall = g_uiCtx.layout.iRowTall * (g_iRowsInScreen - 1);
		if (m_bShowFilterPanel) g_uiCtx.dPanel.tall -= g_uiCtx.layout.iRowTall * FILTER_ROWS;
		NeoUI::BeginSection(true);
		{
			NeoUI::SetPerRowLayout(1);
			if (m_serverBrowser[m_iServerBrowserTab].m_filteredServers.IsEmpty())
			{
				wchar_t wszInfo[128];
				if (m_serverBrowser[m_iServerBrowserTab].m_bSearching)
				{
					V_swprintf_safe(wszInfo, L"Searching %ls queries...", GS_NAMES[m_iServerBrowserTab]);
				}
				else
				{
					V_swprintf_safe(wszInfo, L"No %ls queries found. Press Refresh to re-check", GS_NAMES[m_iServerBrowserTab]);
				}
				NeoUI::HeadingLabel(wszInfo);
#if 0 // NEO NOTE (nullsystem): Flip to 1 to just test layout with no servers
				NeoUI::Button(L""); // dummy button
				if (param.eMode == NeoUI::MODE_PAINT)
				{
					static gameserveritem_t server = {};
					if (server.m_nPing == 0)
					{
						server.m_nPing = 1337;
						server.SetName("123ABC");
						V_strcpy(server.m_szMap, "nt_test123abc_ctg");
						server.m_nPlayers = 0;
						server.m_nMaxPlayers = 32;
						server.m_nBotPlayers = 16;
						server.m_bPassword = true;
						server.m_bSecure = true;
					}
					ServerBrowserDrawRow(server);
				}
#endif
			}
			else
			{
				// NEO TODO (nullsystem): This is fine since there isn't going to be much servers, it'll
				// handle even couple of 100s ok. However if the amount is a concern, NeoUI can be
				// expanded to give Y-offset in the form of filtered array range so it only loops
				// through what's needed instead.
				const auto *sbTab = &m_serverBrowser[m_iServerBrowserTab];
				for (int i = 0; i < sbTab->m_filteredServers.Size(); ++i)
				{
					const auto &server = sbTab->m_filteredServers[i];
					bool bSkipServer = false;
					if (m_sbFilters.bServerNotFull && server.m_nPlayers == server.m_nMaxPlayers) bSkipServer = true;
					else if (m_sbFilters.bHasUsersPlaying && server.m_nPlayers == 0) bSkipServer = true;
					else if (m_sbFilters.bIsNotPasswordProtected && server.m_bPassword) bSkipServer = true;
					else if (m_sbFilters.iAntiCheat == ANTICHEAT_OFF && server.m_bSecure) bSkipServer = true;
					else if (m_sbFilters.iAntiCheat == ANTICHEAT_ON && !server.m_bSecure) bSkipServer = true;
					else if (m_sbFilters.iMaxPing != 0 && server.m_nPing > m_sbFilters.iMaxPing) bSkipServer = true;
					if (bSkipServer)
					{
						continue;
					}

					const auto btn = NeoUI::Button(L"");
					if (btn.bPressed) // Dummy button, draw over it in paint
					{
						m_iSelectedServer = i;
						if (btn.bKeyPressed || btn.bMouseDoublePressed)
						{
							bEnterServer = true;
						}
					}

					if (param.eMode == NeoUI::MODE_PAINT)
					{
						Color drawColor = COLOR_NEOPANELNORMALBG;
						if (m_iSelectedServer == i) drawColor = COLOR_NEOPANELACCENTBG;
						if (btn.bMouseHover) drawColor = COLOR_NEOPANELSELECTBG;

						vgui::surface()->DrawSetColor(drawColor);
						vgui::surface()->DrawFilledRect(g_uiCtx.rWidgetArea.x0, g_uiCtx.rWidgetArea.y0,
														g_uiCtx.rWidgetArea.x1, g_uiCtx.rWidgetArea.y1);
						ServerBrowserDrawRow(server);
					}
				}
			}
		}
		surface()->DrawSetColor(COLOR_NEOPANELACCENTBG);
		NeoUI::EndSection();
		g_uiCtx.dPanel.y += g_uiCtx.dPanel.tall;
		g_uiCtx.dPanel.tall = g_uiCtx.layout.iRowTall;
		if (m_bShowFilterPanel) g_uiCtx.dPanel.tall += g_uiCtx.layout.iRowTall * FILTER_ROWS;
		NeoUI::BeginSection();
		{
			NeoUI::SwapFont(NeoUI::FONT_NTHORIZSIDES);
			g_uiCtx.eButtonTextStyle = NeoUI::TEXTSTYLE_CENTER;
			NeoUI::SetPerRowLayout(6);
			{
				if (NeoUI::Button(L"Back (ESC)").bPressed || NeoUI::Bind(KEY_ESCAPE))
				{
					m_state = STATE_ROOT;
				}
				if (NeoUI::Button(L"Legacy").bPressed)
				{
					GetGameUI()->SendMainMenuCommand("OpenServerBrowser");
				}
				if (NeoUI::Button(m_bShowFilterPanel ? L"Hide Filters" : L"Show Filters").bPressed)
				{
					m_bShowFilterPanel = !m_bShowFilterPanel;
				}
				if (m_iSelectedServer >= 0)
				{
					if (NeoUI::Button(L"Details").bPressed)
					{
						m_state = STATE_SERVERDETAILS;
						const auto *gameServer = &m_serverBrowser[m_iServerBrowserTab].m_filteredServers[m_iSelectedServer];
						m_serverPlayers.RequestList(gameServer->m_NetAdr.GetIP(), gameServer->m_NetAdr.GetQueryPort());
					}
				}
				else
				{
					NeoUI::Pad();
				}
				if (NeoUI::Button(L"Refresh").bPressed || bForceRefresh)
				{
					m_iSelectedServer = -1;
					ISteamMatchmakingServers *steamMM = steamapicontext->SteamMatchmakingServers();
					CNeoServerList *pServerBrowser = &m_serverBrowser[m_iServerBrowserTab];
					pServerBrowser->m_servers.RemoveAll();
					pServerBrowser->m_filteredServers.RemoveAll();
					if (pServerBrowser->m_hdlRequest)
					{
						steamMM->CancelQuery(pServerBrowser->m_hdlRequest);
						steamMM->ReleaseRequest(pServerBrowser->m_hdlRequest);
						pServerBrowser->m_hdlRequest = nullptr;
					}
					pServerBrowser->RequestList();
				}
				if (m_iSelectedServer >= 0)
				{
					if (bEnterServer || NeoUI::Button(L"Enter").bPressed)
					{
						if (IsInGame())
						{
							engine->ClientCmd_Unrestricted("disconnect");
						}

						ConVarRef("password").SetValue("");
						V_memset(m_wszServerPassword, 0, sizeof(m_wszServerPassword));

						// NEO NOTE (nullsystem): Deal with password protected server
						const auto gameServer = m_serverBrowser[m_iServerBrowserTab].m_filteredServers[m_iSelectedServer];
						if (gameServer.m_bPassword)
						{
							m_state = STATE_SERVERPASSWORD;
						}
						else
						{
							char connectCmd[256];
							const char *szAddress = gameServer.m_NetAdr.GetConnectionAddressString();
							V_sprintf_safe(connectCmd, "progress_enable; wait; connect %s", szAddress);
							engine->ClientCmd_Unrestricted(connectCmd);

							if (g_pNeoLoading)
							{
								g_pVGuiLocalize->ConvertANSIToUnicode(gameServer.m_szMap,
																	  g_pNeoLoading->m_wszLoadingMap,
																	  sizeof(g_pNeoLoading->m_wszLoadingMap));
							}

							m_state = STATE_ROOT;
						}
					}
				}
			}
			NeoUI::SwapFont(NeoUI::FONT_NTNORMAL);
			if (m_bShowFilterPanel)
			{
				NeoUI::SetPerRowLayout(2, NeoUI::ROWLAYOUT_TWOSPLIT);
				NeoUI::RingBoxBool(L"Server not full", &m_sbFilters.bServerNotFull);
				NeoUI::RingBoxBool(L"Has users playing", &m_sbFilters.bHasUsersPlaying);
				NeoUI::RingBoxBool(L"Is not password protected", &m_sbFilters.bIsNotPasswordProtected);
				NeoUI::RingBox(L"Anti-cheat", ANTICHEAT_LABELS, ARRAYSIZE(ANTICHEAT_LABELS), &m_sbFilters.iAntiCheat);
				NeoUI::SliderInt(L"Maximum ping", &m_sbFilters.iMaxPing, 0, 500, 10, L"No limit");
			}
		}
		NeoUI::EndSection();
	}
	NeoUI::EndContext();

}

void CNeoRoot::MainLoopMapList(const MainLoopParam param)
{
	const int iTallTotal = g_uiCtx.layout.iRowTall * (g_iRowsInScreen + 2);
	g_uiCtx.dPanel.wide = g_iRootSubPanelWide;
	g_uiCtx.dPanel.x = (param.wide / 2) - (g_iRootSubPanelWide / 2);
	g_uiCtx.dPanel.y = (param.tall / 2) - (iTallTotal / 2);
	g_uiCtx.dPanel.tall = g_uiCtx.layout.iRowTall * (g_iRowsInScreen + 1);
	g_uiCtx.bgColor = COLOR_NEOPANELFRAMEBG;
	NeoUI::BeginContext(&g_uiCtx, param.eMode, L"Pick map", "CtxMapPicker");
	{
		NeoUI::BeginSection(true);
		{
			for (auto &wszMap : m_vWszMaps)
			{
				if (NeoUI::Button(wszMap.wszName).bPressed)
				{
					V_wcscpy_safe(m_newGame.wszMap, wszMap.wszName);
					m_state = STATE_NEWGAME;
				}
			}
		}
		NeoUI::EndSection();
		g_uiCtx.dPanel.y += g_uiCtx.dPanel.tall;
		g_uiCtx.dPanel.tall = g_uiCtx.layout.iRowTall;
		NeoUI::BeginSection();
		{
			NeoUI::SwapFont(NeoUI::FONT_NTHORIZSIDES);
			g_uiCtx.eButtonTextStyle = NeoUI::TEXTSTYLE_CENTER;
			NeoUI::SetPerRowLayout(5);
			{
				if (NeoUI::Button(L"Back (ESC)").bPressed || NeoUI::Bind(KEY_ESCAPE))
				{
					m_state = STATE_NEWGAME;
				}
			}
			NeoUI::SwapFont(NeoUI::FONT_NTNORMAL);
		}
		NeoUI::EndSection();
	}
	NeoUI::EndContext();
}

void CNeoRoot::MainLoopSprayPicker(const MainLoopParam param)
{
	static CUtlVector<SprayInfo> vecStaticSprays;
	if (m_bSprayGalleryRefresh)
	{
		vecStaticSprays.Purge();
		FileFindHandle_t findHdl;
		for (const char *pszFilename = filesystem->FindFirst("materials/vgui/logos/ui/*.vmt", &findHdl);
				pszFilename;
				pszFilename = filesystem->FindNext(findHdl))
		{
			// spray.vmt is only used for currently applying spray
			if (V_strcmp(pszFilename, "spray.vmt") == 0)
			{
				continue;
			}

			if (m_state == STATE_SPRAYDELETER)
			{
				static constexpr const char *DEFAULT_SPRAYS[] = {
					"spray_canned.vmt",
					"spray_combine.vmt",
					"spray_cop.vmt",
					"spray_dog.vmt",
					"spray_freeman.vmt",
					"spray_head.vmt",
					"spray_lambda.vmt",
					"spray_plumbed.vmt",
					"spray_soldier.vmt",
				};
				bool bSkipThis = false;
				for (int i = 0; i < ARRAYSIZE(DEFAULT_SPRAYS); ++i)
				{
					if (V_strcmp(pszFilename, DEFAULT_SPRAYS[i]) == 0)
					{
						bSkipThis = true;
						break;
					}
				}
				if (bSkipThis)
				{
					continue;
				}
			}

			SprayInfo sprayInfo = {};

			V_strcpy_safe(sprayInfo.szBaseName, pszFilename);
			sprayInfo.szBaseName[V_strlen(sprayInfo.szBaseName) - sizeof("vmt")] = '\0';

			V_sprintf_safe(sprayInfo.szPath, "vgui/logos/ui/%s", sprayInfo.szBaseName);
			V_sprintf_safe(sprayInfo.szVtf, "materials/vgui/logos/%s.vtf", sprayInfo.szBaseName);

			if (filesystem->FileExists(sprayInfo.szVtf))
			{
				vecStaticSprays.AddToTail(sprayInfo);
			}
		}
		filesystem->FindClose(findHdl);

		m_bSprayGalleryRefresh = false;
	}
	const int iGalleryRows = g_iRowsInScreen / 4;
	const int iNormTall = g_uiCtx.layout.iRowTall;
	const int iCellTall = iNormTall * 4;

	const int iTallTotal = iNormTall * (g_iRowsInScreen + 2);
	g_uiCtx.dPanel.wide = g_iRootSubPanelWide;
	g_uiCtx.dPanel.x = (param.wide / 2) - (g_iRootSubPanelWide / 2);
	g_uiCtx.dPanel.y = (param.tall / 2) - (iTallTotal / 2);
	g_uiCtx.dPanel.tall = iNormTall * (g_iRowsInScreen + 1);
	g_uiCtx.bgColor = COLOR_NEOPANELFRAMEBG;
	NeoUI::BeginContext(&g_uiCtx, param.eMode,
						(m_state == STATE_SPRAYPICKER) ? L"Pick spray" : L"Delete spray",
						(m_state == STATE_SPRAYPICKER) ? "CtxSprayPicker" : "CtxSprayDeleter");
	{
		NeoUI::BeginSection(true);
		{
			static constexpr int COLS_IN_ROW = 5;
			NeoUI::SetPerRowLayout(COLS_IN_ROW, nullptr, iCellTall);
			for (int i = 0; i < vecStaticSprays.Count(); i += COLS_IN_ROW)
			{
				for (int j = 0; j < COLS_IN_ROW && (i + j) < vecStaticSprays.Count(); ++j)
				{
					const auto &sprayInfo = vecStaticSprays.Element(i + j);
					if (NeoUI::ButtonTexture(sprayInfo.szPath).bPressed)
					{
						if (m_state == STATE_SPRAYPICKER)
						{
							m_eFileIOMode = FILEIODLGMODE_SPRAY;
							OnFileSelected(sprayInfo.szVtf);
							m_state = STATE_SETTINGS;
						}
						else
						{
							V_memcpy(&m_sprayToDelete, &sprayInfo, sizeof(SprayInfo));
							m_state = STATE_SPRAYDELETERCONFIRM;
						}
					}
				}
			}
		}
		NeoUI::EndSection();
		g_uiCtx.dPanel.y += g_uiCtx.dPanel.tall;
		g_uiCtx.dPanel.tall = g_uiCtx.layout.iRowTall;
		NeoUI::BeginSection();
		{
			NeoUI::SwapFont(NeoUI::FONT_NTHORIZSIDES);
			g_uiCtx.eButtonTextStyle = NeoUI::TEXTSTYLE_CENTER;
			NeoUI::SetPerRowLayout(5, nullptr, iNormTall);
			{
				if (NeoUI::Button(L"Back (ESC)").bPressed || NeoUI::Bind(KEY_ESCAPE))
				{
					m_state = STATE_SETTINGS;
				}
			}
			NeoUI::SwapFont(NeoUI::FONT_NTNORMAL);
		}
		NeoUI::EndSection();
	}
	NeoUI::EndContext();
}

void CNeoRoot::MainLoopServerDetails(const MainLoopParam param)
{
	const auto *gameServer = &m_serverBrowser[m_iServerBrowserTab].m_filteredServers[m_iSelectedServer];
	const int iTallTotal = g_uiCtx.layout.iRowTall * (g_iRowsInScreen + 2);
	g_uiCtx.dPanel.wide = g_iRootSubPanelWide;
	g_uiCtx.dPanel.x = (param.wide / 2) - (g_iRootSubPanelWide / 2);
	g_uiCtx.dPanel.y = (param.tall / 2) - (iTallTotal / 2);
	g_uiCtx.dPanel.tall = g_uiCtx.layout.iRowTall * 6;
	g_uiCtx.bgColor = COLOR_NEOPANELFRAMEBG;
	NeoUI::BeginContext(&g_uiCtx, param.eMode, L"Server details", "CtxServerDetail");
	{
		NeoUI::BeginSection(true);
		NeoUI::SetPerRowLayout(2, NeoUI::ROWLAYOUT_TWOSPLIT);
		{
			const bool bP = param.eMode == NeoUI::MODE_PAINT;
			wchar_t wszText[128];
			if (bP) g_pVGuiLocalize->ConvertANSIToUnicode(gameServer->GetName(), wszText, sizeof(wszText));
			NeoUI::Label(L"Name:", wszText);
			if (!cl_neo_streamermode.GetBool())
			{
				if (bP) g_pVGuiLocalize->ConvertANSIToUnicode(gameServer->m_NetAdr.GetConnectionAddressString(), wszText, sizeof(wszText));
				NeoUI::Label(L"Address:", wszText);
			}
			if (bP) g_pVGuiLocalize->ConvertANSIToUnicode(gameServer->m_szMap, wszText, sizeof(wszText));
			NeoUI::Label(L"Map:", wszText);
			if (bP) V_swprintf_safe(wszText, L"%d/%d", gameServer->m_nPlayers, gameServer->m_nMaxPlayers);
			NeoUI::Label(L"Players:", wszText);
			if (bP) V_swprintf_safe(wszText, L"%ls", gameServer->m_bSecure ? L"Enabled" : L"Disabled");
			NeoUI::Label(L"VAC:", wszText);
			if (bP) V_swprintf_safe(wszText, L"%d", gameServer->m_nPing);
			NeoUI::Label(L"Ping:", wszText);

			// TODO: Header same-style as serverlist's header
		}
		NeoUI::EndSection();
		if (!cl_neo_streamermode.GetBool())
		{
			if (m_serverPlayers.m_players.IsEmpty())
			{
				g_uiCtx.dPanel.y += g_uiCtx.dPanel.tall;
				g_uiCtx.dPanel.tall = (iTallTotal - g_uiCtx.layout.iRowTall) - g_uiCtx.dPanel.tall;
				NeoUI::BeginSection();
				{
					NeoUI::SetPerRowLayout(1);
					NeoUI::HeadingLabel(L"There are no players in the server.");
#if 0 // NEO NOTE (nullsystem): Flip to 1 to just test layout with no players
					float flExSecs = 1.0f;
					for (int i = 0; i < 32; ++i)
					{
						CNeoServerPlayers::PlayerInfo player = {};
						player.iScore = i;
						player.flTimePlayed = flExSecs;
						flExSecs *= 2;
						V_swprintf_safe(player.wszName, L"Example %c", 'A' + i);
						m_serverPlayers.m_players.AddToTail(player);
					}
					m_serverPlayers.UpdateSortedList();
#endif
				}
				NeoUI::EndSection();
			}
			else
			{
				static constexpr const int PLAYER_ROW_PROP[] = { 15, 65, -1 };

				const int iInfoTall = g_uiCtx.dPanel.tall;
				g_uiCtx.dPanel.y += iInfoTall;
				g_uiCtx.dPanel.tall = g_uiCtx.layout.iRowTall;
				NeoUI::BeginSection();
				{
					NeoUI::SetPerRowLayout(ARRAYSIZE(PLAYER_ROW_PROP), PLAYER_ROW_PROP);
					// Headers
					static constexpr const wchar_t *PLAYER_HEADERS[GSPS__TOTAL] = {
						L"Score", L"Name", L"Time"
					};
					for (int i = 0; i < GSPS__TOTAL; ++i)
					{
						vgui::surface()->DrawSetColor((m_serverPlayers.m_sortCtx.col == i) ? COLOR_NEOPANELACCENTBG : COLOR_NEOPANELNORMALBG);
						if (NeoUI::Button(PLAYER_HEADERS[i]).bPressed)
						{
							m_bSPlayersSortModified = true;
							if (m_serverPlayers.m_sortCtx.col == i)
							{
								m_serverPlayers.m_sortCtx.bDescending = !m_serverPlayers.m_sortCtx.bDescending;
							}
							else
							{
								m_serverPlayers.m_sortCtx.col = static_cast<GameServerPlayerSort>(i);
							}
						}

						if (m_serverPlayers.m_sortCtx.col == i)
						{
							DrawSortHint(m_serverPlayers.m_sortCtx.bDescending);
						}
					}
				}
				NeoUI::EndSection();

				g_uiCtx.dPanel.y += g_uiCtx.dPanel.tall;
				g_uiCtx.dPanel.tall = iTallTotal - (2 * g_uiCtx.layout.iRowTall) - iInfoTall;
				NeoUI::BeginSection();
				{
					NeoUI::SetPerRowLayout(ARRAYSIZE(PLAYER_ROW_PROP), PLAYER_ROW_PROP);
					// Players - rows
					for (const auto &player : m_serverPlayers.m_sortedPlayers)
					{
						wchar_t wszText[32];

						V_swprintf_safe(wszText, L"%d", player.iScore);
						NeoUI::Label(wszText);
						NeoUI::Label(player.wszName);
						{
							static constexpr float FL_SECSINMIN = 60.0f;
							static constexpr float FL_SECSINHRS = 60.0f * FL_SECSINMIN;
							if (player.flTimePlayed < FL_SECSINMIN)
							{
								V_swprintf_safe(wszText, L"%.0fs", player.flTimePlayed);
							}
							else if (player.flTimePlayed < FL_SECSINMIN * 60.0f)
							{
								const int iMin = player.flTimePlayed / FL_SECSINMIN;
								const int iSec = player.flTimePlayed - (iMin * FL_SECSINMIN);
								V_swprintf_safe(wszText, L"%dm %ds", iMin, iSec);
							}
							else
							{
								const int iHrs = player.flTimePlayed / FL_SECSINHRS;
								const int iMin = (player.flTimePlayed - (iHrs * FL_SECSINHRS)) / FL_SECSINMIN;
								const int iSec = player.flTimePlayed - (iHrs * FL_SECSINHRS) - (iMin * FL_SECSINMIN);
								V_swprintf_safe(wszText, L"%dh %dm %ds", iHrs, iMin, iSec);
							}
						}
						NeoUI::Label(wszText);
					}
				}
				NeoUI::EndSection();
			}
		}
		g_uiCtx.dPanel.y += g_uiCtx.dPanel.tall;
		g_uiCtx.dPanel.tall = g_uiCtx.layout.iRowTall;
		NeoUI::BeginSection();
		{
			NeoUI::SwapFont(NeoUI::FONT_NTHORIZSIDES);
			g_uiCtx.eButtonTextStyle = NeoUI::TEXTSTYLE_CENTER;
			NeoUI::SetPerRowLayout(5);
			{
				if (NeoUI::Button(L"Back (ESC)").bPressed || NeoUI::Bind(KEY_ESCAPE))
				{
					m_state = STATE_SERVERBROWSER;
				}
			}
			NeoUI::SwapFont(NeoUI::FONT_NTNORMAL);
		}
		NeoUI::EndSection();
	}
	NeoUI::EndContext();
}

void CNeoRoot::MainLoopPlayerList(const MainLoopParam param)
{
	if (IsInGame())
	{
		const int iTallTotal = g_uiCtx.layout.iRowTall * (g_iRowsInScreen + 2);
		g_uiCtx.dPanel.wide = g_iRootSubPanelWide;
		g_uiCtx.dPanel.x = (param.wide / 2) - (g_iRootSubPanelWide / 2);
		g_uiCtx.dPanel.y = (param.tall / 2) - (iTallTotal / 2);
		g_uiCtx.dPanel.tall = g_uiCtx.layout.iRowTall * (g_iRowsInScreen + 1);
		g_uiCtx.bgColor = COLOR_NEOPANELFRAMEBG;
		NeoUI::BeginContext(&g_uiCtx, param.eMode, L"Player list", "CtxPlayerList");
		{
			NeoUI::BeginSection(true);
			{
				g_uiCtx.eButtonTextStyle = NeoUI::TEXTSTYLE_LEFT;
				for (int i = 1; i <= gpGlobals->maxClients; i++)
				{
					if (!g_PR->IsConnected(i) || g_PR->IsHLTV(i) || g_PR->IsFakePlayer(i))
					{
						continue;
					}

					const bool bOwnLocalPlayer = g_PR->IsLocalPlayer(i);
					const bool bPlayerMuted = GetClientVoiceMgr()->IsPlayerBlocked(i);
					const char *szPlayerName = g_PR->GetPlayerName(i);
					wchar_t wszPlayerName[MAX_PLAYER_NAME_LENGTH + 1];
					g_pVGuiLocalize->ConvertANSIToUnicode(szPlayerName, wszPlayerName, sizeof(wszPlayerName));

					wchar_t wszInfo[256];
					V_swprintf_safe(wszInfo, L"%ls%ls", bOwnLocalPlayer ? L"[LOCAL] " : bPlayerMuted ? L"[MUTED] " : L"[VOICE] ", wszPlayerName);
					if (NeoUI::Button(wszInfo).bPressed)
					{
						if (!bOwnLocalPlayer) GetClientVoiceMgr()->SetPlayerBlockedState(i, !bPlayerMuted);
					}
				}
				g_uiCtx.eButtonTextStyle = NeoUI::TEXTSTYLE_CENTER;
			}
			NeoUI::EndSection();
			g_uiCtx.dPanel.y += g_uiCtx.dPanel.tall;
			g_uiCtx.dPanel.tall = g_uiCtx.layout.iRowTall;
			NeoUI::BeginSection();
			{
				NeoUI::SwapFont(NeoUI::FONT_NTHORIZSIDES);
				g_uiCtx.eButtonTextStyle = NeoUI::TEXTSTYLE_CENTER;
				NeoUI::SetPerRowLayout(5);
				{
					if (NeoUI::Button(L"Back (ESC)").bPressed || NeoUI::Bind(KEY_ESCAPE))
					{
						m_state = STATE_ROOT;
					}
				}
				NeoUI::SwapFont(NeoUI::FONT_NTNORMAL);
			}
			NeoUI::EndSection();
		}
		NeoUI::EndContext();
	}
	else
	{
		m_state = STATE_ROOT;
	}
}

void CNeoRoot::MainLoopPopup(const MainLoopParam param)
{
	surface()->DrawSetColor(COLOR_NEOPANELPOPUPBG);
	surface()->DrawFilledRect(0, 0, param.wide, param.tall);
	const int tallSplit = param.tall / 3;
	surface()->DrawSetColor(COLOR_NEOPANELNORMALBG);
	surface()->DrawFilledRect(0, tallSplit, param.wide, param.tall - tallSplit);

	g_uiCtx.dPanel.wide = g_iRootSubPanelWide * 0.75f;
	g_uiCtx.dPanel.tall = tallSplit;
	g_uiCtx.dPanel.x = (param.wide / 2) - (g_uiCtx.dPanel.wide / 2);
	g_uiCtx.dPanel.y = tallSplit + (tallSplit / 2) - g_uiCtx.layout.iRowTall;
	g_uiCtx.bgColor = COLOR_TRANSPARENT;
	if (m_state == STATE_SERVERPASSWORD)
	{
		g_uiCtx.dPanel.y -= g_uiCtx.layout.iRowTall;
	}
	// Technically can get away with have a common context name, since these dialogs
	// don't do anything special with it
	NeoUI::BeginContext(&g_uiCtx, param.eMode, nullptr, "CtxCommonPopupDlg");
	{
		NeoUI::BeginSection(true);
		{
			g_uiCtx.eLabelTextStyle = NeoUI::TEXTSTYLE_CENTER;
			g_uiCtx.eButtonTextStyle = NeoUI::TEXTSTYLE_CENTER;
			NeoUI::SwapFont(NeoUI::FONT_NTLARGE);
			switch (m_state)
			{
			case STATE_KEYCAPTURE:
			{
				NeoUI::Label(m_wszBindingText);
				NeoUI::SwapFont(NeoUI::FONT_NTNORMAL);
				NeoUI::Label(L"Press ESC to cancel or DEL to remove keybind");
			}
			break;
			case STATE_CONFIRMSETTINGS:
			{
				NeoUI::Label(L"Settings changed: Do you want to apply the settings?");
				NeoUI::SwapFont(NeoUI::FONT_NTNORMAL);
				NeoUI::SetPerRowLayout(3);
				{
					g_uiCtx.iLayoutX = (g_uiCtx.iMarginX / 2);
					if (NeoUI::Button(L"Save (Enter)").bPressed || NeoUI::Bind(KEY_ENTER))
					{
						NeoSettingsSave(&m_ns);
						m_state = STATE_ROOT;
					}
					if (NeoUI::Button(L"Discard").bPressed)
					{
						m_state = STATE_ROOT;
					}
					if (NeoUI::Button(L"Cancel (ESC)").bPressed || NeoUI::Bind(KEY_ESCAPE))
					{
						m_state = STATE_SETTINGS;
					}
				}
			}
			break;
			case STATE_QUIT:
			{
				NeoUI::Label(L"Do you want to quit the game?");
				NeoUI::SwapFont(NeoUI::FONT_NTNORMAL);
				NeoUI::SetPerRowLayout(3);
				{
					if (NeoUI::Button(L"Quit (Enter)").bPressed || NeoUI::Bind(KEY_ENTER))
					{
						engine->ClientCmd_Unrestricted("quit");
					}
					NeoUI::Pad();
					if (NeoUI::Button(L"Cancel (ESC)").bPressed || NeoUI::Bind(KEY_ESCAPE))
					{
						m_state = STATE_ROOT;
					}
				}
			}
			break;
			case STATE_SERVERPASSWORD:
			{
				NeoUI::Label(L"Enter the server password");
				NeoUI::SwapFont(NeoUI::FONT_NTNORMAL);
				{
					NeoUI::SetPerRowLayout(2, NeoUI::ROWLAYOUT_TWOSPLIT);
					g_uiCtx.bTextEditIsPassword = true;
					NeoUI::TextEdit(L"Password:", m_wszServerPassword, SZWSZ_LEN(m_wszServerPassword));
					g_uiCtx.bTextEditIsPassword = false;
				}
				NeoUI::SetPerRowLayout(3);
				{
					if (NeoUI::Button(L"Enter (Enter)").bPressed || NeoUI::Bind(KEY_ENTER))
					{
						char szServerPassword[ARRAYSIZE(m_wszServerPassword)];
						g_pVGuiLocalize->ConvertUnicodeToANSI(m_wszServerPassword, szServerPassword, sizeof(szServerPassword));
						ConVarRef("password").SetValue(szServerPassword);
						V_memset(m_wszServerPassword, 0, sizeof(m_wszServerPassword));

						const auto gameServer = m_serverBrowser[m_iServerBrowserTab].m_filteredServers[m_iSelectedServer];
						char connectCmd[256];
						const char *szAddress = gameServer.m_NetAdr.GetConnectionAddressString();
						V_sprintf_safe(connectCmd, "progress_enable; wait; connect %s", szAddress);
						engine->ClientCmd_Unrestricted(connectCmd);

						m_state = STATE_ROOT;
					}
					NeoUI::Pad();
					if (NeoUI::Button(L"Cancel (ESC)").bPressed || NeoUI::Bind(KEY_ESCAPE))
					{
						V_memset(m_wszServerPassword, 0, sizeof(m_wszServerPassword));
						m_state = STATE_SERVERBROWSER;
					}
				}
			}
			break;
			case STATE_SETTINGSRESETDEFAULT:
			{
				NeoUI::Label(L"Do you want to reset your settings back to default?");
				NeoUI::SwapFont(NeoUI::FONT_NTNORMAL);
				NeoUI::SetPerRowLayout(3);
				{
					if (NeoUI::Button(L"Yes (Enter)").bPressed || NeoUI::Bind(KEY_ENTER))
					{
						NeoSettingsResetToDefault(&m_ns);
						m_state = STATE_SETTINGS;
					}
					NeoUI::Pad();
					if (NeoUI::Button(L"No (ESC)").bPressed || NeoUI::Bind(KEY_ESCAPE))
					{
						m_state = STATE_SETTINGS;
					}
				}
			}
			break;
			case STATE_SPRAYDELETERCONFIRM:
			{
				static constexpr int TEXWH = 6;
				const int iTexSprayWH = g_uiCtx.layout.iRowTall * TEXWH;
				int scrWide, scrTall;
				vgui::surface()->GetScreenSize(scrWide, scrTall);
				NeoUI::Texture(m_sprayToDelete.szPath,
							   (scrWide / 2) - (iTexSprayWH / 2),
							   (scrTall / 3) - (iTexSprayWH / 2),
							   iTexSprayWH, iTexSprayWH, "", NeoUI::TEXTUREOPTFLAGS_DONOTCROPTOPANEL);

				NeoUI::Label(L"Do you want to delete this spray?");
				NeoUI::SwapFont(NeoUI::FONT_NTNORMAL);
				NeoUI::SetPerRowLayout(3);
				{
					if (NeoUI::Button(L"Yes (Enter)").bPressed || NeoUI::Bind(KEY_ENTER))
					{
						// NOTE (nullsystem): Check if the texture matches byte for byte
						// for the current set spray. If so, replace it with the default spray.
						{
							static constexpr const char PSZ_CURRENT_SPRAY[] = "materials/vgui/logos/spray.vtf";
							char szPathToAboutToDel[MAX_PATH];
							V_sprintf_safe(szPathToAboutToDel, "materials/vgui/logos/%s.vtf", m_sprayToDelete.szBaseName);

							bool bDelCurSpray = false;

							if (filesystem->FileExists(PSZ_CURRENT_SPRAY) &&
									filesystem->FileExists(szPathToAboutToDel))
							{
								const unsigned int uSizeCurSpray = filesystem->Size(PSZ_CURRENT_SPRAY);
								const unsigned int uSizeDelSpray = filesystem->Size(szPathToAboutToDel);
								if (uSizeCurSpray == uSizeDelSpray)
								{
									const unsigned int uSizeSpray = uSizeCurSpray;
									FileHandle_t hdlCurSpray = filesystem->Open(PSZ_CURRENT_SPRAY, "rb");
									FileHandle_t hdlDelSpray = filesystem->Open(szPathToAboutToDel, "rb");
									if (hdlCurSpray && hdlDelSpray)
									{
										unsigned char *pbCurSpray = reinterpret_cast<unsigned char *>(calloc(
													uSizeSpray * 2, sizeof(unsigned char)));

										if (pbCurSpray)
										{
											unsigned char *pbDelSpray = pbCurSpray + uSizeSpray;

											filesystem->Read(pbCurSpray, uSizeSpray, hdlCurSpray);
											filesystem->Read(pbDelSpray, uSizeSpray, hdlDelSpray);

											// Check byte for byte if it matches to determine deletion
											bDelCurSpray = (V_memcmp(pbCurSpray, pbDelSpray, uSizeSpray) == 0);

											pbDelSpray = nullptr;
											free(pbCurSpray);
										}
									}
									if (hdlCurSpray) filesystem->Close(hdlCurSpray);
									if (hdlDelSpray) filesystem->Close(hdlDelSpray);
								}
							}

							// NOTE (nullsystem): If current spray matches deleted one,
							// replace it with the default spray
							if (bDelCurSpray)
							{
								m_eFileIOMode = FILEIODLGMODE_SPRAY;
								OnFileSelected("materials/vgui/logos/spray_lambda.vtf");
							}
						}

						// NOTE (nullsystem): Spray related files to remove:
						// * materials/vgui/logos/SPRAY.vtf
						// * materials/vgui/logos/SPRAY.vmt
						// * materials/vgui/logos/ui/SPRAY.vmt
						// * materials/vgui/logos/ui/SPRAY.vtf
						char szPathToDel[MAX_PATH];

						V_sprintf_safe(szPathToDel, "materials/vgui/logos/%s.vtf", m_sprayToDelete.szBaseName);
						filesystem->RemoveFile(szPathToDel);
						V_sprintf_safe(szPathToDel, "materials/vgui/logos/%s.vmt", m_sprayToDelete.szBaseName);
						filesystem->RemoveFile(szPathToDel);
						V_sprintf_safe(szPathToDel, "materials/vgui/logos/ui/%s.vmt", m_sprayToDelete.szBaseName);
						filesystem->RemoveFile(szPathToDel);
						V_sprintf_safe(szPathToDel, "materials/vgui/logos/ui/%s.vtf", m_sprayToDelete.szBaseName);
						filesystem->RemoveFile(szPathToDel);

						V_memset(&m_sprayToDelete, 0, sizeof(SprayInfo));
						m_state = STATE_SETTINGS;
					}
					NeoUI::Pad();
					if (NeoUI::Button(L"No (ESC)").bPressed || NeoUI::Bind(KEY_ESCAPE))
					{
						V_memset(&m_sprayToDelete, 0, sizeof(SprayInfo));
						m_state = STATE_SPRAYDELETER;
					}
				}
			}
			break;
			default:
				break;
			}
		}
		NeoUI::EndSection();
	}
	NeoUI::EndContext();
}

void CNeoRoot::HTTPCallbackRequest(HTTPRequestCompleted_t *request, bool bIOFailure)
{
	ISteamHTTP *http = steamapicontext->SteamHTTP();
	if (request->m_bRequestSuccessful && !bIOFailure)
	{
		uint32 unBodySize = 0;
		http->GetHTTPResponseBodySize(request->m_hRequest, &unBodySize);

		if (unBodySize > 0)
		{
			uint8 *pData = new uint8[unBodySize + 1];
			http->GetHTTPResponseBodyData(request->m_hRequest, pData, unBodySize);

			CUtlBuffer buf(0, 0, CUtlBuffer::TEXT_BUFFER);
			buf.CopyBuffer(pData, unBodySize);
			filesystem->WriteFile("news.txt", nullptr, buf);
			ReadNewsFile(buf);

			delete[] pData;
		}
	}
	http->ReleaseHTTPRequest(request->m_hRequest);
}

void CNeoRoot::ReadNewsFile(CUtlBuffer &buf)
{
	buf.SeekGet(CUtlBuffer::SEEK_HEAD, 0);
	m_iNewsSize = 0;
	while (buf.IsValid() && m_iNewsSize < MAX_NEWS)
	{
		// TSV row: Path\tDate\tTitle
		char szLine[512] = {};
		buf.GetLine(szLine, ARRAYSIZE(szLine) - 1);
		char *pszDate = strchr(szLine, '\t');
		if (!pszDate)
		{
			continue;
		}

		*pszDate = '\0';
		++pszDate;
		if (!*pszDate)
		{
			continue;
		}

		char *pszTitle = strchr(pszDate, '\t');
		if (!pszTitle)
		{
			continue;
		}

		*pszTitle = '\0';
		++pszTitle;
		if (!*pszTitle)
		{
			continue;
		}

		V_strcpy_safe(m_news[m_iNewsSize].szSitePath, szLine);
		wchar_t wszDate[12];
		wchar_t wszTitle[235];
		g_pVGuiLocalize->ConvertANSIToUnicode(pszDate, wszDate, sizeof(wszDate));
		g_pVGuiLocalize->ConvertANSIToUnicode(pszTitle, wszTitle, sizeof(wszTitle));
		V_swprintf_safe(m_news[m_iNewsSize].wszTitle, L"%ls: %ls", wszDate, wszTitle);
		++m_iNewsSize;
	}
}

void CNeoRoot::OnFileSelectedMode_Crosshair(const char *szFullpath)
{
	((m_ns.crosshair.eFileIOMode == vgui::FOD_OPEN) ?
			&ImportCrosshair : &ExportCrosshair)(&m_ns.crosshair.info, szFullpath);
}

void CNeoRoot::OnFileSelectedMode_Spray(const char *szFullpath)
{
	// Ensure the directories are there to write to
	filesystem->CreateDirHierarchy("materials/vgui/logos");
	filesystem->CreateDirHierarchy("materials/vgui/logos/ui");

	CUtlBuffer sprayBuffer(0, 0, 0);

	if (V_striEndsWith(szFullpath, ".vtf"))
	{
		// Check if we can just direct copy over (is DXT1 and 256x256) or will have
		// to go through reprocessing like a PNG/JPEG format would
		CUtlBuffer buf(0, 0, CUtlBuffer::READ_ONLY);
		if (!filesystem->ReadFile(szFullpath, nullptr, buf))
		{
			return;
		}

		IVTFTexture *pVTFTexture = CreateVTFTexture();
		if (pVTFTexture->Unserialize(buf))
		{
			const bool bDirectCopy = ((pVTFTexture->Width() == pVTFTexture->Height()) &&
									  ((pVTFTexture->Format() == IMAGE_FORMAT_DXT1) ||
									   (pVTFTexture->Format() == IMAGE_FORMAT_DXT5)));
			if (bDirectCopy)
			{
				if (!engine->CopyLocalFile(szFullpath, "materials/vgui/logos/spray.vtf"))
				{
					Msg("ERROR: Cannot copy spray's vtf to: %s", "materials/vgui/logos/spray.vtf");
				}
			}
			else
			{
				pVTFTexture->ConvertImageFormat(IMAGE_FORMAT_RGBA8888, false);
				uint8 *data = NeoUtils::CropScaleTo256(pVTFTexture->ImageData(0, 0, 0),
													   pVTFTexture->Width(), pVTFTexture->Height());
				NeoUtils::SerializeVTFDXTSprayToBuffer(&sprayBuffer, data);
				free(data);
			}
		}
		DestroyVTFTexture(pVTFTexture);
	}
	else
	{
		int width, height, channels;
		uint8 *data = stbi_load(szFullpath, &width, &height, &channels, 0);
		if (!data || width <= 0 || height <= 0)
		{
			return;
		}

		// Convert to RGBA
		if (channels == 3)
		{
			uint8 *rgbaData = reinterpret_cast<uint8 *>(calloc(width * height, sizeof(uint8) * 4));

			ImageLoader::ConvertImageFormat(data, IMAGE_FORMAT_RGB888,
											rgbaData, IMAGE_FORMAT_RGBA8888,
											width, height);

			stbi_image_free(data);
			data = rgbaData;
			channels = 4;
		}

		{
			uint8 *newData = NeoUtils::CropScaleTo256(data, width, height);
			free(data);
			data = newData;
			width = NeoUtils::SPRAY_WH;
			height = NeoUtils::SPRAY_WH;
		}

		NeoUtils::SerializeVTFDXTSprayToBuffer(&sprayBuffer, data);
		free(data);
	}

	char szBaseFName[MAX_PATH] = {};
	{
		static constexpr const char OS_PATH_SEP =
#ifdef WIN32
			'\\'
#else
			'/'
#endif
		;

		const char *pLastSlash = V_strrchr(szFullpath, OS_PATH_SEP);

		const char *pszBaseName = pLastSlash ? pLastSlash + 1 : szFullpath;
		int iSzLen = V_strlen(pszBaseName);
		const char *pszDot = strchr(pszBaseName, '.');
		if (pszDot)
		{
			iSzLen = pszDot - pszBaseName;
		}
		V_sprintf_safe(szBaseFName, "%.*s", iSzLen, pszBaseName);
	}
	char szByBaseName[MAX_PATH];
	V_sprintf_safe(szByBaseName, "materials/vgui/logos/%s.vtf", szBaseFName);

	if (sprayBuffer.Size() > 0)
	{
		if (!filesystem->WriteFile("materials/vgui/logos/spray.vtf", nullptr, sprayBuffer))
		{
			Msg("ERROR: Cannot save spray's vtf to: %s", "materials/vgui/logos/spray.vtf");
		}
		if (!filesystem->WriteFile(szByBaseName, nullptr, sprayBuffer))
		{
			Msg("ERROR: Cannot save spray's vtf to: %s", szByBaseName);
		}
	}
	else if (V_strcmp(szFullpath, szByBaseName) != 0)
	{
		if (!engine->CopyLocalFile(szFullpath, szByBaseName))
		{
			Msg("ERROR: Cannot copy spray's vtf to: %s", szByBaseName);
		}
	}

	// Generate the vmt files, one for spraying, one under "ui" to display in GUI
	for (const char *pszBaseName : {"spray", static_cast<const char *>(szBaseFName)})
	{
		if (pszBaseName[0] == '\0')
		{
			continue;
		}

		char szStrBuffer[1024];
		{
			V_sprintf_safe(szStrBuffer, R"VMT(
LightmappedGeneric
{
"$basetexture"	"vgui/logos/%s"
"$translucent" "1"
"$decal" "1"
"$decalscale" "0.250"
}
)VMT", pszBaseName);

			CUtlBuffer bufVmt(0, 0, CUtlBuffer::TEXT_BUFFER);
			bufVmt.PutString(szStrBuffer);

			char szOutPath[MAX_PATH];
			V_sprintf_safe(szOutPath, "materials/vgui/logos/%s.vmt", pszBaseName);
			if (!filesystem->WriteFile(szOutPath, nullptr, bufVmt))
			{
				Msg("ERROR: Cannot save spray's vmt to: %s", szOutPath);
			}
		}

		{
			V_sprintf_safe(szStrBuffer, R"VMT(
"UnlitGeneric"
{
// Original shader: BaseTimesVertexColorAlphaBlendNoOverbright
"$translucent" 1
"$basetexture" "VGUI/logos/%s"
"$vertexcolor" 1
"$vertexalpha" 1
"$no_fullbright" 1
"$ignorez" 1
}
)VMT", pszBaseName);

			CUtlBuffer bufVmt(0, 0, CUtlBuffer::TEXT_BUFFER);
			bufVmt.PutString(szStrBuffer);

			char szOutPath[MAX_PATH];
			V_sprintf_safe(szOutPath, "materials/vgui/logos/ui/%s.vmt", pszBaseName);
			if (!filesystem->WriteFile(szOutPath, nullptr, bufVmt))
			{
				Msg("ERROR: Cannot save spray's vmt to: %s", szOutPath);
			}
		}
	}

	// Reapply the cl_logofile ConVar, update the texture to the new spray
	ConVarRef("cl_logofile").SetValue("materials/vgui/logos/spray.vtf");
	engine->ClientCmd_Unrestricted("cl_logofile materials/vgui/logos/spray.vtf");

	NeoUI::ResetTextures();
	materials->ReloadMaterials("vgui/logo");
}

void CNeoRoot::OnFileSelected(const char *szFullpath)
{
	static void (CNeoRoot::*FILESELMODEFNS[FILEIODLGMODE__TOTAL])(const char *) = {
		&CNeoRoot::OnFileSelectedMode_Crosshair,	// FILEIODLGMODE_CROSSHAIR
		&CNeoRoot::OnFileSelectedMode_Spray,		// FILEIODLGMODE_SPRAY
	};
	(this->*FILESELMODEFNS[m_eFileIOMode])(szFullpath);
}

// NEO NOTE (nullsystem): NeoRootCaptureESC is so that ESC keybinds can be recognized by non-root states, but root
// state still want to have ESC handled by the game as IsVisible/HasFocus isn't reliable indicator to depend on.
// This goes along with NeoToggleconsole which if the toggleconsole is activated on non-root state that can end up
// blocking ESC key from being usable in-game, so have to force it back to root state first to help with
// NeoRootCaptureESC condition. So this will do.

bool NeoRootCaptureESC()
{
	return (g_pNeoRoot && g_pNeoRoot->IsEnabled() && g_pNeoRoot->m_state != STATE_ROOT);
}

void NeoToggleconsole()
{
	if (cl_neo_toggleconsole.GetBool())
	{
		if (engine->IsInGame() && g_pNeoRoot)
		{
			g_pNeoRoot->m_state = STATE_ROOT;
		}
		// NEO JANK (nullsystem): con_enable 1 is required to allow toggleconsole to
		// work and using the legacy settings will alter that value.
		// It's in here rather than startup so it doesn't trigger the console on startup
		ConVarRef("con_enable").SetValue(true);
		engine->ClientCmd_Unrestricted("toggleconsole");
	}
}
