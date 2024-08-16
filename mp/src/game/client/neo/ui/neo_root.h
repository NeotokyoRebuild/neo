#pragma once

#include <vgui_controls/EditablePanel.h>
#include "GameUI/IGameUI.h"
#include <steam/isteammatchmaking.h>

#include "neo_ui.h"

class CAvatarImage;

struct WLabelWSize
{
	const wchar_t *text;
	int size;
};
#define SZWSZ_LEN(wlabel) ((sizeof(wlabel) / sizeof(wlabel[0])) - 1)
#define LWSNULL WLabelWSize{ nullptr, 0 }
#define LWS(wlabel) WLabelWSize{ wlabel, SZWSZ_LEN(wlabel)}

enum GameServerType
{
	GS_INTERNET = 0,
	GS_LAN,
	GS_FRIENDS,
	GS_FAVORITES,
	GS_HISTORY,
	GS_SPEC,

	GS__TOTAL,
};

enum GameServerInfoW
{
	GSIW_LOCKED = 0,
	GSIW_VAC,
	GSIW_NAME,
	GSIW_MAP,
	GSIW_PLAYERS,
	GSIW_PING,

	GSIW__TOTAL,
};

struct GameServerSortContext
{
	GameServerInfoW col = GSIW_NAME;
	bool bDescending = false;
};

class CNeoServerList : public ISteamMatchmakingServerListResponse
{
public:
	GameServerType m_iType;
	CUtlVector<gameserveritem_t> m_servers;
	CUtlVector<gameserveritem_t> m_filteredServers;

	void UpdateFilteredList();
	void RequestList(MatchMakingKeyValuePair_t **filters, const uint32 iFiltersSize);
	void ServerResponded(HServerListRequest hRequest, int iServer) final;
	void ServerFailedToRespond(HServerListRequest hRequest, int iServer) final;
	void RefreshComplete(HServerListRequest hRequest, EMatchMakingServerResponse response) final;
	HServerListRequest m_hdlRequest = nullptr;
	bool m_bModified = false;
	bool m_bSearching = false;
	GameServerSortContext *m_pSortCtx = nullptr;
};

#define CONVARREF_DEF(_name) ConVarRef _name{#_name}

struct NeoSettings
{
	struct General
	{
		wchar_t wszNeoName[33];
		bool bOnlySteamNick;
		int iFov;
		int iViewmodelFov;
		bool bAimHold;
		bool bReloadEmpty;
		bool bViewmodelRighthand;
		bool bShowPlayerSprays;
		bool bShowPos;
		int iShowFps;
		int iDlFilter;
	};

	struct Keys
	{
		bool bWeaponFastSwitch;
		bool bDeveloperConsole;

		struct Bind
		{
			char szBindingCmd[64];
			wchar_t wszDisplayText[64];
			ButtonCode_t bcNext;
			ButtonCode_t bcCurrent; // Only used for unbinding
		};
		Bind vBinds[64];
		int iBindsSize = 0;
	};

	struct Mouse
	{
		float flSensitivity;
		bool bRawInput;
		bool bFilter;
		bool bReverse;
		bool bCustomAccel;
		float flExponent;
	};

	struct Audio
	{
		float flVolMain;
		float flVolMusic;
		float flVolVictory;
		int iSoundSetup;
		int iSoundQuality;
		bool bMuteAudioUnFocus;
		bool bVoiceEnabled;
		float flVolVoiceRecv;
		bool bMicBoost;
	};

	struct Video
	{
		int iResolution;
		int iWindow;
		int iCoreRendering;
		int iModelDetail;
		int iTextureDetail;
		int iShaderDetail;
		int iWaterDetail;
		int iShadowDetail;
		bool bColorCorrection;
		int iAntiAliasing;
		int iFilteringMode;
		bool bVSync;
		bool bMotionBlur;
		int iHDR;
		float flGamma;

		// Video modes
		int iVMListSize;
		vmode_t *vmList;
		wchar_t **p2WszVmDispList;
	};

	General general;
	Keys keys;
	Mouse mouse;
	Audio audio;
	Video video;

	int iCurTab = 0;
	bool bBack = false;
	bool bModified = false;
	int iNextBinding = -1;

	struct CVR
	{
		// Multiplayer
		CONVARREF_DEF(cl_player_spray_disable);
		CONVARREF_DEF(cl_download_filter);

		// Mouse
		CONVARREF_DEF(m_filter);
		CONVARREF_DEF(pitch);
		CONVARREF_DEF(m_customaccel);
		CONVARREF_DEF(m_customaccel_exponent);
		CONVARREF_DEF(m_raw_input);

		// Audio
		CONVARREF_DEF(volume);
		CONVARREF_DEF(snd_musicvolume);
		CONVARREF_DEF(snd_surround_speakers);
		CONVARREF_DEF(voice_enable);
		CONVARREF_DEF(voice_scale);
		CONVARREF_DEF(snd_mute_losefocus);
		CONVARREF_DEF(snd_pitchquality);
		CONVARREF_DEF(dsp_slow_cpu);

