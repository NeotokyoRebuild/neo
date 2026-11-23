#include "neo_scoreboard.h"

#include "c_neo_player.h"
#include "neo_gamerules.h"
#include "neo_theme.h"
#include "c_neo_killer_damage_infos.h"
#include "neo_hud_deathnotice.h"
#include "neo_hud_draw.h"

#include <inputsystem/iinputsystem.h>
#include <voice_status.h>
#include <c_playerresource.h>
#include <steam/isteamutils.h>
#include <c_team.h>
#include <vgui_controls/ImageList.h>
#include <vgui/ILocalize.h>
#include <vgui/IInput.h>

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

ConVar cl_neo_hud_scoreboard_hide_others("cl_neo_hud_scoreboard_hide_others", "1", FCVAR_ARCHIVE, "Hide some other HUD elements when the scoreboard is displayed to prevent overlap", true, 0.0f, true, 1.0f);
ConVar neo_show_scoreboard_avatars("neo_show_scoreboard_avatars", "1", FCVAR_ARCHIVE, "Show avatars on scoreboard.", true, 0.0f, true, 1.0f);
ConVar cl_neo_hud_scoreboard_padding("cl_neo_hud_scoreboard_padding", "0", FCVAR_ARCHIVE, "Adjust scoreboard space padding. 0 = default, 1 = compact, 2 = spacious", true, 0.0f, true, 2.0f);
extern ConVar cl_neo_streamermode;
extern ConVar cl_neo_squad_hud_original;
extern ConVar cl_neo_hud_team_swap_sides;

namespace
{
// Between each column's text and its edges, and the least a table too narrow for its name column lowers it to
constexpr int SCOREBOARD_CELL_MARGIN_PX = 8;
constexpr int SCOREBOARD_CELL_MARGIN_MIN_PX = 2;
}

enum ENeoScoreBoardPopup
{
	NEOSCOREBOARDPOPUP_CARD = NeoUI::INTERNALPOPUP_NIL + 1,
	NEOSCOREBOARDPOPUP_COPYCROSSHAIR,
};

enum EColsPlayers
{
	COLSPLAYERS_PING = 0,
	COLSPLAYERS_AVATAR,
	COLSPLAYERS_NAME,
	COLSPLAYERS_READYUP,	// Hidden apart from ready-up
	COLSPLAYERS_DMG_DEALT,	// Hidden apart from playing local player's death
	COLSPLAYERS_DMG_TAKEN,	// Hidden apart from playing local player's death
	COLSPLAYERS_CLASS,		// Hidden if team play, enemy team, and local player alive
	COLSPLAYERS_RANK,
	COLSPLAYERS_XP,
	COLSPLAYERS_DEATH,

	COLSPLAYERS__TOTAL,
};

enum EColsNonPlayers
{
	COLSNONPLAYERS_PING = 0,
	COLSNONPLAYERS_AVATAR,
	COLSNONPLAYERS_NAME,

	COLSNONPLAYERS__TOTAL,
};

CNEOScoreBoard *g_pNeoScoreBoard = nullptr;

CNEOScoreBoard::CNEOScoreBoard(IViewPort *pViewPort)
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

	g_pNeoScoreBoard = this;

	for (int i = 0; i < CROSSHAIR_STYLE__TOTAL; ++i)
	{
		if (CROSSHAIR_FILES[i][0])
		{
			Texture *pTex = &m_arTextures[i];
			pTex->iTexId = vgui::surface()->CreateNewTextureID();
			vgui::surface()->DrawSetTextureFile(pTex->iTexId, CROSSHAIR_FILES[i], false, false);
			vgui::surface()->DrawGetTextureSize(pTex->iTexId, pTex->iWide, pTex->iTall);
		}
	}
}

CNEOScoreBoard::~CNEOScoreBoard()
{
	if (g_pNeoScoreBoard == this)
	{
		g_pNeoScoreBoard = nullptr;
	}
}

bool CNEOScoreBoard::ShowAvatars()
{
	return neo_show_scoreboard_avatars.GetBool() && !cl_neo_streamermode.GetBool();
}

const char *CNEOScoreBoard::GetName()
{
	return PANEL_SCOREBOARD;
}

void CNEOScoreBoard::OnPollHideCode(int code)
{
	m_nCloseKey = static_cast<ButtonCode_t>(code);
}

void CNEOScoreBoard::OnThink()
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

