#pragma once

#include <vgui_controls/Panel.h>
#include <game/client/iviewport.h>
#include <GameEventListener.h>
#include <steam/steamclientpublic.h>

#include "neo_player_shared.h"
#include "ui/neo_ui.h"
#include "neo_crosshair.h"

struct MapAvatarValue
{
	int i32Idx;
	int i64Idx;
	int i184Idx;
};

struct CNEOUIScoreBoardPlayer
{
	int iUserID; // More reliable than player index
	int iTeam;
	int iDeaths;
	int iXP;
	int iClass;
	int iPing;
	// NEO TODO (nullsystem): Turn to flags? Add local state?
	bool bDead;
	bool bBot;
	bool bMuted;
	bool bReady;
	CSteamID steamID;
	MapAvatarValue avatar;
	wchar_t wszName[MAX_PLAYER_NAME_LENGTH];
	wchar_t wszClantag[NEO_MAX_CLANTAG_LENGTH];
	char szCrosshair[NEO_XHAIR_SEQMAX];
};

class CNEOUIScoreBoard : public vgui::Panel, public IViewPortPanel, public CGameEventListener
{
	DECLARE_CLASS_SIMPLE(CNEOUIScoreBoard, vgui::Panel);

public:
	CNEOUIScoreBoard(IViewPort *pViewPort);
	~CNEOUIScoreBoard();

	const char *GetName() final;
	void ApplySchemeSettings(vgui::IScheme *pScheme) final;
	void Paint() final;
	void OnMousePressed(vgui::MouseCode code) final;
	void OnMouseReleased(vgui::MouseCode code) final;
	void OnMouseDoublePressed(vgui::MouseCode code) final;
	void OnCursorMoved(int x, int y) final;
	void OnKeyCodePressed(vgui::KeyCode code) final;
	void OnKeyCodeReleased(vgui::KeyCode code) final;
	void Reset() final;
	bool NeedsUpdate() final;
	void Update() final;
	bool IsVisible() final { return BaseClass::IsVisible(); }
	void ToggleMouseCapture(const bool bUseMouse);

	void SetData([[maybe_unused]] KeyValues *data) final {}
	bool HasInputElements() final { return true; }
	void ShowPanel(bool state) final;
	GameActionSet_t GetPreferredActionSet() final { return GAME_ACTION_SET_IN_GAME_HUD; }
	vgui::VPANEL GetVPanel() final { return BaseClass::GetVPanel(); }
	void SetParent(vgui::VPANEL parent) final { BaseClass::SetParent( parent ); }
	void FireGameEvent(IGameEvent *event) final;

	bool ShowAvatars();
	bool UpdateAvatars();

	void OnMainLoop(const NeoUI::Mode eMode);

	MESSAGE_FUNC_INT(OnPollHideCode, "PollHideCode", code);
	void OnThink() final;

	NeoUI::Context m_uiCtx;

	int	m_HLTVSpectators = 0;
	int m_iTotalPlayers = 0;
	CNEOUIScoreBoardPlayer m_playersInfo[MAX_PLAYERS] = {};
	CNEOUIScoreBoardPlayer m_playerPopup = {};

	wchar_t m_wszHostname[128] = {};
	wchar_t m_wszMap[128] = {};

	vgui::ImageList *m_pImageList = nullptr;
	CUtlMap<CSteamID, MapAvatarValue> m_mapAvatarsToImageList;
	float m_flNextUpdateTime = 0.0f;
	ButtonCode_t m_nCloseKey = BUTTON_CODE_INVALID;

	enum EColsPlayers
	{
		COLSPLAYERS_PING = 0,
		COLSPLAYERS_AVATAR,
		COLSPLAYERS_NAME,
		COLSPLAYERS_READYUP,
		COLSPLAYERS_CLASS,
		COLSPLAYERS_RANK,
		COLSPLAYERS_XP,
		COLSPLAYERS_DEATH,

		COLSPLAYERS__TOTAL,
	};
	int m_iColsWidePlayersList[COLSPLAYERS__TOTAL] = {};

	enum EColsNonPlayers
	{
		COLSNONPLAYERS_PING = 0,
		COLSNONPLAYERS_AVATAR,
		COLSNONPLAYERS_NAME,

		COLSNONPLAYERS__TOTAL,
	};
	int m_iColsWideNonPlayersList[COLSNONPLAYERS__TOTAL] = {};
};

extern CNEOUIScoreBoard *g_pNeoUIScoreBoard;

