#pragma once

#include <vgui_controls/EditablePanel.h>
#include <vgui_controls/Frame.h>
#include "GameUI/IGameUI.h"
#include <steam/isteammatchmaking.h>

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

class CNeoOverlay_KeyCapture : public vgui::EditablePanel
{
	DECLARE_CLASS_SIMPLE(CNeoOverlay_KeyCapture, vgui::EditablePanel);
public:
	CNeoOverlay_KeyCapture(Panel *parent);
	void PerformLayout() final;
	void Paint() final;
	void OnThink() final;
	ButtonCode_t m_iButtonCode = BUTTON_CODE_INVALID;
	int m_iIndex = -1;
	wchar_t m_wszBindingText[128];

	vgui::HFont m_fontMain;
	vgui::HFont m_fontSub;
};

class CNeoOverlay_Confirm : public vgui::EditablePanel
{
	DECLARE_CLASS_SIMPLE(CNeoOverlay_Confirm, vgui::EditablePanel);
public:
	CNeoOverlay_Confirm(Panel *parent);
	void PerformLayout() final;
	void Paint() final;
	void OnKeyCodePressed(vgui::KeyCode code) final;
	void OnMousePressed(vgui::MouseCode code) final;
	void OnCursorMoved(int x, int y) final;
	vgui::HFont m_fontMain;
	bool m_bChoice = false;

	enum ButtonHover
	{
		BUTTON_NONE = 0,
		BUTTON_APPLY,
		BUTTON_DISCARD,
	};
	ButtonHover m_buttonHover = BUTTON_NONE;
};

struct CNeoDataRingBox
{
	const wchar_t **wszItems;
	int iItemsSize;
	int iCurIdx;
};

struct CNeoDataSlider
{
	static constexpr int LABEL_MAX = 32;

	int iValMin = 0;
	int iValMax = 0;
	int iValStep = 0;
	float flMulti = 0.0f;
	int iChMax = LABEL_MAX;

	wchar_t wszCacheLabel[LABEL_MAX + 1] = {};
	int iWszCacheLabelSize = 0;

	void SetValue(const float value);
	float GetValue() const;
	void ClampAndUpdate();
	int iValCur = 0;
};

struct CNeoDataTextEntry
{
	static constexpr int ENTRY_MAX = 64;
	wchar_t wszEntry[ENTRY_MAX + 1] = {};
};

struct CNeoDataBindEntry
{
	char szBindingCmd[64];
	wchar_t wszDisplayText[64];
	ButtonCode_t bcNext;
	ButtonCode_t bcCurrent;
};

struct CNeoDataTextLabel
{
	static constexpr int LABEL_MAX = 64;
	wchar_t wszLabel[LABEL_MAX + 1] = {};
};

struct CNeoDataMicTester
{
	float flSpeakingVol;
	float flLastFetchInterval;
};

struct CNeoDataGameServer
{
	gameserveritem_t info;
};

struct CNeoDataVariant
{
	enum Type
	{
		RINGBOX = 0,
		SLIDER,
		TEXTENTRY,
		BINDENTRY,
		TEXTLABEL,
		GAMESERVER,

		// S_ = Special type for specific use
		S_DISPLAYNAME,
		S_MICTESTER,
	};
	Type type;
	const wchar_t *label;
	int labelSize;
	bool disabled = false;
	union
	{
		CNeoDataRingBox ringBox;
		CNeoDataSlider slider;
		CNeoDataTextEntry textEntry;
		CNeoDataBindEntry bindEntry;
		CNeoDataTextLabel textLabel;
		CNeoDataMicTester micTester;
		CNeoDataGameServer gameServer;
	};
};

#define NDV_LABEL(wlabel) .label = wlabel, .labelSize = ((sizeof(wlabel) / sizeof(wchar_t)) - 1)
#define NDV_INIT_SLIDER(wlabel, min, max, step, multi, chmax) { \
	.type = CNeoDataVariant::SLIDER, NDV_LABEL(wlabel), \
	.slider = {(min), (max), (step), (multi), (chmax)} }
#define NDV_INIT_RINGBOX(wlabel, wszitems, wszitemssize) { \
	.type = CNeoDataVariant::RINGBOX, NDV_LABEL(wlabel), \
	.ringBox = { .wszItems = (wszitems), .iItemsSize = (wszitemssize)} }