		// Video
		CONVARREF_DEF(mat_queue_mode);
		CONVARREF_DEF(r_rootlod);
		CONVARREF_DEF(mat_picmip);
		CONVARREF_DEF(mat_reducefillrate);
		CONVARREF_DEF(r_waterforceexpensive);
		CONVARREF_DEF(r_waterforcereflectentities);
		CONVARREF_DEF(r_flashlightdepthtexture);
		CONVARREF_DEF(r_shadowrendertotexture);
		CONVARREF_DEF(mat_colorcorrection);
		CONVARREF_DEF(mat_antialias);
		CONVARREF_DEF(mat_trilinear);
		CONVARREF_DEF(mat_forceaniso);
		CONVARREF_DEF(mat_vsync);
		CONVARREF_DEF(mat_motion_blur_enabled);
		CONVARREF_DEF(mat_hdr_level);
		CONVARREF_DEF(mat_monitorgamma);
	};
	CVR cvr;
};
void NeoSettingsInit(NeoSettings *ns);
void NeoSettingsDeinit(NeoSettings *ns);
void NeoSettingsRestore(NeoSettings *ns);
void NeoSettingsSave(const NeoSettings *ns);
void NeoSettingsMainLoop(NeoSettings *ns, const NeoUI::Mode eMode);

void NeoSettings_General(NeoSettings *ns);
void NeoSettings_Keys(NeoSettings *ns);
void NeoSettings_Mouse(NeoSettings *ns);
void NeoSettings_Audio(NeoSettings *ns);
void NeoSettings_Video(NeoSettings *ns);

struct NeoNewGame
{
	wchar_t wszMap[64] = L"nt_oilstain_ctg";
	wchar_t wszHostname[64] = L"NeoTokyo Rebuild";
	int iMaxPlayers = 24;
	wchar_t wszPassword[64] = L"neo";
	bool bFriendlyFire = true;
};

struct ServerBrowserFilters
{
	bool bServerNotFull = false;
	bool bHasUsersPlaying = false;
	bool bIsNotPasswordProtected = false;
	int iAntiCheat = 0;
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
	STATE_KEYCAPTURE,
	STATE_CONFIRMSETTINGS,
	STATE_QUIT,

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
static constexpr int BTNS_TOTAL = 7;
constexpr WidgetInfo BTNS_INFO[BTNS_TOTAL] = {
	{ "#GameUI_GameMenu_ResumeGame", "ResumeGame", STATE__TOTAL, FLAG_SHOWINGAME },
	{ "#GameUI_GameMenu_FindServers", nullptr, STATE_SERVERBROWSER, FLAG_SHOWINGAME | FLAG_SHOWINMAIN },
	{ "#GameUI_GameMenu_CreateServer", nullptr, STATE_NEWGAME, FLAG_SHOWINGAME | FLAG_SHOWINMAIN },
	{ "#GameUI_GameMenu_Disconnect", "Disconnect", STATE__TOTAL, FLAG_SHOWINGAME },
	{ "#GameUI_GameMenu_PlayerList", "OpenPlayerListDialog", STATE__TOTAL, FLAG_SHOWINGAME },
	{ "#GameUI_GameMenu_Options", nullptr, STATE_SETTINGS, FLAG_SHOWINGAME | FLAG_SHOWINMAIN },
	{ "#GameUI_GameMenu_Quit", nullptr, STATE_QUIT, FLAG_SHOWINGAME | FLAG_SHOWINMAIN },
};

// This class is what is actually used instead of the main menu.
class CNeoRoot : public vgui::EditablePanel
{
	DECLARE_CLASS_SIMPLE(CNeoRoot, vgui::EditablePanel);
public:
	CNeoRoot(vgui::VPANEL parent);
	virtual ~CNeoRoot();
	IGameUI *GetGameUI();

	enum Fonts
	{
		FONT_NTNORMAL,
		FONT_NTSMALL,
		FONT_LOGO,

		FONT__TOTAL,
	};

	bool LoadGameUI();
	void UpdateControls();

	IGameUI *m_gameui = nullptr;
	int m_iHoverBtn = -1;
	RootState m_state = STATE_ROOT;
	CAvatarImage *m_avImage = nullptr;

	vgui::HFont m_hTextFonts[FONT__TOTAL] = {};

	wchar_t m_wszDispBtnTexts[BTNS_TOTAL][64] = {};
	int m_iWszDispBtnTextsSizes[BTNS_TOTAL] = {};
	int m_iBtnWide = 0;

	CNeoRootInput *m_panelCaptureInput = nullptr;
	void OnRelayedKeyCodeTyped(vgui::KeyCode code);
	void OnRelayedKeyTyped(wchar_t unichar);
	void PaintRootMainSection();
	void ApplySchemeSettings(vgui::IScheme *pScheme) final;
	void Paint() final;
	void OnMousePressed(vgui::MouseCode code) final;
	void OnMouseDoublePressed(vgui::MouseCode code) final;
	void OnMouseWheeled(int delta) final;
	void OnCursorMoved(int x, int y) final;
	void OnTick() final;

	void OnMainLoop(const NeoUI::Mode eMode);
	NeoSettings m_ns = {};

	NeoNewGame m_newGame = {};
	struct MapInfo
	{
		wchar_t wszName[64];
	};
	CUtlVector<MapInfo> m_vWszMaps;

	int m_iSelectedServer = -1; // TODO: Select kept with sorting
	int m_iServerBrowserTab = 0;
	CNeoServerList m_serverBrowser[GS__TOTAL]; // TODO: Rename class
	ServerBrowserFilters m_sbFilters;
	bool m_bSBFiltModified = false;
	bool m_bShowFilterPanel = false;
	GameServerSortContext m_sortCtx = {};

	wchar_t m_wszBindingText[128];
	int m_iBindingIdx = -1;
};

extern CNeoRoot *g_pNeoRoot;
