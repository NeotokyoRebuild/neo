#include "neoui_scoreboard.h"

#include "c_neo_player.h"
#include "neo_gamerules.h"
#include "neo_theme.h"

#include <inputsystem/iinputsystem.h>
#include <voice_status.h>
#include <c_playerresource.h>
#include <steam/isteamutils.h>
#include <c_team.h>
#include <vgui_avatarimage.h>
#include <vgui_controls/ImageList.h>
#include <vgui/ILocalize.h>
#include <vgui/IInput.h>

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

ConVar cl_neo_hud_scoreboard_hide_others("cl_neo_hud_scoreboard_hide_others", "1", FCVAR_ARCHIVE, "Hide some other HUD elements when the scoreboard is displayed to prevent overlap", true, 0.0, true, 1.0);
ConVar neo_show_scoreboard_avatars("neo_show_scoreboard_avatars", "1", FCVAR_ARCHIVE, "Show avatars on scoreboard.", true, 0.0, true, 1.0 );
extern ConVar cl_neo_streamermode;
extern ConVar cl_neo_squad_hud_original;
extern ConVar sv_neo_readyup_lobby;
extern ConVar cl_neo_hud_team_swap_sides;

// NEO TODO (nullsystem):
// - [TODO] (As separate PR) Replaces current damage info HUD
// - [CURRENT] Right-click popup to look acceptable to use
// - [DONE] cl_neo_hud_team_swap_sides
// - [DONE] Complements readyup
// - [DONE] Replaces "player-list" mute/unmute toggle + indicator
// - [DONE] Right-click popup steam community profile link

enum ENeoScoreBoardPopup
{
	NEOSCOREBOARDPOPUP_CARD = NeoUI::INTERNALPOPUP_NIL + 1,
	NEOSCOREBOARDPOPUP_COPYCROSSHAIR,
};

CNEOUIScoreBoard *g_pNeoUIScoreBoard = nullptr;

CNEOUIScoreBoard::CNEOUIScoreBoard(IViewPort *pViewPort)
	: Panel(nullptr, PANEL_SCOREBOARD)
{
	SetupNTRETheme(&m_uiCtx);
	SetProportional(true);
	SetKeyBoardInputEnabled(false);
	SetMouseInputEnabled(false);
	vgui::surface()->CreatePopup(GetVPanel(), false, false, false, true, false);

	SetScheme("ClientScheme");

	ListenForGameEvent("server_spawn");
	ListenForGameEvent("game_newmap");
	ListenForGameEvent("hltv_status");

	m_mapAvatarsToImageList.SetLessFunc(DefLessFunc(CSteamID));
	m_mapAvatarsToImageList.RemoveAll();

	g_pNeoUIScoreBoard = this;
}

CNEOUIScoreBoard::~CNEOUIScoreBoard()
{
}

const char *CNEOUIScoreBoard::GetName()
{
	return PANEL_SCOREBOARD;
}

void CNEOUIScoreBoard::OnPollHideCode(int code)
{
	m_nCloseKey = static_cast<ButtonCode_t>(code);
}

void CNEOUIScoreBoard::OnThink()
{
	BaseClass::OnThink();

	// NOTE: this is necessary because of the way input works.
	// If a key down message is sent to vgui, then it will get the key up message
	// Sometimes the scoreboard is activated by other vgui menus,
	// sometimes by console commands. In the case where it's activated by
	// other vgui menus, we lose the key up message because this panel
	// doesn't accept keyboard input. It *can't* accept keyboard input
	// because another feature of the dialog is that if it's triggered
	// from within the game, you should be able to still run around while
	// the scoreboard is up. That feature is impossible if this panel accepts input.
	// because if a vgui panel is up that accepts input, it prevents the engine from
	// receiving that input. So, I'm stuck with a polling solution.
	//
	// Close key is set to non-invalid when something other than a keybind
	// brings the scoreboard up, and it's set to invalid as soon as the
	// dialog becomes hidden.
	if (m_nCloseKey != BUTTON_CODE_INVALID
			&& !g_pInputSystem->IsButtonDown(m_nCloseKey))
	{
		m_nCloseKey = BUTTON_CODE_INVALID;
		gViewPortInterface->ShowPanel(PANEL_SCOREBOARD, false);
		GetClientVoiceMgr()->StopSquelchMode();
	}
}

void CNEOUIScoreBoard::ShowPanel(bool bShow)
{
	if (NEORules() && NEORules()->GetHiddenHudElements() & NEO_HUD_ELEMENT_SCOREBOARD)
	{
		return;
	}
	// Catch the case where we call ShowPanel before ApplySchemeSettings, eg when
	// going from windowed <-> fullscreen
	if (m_pImageList == NULL)
	{
		InvalidateLayout(true, true);
	}

	if (!bShow)
	{
		m_nCloseKey = BUTTON_CODE_INVALID;
	}

	if (BaseClass::IsVisible() == bShow)
	{
		return;
	}

	SetMouseInputEnabled(false);
	SetKeyBoardInputEnabled(false);
	if (bShow)
	{
		Reset();
		Update();
		SetVisible(true);
		MoveToFront();
	}
	else
	{
		BaseClass::SetVisible(false);
		SetMouseInputEnabled(false);
	}
}

void CNEOUIScoreBoard::FireGameEvent(IGameEvent *event)
{
	const char *type = event->GetName();
	if (0 == V_strcmp(type, "hltv_status"))
	{
		// NEO TODO (nullsystem): Show HLTV spec count?
		// spectators = clients - proxies
		m_HLTVSpectators = event->GetInt("clients");
		m_HLTVSpectators -= event->GetInt("proxies");
	}

	if (IsVisible())
	{
		Update();
	}
}

