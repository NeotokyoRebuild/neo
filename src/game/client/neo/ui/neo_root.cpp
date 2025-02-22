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
#include "neo_gamerules.h"
#include "neo_misc.h"

#include <vgui/IInput.h>
#include <vgui_controls/Controls.h>

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
static CDllDemandLoader g_GameUIDLL("GameUI");

extern ConVar neo_name;
extern ConVar cl_onlysteamnick;
extern ConVar neo_cl_streamermode;
extern ConVar neo_clantag;

CNeoRoot *g_pNeoRoot = nullptr;
void NeoToggleconsole();
extern CNeoLoading *g_pNeoLoading;
inline NeoUI::Context g_uiCtx;
inline ConVar neo_cl_toggleconsole("neo_cl_toggleconsole", "0", FCVAR_ARCHIVE,
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

void CNeoRootInput::OnKeyTyped(wchar_t unichar)
{
	m_pNeoRoot->OnRelayedKeyTyped(unichar);
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
	{ "#GameUI_GameMenu_ResumeGame", "ResumeGame", STATE__TOTAL, FLAG_SHOWINGAME },
	{ "#GameUI_GameMenu_FindServers", nullptr, STATE_SERVERBROWSER, FLAG_SHOWINGAME | FLAG_SHOWINMAIN },
	{ "#GameUI_GameMenu_CreateServer", nullptr, STATE_NEWGAME, FLAG_SHOWINGAME | FLAG_SHOWINMAIN },
	{ "#GameUI_GameMenu_Disconnect", "Disconnect", STATE__TOTAL, FLAG_SHOWINGAME },
	{ "#GameUI_GameMenu_PlayerList", nullptr, STATE_PLAYERLIST, FLAG_SHOWINGAME },
	{ "#GameUI_GameMenu_Options", nullptr, STATE_SETTINGS, FLAG_SHOWINGAME | FLAG_SHOWINMAIN },
	{ "#GameUI_GameMenu_Quit", nullptr, STATE_QUIT, FLAG_SHOWINGAME | FLAG_SHOWINMAIN },
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
	g_uiCtx.iActiveDirection = 0;
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

	// In 1080p, g_uiCtx.iRowTall == 40, g_uiCtx.iMarginX = 10, g_iAvatar = 64,
	// other resolution scales up/down from it
	g_uiCtx.iRowTall = tall / 27;
	g_iRowsInScreen = (tall * 0.85f) / g_uiCtx.iRowTall;
	g_uiCtx.iMarginX = wide / 192;
	g_uiCtx.iMarginY = tall / 108;
	g_iAvatar = wide / 30;
	const float flWide = static_cast<float>(wide);
	m_flWideAs43 = static_cast<float>(tall) * (4.0f / 3.0f);
	if (m_flWideAs43 > flWide) m_flWideAs43 = flWide;
	g_iRootSubPanelWide = static_cast<int>(m_flWideAs43 * 0.9f);

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
	OnMainLoop(NeoUI::MODE_PAINT);
}

void CNeoRoot::OnMousePressed(vgui::MouseCode code)
{
	g_uiCtx.eCode = code;
	OnMainLoop(NeoUI::MODE_MOUSEPRESSED);
}

void CNeoRoot::OnMouseReleased(vgui::MouseCode code)
{
	g_uiCtx.eCode = code;
	OnMainLoop(NeoUI::MODE_MOUSERELEASED);
}

void CNeoRoot::OnMouseDoublePressed(vgui::MouseCode code)
{
	g_uiCtx.eCode = code;
	OnMainLoop(NeoUI::MODE_MOUSEDOUBLEPRESSED);
}

void CNeoRoot::OnMouseWheeled(int delta)
{
	g_uiCtx.eCode = (delta > 0) ? MOUSE_WHEEL_UP : MOUSE_WHEEL_DOWN;
	OnMainLoop(NeoUI::MODE_MOUSEWHEELED);
}

void CNeoRoot::OnCursorMoved(int x, int y)
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
		static constexpr void (CNeoRoot:: * P_FN_MAIN_LOOP[STATE__TOTAL])(const MainLoopParam param) = {
		&CNeoRoot::MainLoopRoot,			// STATE_ROOT
		&CNeoRoot::MainLoopSettings,		// STATE_SETTINGS
		&CNeoRoot::MainLoopNewGame,			// STATE_NEWGAME
		&CNeoRoot::MainLoopServerBrowser,	// STATE_SERVERBROWSER

		&CNeoRoot::MainLoopMapList,			// STATE_MAPLIST
		&CNeoRoot::MainLoopServerDetails,	// STATE_SERVERDETAILS
		&CNeoRoot::MainLoopPlayerList,		// STATE_PLAYERLIST

		&CNeoRoot::MainLoopPopup,			// STATE_KEYCAPTURE
		&CNeoRoot::MainLoopPopup,			// STATE_CONFIRMSETTINGS
		&CNeoRoot::MainLoopPopup,			// STATE_QUIT
		&CNeoRoot::MainLoopPopup,			// STATE_SERVERPASSWORD
		&CNeoRoot::MainLoopPopup,			// STATE_SETTINGSRESETDEFAULT
		};
		(this->*P_FN_MAIN_LOOP[m_state])(MainLoopParam{ .eMode = eMode, .wide = wide, .tall = tall });

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
	g_uiCtx.dPanel.y = 0;
	g_uiCtx.iYOffset[0] = (iTitleMarginTop + (2 * iTitleNHeight)) / -g_uiCtx.iRowTall;
	g_uiCtx.bgColor = COLOR_NEOPANELNORMALBG;

	NeoUI::BeginContext(&g_uiCtx, param.eMode, nullptr, "CtxRoot");
	NeoUI::BeginSection(true);
	{
		const int iFlagToMatch = IsInGame() ? FLAG_SHOWINGAME : FLAG_SHOWINMAIN;
		for (int i = 0; i < BTNS_TOTAL; ++i)
		{
			const auto btnInfo = BTNS_INFO[i];
			if (btnInfo.flags & iFlagToMatch)
			{
				const auto retBtn = NeoUI::Button(m_wszDispBtnTexts[i]);
				if (retBtn.bPressed || (i == MMBTN_QUIT && !IsInGame() && NeoUI::Bind(KEY_ESCAPE)))
				{
					surface()->PlaySound("ui/buttonclickrelease.wav");
					if (btnInfo.gamemenucommand)
					{
						m_state = STATE_ROOT;
						GetGameUI()->SendMainMenuCommand(btnInfo.gamemenucommand);
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

#if (0)	// NEO TODO (Adam) place the current player info in the top right corner maybe?
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
								neo_cl_streamermode.GetBool() ? L" [Streamer mode on]" : L"");
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
#endif (0)
	}

#if (0) // NEO TODO (Adam) some kind of drop down for the news section, better position the current server info etc.
	g_uiCtx.dPanel.x = iRightXPos;
	g_uiCtx.dPanel.y = iRightSideYStart;
	if (IsInGame())
	{
		g_uiCtx.dPanel.wide = m_flWideAs43 * 0.7f;
		g_uiCtx.flWgXPerc = 0.25f;
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
#endif (0)
	NeoUI::EndContext();
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

	const int iTallTotal = g_uiCtx.iRowTall * (g_iRowsInScreen + 2);

	g_uiCtx.dPanel.wide = g_iRootSubPanelWide;
	g_uiCtx.dPanel.x = (param.wide / 2) - (g_iRootSubPanelWide / 2);
	g_uiCtx.dPanel.y = (param.tall / 2) - (iTallTotal / 2);
	g_uiCtx.dPanel.tall = g_uiCtx.iRowTall;
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
			g_uiCtx.dPanel.tall = g_uiCtx.iRowTall * g_iRowsInScreen;
			NeoUI::BeginSection(true);
		}
		{
			P_FN[m_ns.iCurTab].func(&m_ns);
		}
		if (!P_FN[m_ns.iCurTab].bUISectionManaged)
		{
			NeoUI::EndSection();
		}
		g_uiCtx.dPanel.y += g_uiCtx.dPanel.tall;
		g_uiCtx.dPanel.tall = g_uiCtx.iRowTall;
		NeoUI::BeginSection();
		{
			NeoUI::SwapFont(NeoUI::FONT_NTHORIZSIDES);
			NeoUI::BeginHorizontal(g_uiCtx.dPanel.wide / 5);
			{
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
			}
			NeoUI::EndHorizontal();
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
	const int iTallTotal = g_uiCtx.iRowTall * (g_iRowsInScreen + 2);
	g_uiCtx.dPanel.wide = g_iRootSubPanelWide;
	g_uiCtx.dPanel.x = (param.wide / 2) - (g_iRootSubPanelWide / 2);
	g_uiCtx.dPanel.y = (param.tall / 2) - (iTallTotal / 2);
	g_uiCtx.dPanel.tall = g_uiCtx.iRowTall * (g_iRowsInScreen + 1);
	g_uiCtx.bgColor = COLOR_NEOPANELFRAMEBG;
	NeoUI::BeginContext(&g_uiCtx, param.eMode, m_wszDispBtnTexts[MMBTN_CREATESERVER], "CtxNewGame");
	{
		NeoUI::BeginSection(true);
		{
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
		g_uiCtx.dPanel.tall = g_uiCtx.iRowTall;
		NeoUI::BeginSection();
		{
			NeoUI::SwapFont(NeoUI::FONT_NTHORIZSIDES);
			NeoUI::BeginHorizontal(g_uiCtx.dPanel.wide / 5);
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
			NeoUI::EndHorizontal();
			NeoUI::SwapFont(NeoUI::FONT_NTNORMAL);
		}
		NeoUI::EndSection();
	}
	NeoUI::EndContext();
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
	const int iTallTotal = g_uiCtx.iRowTall * (g_iRowsInScreen + 2);
	g_uiCtx.dPanel.wide = g_iRootSubPanelWide;
	g_uiCtx.dPanel.x = (param.wide / 2) - (g_iRootSubPanelWide / 2);
	g_uiCtx.dPanel.y = (param.tall / 2) - (iTallTotal / 2);
	g_uiCtx.dPanel.tall = g_uiCtx.iRowTall * 2;
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
			NeoUI::BeginHorizontal(1);
			{
				static constexpr const wchar_t *SBLABEL_NAMES[GSIW__TOTAL] = {
					L"Lock", L"VAC", L"Name", L"Map", L"Players", L"Ping",
				};

				for (int i = 0; i < GS__TOTAL; ++i)
				{
					surface()->DrawSetColor((m_sortCtx.col == i) ? COLOR_NEOPANELACCENTBG : COLOR_NEOPANELNORMALBG);
					g_uiCtx.iHorizontalWidth = g_iGSIX[i];
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

					if (param.eMode == NeoUI::MODE_PAINT && m_sortCtx.col == i)
					{
						int iHintTall = g_uiCtx.iMarginY / 3;
						surface()->DrawSetColor(COLOR_NEOPANELTEXTNORMAL);
						if (!m_sortCtx.bDescending)
						{
							NeoUI::GCtxDrawFilledRectXtoX(-g_uiCtx.iHorizontalWidth, 0, 0, iHintTall);
						}
						else
						{
							NeoUI::GCtxDrawFilledRectXtoX(-g_uiCtx.iHorizontalWidth, g_uiCtx.iRowTall - iHintTall, 0, g_uiCtx.iRowTall);
						}
						surface()->DrawSetColor(COLOR_NEOPANELACCENTBG);
					}
				}
			}

			// TODO: Should give proper controls over colors through NeoUI
			surface()->DrawSetColor(COLOR_NEOPANELACCENTBG);
			surface()->DrawSetTextColor(COLOR_NEOPANELTEXTNORMAL);
			NeoUI::EndHorizontal();
			g_uiCtx.eButtonTextStyle = NeoUI::TEXTSTYLE_CENTER;
		}
		NeoUI::EndSection();
		static constexpr int FILTER_ROWS = 5;
		g_uiCtx.dPanel.y += g_uiCtx.dPanel.tall;
		g_uiCtx.dPanel.tall = g_uiCtx.iRowTall * (g_iRowsInScreen - 1);
		if (m_bShowFilterPanel) g_uiCtx.dPanel.tall -= g_uiCtx.iRowTall * FILTER_ROWS;
		NeoUI::BeginSection(true);
		{
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
				g_uiCtx.eLabelTextStyle = NeoUI::TEXTSTYLE_CENTER;
				NeoUI::Label(wszInfo);
				g_uiCtx.eLabelTextStyle = NeoUI::TEXTSTYLE_LEFT;
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
						NeoUI::GCtxDrawFilledRectXtoX(0, -g_uiCtx.iRowTall, g_uiCtx.dPanel.wide, 0);

						ServerBrowserDrawRow(server);
					}
				}
			}
		}
		surface()->DrawSetColor(COLOR_NEOPANELACCENTBG);
		NeoUI::EndSection();
		g_uiCtx.dPanel.y += g_uiCtx.dPanel.tall;
		g_uiCtx.dPanel.tall = g_uiCtx.iRowTall;
		if (m_bShowFilterPanel) g_uiCtx.dPanel.tall += g_uiCtx.iRowTall * FILTER_ROWS;
		NeoUI::BeginSection();
		{
			NeoUI::SwapFont(NeoUI::FONT_NTHORIZSIDES);
			NeoUI::BeginHorizontal(g_uiCtx.dPanel.wide / 6);
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
			NeoUI::EndHorizontal();
			NeoUI::SwapFont(NeoUI::FONT_NTNORMAL);
			if (m_bShowFilterPanel)
			{
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
	const int iTallTotal = g_uiCtx.iRowTall * (g_iRowsInScreen + 2);
	g_uiCtx.dPanel.wide = g_iRootSubPanelWide;
	g_uiCtx.dPanel.x = (param.wide / 2) - (g_iRootSubPanelWide / 2);
	g_uiCtx.dPanel.y = (param.tall / 2) - (iTallTotal / 2);
	g_uiCtx.dPanel.tall = g_uiCtx.iRowTall * (g_iRowsInScreen + 1);
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
		g_uiCtx.dPanel.tall = g_uiCtx.iRowTall;
		NeoUI::BeginSection();
		{
			NeoUI::SwapFont(NeoUI::FONT_NTHORIZSIDES);
			NeoUI::BeginHorizontal(g_uiCtx.dPanel.wide / 5);
			{
				if (NeoUI::Button(L"Back (ESC)").bPressed || NeoUI::Bind(KEY_ESCAPE))
				{
					m_state = STATE_NEWGAME;
				}
			}
			NeoUI::EndHorizontal();
			NeoUI::SwapFont(NeoUI::FONT_NTNORMAL);
		}
		NeoUI::EndSection();
	}
	NeoUI::EndContext();
}

void CNeoRoot::MainLoopServerDetails(const MainLoopParam param)
{
	const auto *gameServer = &m_serverBrowser[m_iServerBrowserTab].m_filteredServers[m_iSelectedServer];
	const int iTallTotal = g_uiCtx.iRowTall * (g_iRowsInScreen + 2);
	g_uiCtx.dPanel.wide = g_iRootSubPanelWide;
	g_uiCtx.dPanel.x = (param.wide / 2) - (g_iRootSubPanelWide / 2);
	g_uiCtx.dPanel.y = (param.tall / 2) - (iTallTotal / 2);
	g_uiCtx.dPanel.tall = g_uiCtx.iRowTall * 6;
	g_uiCtx.bgColor = COLOR_NEOPANELFRAMEBG;
	NeoUI::BeginContext(&g_uiCtx, param.eMode, L"Server details", "CtxServerDetail");
	{
		NeoUI::BeginSection(true);
		{
			const bool bP = param.eMode == NeoUI::MODE_PAINT;
			wchar_t wszText[128];
			if (bP) g_pVGuiLocalize->ConvertANSIToUnicode(gameServer->GetName(), wszText, sizeof(wszText));
			NeoUI::Label(L"Name:", wszText);
			if (!neo_cl_streamermode.GetBool())
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
		g_uiCtx.dPanel.y += g_uiCtx.dPanel.tall;
		g_uiCtx.dPanel.tall = (iTallTotal - g_uiCtx.iRowTall) - g_uiCtx.dPanel.tall;
		NeoUI::BeginSection();
		{
			if (!neo_cl_streamermode.GetBool())
			{
				if (m_serverPlayers.m_players.IsEmpty())
				{
					g_uiCtx.eLabelTextStyle = NeoUI::TEXTSTYLE_CENTER;
					NeoUI::Label(L"There are no players in the server.");
					g_uiCtx.eLabelTextStyle = NeoUI::TEXTSTYLE_LEFT;
				}
				else
				{
					for (const auto &player : m_serverPlayers.m_players)
					{
						NeoUI::Label(player.wszName);
						// TODO
					}
				}
			}
		}
		NeoUI::EndSection();
		g_uiCtx.dPanel.y += g_uiCtx.dPanel.tall;
		g_uiCtx.dPanel.tall = g_uiCtx.iRowTall;
		NeoUI::BeginSection();
		{
			NeoUI::SwapFont(NeoUI::FONT_NTHORIZSIDES);
			NeoUI::BeginHorizontal(g_uiCtx.dPanel.wide / 5);
			{
				if (NeoUI::Button(L"Back (ESC)").bPressed || NeoUI::Bind(KEY_ESCAPE))
				{
					m_state = STATE_SERVERBROWSER;
				}
			}
			NeoUI::EndHorizontal();
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
		const int iTallTotal = g_uiCtx.iRowTall * (g_iRowsInScreen + 2);
		g_uiCtx.dPanel.wide = g_iRootSubPanelWide;
		g_uiCtx.dPanel.x = (param.wide / 2) - (g_iRootSubPanelWide / 2);
		g_uiCtx.dPanel.y = (param.tall / 2) - (iTallTotal / 2);
		g_uiCtx.dPanel.tall = g_uiCtx.iRowTall * (g_iRowsInScreen + 1);
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
			g_uiCtx.dPanel.tall = g_uiCtx.iRowTall;
			NeoUI::BeginSection();
			{
				NeoUI::SwapFont(NeoUI::FONT_NTHORIZSIDES);
				NeoUI::BeginHorizontal(g_uiCtx.dPanel.wide / 5);
				{
					if (NeoUI::Button(L"Back (ESC)").bPressed || NeoUI::Bind(KEY_ESCAPE))
					{
						m_state = STATE_ROOT;
					}
				}
				NeoUI::EndHorizontal();
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
	g_uiCtx.dPanel.y = tallSplit + (tallSplit / 2) - g_uiCtx.iRowTall;
	g_uiCtx.bgColor = COLOR_TRANSPARENT;
	if (m_state == STATE_SERVERPASSWORD)
	{
		g_uiCtx.dPanel.y -= g_uiCtx.iRowTall;
	}
	// Technically can get away with have a common context name, since these dialogs
	// don't do anything special with it
	NeoUI::BeginContext(&g_uiCtx, param.eMode, nullptr, "CtxCommonPopupDlg");
	{
		NeoUI::BeginSection(true);
		{
			g_uiCtx.eLabelTextStyle = NeoUI::TEXTSTYLE_CENTER;
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
				NeoUI::BeginHorizontal((g_uiCtx.dPanel.wide / 3) - g_uiCtx.iMarginX, g_uiCtx.iMarginX);
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
				NeoUI::EndHorizontal();
			}
			break;
			case STATE_QUIT:
			{
				NeoUI::Label(L"Do you want to quit the game?");
				NeoUI::SwapFont(NeoUI::FONT_NTNORMAL);
				NeoUI::BeginHorizontal(g_uiCtx.dPanel.wide / 3);
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
				NeoUI::EndHorizontal();
			}
			break;
			case STATE_SERVERPASSWORD:
			{
				NeoUI::Label(L"Enter the server password");
				NeoUI::SwapFont(NeoUI::FONT_NTNORMAL);
				g_uiCtx.bTextEditIsPassword = true;
				NeoUI::TextEdit(L"Password:", m_wszServerPassword, SZWSZ_LEN(m_wszServerPassword));
				g_uiCtx.bTextEditIsPassword = false;
				NeoUI::BeginHorizontal(g_uiCtx.dPanel.wide / 3);
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
				NeoUI::EndHorizontal();
			}
			break;
			case STATE_SETTINGSRESETDEFAULT:
			{
				NeoUI::Label(L"Do you want to reset your settings back to default?");
				NeoUI::SwapFont(NeoUI::FONT_NTNORMAL);
				NeoUI::BeginHorizontal((g_uiCtx.dPanel.wide / 3) - g_uiCtx.iMarginX, g_uiCtx.iMarginX);
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
				NeoUI::EndHorizontal();
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

void CNeoRoot::OnFileSelected(const char *szFullpath)
{
	((m_ns.crosshair.eFileIOMode == vgui::FOD_OPEN) ?
				&ImportCrosshair : &ExportCrosshair)(&m_ns.crosshair.info, szFullpath);
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
	if (neo_cl_toggleconsole.GetBool())
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