#define NDV_INIT_RINGBOX_ONOFF(wlabel) NDV_INIT_RINGBOX(wlabel, ENABLED_LABELS, 2)
#define NDV_INIT_TEXTENTRY(wlabel) { \
	.type = CNeoDataVariant::TEXTENTRY, NDV_LABEL(wlabel), \
	.textEntry = {} }
#define NDV_INIT_S_DISPLAYNAME(wlabel) { \
	.type = CNeoDataVariant::S_DISPLAYNAME, NDV_LABEL(wlabel) }
#define NDV_INIT_S_MICTESTER(wlabel) { \
	.type = CNeoDataVariant::S_MICTESTER, NDV_LABEL(wlabel) }

struct CNeoDataSettings_Base
{
	virtual ~CNeoDataSettings_Base() {}
	virtual CNeoDataVariant *NdvList() = 0;
	virtual int NdvListSize() = 0;
	virtual void UserSettingsRestore() = 0;
	virtual void UserSettingsSave() = 0;
	virtual WLabelWSize Title() = 0;
};

struct CNeoDataSettings_Multiplayer : CNeoDataSettings_Base
{
	enum Options
	{
		OPT_MULTI_NEONAME = 0,
		OPT_MULTI_ONLYSTEAMNICK,
		OPT_MULTI_DISPNAME,
		OPT_MULTI_FOV,
		OPT_MULTI_VMFOV,
		OPT_MULTI_AIMHOLD,
		OPT_MULTI_RELOADEMPTY,
		OPT_MULTI_VMRIGHTHAND,
		OPT_MULTI_PLAYERSPRAYS,
		OPT_MULTI_SHOWPOS,
		OPT_MULTI_SHOWFPS,
		OPT_MULTI_DLFILTER,

		OPT_MULTI__TOTAL,
	};
	CNeoDataVariant m_ndvList[OPT_MULTI__TOTAL];

	CNeoDataSettings_Multiplayer();
	CNeoDataVariant *NdvList() final { return m_ndvList; }
	int NdvListSize() final { return OPT_MULTI__TOTAL; }
	void UserSettingsRestore() final;
	void UserSettingsSave() final;
	WLabelWSize Title() final { return LWS(L"Multiplayer"); }

	ConVarRef m_cvrClPlayerSprayDisable;
	ConVarRef m_cvrClDownloadFilter;
};

struct CNeoDataSettings_Keys : CNeoDataSettings_Base
{
	enum OptionsFixed
	{
		OPT_KEYS_FIXED_WEPFASTSWITCH = 0,
		OPT_KEYS_FIXED_DEVCONSOLE,

		OPT_KEYS_FIRST_NONFIXED,
	};

	CUtlVector<CNeoDataVariant> m_ndvVec;

	CNeoDataSettings_Keys();
	CNeoDataVariant *NdvList() final { return m_ndvVec.Base(); }
	int NdvListSize() final { return m_ndvVec.Size(); }
	void UserSettingsRestore() final;
	void UserSettingsSave() final;
	WLabelWSize Title() final { return LWS(L"Keybinds"); }
};

struct CNeoDataSettings_Mouse : CNeoDataSettings_Base
{
	enum Options
	{
		OPT_MOUSE_SENSITIVITY = 0,
		OPT_MOUSE_RAWINPUT,
		OPT_MOUSE_FILTER,
		OPT_MOUSE_REVERSE,
		OPT_MOUSE_CUSTOMACCEL,
		OPT_MOUSE_EXPONENT,

		OPT_MOUSE__TOTAL,
	};
	CNeoDataVariant m_ndvList[OPT_MOUSE__TOTAL] = {};

	CNeoDataSettings_Mouse();
	CNeoDataVariant *NdvList() final { return m_ndvList; }
	int NdvListSize() final { return OPT_MOUSE__TOTAL; }
	void UserSettingsRestore() final;
	void UserSettingsSave() final;
	WLabelWSize Title() final { return LWS(L"Mouse"); }

	ConVarRef m_cvrMFilter;
	ConVarRef m_cvrPitch;
	ConVarRef m_cvrCustomAccel;
	ConVarRef m_cvrCustomAccelExponent;
	ConVarRef m_cvrMRawInput;
};