void CNEOUIScoreBoard::ApplySchemeSettings(vgui::IScheme *pScheme)
{
	BaseClass::ApplySchemeSettings(pScheme);

	int wide, tall;
	vgui::surface()->GetScreenSize(wide, tall);
	SetSize(wide, tall);
	SetFgColor(COLOR_TRANSPARENT);
	SetBgColor(COLOR_TRANSPARENT);

	static constexpr const char *FONT_NAMES[NeoUI::FONT__TOTAL] = {
		"HudSelectionText", //"NeoUIScoreboard",
		"NHudOCR",
		"NHudOCRSmallNoAdditive",
		"ClientTitleFont",
		"ClientTitleFontSmall",
		"NeoUINormal",
	};
	for (int i = 0; i < NeoUI::FONT__TOTAL; ++i)
	{
		m_uiCtx.fonts[i].hdl = pScheme->GetFont(FONT_NAMES[i], true);
	}

	m_playerPopup = {};
	m_iTotalPlayers = 0;
	if (m_pImageList)
	{
		delete m_pImageList;
	}
	m_pImageList = new vgui::ImageList(false);
	m_mapAvatarsToImageList.RemoveAll();

	V_memset(m_iColsWidePlayersList, 0, sizeof(m_iColsWidePlayersList));
	V_memset(m_iColsWideNonPlayersList, 0, sizeof(m_iColsWideNonPlayersList));

	m_flNextUpdateTime = gpGlobals->curtime + 0.1f;

	NeoUI::ClosePopup();
}

void CNEOUIScoreBoard::Paint()
{
	OnMainLoop(NeoUI::MODE_PAINT);
}

void CNEOUIScoreBoard::OnMousePressed(vgui::MouseCode code)
{
	if (IsMouseInputEnabled())
	{
		m_uiCtx.eCode = code;
		OnMainLoop(NeoUI::MODE_MOUSEPRESSED);
	}
}

void CNEOUIScoreBoard::OnMouseReleased(vgui::MouseCode code)
{
	if (IsMouseInputEnabled())
	{
		m_uiCtx.eCode = code;
		OnMainLoop(NeoUI::MODE_MOUSERELEASED);
	}
}

void CNEOUIScoreBoard::OnMouseDoublePressed(vgui::MouseCode code)
{
	if (IsMouseInputEnabled())
	{
		m_uiCtx.eCode = code;
		OnMainLoop(NeoUI::MODE_MOUSEDOUBLEPRESSED);
	}
}

void CNEOUIScoreBoard::OnCursorMoved(int x, int y)
{
	if (IsMouseInputEnabled())
	{
		m_uiCtx.iMouseAbsX = x;
		m_uiCtx.iMouseAbsY = y;
		OnMainLoop(NeoUI::MODE_MOUSEMOVED);
	}
}

void CNEOUIScoreBoard::OnKeyCodePressed([[maybe_unused]] vgui::KeyCode code)
{
	// no-op
}

void CNEOUIScoreBoard::OnKeyCodeReleased([[maybe_unused]] vgui::KeyCode code)
{
	// no-op
}

void CNEOUIScoreBoard::Reset()
{
	m_iTotalPlayers = 0;
	V_memset(m_playersInfo, 0, sizeof(m_playersInfo));
	m_flNextUpdateTime = 0;
	m_mapAvatarsToImageList.RemoveAll();
}

void CNEOUIScoreBoard::ToggleMouseCapture(const bool bUseMouse)
{
	SetMouseInputEnabled(bUseMouse);
	m_uiCtx.iHotPersist = m_uiCtx.iActive = m_uiCtx.iHot = NeoUI::FOCUSOFF_NUM;
	m_uiCtx.iHotPersistSection = m_uiCtx.iActiveSection = m_uiCtx.iHotSection = -1;
	NeoUI::ClosePopup();
}

bool CNEOUIScoreBoard::NeedsUpdate()
{
	return (m_flNextUpdateTime < gpGlobals->curtime);
}

