#pragma once

#include <vgui_controls/EditablePanel.h>
#include "GameUI/IGameUI.h"
#include <steam/isteamhttp.h>
#include <steam/steam_api.h>

#include "neo_ui.h"
#include "neo_root_serverbrowser.h"
#include "neo_root_settings.h"

class CAvatarImage;

struct NeoNewGame
{
	wchar_t wszMap[64] = L"nt_oilstain_ctg";
	wchar_t wszHostname[64] = L"NeoTokyo Rebuild";
	int iMaxPlayers = 24;
	wchar_t wszPassword[64] = L"neo";
	bool bFriendlyFire = true;
	bool bUseSteamNetworking = true;
};

class CNeoRoot;
// NEO JANK (nullsystem): This is really more of a workaround that
// keyboard inputs are not sent to panels (even if they're marked
// for allowing keyboard inputs) that are not popups or locked-in modal,
// so a panel with nothing drawn is used to relay the keys to the
// intended root panel.
// Also the root panel shouldn't be a popup otherwise that'll screw
// up with panel front/back ordering with other popup panels, the
// same goes for modal which locks input into the panel and that's
// not ideal either. Fetching keys via input() didn't work either,
// so we're left with this crap.
class CNeoRootInput : public vgui::Panel
{
	DECLARE_CLASS_SIMPLE(CNeoRootInput, vgui::Panel);
public:
	CNeoRootInput(CNeoRoot *rootPanel);
	void PerformLayout() final;
	void OnKeyCodeTyped(vgui::KeyCode code) final;
	void OnKeyTyped(wchar_t unichar) final;
	void OnThink();
	CNeoRoot *m_pNeoRoot = nullptr;
};

enum RootState
{
	STATE_ROOT,
	STATE_SETTINGS,
	STATE_NEWGAME,
	STATE_SERVERBROWSER,

	STATE_MAPLIST,
	STATE_SERVERDETAILS,
	STATE_PLAYERLIST,

	// Those that uses CNeoRoot::MainLoopPopup only starts here
	STATE__POPUPSTART,
	STATE_KEYCAPTURE = STATE__POPUPSTART,
	STATE_CONFIRMSETTINGS,
	STATE_QUIT,
	STATE_SERVERPASSWORD,
	STATE_SETTINGSRESETDEFAULT,

	STATE__TOTAL,
};

struct WidgetInfo
{
	const char *label;
	const char *gamemenucommand; // TODO: Replace
	RootState nextState;
	int flags;
};

enum WidgetInfoFlags
{
	FLAG_NONE = 0,
	FLAG_SHOWINGAME = 1 << 0,
	FLAG_SHOWINMAIN = 1 << 1,
};

enum MainMenuButtons
{
	MMBTN_RESUME = 0,
	MMBTN_FINDSERVER,
	MMBTN_CREATESERVER,
	MMBTN_DISCONNECT,
	MMBTN_PLAYERLIST,
	MMBTN_OPTIONS,
	MMBTN_QUIT,

	BTNS_TOTAL,
};

// This class is what is actually used instead of the main menu.
class CNeoRoot : public vgui::EditablePanel, public CGameEventListener
{
	DECLARE_CLASS_SIMPLE(CNeoRoot, vgui::EditablePanel);
public:
	CNeoRoot(vgui::VPANEL parent);
	virtual ~CNeoRoot();
	IGameUI *GetGameUI();

	bool LoadGameUI();
	void UpdateControls();

	IGameUI *m_gameui = nullptr;
	int m_iHoverBtn = -1;
	RootState m_state = STATE_ROOT;
	CAvatarImage *m_avImage = nullptr;

	wchar_t m_wszDispBtnTexts[BTNS_TOTAL][64] = {};
	int m_iWszDispBtnTextsSizes[BTNS_TOTAL] = {};

	CNeoRootInput *m_panelCaptureInput = nullptr;
	void OnRelayedKeyCodeTyped(vgui::KeyCode code);
	void OnRelayedKeyTyped(wchar_t unichar);
	void ApplySchemeSettings(vgui::IScheme *pScheme) final;
	void Paint() final;
	void OnMousePressed(vgui::MouseCode code) final;
	void OnMouseReleased(vgui::MouseCode code) final;
	void OnMouseDoublePressed(vgui::MouseCode code) final;
	void OnMouseWheeled(int delta) final;
	void OnCursorMoved(int x, int y) final;
	void OnTick() final;
	void FireGameEvent(IGameEvent *event) final;
	bool IsInGame() { return (engine->IsInGame() && !engine->IsLevelMainMenuBackground()); }

	void OnMainLoop(const NeoUI::Mode eMode);

	struct MainLoopParam
	{
		NeoUI::Mode eMode;
		int wide;
		int tall;
	};

	void MainLoopRoot(const MainLoopParam param);
	void MainLoopSettings(const MainLoopParam param);
	void MainLoopNewGame(const MainLoopParam param);
	void MainLoopServerBrowser(const MainLoopParam param);
	void MainLoopMapList(const MainLoopParam param);
	void MainLoopServerDetails(const MainLoopParam param);
	void MainLoopPlayerList(const MainLoopParam param);
	void MainLoopPopup(const MainLoopParam param);

	NeoSettings m_ns = {};

	NeoNewGame m_newGame = {};
	struct MapInfo
	{
		wchar_t wszName[64];
	};
	CUtlVector<MapInfo> m_vWszMaps;

	int m_iSelectedServer = -1; // TODO: Select kept with sorting
	int m_iServerBrowserTab = 0;
	CNeoServerList m_serverBrowser[GS__TOTAL];
	CNeoServerPlayers m_serverPlayers;
	ServerBrowserFilters m_sbFilters;
	bool m_bSBFiltModified = false;
	bool m_bShowFilterPanel = false;
	GameServerSortContext m_sortCtx = {};

	wchar_t m_wszBindingText[128];
	int m_iBindingIdx = -1;

	int m_iTitleWidth;
	int m_iTitleHeight;
	wchar_t m_wszHostname[128];
	wchar_t m_wszMap[128];

	wchar_t m_wszServerPassword[128] = {};

	CCallResult<CNeoRoot, HTTPRequestCompleted_t> m_ccallbackHttp;
	void HTTPCallbackRequest(HTTPRequestCompleted_t *request, bool bIOFailure);

	// Display maximum of 5 items on home screen
	struct NewsEntry
	{
		char szSitePath[64];
		wchar_t wszTitle[256];
	};
	static constexpr int MAX_NEWS = 5;
	NewsEntry m_news[MAX_NEWS] = {};
	int m_iNewsSize = 0;
	void ReadNewsFile(CUtlBuffer &buf);
	bool m_bShowBrowserLabel = false;

	vgui::FileOpenDialog *m_pFileIODialog = nullptr;
	MESSAGE_FUNC_CHARPTR(OnFileSelected, "FileSelected", fullpath);

	bool m_bOnLoadingScreen = false;
	int m_iSavedYOffsets[NeoUI::MAX_SECTIONS] = {};
	float m_flWideAs43 = 0.0f;
};

extern CNeoRoot *g_pNeoRoot;