struct CNeoDataSettings_Audio : CNeoDataSettings_Base
{
	enum Options
	{
		OPT_AUDIO_INP_VOLMAIN,
		OPT_AUDIO_INP_VOLMUSIC,
		OPT_AUDIO_INP_VOLVICTORY,
		OPT_AUDIO_INP_SURROUND,
		OPT_AUDIO_INP_QUALITY,
		OPT_AUDIO_INP_MUTELOSEFOCUS,

		OPT_AUDIO_OUT_VOICEENABLED,
		OPT_AUDIO_OUT_VOICERECV,
		//OPT_AUDIO_OUT_VOICESEND,	// This doesn't actually work, original dialog also fails on this
		OPT_AUDIO_OUT_MICBOOST,
		OPT_AUDIO_OUT_MICTESTER,

		OPT_AUDIO__TOTAL,
	};
	CNeoDataVariant m_ndvList[OPT_AUDIO__TOTAL] = {};

	CNeoDataSettings_Audio();
	CNeoDataVariant *NdvList() final { return m_ndvList; }
	int NdvListSize() final { return OPT_AUDIO__TOTAL; }
	void UserSettingsRestore() final;
	void UserSettingsSave() final;
	WLabelWSize Title() final { return LWS(L"Sound"); }

	ConVarRef m_cvrVolume;
	ConVarRef m_cvrSndMusicVolume;
	ConVarRef m_cvrSndSurroundSpeakers;
	ConVarRef m_cvrVoiceEnabled;
	ConVarRef m_cvrVoiceScale;
	ConVarRef m_cvrSndMuteLoseFocus;
	ConVarRef m_cvrSndPitchquality;
	ConVarRef m_cvrDspSlowCpu;
};

struct CNeoDataSettings_Video : CNeoDataSettings_Base
{
	enum LabelRingBoxEnum
	{
		OPT_VIDEO_RESOLUTION,
		OPT_VIDEO_WINDOWED,
		OPT_VIDEO_QUEUEMODE,
		OPT_VIDEO_ROOTLOD,
		OPT_VIDEO_MATPICMAP,
		OPT_VIDEO_MATREDUCEFILLRATE,
		OPT_VIDEO_WATERDETAIL,
		OPT_VIDEO_SHADOW,
		OPT_VIDEO_COLORCORRECTION,
		OPT_VIDEO_ANTIALIAS,
		OPT_VIDEO_FILTERING,
		OPT_VIDEO_VSYNC,
		OPT_VIDEO_MOTIONBLUR,
		OPT_VIDEO_HDR,
		OPT_VIDEO_GAMMA,

		OPT_VIDEO__TOTAL,
	};
	CNeoDataVariant m_ndvList[OPT_VIDEO__TOTAL] = {};

	CNeoDataSettings_Video();
	~CNeoDataSettings_Video();
	CNeoDataVariant *NdvList() final { return m_ndvList; }
	int NdvListSize() final { return OPT_VIDEO__TOTAL; }
	void UserSettingsRestore() final;
	void UserSettingsSave() final;
	WLabelWSize Title() final { return LWS(L"Video"); }

	ConVarRef m_cvrMatQueueMode;
	ConVarRef m_cvrRRootLod;
	ConVarRef m_cvrMatPicmip;
	ConVarRef m_cvrMatReducefillrate;
	ConVarRef m_cvrRWaterForceExpensive;
	ConVarRef m_cvrRWaterForceReflectEntities;
	ConVarRef m_cvrRFlashlightdepthtexture;
	ConVarRef m_cvrRShadowrendertotexture;
	ConVarRef m_cvrMatColorcorrection;
	ConVarRef m_cvrMatAntialias;
	ConVarRef m_cvrMatTrilinear;
	ConVarRef m_cvrMatForceaniso;
	ConVarRef m_cvrMatVsync;
	ConVarRef m_cvrMatMotionBlurEnabled;
	ConVarRef m_cvrMatHdrLevel;
	ConVarRef m_cvrMatMonitorGamma;

	int m_vmListSize = 0;
	vmode_t *m_vmList = nullptr;
	int m_iNativeWidth;
	int m_iNativeHeight;

	wchar_t *m_wszVmDispMem = nullptr;
	wchar_t **m_wszVmDispList = nullptr;
};