void CNEOUIScoreBoard::Update()
{
	if (!g_PR || !NEORules())
	{
		return;
	}

	C_NEO_Player *pLocalPlayer = C_NEO_Player::GetLocalNEOPlayer();
	const int iLocalPlayerTeam = pLocalPlayer->GetTeamNumber();
	const bool bLocalPlaying = (TEAM_JINRAI == iLocalPlayerTeam || TEAM_NSF == iLocalPlayerTeam);
	const bool bNotTeamplay = !NEORules()->IsTeamplay();

	m_iTotalPlayers = 0;
	for (int i = 1; i <= gpGlobals->maxClients; ++i)
	{
		if (false == g_PR->IsConnected(i))
		{
			continue;
		}

		C_NEO_Player *pNeoPlayer = ToNEOPlayer(UTIL_PlayerByIndex(i));
		C_NEO_Player *pNeoImpersonator = pNeoPlayer
				? ToNEOPlayer(pNeoPlayer->m_hSpectatorTakeoverPlayerImpersonatingMe.Get())
				: nullptr;

		const bool bIsImpersonating = pNeoPlayer
				&& pNeoPlayer->m_hSpectatorTakeoverPlayerTarget.Get();

		CNEOUIScoreBoardPlayer *pPlayerInfo = &m_playersInfo[m_iTotalPlayers];
		pPlayerInfo->iUserID = g_PR->GetUserID(i);
		pPlayerInfo->iTeam = g_PR->GetTeam(i);
		pPlayerInfo->iPing = (g_PR->IsFakePlayer(i))
				? -1
				: g_PR->GetPing(i);
		pPlayerInfo->bBot = g_PR->IsFakePlayer(i);
		pPlayerInfo->bMuted = GetClientVoiceMgr()->IsPlayerBlocked(i);
		V_strcpy_safe(pPlayerInfo->szCrosshair, g_PR->GetNeoCrosshair(i));
		pPlayerInfo->bReady = g_PR->IsReady(i);

		// pPlayerInfo->steamID
		player_info_t pi;
		if (SteamUtils() && engine->GetPlayerInfo(i, &pi) && pi.friendsID)
		{
			pPlayerInfo->steamID = CSteamID{
					pi.friendsID,
					1,
					SteamUtils()->GetConnectedUniverse(),
					k_EAccountTypeIndividual};
		}
		else
		{
			pPlayerInfo->steamID.Clear();
		}

		const bool bIsPlaying = (TEAM_JINRAI == pPlayerInfo->iTeam || TEAM_NSF == pPlayerInfo->iTeam);
		pPlayerInfo->iXP = bIsPlaying ? g_PR->GetXP(i) : 0;
		pPlayerInfo->iDeaths = bIsPlaying ? g_PR->GetDeaths(i) : 0;

		// pPlayerInfo->iClass
		const bool bShowClass = 
				   (false == bLocalPlaying) // Spectating
				|| (bNotTeamplay && g_PR->IsLocalPlayer(i)) // DM - Only see own class
				|| (false == bNotTeamplay && iLocalPlayerTeam == pPlayerInfo->iTeam); // Team - See own team's classes
		if (bShowClass)
		{
			pPlayerInfo->iClass = (bIsImpersonating)
					? pNeoPlayer->m_iClassBeforeTakeover
					: g_PR->GetClass(i);
		}
		else
		{
			pPlayerInfo->iClass = -1;
		}

		// pPlayerInfo->wszName and pPlayerInfo->wszClantag
		char szName[MAX_PLAYER_NAME_LENGTH] = {};
		char szClantag[NEO_MAX_CLANTAG_LENGTH] = {};
		if (pNeoImpersonator)
		{
			UTIL_MakeSafeName(
					pNeoImpersonator->GetPlayerNameWithTakeoverContext(
							pNeoImpersonator->entindex())
					, szName, ARRAYSIZE(szName));
		}
		else
		{
			const char *pClantag = g_PR->GetClanTag(i);
			if (pClantag && pClantag[0]
					&& (!cl_neo_streamermode.GetBool() || g_PR->IsLocalPlayer(i)))
			{
				UTIL_MakeSafeName(pClantag, szClantag, ARRAYSIZE(szClantag));
			}
			UTIL_MakeSafeName(g_PR->GetPlayerName(i), szName, ARRAYSIZE(szName));
		}
		g_pVGuiLocalize->ConvertANSIToUnicode(
				szName, pPlayerInfo->wszName, sizeof(pPlayerInfo->wszName));
		g_pVGuiLocalize->ConvertANSIToUnicode(
				szClantag, pPlayerInfo->wszClantag, sizeof(pPlayerInfo->wszClantag));

		// pPlayerInfo->bDead
		pPlayerInfo->bDead = (false == g_PR->IsAlive(i));
		if (pPlayerInfo->iTeam == TEAM_JINRAI || pPlayerInfo->iTeam == TEAM_NSF)
		{
			if (bIsImpersonating)
			{
				// Former spectators impersonating other players are (un)dead
				pPlayerInfo->bDead = true;
			}
			else if (pNeoImpersonator)
			{
				// Do not show death icon for players being impersonated
				pPlayerInfo->bDead = false;
			}
		}

		// pPlayerInfo->avatar
		if (UpdateAvatars() && pPlayerInfo->steamID.IsValid())
		{
			// See if we already have that avatar in our list
			int iMapIndex = m_mapAvatarsToImageList.Find(pPlayerInfo->steamID);
			if (iMapIndex == m_mapAvatarsToImageList.InvalidIndex())
			{
				auto *pImage32 = new CAvatarImage;
				auto *pImage64 = new CAvatarImage;
				auto *pImage184 = new CAvatarImage;

				pImage32->SetAvatarSteamID(pPlayerInfo->steamID, k_EAvatarSize32x32);
				pImage64->SetAvatarSteamID(pPlayerInfo->steamID, k_EAvatarSize64x64);
				pImage184->SetAvatarSteamID(pPlayerInfo->steamID, k_EAvatarSize184x184);

				pPlayerInfo->avatar = {
					.i32Idx = m_pImageList->AddImage(pImage32),
					.i64Idx = m_pImageList->AddImage(pImage64),
					.i184Idx = m_pImageList->AddImage(pImage184),
				};

				m_mapAvatarsToImageList.Insert(pPlayerInfo->steamID, pPlayerInfo->avatar);
			}
			else
			{
				pPlayerInfo->avatar = m_mapAvatarsToImageList[iMapIndex];
			}
		}
		else
		{
			pPlayerInfo->avatar.i32Idx = -1;
			pPlayerInfo->avatar.i64Idx = -1;
			pPlayerInfo->avatar.i184Idx = -1;
		}

		++m_iTotalPlayers;
	}

	// Sort players by XP, if equal by death
	V_qsort_s(m_playersInfo, m_iTotalPlayers, sizeof(CNEOUIScoreBoardPlayer),
			[]([[maybe_unused]] void *vpCtx, const void *vpLeft, const void *vpRight) -> int {
		auto *pLeft = static_cast<const CNEOUIScoreBoardPlayer *>(vpLeft);
		auto *pRight = static_cast<const CNEOUIScoreBoardPlayer *>(vpRight);
		if (pLeft->iXP == pRight->iXP)
		{
			if (pLeft->iDeaths == pRight->iDeaths)
			{
				// Alphabetical order
				return V_wcscmp(pLeft->wszName, pRight->wszName);
			}
			// More deaths = lower
			return pLeft->iDeaths - pRight->iDeaths;
		}
		// More XP = higher
		return pRight->iXP - pLeft->iXP;
	}, nullptr);

	// NEO JANK (nullsystem): FireGameEvent is unreliable for fetching
	// hostname and current map so just poll it from ConVar/NEORules instead
	const ConVarRef cvr_hostname("hostname");
	g_pVGuiLocalize->ConvertANSIToUnicode(cvr_hostname.GetString(),
			m_wszHostname, sizeof(m_wszHostname));
	g_pVGuiLocalize->ConvertANSIToUnicode(NEORules()->MapName(),
			m_wszMap, sizeof(m_wszMap));

	m_flNextUpdateTime = gpGlobals->curtime + 1.0f;
}

bool CNEOUIScoreBoard::ShowAvatars()
{
	return neo_show_scoreboard_avatars.GetBool() && !cl_neo_streamermode.GetBool();
}

bool CNEOUIScoreBoard::UpdateAvatars()
{
	return !cl_neo_streamermode.GetBool() && (neo_show_scoreboard_avatars.GetBool() || !cl_neo_squad_hud_original.GetBool());
}