void CNEOScoreBoard::ShowPanel(bool bShow)
{
	if (NEORules() && NEORules()->GetHiddenHudElements() & NEO_HUD_ELEMENT_SCOREBOARD)
	{
		return;
	}
	// Catch the case where we call ShowPanel before ApplySchemeSettings, eg when
	// going from windowed <-> fullscreen
	if (!m_bAppliedApplyScheme)
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

void CNEOScoreBoard::FireGameEvent(IGameEvent *event)
{
	const char *type = event->GetName();
	if (0 == V_strcmp(type, "hltv_status"))
	{
		m_HLTVSpectators = event->GetInt("clients") - event->GetInt("proxies");
	}
	else if (0 == V_strcmp(type, "server_spawn"))
	{
		const char *hostname = event->GetString("hostname");
		g_pVGuiLocalize->ConvertANSIToUnicode(hostname,
				m_wszHostname, sizeof(m_wszHostname));
	}

	if (IsVisible())
	{
		Update();
	}
}

// The widest rank glyph, so the rank names line up after it
static int GetRankGlyphWideMax(const vgui::HFont hIconFont)
{
	int iWide = 0;
	for (int i = NEO_HUD_DEATHNOTICEICON_RANKLESS_DOG; i <= NEO_HUD_DEATHNOTICEICON_LT; ++i)
	{
		int a = 0, b = 0, c = 0;
		vgui::surface()->GetCharABCwide(hIconFont, static_cast<wchar_t>(i), a, b, c);
		iWide = Max(iWide, b);
	}
	return iWide;
}

void CNEOScoreBoard::ApplySchemeSettings(vgui::IScheme *pScheme)
{
	BaseClass::ApplySchemeSettings(pScheme);

	int wide, tall;
	vgui::surface()->GetScreenSize(wide, tall);
	SetSize(wide, tall);
	SetFgColor(COLOR_TRANSPARENT);
	SetBgColor(COLOR_TRANSPARENT);

	static constexpr NeoUI::FontOverride FONT_OVERRIDES[] = {
		{ NeoUI::FONT_NTNORMAL, "NHudScoreboardMono" },
		{ NeoUI::FONT_NTLARGE, "NeoUINormal" },
		{ NeoUI::FONT_NTSMALLTEXT, "NHudScoreboardText" },
	};
	NeoUI::LoadFonts(&m_uiCtx, pScheme, FONT_OVERRIDES, ARRAYSIZE(FONT_OVERRIDES));
	m_hRankIconFont = NeoHud::GetFont(pScheme, "NHudScoreboardIcons");
	m_iRankGlyphWide = m_hRankIconFont ? GetRankGlyphWideMax(m_hRankIconFont) : 0;

	m_playerPopup = {};
	m_iTotalPlayers = 0;
	m_bAppliedApplyScheme = true;

	m_flNextUpdateTime = gpGlobals->curtime + 0.1f;

	NeoUI::ClosePopup();
}

int CNEOScoreBoard::GetRankIconWide() const
{
	if (!m_hRankIconFont)
	{
		return 0;
	}
	// The margin is live: a narrow table lowers it
	return m_iRankGlyphWide + m_uiCtx.iMarginX;
}

void CNEOScoreBoard::DrawRankCell(const int iXP) const
{
	const NeoUI::FontInfo &textFont = m_uiCtx.fonts[NeoUI::FONT_NTSMALLTEXT];
	const int xStart = m_uiCtx.rWidgetArea.x0 + m_uiCtx.iMarginX;
	const int yText = m_uiCtx.rWidgetArea.y0 + textFont.iYFontOffset;
	const int iIconColumnWide = GetRankIconWide();
	vgui::surface()->DrawSetTextColor(m_uiCtx.colors.normalFg);

	if (m_hRankIconFont)
	{
		// Centered in a column as wide as the widest rank icon, and on the row as the kill feed draws it, then
		// lowered so its ink centers on the rank name's capitals: at 1080p a glyph centered on the row stands from
		// 2 px above the capitals to their baseline
		static constexpr int RANK_GLYPH_DROP_PX = 1;
		const int yRow = m_uiCtx.rWidgetArea.y0 + NeoHud::ScaledPx(RANK_GLYPH_DROP_PX);
		const int yBaseline = yRow + ((m_uiCtx.irWidgetTall - vgui::surface()->GetFontTall(m_hRankIconFont)) / 2)
				+ vgui::surface()->GetFontAscent(m_hRankIconFont, L'0');
		NeoHud::DrawGlyphInColumn(m_hRankIconFont, xStart, iIconColumnWide - m_uiCtx.iMarginX, yBaseline, GetRankGlyph(iXP));
	}

	const wchar_t *pwszRank = GetRankNameW(iXP);
	vgui::surface()->DrawSetTextFont(textFont.hdl);
	vgui::surface()->DrawSetTextPos(xStart + iIconColumnWide, yText);
	vgui::surface()->DrawPrintText(pwszRank, V_wcslen(pwszRank));
}

void CNEOScoreBoard::Paint()
{
	OnMainLoop(NeoUI::MODE_PAINT);
}

void CNEOScoreBoard::OnMousePressed(vgui::MouseCode code)
{
	if (IsMouseInputEnabled())
	{
		m_uiCtx.eCode = code;
		OnMainLoop(NeoUI::MODE_MOUSEPRESSED);
	}
}

void CNEOScoreBoard::OnMouseReleased(vgui::MouseCode code)
{
	if (IsMouseInputEnabled())
	{
		m_uiCtx.eCode = code;
		OnMainLoop(NeoUI::MODE_MOUSERELEASED);
	}
}

void CNEOScoreBoard::OnMouseDoublePressed(vgui::MouseCode code)
{
	if (IsMouseInputEnabled())
	{
		m_uiCtx.eCode = code;
		OnMainLoop(NeoUI::MODE_MOUSEDOUBLEPRESSED);
	}
}

void CNEOScoreBoard::OnCursorMoved(int x, int y)
{
	if (IsMouseInputEnabled())
	{
		m_uiCtx.iMouseAbsX = x;
		m_uiCtx.iMouseAbsY = y;
		OnMainLoop(NeoUI::MODE_MOUSEMOVED);
	}
}

void CNEOScoreBoard::OnKeyCodePressed([[maybe_unused]] vgui::KeyCode code)
{
	// no-op
}

void CNEOScoreBoard::OnKeyCodeReleased([[maybe_unused]] vgui::KeyCode code)
{
	// no-op
}

void CNEOScoreBoard::Reset()
{
	m_iTotalPlayers = 0;
	V_memset(m_playersInfo, 0, sizeof(m_playersInfo));
	m_flNextUpdateTime = 0;
}

void CNEOScoreBoard::ToggleMouseCapture(const bool bUseMouse)
{
	SetMouseInputEnabled(bUseMouse);
	m_uiCtx.iHotPersist = m_uiCtx.iActive = m_uiCtx.iHot = NeoUI::FOCUSOFF_NUM;
	m_uiCtx.iHotPersistSection = m_uiCtx.iActiveSection = m_uiCtx.iHotSection = -1;
	NeoUI::ClosePopup();
}

bool CNEOScoreBoard::InCopyCrosshairPopup() const
{
	return m_uiCtx.iCurPopupId == NEOSCOREBOARDPOPUP_COPYCROSSHAIR;
}

bool CNEOScoreBoard::NeedsUpdate()
{
	return (m_flNextUpdateTime < gpGlobals->curtime);
}

void CNEOScoreBoard::Update()
{
	if (!g_PR || !NEORules())
	{
		return;
	}

	C_NEO_Player *pLocalPlayer = C_NEO_Player::GetLocalNEOPlayer();
	if (!pLocalPlayer)
	{
		return;
	}

	const int iLocalPlayerTeam = pLocalPlayer->GetTeamNumber();
	const bool bLocalPlaying = (iLocalPlayerTeam >= FIRST_GAME_TEAM);
	const bool bIsTeamplay = NEORules()->IsTeamplay();

	const bool bShowDamageInfo = pLocalPlayer->IsPlayerDead()
			&& NEORules()->InRoundState()
			&& bLocalPlaying;

	// Zero array every update
	m_iTotalPlayers = 0;
	V_memset(m_playersInfo, 0, sizeof(m_playersInfo));

	m_bSourceTVEnabled = false;
	for (int i = 1; i <= gpGlobals->maxClients; ++i)
	{
		if (false == g_PR->IsConnected(i))
		{
			continue;
		}

		if (g_PR->IsHLTV(i))
		{
			m_bSourceTVEnabled = true;
			continue;
		}

		C_NEO_Player *pNeoPlayer = ToNEOPlayer(UTIL_PlayerByIndex(i));
		C_NEO_Player *pNeoImpersonator = pNeoPlayer
				? ToNEOPlayer(pNeoPlayer->m_hSpectatorTakeoverPlayerImpersonatingMe.Get())
				: nullptr;

		const bool bIsImpersonating = pNeoPlayer
				&& pNeoPlayer->m_hSpectatorTakeoverPlayerTarget.Get();

		CNEOScoreBoardPlayer *pPlayerInfo = &m_playersInfo[m_iTotalPlayers];
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

		const bool bIsPlaying = (pPlayerInfo->iTeam >= FIRST_GAME_TEAM);

		if (bIsPlaying)
		{
			pPlayerInfo->iXP = g_PR->GetXP(i);
			pPlayerInfo->iDeaths = g_PR->GetDeaths(i);

			// Damage infos
			if (bShowDamageInfo)
			{
				AttackersTotals *pDmgReport = nullptr;
				for (int j = 0; j < g_neoDamageReportSize; ++j)
				{
					if (g_neoDamageReport[j].iUserID == pPlayerInfo->iUserID)
					{
						pDmgReport = &g_neoDamageReport[j];
						break;
					}
				}
				if (pDmgReport)
				{
					pPlayerInfo->iDealtDmgs = pDmgReport->iDealtDmgs;
					pPlayerInfo->iDealtHits = pDmgReport->iDealtHits;
					pPlayerInfo->iTakenDmgs = pDmgReport->iTakenDmgs;
					pPlayerInfo->iTakenHits = pDmgReport->iTakenHits;
					pPlayerInfo->bKilledYou = pNeoPlayer ? (pNeoPlayer->entindex() == g_neoKillerInfos.iEntIndex) : false;
					pPlayerInfo->bYouKilled = false;
					for (int j = 0; j < g_neoUserIDsLocalKilledSize; ++j)
					{
						if (g_neoUserIDsLocalKilled[j] == pPlayerInfo->iUserID)
						{
							pPlayerInfo->bYouKilled = true;
							break;
						}
					}
				}
			}
		}

		// pPlayerInfo->iClass
		const bool bShowClass = 
				   (false == bLocalPlaying) // Spectating
				|| (false == bIsTeamplay && g_PR->IsLocalPlayer(i)) // DM - Only see own class
				|| (bIsTeamplay && iLocalPlayerTeam == pPlayerInfo->iTeam) // Team - See own team's classes
				|| (bShowDamageInfo
						&& ((pPlayerInfo->iDealtDmgs > 0 && pPlayerInfo->iDealtHits > 0)
							|| (pPlayerInfo->iTakenDmgs > 0 && pPlayerInfo->iTakenHits > 0))); // Dead and have damage dealt/taken
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
		if (bIsPlaying)
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

		pPlayerInfo->iAvatarIdx = m_iTotalPlayers;

		NeoAvatar *pAvatar = &m_avatars[m_iTotalPlayers];
		pAvatar->SetSteamID((ShowAvatars() && pPlayerInfo->steamID.IsValid())
				? pPlayerInfo->steamID : CSteamID{});
		pAvatar->Fetch(64);

		++m_iTotalPlayers;
	}

	// Sort players by XP, if equal by death
	V_qsort_s(m_playersInfo, m_iTotalPlayers, sizeof(CNEOScoreBoardPlayer),
			[]([[maybe_unused]] void *vpCtx, const void *vpLeft, const void *vpRight) -> int {
		auto *pLeft = static_cast<const CNEOScoreBoardPlayer *>(vpLeft);
		auto *pRight = static_cast<const CNEOScoreBoardPlayer *>(vpRight);
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
	// current map so just poll it from NEORules instead
	g_pVGuiLocalize->ConvertANSIToUnicode(NEORules()->MapName(),
			m_wszMap, sizeof(m_wszMap));

	m_flNextUpdateTime = gpGlobals->curtime + 1.0f;
}

void CNEOScoreBoard::OnMainLoop(const NeoUI::Mode eMode)
{
	C_NEO_Player *pLocalPlayer = C_NEO_Player::GetLocalNEOPlayer();

	if (!NEORules() || !pLocalPlayer)
	{
		return;
	}

	int wide, tall;
	vgui::surface()->GetScreenSize(wide, tall);

	// other resolution scales up/down from it
	const auto ePadType = static_cast<ENeoScoreBoardPadding>(cl_neo_hud_scoreboard_padding.GetInt());
	const int iDivBy = (ePadType == NEOSCOREBOARDPADDING_DEFAULT)
			? 30
			: (ePadType == NEOSCOREBOARDPADDING_COMPACT)
				? 35
				: 25;
	m_uiCtx.layout.iRowTall = m_uiCtx.layout.iDefRowTall = tall / iDivBy;
	m_uiCtx.iMarginX = NeoHud::ScaledPx(SCOREBOARD_CELL_MARGIN_PX);
	m_uiCtx.iMarginY = tall / 108 / 2;
	int iWideAs43 = static_cast<float>(tall) * (4.0f / 3.0f);
	if (iWideAs43 > wide) iWideAs43 = wide;
	const int iRootSubPanelWide = static_cast<int>(iWideAs43 * 0.975f);
	const int iPopupCardRowTallBase = tall / 35;
	const int iPopupCardPerRowTallAvatarName = iPopupCardRowTallBase * 3;
	const int iPopupCardPerRowTallButtons = iPopupCardRowTallBase * 2.25f;
	const int iAvatarOffset = m_uiCtx.iMarginX;
	const int iAvatarWT = ShowAvatars() ? (iPopupCardPerRowTallAvatarName - (iAvatarOffset * 2)) : 0;
	const int iLocalPlayerTeam = pLocalPlayer->GetTeamNumber();
	const bool bLocalPlaying = (iLocalPlayerTeam >= FIRST_GAME_TEAM);
	const bool bIsTeamplay = NEORules()->IsTeamplay();
	const bool bShowReadyUp = NEORules()->InReadyUpState();
	const bool bShowDamageInfo = pLocalPlayer->IsPlayerDead()
			&& NEORules()->InRoundState()
			&& g_neoKillerInfos.bHasDmgInfos
			&& bLocalPlaying;
	static CrosshairInfo staticXhairInfo = {};

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

	const int iGap = m_uiCtx.iMarginX;
	const int iMaxSidePlayers = (bIsTeamplay)
			? Max(iaTeamTally[TEAM_JINRAI], iaTeamTally[TEAM_NSF])
			: Ceil2Int((iaTeamTally[TEAM_JINRAI] + iaTeamTally[TEAM_NSF]) / 2.0f);
	const int iLocalUserID = pLocalPlayer->GetUserID();

	int iTies = 0;
	if (bIsTeamplay)
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

		int iColsWidePlayersList[COLSPLAYERS__TOTAL] = {};
		int iColsWideNonPlayersList[COLSNONPLAYERS__TOTAL] = {};
		int iColsWidePlayersTotalNonName = 0;
		int iColsWideNonPlayersTotalNonName = 0;
		// Each column's text, measured once a pass, since only the cell margin changes between the calls below
		const auto TextWide = [this](const wchar_t *pwszText, const NeoUI::EFont eFont)
		{
			[[maybe_unused]] int iWide = 0, iTall = 0;
			vgui::surface()->GetTextSize(m_uiCtx.fonts[eFont].hdl, pwszText, iWide, iTall);
			return iWide;
		};
		const int iPingTextWide = TextWide(L"BOT", m_uiCtx.eFont);
		const int iReadyUpTextWide = TextWide(L"NOT READY", m_uiCtx.eFont);
		const int iDmgTextWide = TextWide(L"Dmg from", m_uiCtx.eFont);
		// Juggernaut is the longest class name
		const int iClassTextWide = TextWide(L"Juggernaut", NeoUI::FONT_NTSMALLTEXT);
		const int iRankTextWide = TextWide(bHasRanklessDog ? L"Rankless Dog" : L"Lieutenant", NeoUI::FONT_NTSMALLTEXT);
		const int iXPTextWide = TextWide(L"-99", m_uiCtx.eFont);
		const int iDeathTextWide = TextWide(L"D#", m_uiCtx.eFont);

		// Every column but the name, which takes what is left. They depend on the cell margin, which a narrow table lowers.
		const auto ComputeColsWide = [this, bShowReadyUp, bShowDamageInfo, iPingTextWide, iReadyUpTextWide, iDmgTextWide,
				iClassTextWide, iRankTextWide, iXPTextWide, iDeathTextWide,
				&iColsWidePlayersList, &iColsWideNonPlayersList, &iColsWidePlayersTotalNonName, &iColsWideNonPlayersTotalNonName]
				(const bool bShowClass)
		{
			const int iMargins = m_uiCtx.iMarginX * 2;
			iColsWidePlayersList[COLSPLAYERS_PING] = iPingTextWide + iMargins;
			iColsWidePlayersList[COLSPLAYERS_AVATAR] = m_uiCtx.layout.iRowTall + iMargins;
			iColsWidePlayersList[COLSPLAYERS_NAME] = 0;
			iColsWidePlayersList[COLSPLAYERS_READYUP] = bShowReadyUp ? iReadyUpTextWide + iMargins : 0;
			iColsWidePlayersList[COLSPLAYERS_DMG_DEALT] = bShowDamageInfo ? iDmgTextWide + iMargins : 0;
			iColsWidePlayersList[COLSPLAYERS_DMG_TAKEN] = bShowDamageInfo ? iDmgTextWide + iMargins : 0;
			iColsWidePlayersList[COLSPLAYERS_CLASS] = bShowClass ? iClassTextWide + iMargins : 0;
			iColsWidePlayersList[COLSPLAYERS_RANK] = iRankTextWide + iMargins + GetRankIconWide();
			iColsWidePlayersList[COLSPLAYERS_XP] = iXPTextWide + iMargins;
			iColsWidePlayersList[COLSPLAYERS_DEATH] = iDeathTextWide + iMargins;
			iColsWidePlayersTotalNonName = 0;
			for (int i = 0; i < COLSPLAYERS__TOTAL; ++i)
			{
				iColsWidePlayersTotalNonName += iColsWidePlayersList[i];
			}

			iColsWideNonPlayersList[COLSNONPLAYERS_PING] = iPingTextWide + iMargins;
			iColsWideNonPlayersList[COLSNONPLAYERS_AVATAR] = m_uiCtx.layout.iRowTall + iMargins;
			iColsWideNonPlayersList[COLSNONPLAYERS_NAME] = 0;
			iColsWideNonPlayersTotalNonName = 0;
			for (int i = 0; i < COLSNONPLAYERS__TOTAL; ++i)
			{
				iColsWideNonPlayersTotalNonName += iColsWideNonPlayersList[i];
			}
		};
		const int iBaseMarginX = m_uiCtx.iMarginX;

		// The table's background keeps its size, and its columns stand in from each side by the space
		// above and below a row's text, less the margin each column already has
		const int iCapTall = vgui::surface()->GetFontAscent(m_uiCtx.fonts[NeoUI::FONT_NTNORMAL].hdl, L'H');
		const auto GetEdgePad = [this, iCapTall]()
		{
			return Max(0, ((m_uiCtx.layout.iRowTall - iCapTall) / 2) - m_uiCtx.iMarginX);
		};

		// The name column stays the widest. A table too narrow for that lowers its cell margin. Both game teams
		// share one margin, found with the class column shown, so their columns match side by side.
		const auto NameIsWidest = [&iColsWidePlayersList, &iColsWidePlayersTotalNonName](const int iTableWide)
		{
			const int iNameWide = iTableWide - iColsWidePlayersTotalNonName;
			for (int i = 0; i < COLSPLAYERS__TOTAL; ++i)
			{
				if (i != COLSPLAYERS_NAME && iColsWidePlayersList[i] > iNameWide)
				{
					return false;
				}
			}
			return true;
		};
		const int iGameTeamWide = (iRootSubPanelWide / 2) - (iGap / 2);
		const int iMinMarginX = NeoHud::ScaledPx(SCOREBOARD_CELL_MARGIN_MIN_PX);
		ComputeColsWide(true);
		while (!NameIsWidest(iGameTeamWide - (GetEdgePad() * 2)) && m_uiCtx.iMarginX > iMinMarginX)
		{
			--m_uiCtx.iMarginX;
			ComputeColsWide(true);
		}
		const int iGameTeamMarginX = m_uiCtx.iMarginX;

		// Output all the players in the server
		for (int iCurTeam = TEAM_UNASSIGNED; iCurTeam < TEAM__TOTAL; ++iCurTeam)
		{
			if (iCurTeam == TEAM_UNASSIGNED && iaTeamTally[iCurTeam] <= 0)
			{
				continue;
			}
			else if (iCurTeam == TEAM_SPECTATOR && iaTeamTally[iCurTeam] <= 0 && !m_bSourceTVEnabled)
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
			const bool bShowClass = (iLocalPlayerTeam <= TEAM_SPECTATOR)
					|| false == bIsTeamplay
					|| bShowDamageInfo
					|| iLocalPlayerTeam == iCurTeam;
			m_uiCtx.iMarginX = (iCurTeam >= FIRST_GAME_TEAM) ? iGameTeamMarginX : iBaseMarginX;
			ComputeColsWide(bShowClass);

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

			const int iEdgePad = GetEdgePad();
			if (NeoUI::MODE_PAINT == eMode)
			{
				vgui::surface()->DrawSetColor(COLOR_BLACK_TRANSPARENT);
				vgui::surface()->DrawFilledRect(m_uiCtx.dPanel.x, m_uiCtx.dPanel.y,
						m_uiCtx.dPanel.x + m_uiCtx.dPanel.wide, m_uiCtx.dPanel.y + m_uiCtx.dPanel.tall);
			}
			m_uiCtx.dPanel.x += iEdgePad;
			m_uiCtx.dPanel.wide -= iEdgePad * 2;
			m_uiCtx.colors.sectionBg = COLOR_TRANSPARENT;

			iColsWidePlayersList[COLSPLAYERS_NAME] = m_uiCtx.dPanel.wide - iColsWidePlayersTotalNonName;
			iColsWideNonPlayersList[COLSNONPLAYERS_NAME] = m_uiCtx.dPanel.wide - iColsWideNonPlayersTotalNonName;

			NeoUI::BeginSection(NeoUI::SECTIONFLAG_DISABLEOFFSETS);
			{
				NeoUI::BeginTable(
						(iCurTeam >= FIRST_GAME_TEAM)
								? iColsWidePlayersList
								: iColsWideNonPlayersList,
						(iCurTeam >= FIRST_GAME_TEAM)
								? ARRAYSIZE(iColsWidePlayersList)
								: ARRAYSIZE(iColsWideNonPlayersList));

				m_uiCtx.colors.normalFg = (false == bIsTeamplay && iCurTeam >= FIRST_GAME_TEAM)
						? COLOR_NEO_WHITE
						: g_PR->GetTeamColor(iCurTeam);

				wchar_t wszText[NEO_MAX_DISPLAYNAME];
				NeoUI::NextTableRow();

				NeoUI::Pad(); // Ping/BOT column - Show team rounds won
				if (iCurTeam >= FIRST_GAME_TEAM)
				{
					if (bIsTeamplay)
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
							// The dash already shows rounds won, unless the map hides it
							wchar_t wszRoundsWon[16] = {};
							if (NEORules()->GetHiddenHudElements() & NEO_HUD_ELEMENT_ROUND_STATE)
							{
								V_swprintf_safe(wszRoundsWon, L": %d", pTeam->GetRoundsWon());
							}
							if (bShowReadyUp)
							{
								V_swprintf_safe(wszText, L"%ls%ls (%d player%ls - %d ready)",
										wszTeamtag, wszRoundsWon, iaTeamTally[iCurTeam],
										iaTeamTally[iCurTeam] == 1 ? L"" : L"s",
										iaTeamReadyTally[iCurTeam]);
							}
							else
							{
								V_swprintf_safe(wszText, L"%ls%ls (%d player%ls)",
										wszTeamtag, wszRoundsWon, iaTeamTally[iCurTeam],
										iaTeamTally[iCurTeam] == 1 ? L"" : L"s");
							}
						}
					}
					else
					{
						// DM - Print players total on the left side only
						if (iCurTeam == TEAM_JINRAI)
						{
							const int total = iaTeamTally[TEAM_JINRAI] + iaTeamTally[TEAM_NSF];
							V_swprintf_safe(wszText, L"Player%ls: %d",
									total == 1 ? L"" : L"s", total);
						}
						else
						{
							wszText[0] = L'\0';
						}
					}
				}
				else
				{
					V_swprintf_safe(wszText, L"%ls (%d player%ls)",
							SZWSZ_NEO_TEAM_STRS[iCurTeam].wszStr, iaTeamTally[iCurTeam],
							iaTeamTally[iCurTeam] == 1 ? L"" : L"s");
				}
				NeoUI::Label(wszText, true);

				if (TEAM_SPECTATOR == iCurTeam && m_bSourceTVEnabled)
				{
					V_swprintf_safe(wszText, L"SourceTV Enabled (%d watching)", m_HLTVSpectators);
					int x = 0;
					[[maybe_unused]] int y = 0;
					vgui::surface()->GetTextSize(m_uiCtx.fonts[m_uiCtx.eFont].hdl, wszText, x, y);
					vgui::surface()->DrawSetTextPos(m_uiCtx.rWidgetArea.x0 + m_uiCtx.dPanel.wide - x - m_uiCtx.iMarginX, m_uiCtx.rWidgetArea.y0 + m_uiCtx.fonts[m_uiCtx.eFont].iYFontOffset);
					vgui::surface()->DrawPrintText(wszText, V_wcslen(wszText));
				}

				NeoUI::Pad(); // Avatar/Dead-indicator
				NeoUI::Pad(); // Name column
				if (iCurTeam >= FIRST_GAME_TEAM)
				{
					NeoUI::Label(L"Ready"); // Hidden when not used/not in ready-up
					NeoUI::Label(L"Dmg to"); // Dealt - Hidden when not showing damage info
					NeoUI::Label(L"Dmg from"); // Taken - Hidden when not showing damage info
					NeoUI::Label(L"Class");
					NeoUI::Label(L"Rank");
					// The number columns center their header and values on one axis
					NeoUI::Label(L"XP", NeoUI::LabelExOpt{ .eTextStyle = NeoUI::TEXTSTYLE_CENTER, .eFont = m_uiCtx.eFont });
					NeoUI::Label(L"D", NeoUI::LabelExOpt{ .eTextStyle = NeoUI::TEXTSTYLE_CENTER, .eFont = m_uiCtx.eFont });
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
					const CNEOScoreBoardPlayer *pPlayerInfo = &m_playersInfo[i];
					const NeoAvatar *pAvatar = &m_avatars[pPlayerInfo->iAvatarIdx];
					const bool bIsPlaying = (pPlayerInfo->iTeam >= FIRST_GAME_TEAM);
					const bool bIsDMPlaying = false == bIsTeamplay && bIsPlaying;
					const bool bIsMuted = pPlayerInfo->bMuted && false == pPlayerInfo->bBot;
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
						m_avatarPopup = *pAvatar;
						m_avatarPopup.Fetch(184);
						const bool bHaveFriendReq = (SteamFriends()
								&& k_EFriendRelationshipRequestInitiator == SteamFriends()->GetFriendRelationship(m_playerPopup.steamID));

						const int iWidePopupCover = iAvatarWT + ((1 + ShowAvatars()) * iAvatarOffset)
								+ NeoUI::SuitableWideByWStr(
										m_playerPopup.wszName,
										NeoUI::SUITABLEWIDE_TABLE,
										NeoUI::FONT_NTLARGE);

						const int iMinWide = ((5 + bHaveFriendReq) * iPopupCardPerRowTallButtons);

						NeoUI::OpenPopup(NEOSCOREBOARDPOPUP_CARD, NeoUI::Dim{
								.x = m_uiCtx.iMouseAbsX,
								.y = m_uiCtx.iMouseAbsY,
								.wide = Max(iMinWide, iWidePopupCover),
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
					else if (bIsMuted)
					{
						vgui::surface()->DrawSetColor(COLOR_MUTE_BG);
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
					// The avatar stands in the middle of its column, a cell margin from the columns either side
					const int iAvatarX = m_uiCtx.rWidgetArea.x0 + m_uiCtx.iMarginX;
					const bool bValidAvatar = (pAvatar->m_iTextureID > 0);
					if (bValidAvatar)
					{
						pAvatar->Paint(iAvatarX,
								m_uiCtx.rWidgetArea.y0,
								m_uiCtx.irWidgetTall,
								pPlayerInfo->bDead && bIsPlaying);
					}
					if (bIsMuted && bValidAvatar)
					{
						// Slightly redden the avatar
						vgui::surface()->DrawSetColor(100, 0, 0, 75);
						vgui::surface()->DrawFilledRect(
								iAvatarX,
								m_uiCtx.rWidgetArea.y0,
								iAvatarX + m_uiCtx.irWidgetTall,
								m_uiCtx.rWidgetArea.y0 + m_uiCtx.irWidgetTall);
					}
					if (iCurTeam >= FIRST_GAME_TEAM)
					{
						// Darken the avatar if not ready
						if (bValidAvatar
								&& (bShowReadyUp && false == pPlayerInfo->bReady))
						{
							vgui::surface()->DrawSetColor(0, 0, 0, 200);
							vgui::surface()->DrawFilledRect(
									iAvatarX,
									m_uiCtx.rWidgetArea.y0,
									iAvatarX + m_uiCtx.irWidgetTall,
									m_uiCtx.rWidgetArea.y0 + m_uiCtx.irWidgetTall);
						}

						// Dead icon - layered over avatar
						if (pPlayerInfo->bDead)
						{
							NeoUI::Texture("vgui/hud/kill_kill",
									iAvatarX,
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

					// Voice mute icon - after name, only do for actual players
					if (bIsMuted)
					{
						const auto *pFontI = &m_uiCtx.fonts[m_uiCtx.eFont];

						int iNameWide = 0, iNameTall = 0;
						vgui::surface()->GetTextSize(pFontI->hdl, wszText, iNameWide, iNameTall);

						const int iMuteIconWH = (m_uiCtx.irWidgetTall * 0.75f);
						const int iStartX = m_uiCtx.rWidgetArea.x0 + iNameWide + (2 * m_uiCtx.iMarginX);
						if ((iStartX + iMuteIconWH) < m_uiCtx.rWidgetArea.x1)
						{
							NeoUI::Texture("vgui/hud/voice_mute",
									iStartX,
									m_uiCtx.rWidgetArea.y0 + ((m_uiCtx.irWidgetTall - iMuteIconWH) / 2.0f),
									iMuteIconWH,
									iMuteIconWH,
									"",
									NeoUI::TEXTUREOPTFLAGS_DONOTCROPTOPANEL);
						}
					}

					if (iCurTeam >= FIRST_GAME_TEAM)
					{
						// Ready-up (Hidden when not used)
						NeoUI::Label(pPlayerInfo->bReady ? L"READY" : L"NOT READY");

						// Damage infos (Hidden when not used)
						// * Dealt
						const Color bkupNormalFg = m_uiCtx.colors.normalFg;
						if (bShowDamageInfo && pPlayerInfo->iDealtDmgs > 0 && pPlayerInfo->iDealtHits > 0)
						{
							if (pPlayerInfo->bYouKilled)
							{
								m_uiCtx.colors.normalFg = COLOR_RED;
							}
							V_swprintf_safe(wszText, L"%d in %d", pPlayerInfo->iDealtDmgs, pPlayerInfo->iDealtHits);
							NeoUI::Label(wszText);
							m_uiCtx.colors.normalFg = bkupNormalFg;
						}
						else
						{
							NeoUI::Pad();
						}
						// * Taken
						if (bShowDamageInfo && pPlayerInfo->iTakenDmgs > 0 && pPlayerInfo->iTakenHits > 0)
						{
							if (pPlayerInfo->bKilledYou)
							{
								m_uiCtx.colors.normalFg = COLOR_RED;
							}
							V_swprintf_safe(wszText, L"%d in %d", pPlayerInfo->iTakenDmgs, pPlayerInfo->iTakenHits);
							NeoUI::Label(wszText);
							m_uiCtx.colors.normalFg = bkupNormalFg;
						}
						else
						{
							NeoUI::Pad();
						}

						// Class and rank are words, so they stay readable in the proportional font
						NeoUI::SwapFont(NeoUI::FONT_NTSMALLTEXT);
						NeoUI::Label(GetNeoClassNameW(pPlayerInfo->iClass));
						NeoUI::SwapFont(NeoUI::FONT_NTNORMAL);

						// Rank, with the kill feed's rank icon before its name
						NeoUI::Pad();
						if (NeoUI::MODE_PAINT == eMode)
						{
							DrawRankCell(pPlayerInfo->iXP);
							// The cell drew in its own fonts, so give NeoUI back the one it expects
							NeoUI::SwapFont(NeoUI::FONT_NTNORMAL, true);
						}

						// XP and deaths, centered under their headers
						const NeoUI::LabelExOpt numberOpt{ .eTextStyle = NeoUI::TEXTSTYLE_CENTER, .eFont = m_uiCtx.eFont };
						V_swprintf_safe(wszText, L"%d", pPlayerInfo->iXP);
						NeoUI::Label(wszText, numberOpt);
						V_swprintf_safe(wszText, L"%d", pPlayerInfo->iDeaths);
						NeoUI::Label(wszText, numberOpt);
					}
				}
				NeoUI::EndTable();
			}
			NeoUI::EndSection();
		}
		m_uiCtx.iMarginX = iBaseMarginX;

		m_uiCtx.colors.normalFg = COLOR_NEO_WHITE;

		if (NeoUI::BeginPopup(NEOSCOREBOARDPOPUP_CARD))
		{
			const bool bValidAvatar = (m_avatarPopup.m_iTextureID > 0);
			if (bValidAvatar)
			{
				m_avatarPopup.Paint(m_uiCtx.dPanel.x + iAvatarOffset,
						m_uiCtx.dPanel.y + iAvatarOffset,
						iAvatarWT);
			}

			NeoUI::SetPerRowLayout(1, nullptr, iPopupCardPerRowTallAvatarName);
			NeoUI::SwapFont(NeoUI::FONT_NTNORMAL);
			NeoUI::Pad();

			vgui::surface()->DrawSetTextPos(
					m_uiCtx.dPanel.x + iAvatarOffset + (bValidAvatar ? (iAvatarWT + iAvatarOffset) : 0),
					m_uiCtx.dPanel.y + iAvatarOffset);
			if (m_playerPopup.wszClantag[0])
			{
				vgui::surface()->DrawSetTextColor(m_uiCtx.colors.normalFg);
				vgui::surface()->DrawPrintText(m_playerPopup.wszClantag,
						V_wcslen(m_playerPopup.wszClantag));

				const auto *pFontI = &m_uiCtx.fonts[m_uiCtx.eFont];
				const int iClantagTall = vgui::surface()->GetFontTall(pFontI->hdl);
				vgui::surface()->DrawSetTextPos(
						m_uiCtx.dPanel.x + iAvatarOffset + (bValidAvatar ? (iAvatarWT + iAvatarOffset) : 0),
						m_uiCtx.dPanel.y + iAvatarOffset + iClantagTall + iAvatarOffset);
			}
			NeoUI::SwapFont(NeoUI::FONT_NTLARGE);
			vgui::surface()->DrawSetTextColor(m_uiCtx.colors.hotFg);
			vgui::surface()->DrawPrintText(m_playerPopup.wszName,
					V_wcslen(m_playerPopup.wszName));

			const bool bHaveFriendReq = (SteamFriends()
					&& k_EFriendRelationshipRequestInitiator == SteamFriends()->GetFriendRelationship(m_playerPopup.steamID));
			const int iBtnCols = 5 + bHaveFriendReq;

			NeoUI::BeginOverridePanelWide(iBtnCols * iPopupCardPerRowTallButtons);

			NeoUI::SetPerRowLayout(iBtnCols, nullptr, iPopupCardPerRowTallButtons);
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
					const char *pszOverlay = nullptr;

					if (NeoUI::ButtonTexture("vgui/hud/player_profile", "GAME",
								L"Profile").bPressed)
					{
						pszOverlay = "steamid";
					}

					if (m_playerPopup.iUserID != iLocalUserID)
					{
						if (NeoUI::ButtonTexture("vgui/hud/player_message", "GAME",
									L"Message").bPressed)
						{
							pszOverlay = "chat";
						}

						const EFriendRelationship eFriendRel =
								SteamFriends()->GetFriendRelationship(steamID);
						switch (eFriendRel)
						{
						case k_EFriendRelationshipNone:
							if (NeoUI::ButtonTexture("vgui/hud/friend_add", "GAME",
										L"Send").bPressed)
							{
								pszOverlay = "friendadd";
							}
							break;
						case k_EFriendRelationshipRequestRecipient:
							if (NeoUI::ButtonTexture("vgui/hud/friend_remove", "GAME",
										L"Cancel").bPressed)
							{
								pszOverlay = "friendremove";
							}
							break;
						case k_EFriendRelationshipRequestInitiator:
							if (NeoUI::ButtonTexture("vgui/hud/friend_request_accept", "GAME",
										L"Accept").bPressed)
							{
								pszOverlay = "friendrequestaccept";
							}
							if (NeoUI::ButtonTexture("vgui/hud/friend_request_ignore", "GAME",
										L"Ignore").bPressed)
							{
								pszOverlay = "friendrequestignore";
							}
							break;
						default:
							break;
						}
					}

					if (pszOverlay)
					{
						SteamFriends()->ActivateGameOverlayToUser(pszOverlay, steamID);
						NeoUI::ClosePopup();
					}
				}

				if (m_playerPopup.iUserID == iLocalUserID)
				{
					if (bShowReadyUp && bLocalPlaying)
					{
						if (NeoUI::ButtonTexture(
									m_playerPopup.bReady
										? "vgui/hud/player_not_ready"
										: "vgui/hud/player_ready",
									"GAME",
									m_playerPopup.bReady
										? L"Unready"
										: L"Ready").bPressed)
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
					if (NeoUI::ButtonTexture("vgui/hud/player_crosshair_icon", "GAME",
								L"Crosshair").bPressed)
					{
						NeoUI::ClosePopup();
						if (m_playerPopup.szCrosshair[0])
						{
							ImportCrosshair(&staticXhairInfo, m_playerPopup.szCrosshair);
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
			NeoUI::EndOverridePanelWide();

			NeoUI::EndPopup();
		}

		// NEO NOTE (nullsystem): With in_main, the special clause for copy crosshair popup
		// means the player don't need to be holding the tab bind while this popup is open.
		const bool bWasCopyXHairPopup = NeoUI::CurrentPopup() == NEOSCOREBOARDPOPUP_COPYCROSSHAIR;
		if (NeoUI::BeginPopup(NEOSCOREBOARDPOPUP_COPYCROSSHAIR))
		{
			NeoUI::SwapFont(NeoUI::FONT_NTLARGE);
			const auto tmpButtonTextStyle = m_uiCtx.eButtonTextStyle;
			m_uiCtx.eButtonTextStyle = NeoUI::TEXTSTYLE_CENTER;

			NeoUI::SetPerRowLayout(1);
			NeoUI::Pad();
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

			// Showcase the crosshair
			const CrosshairWepInfo *pDefXHair = &staticXhairInfo.wep[CROSSHAIR_WEP_DEFAULT];
			if (pDefXHair->iStyle == CROSSHAIR_STYLE_CUSTOM)
			{
				PaintCrosshair(pDefXHair, 0, wide / 2, tall / 2);
			}
			else if (IN_BETWEEN_AR(0, pDefXHair->iStyle, CROSSHAIR_STYLE__TOTAL)
					&& CROSSHAIR_FILES[pDefXHair->iStyle][0])
			{
				Texture *pTex = &m_arTextures[pDefXHair->iStyle];
				vgui::surface()->DrawSetTexture(pTex->iTexId);
				vgui::surface()->DrawSetColor(pDefXHair->color);
				vgui::surface()->DrawTexturedRect(
						(wide / 2) - (pTex->iWide / 2),
						(tall / 2) - (pTex->iTall / 2),
						(wide / 2) + (pTex->iWide / 2),
						(tall / 2) + (pTex->iTall / 2)
						);
			}

			m_uiCtx.eButtonTextStyle = tmpButtonTextStyle;

			NeoUI::EndPopup();
		}
		if (bWasCopyXHairPopup && NeoUI::CurrentPopup() != NEOSCOREBOARDPOPUP_COPYCROSSHAIR)
		{
			engine->ClientCmd_Unrestricted("-score");
		}
	}
	NeoUI::EndContext();
}