// NEO NOTE (nullsystem): If there's a client convar not saved, it's likely the way it been defined is wrong
// AKA they've been defined in both client + server when it should only be for client
//
// TODO: Partition to deal with > 1 row length widgets?
class CNeoPanel_Base : public vgui::EditablePanel
{
	DECLARE_CLASS_SIMPLE(CNeoPanel_Base, vgui::EditablePanel);
public:
	CNeoPanel_Base(vgui::Panel *parent);
	virtual ~CNeoPanel_Base() {}
	void UserSettingsRestore();
	void UserSettingsSave();

	void PerformLayout() final;
	void Paint() final;
	void OnMousePressed(vgui::MouseCode code) final;
	void OnMouseDoublePressed(vgui::MouseCode code) final;
	void OnMouseWheeled(int delta) final;

	void OnKeyCodeTyped(vgui::KeyCode code) final;
	void OnKeyTyped(wchar_t unichar) final;

	void OnCursorMoved(int x, int y) final;

	virtual void OnEnterBindEntry(CNeoDataVariant *ndv) = 0;
	virtual void OnBottomAction(const int btn) = 0;
	void OnExitTextEditMode(const int iOverrideNdsActive = -1);

	virtual CNeoDataSettings_Base **TabsList() = 0;
	virtual int TabsListSize() const = 0;
	virtual const WLabelWSize *BottomSectionList() const = 0;
	virtual int BottomSectionListSize() const = 0;
	virtual int KeyCodeToBottomAction(vgui::KeyCode code) const = 0;
	virtual void OnEnterServer([[maybe_unused]] const gameserveritem_t gameserver) { Assert(false); }
	virtual int TopAreaRows() const = 0;
	int TopAreaTall() const;
	virtual void TopAreaPaint() {}
	virtual void OnCursorMovedTopArea([[maybe_unused]] int x, [[maybe_unused]] int y) {}

	int m_iNdsCurrent = 0;
	int m_iNdsHover = -1;

	int m_iPosX = 0;
	//int m_iNdvHover = -1; // TODO: Also m_iNdvActive replaces m_bTextEditMode
	int m_iNdvActive = -1;
	int m_iBottomHover = -1;
	int m_iScrollOffset = 0;
	bool m_bTextEditMode = false;

	// NEO NOTE (nullsystem): Just to make it simple, any actions that changes
	// settings (key or mouse) just trigger this regardless if the value has been changed or not
	bool m_bModified = false;

	enum WidgetPos
	{
		WDG_NONE = 0,
		WDG_LEFT,
		WDG_CENTER,
		WDG_RIGHT,
	};
	WidgetPos m_curMouse = WDG_NONE;
};

class CNeoPanel_Settings : public CNeoPanel_Base
{
	DECLARE_CLASS_SIMPLE(CNeoPanel_Settings, CNeoPanel_Base);
public:
	CNeoPanel_Settings(vgui::Panel *parent);
	~CNeoPanel_Settings() override;
	void ExitSettings();

	enum Tabs
	{
		TAB_MULTI = 0,
		TAB_KEYS,
		TAB_MOUSE,
		TAB_SOUND,
		TAB_VIDEO,

		TAB__TOTAL,
	};

	enum BottomBtns
	{
		BBTN_BACK = 0,
		BBTN_LEGACY,
		BBTN_RESET,
		BBTN_UNUSEDIDX3,
		BBTN_APPLY,

		BBTN__TOTAL,
	};

	CNeoDataSettings_Multiplayer m_ndsMulti;
	CNeoDataSettings_Keys m_ndsKeys;
	CNeoDataSettings_Mouse m_ndsMouse;
	CNeoDataSettings_Audio m_ndsSound;
	CNeoDataSettings_Video m_ndsVideo;

	CNeoDataSettings_Base *m_pNdsBases[TAB__TOTAL] = {
		&m_ndsMulti,
		&m_ndsKeys,
		&m_ndsMouse,
		&m_ndsSound,
		&m_ndsVideo,
	};
	CNeoDataSettings_Base **TabsList() final { return m_pNdsBases; };
	int TabsListSize() const final { return TAB__TOTAL; };

	const WLabelWSize *BottomSectionList() const override;
	int BottomSectionListSize() const override { return BBTN__TOTAL; }
	int KeyCodeToBottomAction(vgui::KeyCode code) const override;
	void OnBottomAction(const int btn) final;
	int TopAreaRows() const final { return 1; }

	void OnEnterBindEntry(CNeoDataVariant *ndv) override;