void CNEOUIScoreBoard::OnMainLoop(const NeoUI::Mode eMode)
{
	if (!NEORules())
	{
		return;
	}

	int wide, tall;
	vgui::surface()->GetScreenSize(wide, tall);

	// other resolution scales up/down from it
	m_uiCtx.layout.iRowTall = m_uiCtx.layout.iDefRowTall = tall / 35;
	m_uiCtx.iMarginX = wide / 192 / 2;
	m_uiCtx.iMarginY = tall / 108 / 2;
	int iWideAs43 = static_cast<float>(tall) * (4.0f / 3.0f);
	if (iWideAs43 > wide) iWideAs43 = wide;
	const int iRootSubPanelWide = static_cast<int>(iWideAs43 * 0.975f);
	const int iPopupCardPerRowTallAvatarName = m_uiCtx.layout.iDefRowTall * 3;
	const int iPopupCardPerRowTallButtons = m_uiCtx.layout.iDefRowTall * 2.25f;
	const bool bShowReadyUp = sv_neo_readyup_lobby.GetBool()
			&& NEORules()->m_nRoundStatus == NeoRoundStatus::Idle;

	bool bHasRanklessDog = false;
	int iaTeamTally[TEAM__TOTAL] = {};
	int iaTeamReadyTally[TEAM__TOTAL] = {};
	for (int i = 0; i < m_iTotalPlayers; ++i)
	{
		const int iTeam = m_playersInfo[i].iTeam;
		if (IN_BETWEEN_AR(0, iTeam, TEAM__TOTAL))
		{
			++iaTeamTally[iTeam];
			iaTeamReadyTally[iTeam] += m_playersInfo[i].bReady;

			if (iTeam >= FIRST_GAME_TEAM &&
					m_playersInfo[i].iXP < 0)
			{
				bHasRanklessDog = true;
			}
		}
	}

	// Reset ranked column size depending if there's
	// rankless dog or not
	//
	// Reset column sizes if wide/tall differs
	static bool bPrevHasRanklessDog = false, bPrevShowReadyUp = false;
	static int prevWide = 0, prevTall = 0;
	if (bPrevHasRanklessDog != bHasRanklessDog
			|| bPrevShowReadyUp != bShowReadyUp
			|| wide != prevWide
			|| tall != prevTall)
	{
		V_memset(m_iColsWidePlayersList, 0, sizeof(m_iColsWidePlayersList));
		V_memset(m_iColsWideNonPlayersList, 0, sizeof(m_iColsWideNonPlayersList));
	}
	bPrevHasRanklessDog = bHasRanklessDog;
	bPrevShowReadyUp = bShowReadyUp;
	prevWide = wide;
	prevTall = tall;

	const int iGap = m_uiCtx.iMarginX;
	const bool bNotTeamplay = !NEORules()->IsTeamplay();
	const int iMaxSidePlayers = (bNotTeamplay)
			? Ceil2Int((iaTeamTally[TEAM_JINRAI] + iaTeamTally[TEAM_NSF]) / 2.0f)
			: Max(iaTeamTally[TEAM_JINRAI], iaTeamTally[TEAM_NSF]);
	C_NEO_Player *pLocalPlayer = C_NEO_Player::GetLocalNEOPlayer();
	const int iLocalUserID = pLocalPlayer->GetUserID();
	const int iLocalPlayerTeam = pLocalPlayer->GetTeamNumber();

	int iTies = 0;
	if (false == bNotTeamplay)
	{
		auto pTeamJinrai = GetGlobalTeam(TEAM_JINRAI);
		auto pTeamNSF = GetGlobalTeam(TEAM_NSF);
		if (pTeamJinrai && pTeamNSF)
		{
			// NEO NOTE (nullsystem): PostRound have GetRoundsWon updated, but not
			// roundNumber, so assume ties like next round
			iTies = Max(0,
					NEORules()->roundNumber()
						+ ((NEORules()->GetRoundStatus() == PostRound) ? +0 : -1)
						- pTeamJinrai->GetRoundsWon()
						- pTeamNSF->GetRoundsWon());
		}
	}

	NeoUI::BeginContext(&m_uiCtx, eMode, nullptr, "ScoreboardCtx");
	{
		// Figure out tall length of the scoreboard
		int iTallTotal = m_uiCtx.layout.iRowTall * (1 + 1 + iMaxSidePlayers);
		if (iaTeamTally[TEAM_UNASSIGNED] > 0) iTallTotal += m_uiCtx.layout.iRowTall * (1 + iaTeamTally[TEAM_UNASSIGNED]);
		if (iaTeamTally[TEAM_SPECTATOR] > 0) iTallTotal += m_uiCtx.layout.iRowTall * (1 + iaTeamTally[TEAM_SPECTATOR]);

		// Output server's name left-aligned, map's name right-aligned
		m_uiCtx.dPanel.x = (wide / 2) - (iRootSubPanelWide / 2);
		m_uiCtx.dPanel.y = (tall / 2) - (iTallTotal / 2);
		m_uiCtx.dPanel.wide = iRootSubPanelWide;
		m_uiCtx.dPanel.tall = m_uiCtx.layout.iRowTall;
		m_uiCtx.colors.normalFg = COLOR_NEO_WHITE;
		m_uiCtx.colors.sectionBg = COLOR_TRANSPARENT;
		m_uiCtx.colors.divider = COLOR_TRANSPARENT;

		NeoUI::BeginSection(NeoUI::SECTIONFLAG_DISABLEOFFSETS);
		{
			NeoUI::SetPerRowLayout(3);

			NeoUI::LabelExOpt opt = {
				.eTextStyle = NeoUI::TEXTSTYLE_LEFT,
				.eFont = m_uiCtx.eFont,
			};

			// Server's name
			NeoUI::Label(m_wszHostname, opt);

			// Tie counter
			if (iTies > 0)
			{
				opt.eTextStyle = NeoUI::TEXTSTYLE_CENTER;
				wchar_t wszText[32];
				V_swprintf_safe(wszText, L"Ties: %d", iTies);
				NeoUI::Label(wszText, opt);
			}
			else
			{
				NeoUI::Pad();
			}

			// Map's name
			opt.eTextStyle = NeoUI::TEXTSTYLE_RIGHT;
			NeoUI::Label(m_wszMap, opt);
		}
		NeoUI::EndSection();

		m_uiCtx.colors.sectionBg = COLOR_BLACK_TRANSPARENT;

		// Output all the players in the server
		for (int iCurTeam = TEAM_UNASSIGNED; iCurTeam < TEAM__TOTAL; ++iCurTeam)
		{
			if (iaTeamTally[iCurTeam] <= 0 && iCurTeam <= TEAM_SPECTATOR)
			{
				continue;
			}
			m_uiCtx.dPanel.wide = (iCurTeam <= TEAM_SPECTATOR)
					? iRootSubPanelWide
					: iRootSubPanelWide / 2;
			if (iCurTeam >= FIRST_GAME_TEAM)
			{
				m_uiCtx.dPanel.wide -= (iGap / 2);
			}

			const bool bNSFFirst = cl_neo_hud_team_swap_sides.GetBool() && TEAM_NSF == iLocalPlayerTeam;

			m_uiCtx.dPanel.x = (wide / 2) - (iRootSubPanelWide / 2);
			if ((bNSFFirst && TEAM_JINRAI == iCurTeam)
					|| (false == bNSFFirst && TEAM_NSF == iCurTeam))
			{
				m_uiCtx.dPanel.x += (iRootSubPanelWide / 2) + (iGap / 2);
			}

			m_uiCtx.dPanel.y = (tall / 2) - (iTallTotal / 2) + m_uiCtx.layout.iRowTall;
			if (iCurTeam <= TEAM_SPECTATOR)
			{
				m_uiCtx.dPanel.y += (m_uiCtx.layout.iRowTall * (1 + iMaxSidePlayers)) + iGap;
				if (TEAM_SPECTATOR == iCurTeam && iaTeamTally[TEAM_UNASSIGNED] > 0)
				{
					m_uiCtx.dPanel.y += (m_uiCtx.layout.iRowTall * (1 + iaTeamTally[TEAM_UNASSIGNED]));
				}
			}
			// One for the heading
			m_uiCtx.dPanel.tall = m_uiCtx.layout.iRowTall * (1 + ((iCurTeam >= FIRST_GAME_TEAM) ? iMaxSidePlayers : iaTeamTally[iCurTeam]));

			NeoUI::BeginSection(NeoUI::SECTIONFLAG_DISABLEOFFSETS);
			{
				if (0 == m_iColsWidePlayersList[0])
				{
					m_iColsWidePlayersList[COLSPLAYERS_PING] = NeoUI::SuitableWideByWStr(L"BOT", NeoUI::SUITABLEWIDE_TABLE);
					m_iColsWidePlayersList[COLSPLAYERS_AVATAR] = m_uiCtx.layout.iRowTall;
					m_iColsWidePlayersList[COLSPLAYERS_NAME] = 0;
					m_iColsWidePlayersList[COLSPLAYERS_READYUP] = bShowReadyUp ? NeoUI::SuitableWideByWStr(L"NOT READY", NeoUI::SUITABLEWIDE_TABLE) : 0;
					m_iColsWidePlayersList[COLSPLAYERS_CLASS] = NeoUI::SuitableWideByWStr(L"Support", NeoUI::SUITABLEWIDE_TABLE);
					m_iColsWidePlayersList[COLSPLAYERS_RANK] = NeoUI::SuitableWideByWStr(bHasRanklessDog ? L"Rankless Dog" : L"Lieutenant", NeoUI::SUITABLEWIDE_TABLE);
					m_iColsWidePlayersList[COLSPLAYERS_XP] = NeoUI::SuitableWideByWStr(L"-99", NeoUI::SUITABLEWIDE_TABLE);
					m_iColsWidePlayersList[COLSPLAYERS_DEATH] = NeoUI::SuitableWideByWStr(L"Deaths", NeoUI::SUITABLEWIDE_TABLE);

					int iTotalColsUsed = 0;
					for (int i = 0; i < COLSPLAYERS__TOTAL; ++i)
					{
						iTotalColsUsed += m_iColsWidePlayersList[i];
					}
					m_iColsWidePlayersList[COLSPLAYERS_NAME] = m_uiCtx.dPanel.wide - iTotalColsUsed;
				}
				if (0 == m_iColsWideNonPlayersList[0])
				{
					m_iColsWideNonPlayersList[COLSNONPLAYERS_PING] = NeoUI::SuitableWideByWStr(L"BOT", NeoUI::SUITABLEWIDE_TABLE);
					m_iColsWideNonPlayersList[COLSNONPLAYERS_AVATAR] = m_uiCtx.layout.iRowTall;
					m_iColsWideNonPlayersList[COLSNONPLAYERS_NAME] = 0;

					int iTotalColsUsed = 0;
					for (int i = 0; i < COLSNONPLAYERS__TOTAL; ++i)
					{
						iTotalColsUsed += m_iColsWideNonPlayersList[i];
					}
					m_iColsWideNonPlayersList[COLSNONPLAYERS_NAME] = m_uiCtx.dPanel.wide - iTotalColsUsed;
				}

				NeoUI::BeginTable(
						(iCurTeam >= FIRST_GAME_TEAM)
								? m_iColsWidePlayersList
								: m_iColsWideNonPlayersList,
						(iCurTeam >= FIRST_GAME_TEAM)
								? ARRAYSIZE(m_iColsWidePlayersList)
								: ARRAYSIZE(m_iColsWideNonPlayersList));

				m_uiCtx.colors.normalFg = (bNotTeamplay && iCurTeam >= FIRST_GAME_TEAM)
						? COLOR_NEO_ORANGE
						: g_PR->GetTeamColor(iCurTeam);

				wchar_t wszText[NEO_MAX_DISPLAYNAME];
				NeoUI::NextTableRow();

				NeoUI::Pad(); // Ping/BOT column - Show team rounds won
				if (iCurTeam >= FIRST_GAME_TEAM)
				{
					if (bNotTeamplay)
					{
						// DM - Print players total on the left side only
						if (iCurTeam == TEAM_JINRAI)
						{
							V_swprintf_safe(wszText, L"Players: %d",
									iaTeamTally[TEAM_JINRAI] + iaTeamTally[TEAM_NSF]);
						}
						else
						{
							wszText[0] = L'\0';
						}
					}
					else
					{
						C_Team *pTeam = GetGlobalTeam(iCurTeam);
						Assert(pTeam);
						if (pTeam)
						{
							wchar_t wszTeamtag[NEO_MAX_CLANTAG_LENGTH] = {};
							const char *pSzClantag = NEORules()->GetTeamClantag(iCurTeam);
							if (pSzClantag && pSzClantag[0])
							{
								g_pVGuiLocalize->ConvertANSIToUnicode(pSzClantag, wszTeamtag, sizeof(wszTeamtag));
							}
							else
							{
								V_wcscpy_safe(wszTeamtag, SZWSZ_NEO_TEAM_STRS[iCurTeam].wszStr);
							}
							if (bShowReadyUp)
							{
								V_swprintf_safe(wszText, L"%ls: %d (%d players - %d ready)",
										wszTeamtag, pTeam->GetRoundsWon(), iaTeamTally[iCurTeam],
										iaTeamReadyTally[iCurTeam]);
							}
							else
							{
								V_swprintf_safe(wszText, L"%ls: %d (%d players)",
										wszTeamtag, pTeam->GetRoundsWon(), iaTeamTally[iCurTeam]);
							}
						}
					}
				}
				else
				{
					V_wcscpy_safe(wszText, SZWSZ_NEO_TEAM_STRS[iCurTeam].wszStr);
				}
				NeoUI::Label(wszText, true);

				NeoUI::Pad(); // Avatar/Dead-indicator
				NeoUI::Pad(); // Name column
				if (iCurTeam >= FIRST_GAME_TEAM)
				{
					NeoUI::Label(L"Ready"); // Hidden when not used
					NeoUI::Label(L"Class");
					NeoUI::Label(L"Rank");
					NeoUI::Label(L"XP");
					NeoUI::Label(L"Deaths");
				}

				if (NeoUI::MODE_PAINT == m_uiCtx.eMode)
				{
					const int iMargin = Max(static_cast<int>(0.25f * m_uiCtx.iMarginY), 1);
					vgui::surface()->DrawSetColor(m_uiCtx.colors.normalFg);
					vgui::surface()->DrawFilledRect(
							m_uiCtx.dPanel.x,
							m_uiCtx.dPanel.y + m_uiCtx.layout.iRowTall - iMargin,
							m_uiCtx.dPanel.x + m_uiCtx.dPanel.wide,
							m_uiCtx.dPanel.y + m_uiCtx.layout.iRowTall);
				}

				if (iCurTeam < FIRST_GAME_TEAM)
				{
					m_uiCtx.colors.normalFg = COLOR_NEO_WHITE;
				}

				Color colorAliveFg = m_uiCtx.colors.normalFg;
				Color colorDeadFg = colorAliveFg;
				colorDeadFg[0] *= 0.75f;
				colorDeadFg[1] *= 0.75f;
				colorDeadFg[2] *= 0.75f;
				Color colorLocalBg = colorAliveFg;
				colorLocalBg[0] *= 0.33f;
				colorLocalBg[1] *= 0.33f;
				colorLocalBg[2] *= 0.33f;
				colorLocalBg[3] = 75;

				int iInPlaying = 0;
				for (int i = 0; i < m_iTotalPlayers; ++i)
				{
					const CNEOUIScoreBoardPlayer *pPlayerInfo = &m_playersInfo[i];
					const bool bIsPlaying = (TEAM_JINRAI == pPlayerInfo->iTeam
							|| TEAM_NSF == pPlayerInfo->iTeam);
					const bool bIsDMPlaying = bNotTeamplay && bIsPlaying;
					iInPlaying += (bIsPlaying);

					const bool bDMNotThisSide = bIsDMPlaying &&
							((iCurTeam >= FIRST_GAME_TEAM && (static_cast<int>(iInPlaying % 2) == static_cast<int>(TEAM_NSF == iCurTeam)))
									|| (iCurTeam <= TEAM_SPECTATOR && pPlayerInfo->iTeam != iCurTeam));
					if (bDMNotThisSide || (false == bIsDMPlaying && pPlayerInfo->iTeam != iCurTeam))
					{
						continue;
					}

					if (NeoUI::NextTableRow(NeoUI::NEXTTABLEROWFLAG_SELECTABLE).bPressed
							&& false == pPlayerInfo->bBot)
					{
						m_playerPopup = *pPlayerInfo;
						const bool bHaveFriendReq = (SteamFriends()
								&& k_EFriendRelationshipRequestInitiator == SteamFriends()->GetFriendRelationship(m_playerPopup.steamID));
						NeoUI::OpenPopup(NEOSCOREBOARDPOPUP_CARD, NeoUI::Dim{
								.x = m_uiCtx.iMouseAbsX,
								.y = m_uiCtx.iMouseAbsY,
								.wide = 5 * iPopupCardPerRowTallButtons
										+ (bHaveFriendReq ? iPopupCardPerRowTallButtons : 0),
								.tall = iPopupCardPerRowTallAvatarName + iPopupCardPerRowTallButtons,
								});
					}

					if (iCurTeam >= FIRST_GAME_TEAM)
					{
						if (bShowReadyUp)
						{
							m_uiCtx.colors.normalFg = (pPlayerInfo->bReady) ? colorAliveFg : colorDeadFg;
						}
						else
						{
							m_uiCtx.colors.normalFg = (pPlayerInfo->bDead) ? colorDeadFg : colorAliveFg;
						}
					}
					if (pPlayerInfo->iUserID == iLocalUserID)
					{
						vgui::surface()->DrawSetColor(colorLocalBg);
						vgui::surface()->DrawFilledRect(
								m_uiCtx.dPanel.x,
								m_uiCtx.dPanel.y + m_uiCtx.iLayoutY,
								m_uiCtx.dPanel.x + m_uiCtx.dPanel.wide,
								m_uiCtx.dPanel.y + m_uiCtx.iLayoutY + m_uiCtx.layout.iRowTall);
					}

					// Ping/BOT
					if (pPlayerInfo->iPing >= 0)
					{
						V_swprintf_safe(wszText, L"%d", pPlayerInfo->iPing);
						NeoUI::Label(wszText);
					}
					else if (pPlayerInfo->bBot)
					{
						NeoUI::Label(L"BOT");
					}
					else
					{
						NeoUI::Pad();
					}

					// Avatar/Dead-indicator
					NeoUI::Pad();
					CAvatarImage *pAvatarImg = nullptr;
					if (ShowAvatars() && pPlayerInfo->avatar.i32Idx >= 0)
					{
						// Use higher px image if wanted, otherwise fallback to i32Idx
						if (pPlayerInfo->avatar.i64Idx >= 0 && IN_BETWEEN_EQ(32, m_uiCtx.irWidgetTall, 64))
						{
							pAvatarImg = (CAvatarImage *)(m_pImageList->GetImage(pPlayerInfo->avatar.i64Idx));
						}
						else if (pPlayerInfo->avatar.i184Idx >= 0 && m_uiCtx.irWidgetTall > 64)
						{
							pAvatarImg = (CAvatarImage *)(m_pImageList->GetImage(pPlayerInfo->avatar.i184Idx));
						}
						else
						{
							pAvatarImg = (CAvatarImage *)(m_pImageList->GetImage(pPlayerInfo->avatar.i32Idx));
						}

						if (pAvatarImg)
						{
							pAvatarImg->SetPos(m_uiCtx.rWidgetArea.x0, m_uiCtx.rWidgetArea.y0);
							pAvatarImg->SetSize(m_uiCtx.irWidgetTall, m_uiCtx.irWidgetTall);
							pAvatarImg->Paint();
						}
					}
					// Voice mute icon - layered over avatar, only do for actual players
					if (pPlayerInfo->bMuted && false == pPlayerInfo->bBot)
					{
						// Slightly redden the avatar
						if (pAvatarImg)
						{
							vgui::surface()->DrawSetColor(100, 0, 0, 75);
							vgui::surface()->DrawFilledRect(
									m_uiCtx.rWidgetArea.x0,
									m_uiCtx.rWidgetArea.y0,
									m_uiCtx.rWidgetArea.x0 + m_uiCtx.irWidgetTall,
									m_uiCtx.rWidgetArea.y0 + m_uiCtx.irWidgetTall);
						}
						NeoUI::Texture("vgui/hud/voice_mute",
								m_uiCtx.rWidgetArea.x0,
								m_uiCtx.rWidgetArea.y0,
								m_uiCtx.irWidgetTall,
								m_uiCtx.irWidgetTall,
								"",
								NeoUI::TEXTUREOPTFLAGS_DONOTCROPTOPANEL);
					}
					if (iCurTeam >= FIRST_GAME_TEAM)
					{
						// Darken the avatar if dead or not ready
						if (pAvatarImg
								&& (pPlayerInfo->bDead
									|| (bShowReadyUp && false == pPlayerInfo->bReady)))
						{
							vgui::surface()->DrawSetColor(0, 0, 0, 200);
							vgui::surface()->DrawFilledRect(
									m_uiCtx.rWidgetArea.x0,
									m_uiCtx.rWidgetArea.y0,
									m_uiCtx.rWidgetArea.x0 + m_uiCtx.irWidgetTall,
									m_uiCtx.rWidgetArea.y0 + m_uiCtx.irWidgetTall);
						}

						// Dead icon - layered over avatar
						if (pPlayerInfo->bDead)
						{
							NeoUI::Texture("vgui/hud/kill_kill",
									m_uiCtx.rWidgetArea.x0,
									m_uiCtx.rWidgetArea.y0,
									m_uiCtx.irWidgetTall,
									m_uiCtx.irWidgetTall,
									"",
									NeoUI::TEXTUREOPTFLAGS_DONOTCROPTOPANEL);
						}
					}

					// Player's name
					if (pPlayerInfo->wszClantag[0])
					{
						V_swprintf_safe(wszText, L"[%ls] %ls",
								pPlayerInfo->wszClantag, pPlayerInfo->wszName);
					}
					else
					{
						V_wcscpy_safe(wszText, pPlayerInfo->wszName);
					}
					NeoUI::Label(wszText);

					if (iCurTeam >= FIRST_GAME_TEAM)
					{
						// Ready-up (Hidden when not used)
						NeoUI::Label(pPlayerInfo->bReady ? L"READY" : L"NOT READY");

						// Class
						NeoUI::Label(GetNeoClassNameW(pPlayerInfo->iClass));

						// Rank
						NeoUI::Label(GetRankNameW(pPlayerInfo->iXP));

						// XP
						V_swprintf_safe(wszText, L"%d", pPlayerInfo->iXP);
						NeoUI::Label(wszText);

						// Deaths count
						V_swprintf_safe(wszText, L"%d", pPlayerInfo->iDeaths);
						NeoUI::Label(wszText);
					}
				}
				NeoUI::EndTable();
			}
			NeoUI::EndSection();
		}

		m_uiCtx.colors.normalFg = COLOR_NEO_WHITE;

		if (NeoUI::BeginPopup(NEOSCOREBOARDPOPUP_CARD))
		{
			// NEO TODO (nullsystem): If name longer than popup box, paint over the remaining
			// area

			const int iAvatarOffset = m_uiCtx.iMarginX;
			const int iAvatarWT = iPopupCardPerRowTallAvatarName - (iAvatarOffset * 2);

			CAvatarImage *pAvatarImg = nullptr;
			if (m_playerPopup.avatar.i184Idx >= 0)
			{
				pAvatarImg = (CAvatarImage *)(m_pImageList->GetImage(m_playerPopup.avatar.i184Idx));
			}
			if (pAvatarImg)
			{
				pAvatarImg->SetPos(m_uiCtx.dPanel.x + iAvatarOffset,
						m_uiCtx.dPanel.y + iAvatarOffset);
				pAvatarImg->SetSize(iAvatarWT, iAvatarWT);
				pAvatarImg->Paint();
			}

			NeoUI::SetPerRowLayout(1, nullptr, iPopupCardPerRowTallAvatarName);
			NeoUI::SwapFont(NeoUI::FONT_NTNORMAL);
			NeoUI::Pad();

			vgui::surface()->DrawSetTextPos(
					m_uiCtx.dPanel.x + iAvatarOffset + iAvatarWT + iAvatarOffset,
					m_uiCtx.dPanel.y + iAvatarOffset);
			if (m_playerPopup.wszClantag[0])
			{
				vgui::surface()->DrawSetTextColor(m_uiCtx.colors.normalFg);
				vgui::surface()->DrawPrintText(m_playerPopup.wszClantag,
						V_wcslen(m_playerPopup.wszClantag));

				const auto *pFontI = &m_uiCtx.fonts[m_uiCtx.eFont];
				const int iClantagTall = vgui::surface()->GetFontTall(pFontI->hdl);
				vgui::surface()->DrawSetTextPos(
						m_uiCtx.dPanel.x + iAvatarOffset + iAvatarWT + iAvatarOffset,
						m_uiCtx.dPanel.y + iAvatarOffset + iClantagTall + iAvatarOffset);
			}
			NeoUI::SwapFont(NeoUI::FONT_NTLARGE);
			vgui::surface()->DrawSetTextColor(m_uiCtx.colors.hotFg);
			vgui::surface()->DrawPrintText(m_playerPopup.wszName,
					V_wcslen(m_playerPopup.wszName));

			NeoUI::SetPerRowLayout(5, nullptr, iPopupCardPerRowTallButtons);
			NeoUI::SwapFont(NeoUI::FONT_NTNORMAL);

			// NEO NOTE (nullsystem): Currently bots won't have a popup card
			Assert(false == m_playerPopup.bBot);
			if (false == m_playerPopup.bBot)
			{
				// NEO NOTE (nullsystem): CVoiceBanMgr can mute (and unmute) yourself, so no checks
				// for local player here
				if (m_playerPopup.iUserID > 0)
				{
					if (NeoUI::ButtonTexture(
								m_playerPopup.bMuted
										? "vgui/hud/voice_transmit"
										: "vgui/hud/voice_mute"
								, "GAME"
								, m_playerPopup.bMuted
										? L"Unmute"
										: L"Mute").bPressed)
					{
						int iPlayerIdx = 0;
						for (int i = 1; i <= gpGlobals->maxClients; i++)
						{
							if (g_PR->GetUserID(i) == m_playerPopup.iUserID)
							{
								iPlayerIdx = i;
								break;
							}
						}
						if (iPlayerIdx > 0)
						{
							GetClientVoiceMgr()->SetPlayerBlockedState(iPlayerIdx,
									!m_playerPopup.bMuted);
						}
						NeoUI::ClosePopup();
					}
				}

				if (SteamFriends())
				{
					CSteamID &steamID = m_playerPopup.steamID;

					// NEO TODO (nullsystem): Turn to proper texture when done
					// Replace every .png with actual vmt/vtf
					if (NeoUI::ButtonTexture("materials/vgui/hud/player_profile.png", "GAME",
								L"Profile").bPressed)
					{
						SteamFriends()->ActivateGameOverlayToUser("steamid", steamID);
						NeoUI::ClosePopup();
					}
					if (m_playerPopup.iUserID != iLocalUserID)
					{
						const char *pszOverlay = nullptr;
						// NEO TODO (nullsystem) png -> vtf/vmt
						if (NeoUI::ButtonTexture("materials/vgui/hud/player_message.png", "GAME",
									L"Message").bPressed)
						{
							pszOverlay = "chat";
						}

						const EFriendRelationship eFriendRel =
								SteamFriends()->GetFriendRelationship(steamID);
						switch (eFriendRel)
						{
						case k_EFriendRelationshipNone:
							if (NeoUI::ButtonTexture("", "GAME",
										L"Send").bPressed)
							{
								pszOverlay = "friendadd";
							}
							break;
						case k_EFriendRelationshipRequestRecipient:
							if (NeoUI::ButtonTexture("", "GAME",
										L"Cancel").bPressed)
							{
								pszOverlay = "friendremove";
							}
							break;
						case k_EFriendRelationshipRequestInitiator:
							if (NeoUI::ButtonTexture("", "GAME",
										L"Accept").bPressed)
							{
								pszOverlay = "friendrequestaccept";
							}
							if (NeoUI::ButtonTexture("", "GAME",
										L"Ignore").bPressed)
							{
								pszOverlay = "friendrequestignore";
							}
							break;
						default:
							break;
						}

						if (pszOverlay)
						{
							SteamFriends()->ActivateGameOverlayToUser(pszOverlay, steamID);
							NeoUI::ClosePopup();
						}
					}
				}

				if (m_playerPopup.iUserID == iLocalUserID)
				{
					if (bShowReadyUp)
					{
						if (NeoUI::ButtonTexture("", "GAME",
									m_playerPopup.bReady ? L"Unready" : L"Ready").bPressed)
						{
							engine->ClientCmd(m_playerPopup.bReady
									? "readytoggle unready"
									: "readytoggle ready");
							NeoUI::ClosePopup();
						}
					}
				}
				else
				{
					// NEO TODO (nullsystem) png -> vtf/vmt
					if (NeoUI::ButtonTexture("materials/vgui/hud/player_crosshair_icon.png", "GAME",
								L"Crosshair").bPressed)
					{
						NeoUI::ClosePopup();
						if (m_playerPopup.szCrosshair[0])
						{
							NeoUI::OpenPopup(NEOSCOREBOARDPOPUP_COPYCROSSHAIR,
									NeoUI::Dim{
											.x = wide / 2 - wide / 4,
											.y = tall / 2 - ((m_uiCtx.layout.iRowTall * 3) / 2),
											.wide = wide / 2,
											.tall = m_uiCtx.layout.iRowTall * 3,
									});
						}
					}
				}
			}

			NeoUI::EndPopup();
		}

		// NEO TODO (nullsystem): Probably another separate dialog without having to
		// hold scoreboard bind instead?
		if (NeoUI::BeginPopup(NEOSCOREBOARDPOPUP_COPYCROSSHAIR))
		{
			NeoUI::SwapFont(NeoUI::FONT_NTLARGE);
			const auto tmpButtonTextStyle = m_uiCtx.eButtonTextStyle;
			m_uiCtx.eButtonTextStyle = NeoUI::TEXTSTYLE_CENTER;

			NeoUI::SetPerRowLayout(1);
			NeoUI::HeadingLabel(L"Replace your crosshair?");
			NeoUI::Pad();

			NeoUI::SetPerRowLayout(3);
			if (NeoUI::Button(L"Yes").bPressed)
			{
				ConVarRef cvr_cl_neo_crosshair("cl_neo_crosshair");
				cvr_cl_neo_crosshair.SetValue(m_playerPopup.szCrosshair);
				NeoUI::ClosePopup();
			}
			NeoUI::Pad();
			if (NeoUI::Button(L"No").bPressed)
			{
				NeoUI::ClosePopup();
			}

			m_uiCtx.eButtonTextStyle = tmpButtonTextStyle;

			NeoUI::EndPopup();
		}
	}
	NeoUI::EndContext();
}