	// Only for keybindings
	CNeoOverlay_KeyCapture *m_opKeyCapture = nullptr;
	MESSAGE_FUNC_PARAMS(OnKeybindUpdate, "KeybindUpdate", data);

	CNeoOverlay_Confirm *m_opConfirm = nullptr;
	MESSAGE_FUNC_PARAMS(OnConfirmDialogUpdate, "ConfirmDialogUpdate", data);
};

struct CNeoDataNewGame_General : CNeoDataSettings_Base
{
	enum Options
	{
		OPT_NEW_MAPLIST = 0,
		OPT_NEW_HOSTNAME,
		OPT_NEW_MAXPLAYERS,
		OPT_NEW_SVPASSWORD,
		OPT_NEW_FRIENDLYFIRE,

		OPT_NEW__TOTAL,
	};
	CNeoDataVariant m_ndvList[OPT_NEW__TOTAL];

	CNeoDataNewGame_General();
	CNeoDataVariant *NdvList() override { return m_ndvList; }
	int NdvListSize() override { return OPT_NEW__TOTAL; }
	void UserSettingsRestore() override {}
	void UserSettingsSave() override {}
	WLabelWSize Title() override { return LWS(L"New Game"); }
};

class CNeoPanel_NewGame : public CNeoPanel_Base
{
	DECLARE_CLASS_SIMPLE(CNeoPanel_NewGame, CNeoPanel_Base);
public:
	CNeoPanel_NewGame(vgui::Panel *parent);
	~CNeoPanel_NewGame() override {}

	void OnEnterBindEntry(CNeoDataVariant *ndv) override {}
	void OnBottomAction(const int btn) override;

	enum Tabs
	{
		TAB_GENERAL = 0,
		TAB__TOTAL,
	};
	CNeoDataNewGame_General m_ndsGeneral;
	CNeoDataSettings_Base *m_pNdsBases[TAB__TOTAL] = {
		&m_ndsGeneral,
	};
	CNeoDataSettings_Base **TabsList() final { return m_pNdsBases; };
	int TabsListSize() const final { return TAB__TOTAL; }

	enum BottomBtns
	{
		BBTN_BACK = 0,
		BBTN_UNUSEDIDX1,
		BBTN_UNUSEDIDX2,
		BBTN_UNUSEDIDX3,
		BBTN_GO,

		BBTN__TOTAL,
	};
	const WLabelWSize *BottomSectionList() const override;
	int BottomSectionListSize() const override { return BBTN__TOTAL; }
	int KeyCodeToBottomAction(vgui::KeyCode code) const override;
	int TopAreaRows() const final { return 0; }
};

class CNeoDataServerBrowser_General : public CNeoDataSettings_Base, public ISteamMatchmakingServerListResponse
{
public:
	GameServerType m_iType;
	CUtlVector<CNeoDataVariant> m_ndvVec;
	// CUtlVector<CNeoDataVariant> m_ndvFilteredVec;

	CNeoDataServerBrowser_General();
	CNeoDataVariant *NdvList() override { return m_ndvVec.Base(); }
	int NdvListSize() override { return m_ndvVec.Size(); }
	void UserSettingsRestore() override {}
	void UserSettingsSave() override {}
	WLabelWSize Title() override;

	void RequestList(MatchMakingKeyValuePair_t **filters, const uint32 iFiltersSize);
	void ServerResponded(HServerListRequest hRequest, int iServer) final;
	void ServerFailedToRespond(HServerListRequest hRequest, int iServer) final;
	void RefreshComplete(HServerListRequest hRequest, EMatchMakingServerResponse response) final;
	HServerListRequest m_hdlRequest;
};

struct CNeoDataServerBrowser_Filters : CNeoDataSettings_Base
{
	enum Options
	{
		OPT_FILTER_VAC = 0,

		OPT_FILTER__TOTAL,
	};
	CNeoDataVariant m_ndvList[OPT_FILTER__TOTAL];

	CNeoDataServerBrowser_Filters();
	CNeoDataVariant *NdvList() override { return m_ndvList; }
	int NdvListSize() override { return OPT_FILTER__TOTAL; }
	void UserSettingsRestore() override {}
	void UserSettingsSave() override {}
	WLabelWSize Title() override { return LWS(L"Filters"); }
};

class CNeoPanel_ServerBrowser : public CNeoPanel_Base
{
	DECLARE_CLASS_SIMPLE(CNeoPanel_ServerBrowser, CNeoPanel_Base);
public:
	CNeoPanel_ServerBrowser(vgui::Panel *parent);
	~CNeoPanel_ServerBrowser() override {}

	void OnEnterBindEntry(CNeoDataVariant *ndv) override {}
	void OnBottomAction(const int btn) override;
	void OnEnterServer(const gameserveritem_t gameserver) override;

	enum Tabs
	{
		TAB_FILTERS = GS__TOTAL,
		TAB__TOTAL,
	};
	CNeoDataServerBrowser_General m_ndsGeneral[GS__TOTAL];
	CNeoDataServerBrowser_Filters m_ndsFilters;
	CNeoDataSettings_Base *m_pNdsBases[TAB__TOTAL] = {
		&m_ndsGeneral[GS_INTERNET],
		&m_ndsGeneral[GS_LAN],
		&m_ndsGeneral[GS_FRIENDS],
		&m_ndsGeneral[GS_FAVORITES],
		&m_ndsGeneral[GS_HISTORY],
		&m_ndsGeneral[GS_SPEC],
		&m_ndsFilters,
	};
	CNeoDataSettings_Base **TabsList() final { return m_pNdsBases; };
	int TabsListSize() const final { return TAB__TOTAL; }

	enum BottomBtns
	{
		BBTN_BACK = 0,
		BBTN_LEGACY,
		BBTN_UNUSEDIDX2,
		BBTN_REFRESH,
		BBTN_GO,

		BBTN__TOTAL,
	};
	const WLabelWSize *BottomSectionList() const override;
	int BottomSectionListSize() const override { return BBTN__TOTAL; }
	int KeyCodeToBottomAction(vgui::KeyCode code) const override;
	int TopAreaRows() const final { return m_iNdsCurrent == TAB_FILTERS ? 1 : 2; }
	void TopAreaPaint() final;
	void OnCursorMovedTopArea(int x, int y) final;
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
	CNeoRoot *m_pNeoRoot = nullptr;
};

// This class is what is actually used instead of the main menu.
class CNeoRoot : public vgui::EditablePanel
{
	DECLARE_CLASS_SIMPLE(CNeoRoot, vgui::EditablePanel);
public:
	CNeoRoot(vgui::VPANEL parent);
	virtual ~CNeoRoot();
	IGameUI *GetGameUI();

	enum Buttons
	{
		BTN_RESUME = 0,
		BTN_SERVERBROWSER,
		BTN_SERVERCREATE,
		BTN_DISCONNECT,
		BTN_PLAYERLIST,
		BTN_SETTINGS,
		BTN_QUIT,

		BTN__TOTAL,
	};

	enum Fonts
	{
		FONT_NTNORMAL,
		FONT_NTSMALL,
		FONT_LOGO,

		FONT__TOTAL,
	};

	enum State
	{
		STATE_ROOT,
		STATE_SETTINGS,
		STATE_NEWGAME,
		STATE_SERVERBROWSER,

		STATE__TOTAL,
	};

	bool LoadGameUI();
	void UpdateControls();

	IGameUI *m_gameui = nullptr;
	int m_iHoverBtn = -1;
	State m_state = STATE_ROOT;
	CAvatarImage *m_avImage = nullptr;

	vgui::HFont m_hTextFonts[FONT__TOTAL] = {};

	CNeoPanel_Settings *m_panelSettings = nullptr;
	CNeoPanel_NewGame *m_panelNewGame = nullptr;
	CNeoPanel_ServerBrowser *m_panelServerBrowser = nullptr;

	CNeoOverlay_KeyCapture *m_opKeyCapture = nullptr;
	CNeoRootInput *m_panelCaptureInput = nullptr;

	wchar_t m_wszDispBtnTexts[BTN__TOTAL][64] = {};
	int m_iWszDispBtnTextsSizes[BTN__TOTAL] = {};
	int m_iBtnWide = 0;

	void OnRelayedKeyCodeTyped(vgui::KeyCode code);
protected:
	void ApplySchemeSettings(vgui::IScheme *pScheme) final;
	void Paint() final;
	void OnMousePressed(vgui::MouseCode code) final;
	void OnCursorExited() final;
	void OnCursorMoved(int x, int y) final;
};

extern CNeoRoot *g_pNeoRoot;
