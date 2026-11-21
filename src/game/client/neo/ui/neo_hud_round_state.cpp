#include "cbase.h"

#include <vgui_controls/ImageList.h>
#include "neo_hud_round_state.h"
#include "neo_hud_draw.h"

#include "iclientmode.h"
#include "ienginevgui.h"
#include "engine/IEngineSound.h"

#include <vgui/ILocalize.h>
#include <vgui/ISurface.h>

#include <initializer_list>

#include "c_neo_player.h"
#include "neo_weapon_loadout.h"
#include "c_team.h"
#include "c_playerresource.h"
#include "neo_scoreboard.h"
#include "neo_hud_spectator_overlay.h"

#include "hltvcamera.h"

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

using vgui::surface;

DECLARE_NAMED_HUDELEMENT(CNEOHud_RoundState, NRoundState);

NEO_HUD_ELEMENT_DECLARE_FREQ_CVAR(RoundState, 0.1)

ConVar cl_neo_hud_team_swap_sides("cl_neo_hud_team_swap_sides", "1", FCVAR_ARCHIVE, "Make the team of the local player always appear on the left side of the round info and scoreboard", true, 0.0, true, 1.0);
ConVar cl_neo_squad_hud_original("cl_neo_squad_hud_original", "1", FCVAR_ARCHIVE, "Use the old squad HUD", true, 0.0, true, 1.0);
ConVar cl_neo_squad_hud_star_scale("cl_neo_squad_hud_star_scale", "0", FCVAR_ARCHIVE, "Scaling to apply from 1080p, 0 disables scaling", 
	[](IConVar* pConVar, char const* pOldString, float flOldValue) -> void {
		if (g_pNeoHudRoundState)
			g_pNeoHudRoundState->UpdateStarSize();
});
extern ConVar sv_neo_dm_win_xp;
extern ConVar cl_neo_streamermode;
extern ConVar sv_neo_readyup_countdown;
extern ConVar cl_neo_hud_scoreboard_hide_others;
extern ConVar sv_neo_ctg_ghost_overtime_grace;
extern ConVar cl_neo_hud_health_mode;
extern ConVar sv_neo_bot_cmdr_enable;

namespace {
constexpr int Y_POS = 0;
constexpr bool STARS_HW_FILTERED = false;

// The star texture holds the icon and squad name in one 256x64 image, with the name below the icon's center.
// They are drawn as two parts at their own positions, so they share a center.
constexpr float STAR_ICON_TEXS1 = 64.0f / 256.0f;
constexpr float STAR_NAME_TEXT0 = 30.0f / 64.0f;
constexpr float STAR_NAME_TEXT1 = 62.0f / 64.0f;
constexpr int STAR_ICON_X = 3;
constexpr int STAR_ICON_SIZE = 48;
constexpr int STAR_NAME_Y = 12;
constexpr int STAR_NAME_WIDE = 144;
constexpr int STAR_NAME_TALL = 24;

constexpr int SQUAD_LIST_Y = 50;
constexpr int SQUAD_LIST_GROUP_GAP = 10;
// Pixels between one row's line height and the next row
constexpr int SQUAD_ROW_GAP = 1;
constexpr int SQUAD_ROW_ALPHA = 225;
constexpr int SQUAD_ROW_SMALL_ALPHA = 170;
// A dead teammate's row keeps this share of the alpha its group draws in
constexpr float SQUAD_ROW_DEAD_ALPHA_SCALE = 0.4f;
constexpr float SQUAD_DETAIL_ALPHA_SCALE = 0.72f;

// Sizes written in pixels at 1080p
constexpr int DASH_LOGO_PAD_PX = 2;
// The round, clock and alive rows have even gaps, about 7px, between one's baseline and the next's capitals
constexpr int DASH_ROUND_BASELINE_PX = 17;
constexpr int DASH_CLOCK_BASELINE_PX = 40;
constexpr int DASH_ALIVE_BASELINE_PX = 58;
constexpr int DASH_CLOCK_COLON_WIDE_PX = 4;
constexpr int DASH_CLOCK_COLON_GAP_PX = 7;
constexpr int DASH_ALIVE_VS_GAP_PX = 5;
constexpr int AVATAR_DASH_GAP_PX = 7;
constexpr int AVATAR_GAP_PX = 4;
constexpr int AVATAR_FADE_PAD_PX = 2;
constexpr int AVATAR_NAME_BASELINE_PX = 11;
constexpr int AVATAR_TOP_PX = 13;
constexpr int AVATAR_HEALTH_GAP_PX = 2;
constexpr int AVATAR_HEALTH_TALL_PX = 4;
constexpr int SQUAD_ROW_X_PX = 13;
constexpr int SQUAD_COLUMN_GAP_PX = 10;

// The squad list calls the Juggernaut a Jaeger
const wchar_t *GetSquadClassNameW(const int iClass)
{
	return (iClass == NEO_CLASS_JUGGERNAUT) ? L"Jaeger" : GetNeoClassNameW(iClass);
}

// Master's box at 1080p, where NHudOCR and NHudOCRSmall sized it, 66px tall. The logos fill its height but for their
// pad, and the box's height follows from them. Written in 1080p pixels, the box and its logos scale with the rows
// inside them, since those fonts stop growing above 1600p.
constexpr int DASH_BOX_HALF_WIDE_PX = 130;
constexpr int DASH_LOGO_PX = 62;

const Color COLOR_DASH_ROUND = Color(255, 255, 255, 165);
const Color COLOR_DASH_ALIVE = Color(255, 255, 255, 235);
const Color COLOR_DASH_VS = Color(255, 255, 255, 130);
const Color COLOR_AVATAR_NAME = Color(255, 255, 255, 225);
const Color COLOR_AVATAR_NAME_DEAD = Color(255, 255, 255, 120);
}

CNEOHud_RoundState *g_pNeoHudRoundState;

CNEOHud_RoundState::CNEOHud_RoundState(const char *pElementName, vgui::Panel *parent)
	: CHudElement(pElementName)
	, Panel(parent, pElementName)
{
	g_pNeoHudRoundState = this;
	m_pWszStatusUnicode = L"";
	SetAutoDelete(true);
	m_iHideHudElementNumber = NEO_HUD_ELEMENT_ROUND_STATE;

	if (parent)
	{
		SetParent(parent);
	}
	else
	{
		SetParent(g_pClientMode->GetViewport());
	}

	// Initialize star textures
	for (int i = 0; i < STAR__TOTAL; ++i)
	{
		static constexpr const char *STAR_TEXTURES[STAR__TOTAL] = {
			"vgui/hud/star_alpha", "vgui/hud/star_bravo", "vgui/hud/star_charlie", "vgui/hud/star_delta",
			"vgui/hud/star_echo", "vgui/hud/star_foxtrot", "vgui/hud/star_none"
		};
		m_iStarTextures[i] = surface()->CreateNewTextureID();
		surface()->DrawSetTextureFile(m_iStarTextures[i], STAR_TEXTURES[i], STARS_HW_FILTERED, false);
	}

	m_iClassIcons = surface()->CreateNewTextureID();
	surface()->DrawSetTextureFile(m_iClassIcons, "vgui/classIcons", true, false);
	
	m_nPlayerList.RemoveAll();
	for (int i = 0; i < MAX_PLAYERS; i++)
	{
		m_nPlayerList.AddToTail(playerIndexAndTheirValue(i + 1, -1));
	}

	struct TeamLogoColorInfo
	{
		const char *logo;
		const char *totalLogo;
		Color color;
	};
	for (int i = FIRST_GAME_TEAM; i < TEAM__TOTAL; ++i)
	{
		static const TeamLogoColorInfo TEAM_TEX_INFO[TEAM__TOTAL - FIRST_GAME_TEAM] = {
			{.logo = "vgui/jinrai_128tm", .totalLogo = "vgui/ts_jinrai", .color = COLOR_JINRAI},
			{.logo = "vgui/nsf_128tm", .totalLogo = "vgui/ts_nsf", .color = COLOR_NSF},
		};
		const int texIdx = i - FIRST_GAME_TEAM;
		m_teamLogoColors[i] = TeamLogoColor{
			.logo = surface()->CreateNewTextureID(),
			.totalLogo = surface()->CreateNewTextureID(),
			.color = TEAM_TEX_INFO[texIdx].color,
		};
		surface()->DrawSetTextureFile(m_teamLogoColors[i].logo, TEAM_TEX_INFO[texIdx].logo, true, false);
		surface()->DrawSetTextureFile(m_teamLogoColors[i].totalLogo, TEAM_TEX_INFO[texIdx].totalLogo, true, false);
	}

}

void CNEOHud_RoundState::LevelShutdown(void)
{
	m_iStarToDraw = -1;

	// NEO NOTE (Adam) set m_iPreviouslyActiveStar && m_iPreviouslyActiveTeam to -1? Seems to work fine without 
}

ConVar cl_neo_squad_hud_avatar_size("cl_neo_squad_hud_avatar_size", "0", FCVAR_ARCHIVE, "Size of squad hud avatars, 0 to scale with screen size (specifically size of font used in the hud)", true, 0, false, 0,
	[](IConVar* pConVar, char const* pOldString, float flOldValue) -> void {
		if (g_pNeoHudRoundState)
			g_pNeoHudRoundState->UpdateAvatarSize();
});

void CNEOHud_RoundState::UpdateAvatarSize()
{
	const int overrideAvatarSize = cl_neo_squad_hud_avatar_size.GetInt();
	if (overrideAvatarSize)
	{
		m_ilogoSize = overrideAvatarSize;
	}
	else
	{
		int iFontHeight = 0;
		int iFontWidth = 0;
		surface()->GetTextSize(m_hOCRFont, L"ROUND 99", iFontWidth, iFontHeight);
		iFontHeight *= 0.85;
		m_ilogoSize = m_iSmallFontHeight + iFontHeight;
	}
	m_iAvatarsYEnd = Max(m_iBoxYEnd, m_iAvatarTop + m_ilogoSize + m_iAvatarHealthGap + m_iAvatarHealthTall);
}

void CNEOHud_RoundState::UpdateStarSize()
{
	IntDim res = {};
	surface()->GetScreenSize(res.w, res.h);
	m_flStarScale = cl_neo_squad_hud_star_scale.GetFloat() != 0 ? cl_neo_squad_hud_star_scale.GetFloat() * (res.h / 1080.0f)
																: 1.f;
}

void CNEOHud_RoundState::ApplySchemeSettings(vgui::IScheme* pScheme)
{
	BaseClass::ApplySchemeSettings(pScheme);

	m_hOCRLargeFont = NeoHud::GetFont(pScheme, "NHudOCRLarge", vgui::INVALID_FONT, false);
	m_hOCRFont = NeoHud::GetFont(pScheme, "NHudOCR", vgui::INVALID_FONT, false);
	m_hOCRSmallFont = NeoHud::GetFont(pScheme, "NHudOCRSmall", vgui::INVALID_FONT, false);
	m_hOCRSmallerFont = NeoHud::GetFont(pScheme, "NHudOCRSmaller", vgui::INVALID_FONT, false);
	m_hTinyText = NeoHud::GetFont(pScheme, "NHudMonoTiny", NeoHud::GetFont(pScheme, "NHudTinyText", vgui::INVALID_FONT, false), false);
	m_hRoundFont = NeoHud::GetFont(pScheme, "NHudDashRound", m_hOCRSmallerFont, false);
	m_hClockFont = NeoHud::GetFont(pScheme, "NHudDashClock", m_hOCRFont, false);
	m_hAliveFont = NeoHud::GetFont(pScheme, "NHudDashAlive", m_hOCRSmallerFont, false);
	m_hAliveVsFont = NeoHud::GetFont(pScheme, "NHudDashVs", m_hOCRSmallerFont, false);
	m_hScoreFont = NeoHud::GetFont(pScheme, "NHudDashScore", m_hOCRLargeFont, false);
	m_hStatusFont = NeoHud::GetFont(pScheme, "NHudDashStatus", m_hOCRSmallFont, false);

	SetFgColor(COLOR_TRANSPARENT);
	SetBgColor(COLOR_TRANSPARENT);

	// Screen dimensions
	IntDim res = {};
	surface()->GetScreenSize(res.w, res.h);
	m_iXpos = (res.w / 2);

	// The player avatars are sized from the fonts, as in master
	[[maybe_unused]] int iSmallFontWidth = 0;
	surface()->GetTextSize(m_hOCRSmallFont, L"ROUND 99", iSmallFontWidth, m_iSmallFontHeight);
	m_iSmallFontHeight *= 0.85;

	// Box dimensions
	const int iLogoPad = ScaledPx(DASH_LOGO_PAD_PX);
	const int iLogoSize = ScaledPx(DASH_LOGO_PX);
	const int iBoxWidthHalf = ScaledPx(DASH_BOX_HALF_WIDE_PX);
	m_iLeftOffset = m_iXpos - iBoxWidthHalf;
	m_iRightOffset = m_iXpos + iBoxWidthHalf;
	m_iBoxYEnd = Y_POS + iLogoSize + iLogoPad * 2;

	m_iRoundBaseline = ScaledPx(DASH_ROUND_BASELINE_PX);
	m_iClockBaseline = ScaledPx(DASH_CLOCK_BASELINE_PX);
	m_iAliveBaseline = ScaledPx(DASH_ALIVE_BASELINE_PX);
	m_iClockColonWide = ScaledPx(DASH_CLOCK_COLON_WIDE_PX);
	m_iClockColonGap = ScaledPx(DASH_CLOCK_COLON_GAP_PX);
	m_iAliveVsGap = ScaledPx(DASH_ALIVE_VS_GAP_PX);

	m_iAvatarDashGap = ScaledPx(AVATAR_DASH_GAP_PX);
	m_iAvatarGap = ScaledPx(AVATAR_GAP_PX);
	m_iAvatarFadePad = ScaledPx(AVATAR_FADE_PAD_PX);
	m_iAvatarNameBaseline = ScaledPx(AVATAR_NAME_BASELINE_PX);
	m_iAvatarTop = ScaledPx(AVATAR_TOP_PX);
	m_iAvatarHealthGap = ScaledPx(AVATAR_HEALTH_GAP_PX);
	m_iAvatarHealthTall = ScaledPx(AVATAR_HEALTH_TALL_PX);

	m_iSquadRowX = ScaledPx(SQUAD_ROW_X_PX);
	m_iSquadColumnGap = ScaledPx(SQUAD_COLUMN_GAP_PX);
	// The squad list's fixed columns, each as wide as the widest text it can hold in the squad's rows, whose font is the larger
	m_iSquadClassWide = 0;
	for (int i = 0; i < NEO_CLASS__ENUM_COUNT; ++i)
	{
		m_iSquadClassWide = Max(m_iSquadClassWide, NeoHud::GetTextWide(m_hOCRSmallFont, GetSquadClassNameW(i)));
	}
	m_iSquadRankWide = 0;
	for (const int iXP : { XP_PRIVATE - 1, XP_PRIVATE, XP_CORPORAL, XP_SERGEANT, XP_LIEUTENANT })
	{
		m_iSquadRankWide = Max(m_iSquadRankWide, NeoHud::GetTextWide(m_hOCRSmallFont, GetRankNameW(iXP, true)));
	}
	// Health reaches three digits, measured here at the widest digit, and a dead teammate's row reads DEAD
	const int iDeadWide = NeoHud::GetTextWide(m_hOCRSmallFont, L"DEAD");
	m_iSquadHealthHpWide = Max(iDeadWide, NeoHud::GetNumberWide(m_hOCRSmallFont, L"000hp"));
	m_iSquadHealthPercentWide = Max(iDeadWide, NeoHud::GetNumberWide(m_hOCRSmallFont, L"000%"));

	UpdateAvatarSize();
	UpdateStarSize();

	m_rectLeftTeamTotalLogo = vgui::IntRect{
		.x0 = m_iLeftOffset + iLogoPad,
		.y0 = Y_POS + iLogoPad,
		.x1 = m_iLeftOffset + iLogoPad + iLogoSize,
		.y1 = Y_POS + iLogoPad + iLogoSize,
	};
	m_rectRightTeamTotalLogo = vgui::IntRect{
		.x0 = m_iRightOffset - iLogoPad - iLogoSize,
		.y0 = Y_POS + iLogoPad,
		.x1 = m_iRightOffset - iLogoPad,
		.y1 = Y_POS + iLogoPad + iLogoSize,
	};

	// The scores share the clock's baseline
	m_posLeftTeamScore = IntPos{
		.x = m_rectLeftTeamTotalLogo.x0 + (iLogoSize / 2),
		.y = m_iClockBaseline,
	};
	m_posRightTeamScore = IntPos{
		.x = m_rectRightTeamTotalLogo.x0 + (iLogoSize / 2),
		.y = m_iClockBaseline,
	};

	// Clear player avatars

	SetBounds(0, Y_POS, res.w, res.h);
	SetZPos(90);
}

void CNEOHud_RoundState::UpdateStateForNeoHudElementDraw()
{
	float roundTimeLeft = NEORules()->GetRoundRemainingTime();
	const NeoRoundStatus roundStatus = NEORules()->GetRoundStatus();
	const bool inSuddenDeath = NEORules()->RoundIsInSuddenDeath();
	const bool inDoOrDie = NEORules()->RoundIsDoOrDie();
	const bool inMatchPoint = !inDoOrDie && NEORules()->RoundIsMatchPoint(); // we don't care about matchpoint if in do or die

	m_pWszStatusUnicode = L"";
	if (roundStatus == NeoRoundStatus::Idle)
	{
		m_pWszStatusUnicode = NEORules()->IsReadyUpEnabled() ? L"Waiting for players to ready up" : L"Waiting for players";
	}
	else if (roundStatus == NeoRoundStatus::Warmup)
	{
		m_pWszStatusUnicode = L"Warmup";
	}
	else if (roundStatus == NeoRoundStatus::Countdown)
	{
		m_pWszStatusUnicode = L"Match is starting...";
	}
	else if (inSuddenDeath)
	{
		m_pWszStatusUnicode = L"Sudden death";
	}
	else if (inDoOrDie)
	{
		m_pWszStatusUnicode = L"Do or Die";
	}
	else if (inMatchPoint)
	{
		m_pWszStatusUnicode = L"Match point";
	}
	else if (NEORules()->GetRoundStatus() != NeoRoundStatus::Pause && (NEORules()->IsRoundPreRoundFreeze() || g_pNeoScoreBoard->IsVisible()))
	{
		// Update Objective
		switch (NEORules()->GetGameType()) {
		case NEO_GAME_TYPE_DM:
			// Don't print objective for deathmatch
			m_pWszStatusUnicode = L"\0";
			break;
		case NEO_GAME_TYPE_TDM:
			m_pWszStatusUnicode = L"Score the most Points\n";
			break;
		case NEO_GAME_TYPE_CTG:
			m_pWszStatusUnicode = L"Capture the Ghost\n";
			break;
		case NEO_GAME_TYPE_VIP:
			if (GetLocalPlayerTeam() == NEORules()->m_iEscortingTeam.Get())
			{
				if (NEORules()->GhostExists())
				{
					m_pWszStatusUnicode = L"VIP down, prevent Ghost capture\n";
				}
				else
				{
					m_pWszStatusUnicode = L"Escort the VIP\n";
				}
			}
			else
			{
				if (NEORules()->GhostExists())
				{
					m_pWszStatusUnicode = L"HVT down, secure the Ghost\n";
				}
				else
				{
					m_pWszStatusUnicode = L"Eliminate the HVT\n";
				}
			}
			break;
		case NEO_GAME_TYPE_JGR:
			m_pWszStatusUnicode = L"Control the Juggernaut\n";
			break;
		default:
			m_pWszStatusUnicode = L"Await further orders\n";
			break;
		}
	}
	m_iStatusUnicodeSize = V_wcslen(m_pWszStatusUnicode);

	// Clear the strings so zero roundTimeLeft also picks it up as to not draw
	memset(m_wszRoundUnicode, 0, sizeof(m_wszRoundUnicode));
	memset(m_wszPlayersAliveUnicode, 0, sizeof(m_wszPlayersAliveUnicode));
	memset(m_wszTime, 0, sizeof(m_wszTime));
	memset(m_wszLeftTeamScore, 0, sizeof(m_wszLeftTeamScore));
	memset(m_wszRightTeamScore, 0, sizeof(m_wszRightTeamScore));
	memset(m_wszLeftPlayersAlive, 0, sizeof(m_wszLeftPlayersAlive));
	memset(m_wszRightPlayersAlive, 0, sizeof(m_wszRightPlayersAlive));
	m_bPlayersAliveAreCounts = false;

	// Exactly zero means there's no time limit, so we don't need to draw anything.
	if (roundTimeLeft == 0)
	{
		return;
	}
	// Less than 0 means round is over, but cap the timer to zero for nicer display.
	else if (roundTimeLeft < 0)
	{
		roundTimeLeft = 0;
	}

	if (NEORules()->GetRoundStatus() == NeoRoundStatus::Pause)
	{
		V_swprintf_safe(m_wszRoundUnicode, L"PAUSED");
		roundTimeLeft = NEORules()->m_flPauseEnd.Get() - gpGlobals->curtime;
	}
	else if (NEORules()->GetRoundStatus() == NeoRoundStatus::Countdown)
	{
		V_swprintf_safe(m_wszRoundUnicode, L"STARTING");
	}
	else if (NEORules()->GetRoundStatus() == NeoRoundStatus::Overtime)
	{
		V_swprintf_safe(m_wszRoundUnicode, L"OVERTIME");
	}
	else
	{
		if (NEORules()->GetGameType() == NEO_GAME_TYPE_DM)
		{
			V_swprintf_safe(m_wszRoundUnicode, L"DEATHMATCH");
		}
		else
		{
			V_swprintf_safe(m_wszRoundUnicode, L"ROUND %i", NEORules()->roundNumber());
		}
	}

	if (roundStatus == NeoRoundStatus::PreRoundFreeze)
		roundTimeLeft = NEORules()->GetRemainingPreRoundFreezeTime(true);

	int secsTotal = 0.0f;
	if (roundStatus == NeoRoundStatus::Overtime && (NEORules()->GetGameType() == NEO_GAME_TYPE_CTG || NEORules()->GetGameType() == NEO_GAME_TYPE_ATK))
	{
		secsTotal = RoundFloatToInt(NEORules()->GetOverTime((NeoGameType)NEORules()->GetGameType()));
	}
	else
	{
		secsTotal = RoundFloatToInt(roundTimeLeft);
	}

	const int secsRemainder = secsTotal % 60;
	const int minutes = (secsTotal - secsRemainder) / 60;
	V_snwprintf(m_wszTime, 6, L"%02d:%02d", minutes, secsRemainder);

	int iDMHighestXP = 0;

	const int localPlayerTeam = GetLocalPlayerTeam();
	if (NEORules()->IsTeamplay())
	{
		if (cl_neo_hud_team_swap_sides.GetBool() && (localPlayerTeam == TEAM_JINRAI || localPlayerTeam == TEAM_NSF)) {
			V_snwprintf(m_wszLeftTeamScore, 3, L"%i", GetGlobalTeam(localPlayerTeam)->GetRoundsWon());
			V_snwprintf(m_wszRightTeamScore, 3, L"%i", GetGlobalTeam(NEORules()->GetOpposingTeam(localPlayerTeam))->GetRoundsWon());
		}
		else {
			V_snwprintf(m_wszLeftTeamScore, 3, L"%i", GetGlobalTeam(TEAM_JINRAI)->GetRoundsWon());
			V_snwprintf(m_wszRightTeamScore, 3, L"%i", GetGlobalTeam(TEAM_NSF)->GetRoundsWon());
		}
	}
	else
	{
		[[maybe_unused]] int iDMHighestTotal;
		NEORules()->GetDMHighestScorers(&iDMHighestTotal, &iDMHighestXP);
	}

	char szPlayersAliveANSI[ARRAYSIZE(m_wszPlayersAliveUnicode)] = {};
	if (NEORules()->GetGameType() == NEO_GAME_TYPE_DM)
	{
		// NEO NOTE (nullsystem): Show highest player score
		if (sv_neo_dm_win_xp.GetInt() > 0)
		{
			V_sprintf_safe(szPlayersAliveANSI, "Lead: %d/%d", iDMHighestXP, sv_neo_dm_win_xp.GetInt());
		}
		else
		{
			V_sprintf_safe(szPlayersAliveANSI, "Lead: %d", iDMHighestXP);
		}
	}
	else if (NEORules()->GetGameType() == NEO_GAME_TYPE_TDM || NEORules()->GetGameType() == NEO_GAME_TYPE_JGR)
	{
		if (cl_neo_hud_team_swap_sides.GetBool() && (localPlayerTeam == TEAM_JINRAI || localPlayerTeam == TEAM_NSF)) {
			V_sprintf_safe(szPlayersAliveANSI, "%i:%i", GetGlobalTeam(localPlayerTeam)->Get_Score(), GetGlobalTeam(NEORules()->GetOpposingTeam(localPlayerTeam))->Get_Score());
		}
		else {
			V_sprintf_safe(szPlayersAliveANSI, "%i:%i", GetGlobalTeam(TEAM_JINRAI)->Get_Score(), GetGlobalTeam(TEAM_NSF)->Get_Score());
		}
	}
	else
	{
		const int localPlayerTeam = GetLocalPlayerTeam();
		const bool localPlayerSpecOrNoTeam = !NEORules()->IsTeamplay() || !(localPlayerTeam == TEAM_JINRAI || localPlayerTeam == TEAM_NSF);
		const bool swapTeamSides = cl_neo_hud_team_swap_sides.GetBool();
		const int leftTeam = swapTeamSides ? (localPlayerSpecOrNoTeam ? TEAM_JINRAI : localPlayerTeam) : TEAM_JINRAI;

		int iLeftPlayersAlive = 0;
		int iRightPlayersAlive = 0;
		for (int i = 1; i <= gpGlobals->maxClients; i++)
		{
			if (!g_PR->IsConnected(i))
			{
				continue;
			}
			const int playerTeam = g_PR->GetTeam(i);
			if (playerTeam != leftTeam)
			{
				if (g_PR->IsAlive(i))
				{
					iRightPlayersAlive++;
				}
			}
			else
			{
				if (g_PR->IsAlive(i))
				{
					iLeftPlayersAlive++;
				}
			}
		}
		V_sprintf_safe(szPlayersAliveANSI, "%i vs %i", iLeftPlayersAlive, iRightPlayersAlive);
		V_swprintf_safe(m_wszLeftPlayersAlive, L"%i", iLeftPlayersAlive);
		V_swprintf_safe(m_wszRightPlayersAlive, L"%i", iRightPlayersAlive);
		m_bPlayersAliveAreCounts = true;
	}
	g_pVGuiLocalize->ConvertANSIToUnicode(szPlayersAliveANSI, m_wszPlayersAliveUnicode, sizeof(m_wszPlayersAliveUnicode));

	C_NEO_Player* localPlayer = C_NEO_Player::GetLocalNEOPlayer();
	if (localPlayer)
	{

		if (NEORules()->GetRoundStatus() == NeoRoundStatus::Countdown)
		{
			// NEO NOTE (nullsystem): Keep secs in integer and previous round state
			// so it beeps client-side in sync with time change
			if (m_ePrevRoundStatus != NeoRoundStatus::Countdown)
			{
				m_iBeepSecsTotal = sv_neo_readyup_countdown.GetInt();
			}

			if (m_iBeepSecsTotal != secsTotal)
			{
				const bool bEndBeep = secsTotal == 0;
				const float flVol = bEndBeep ? 1.0f : 0.7f;
				static constexpr int PITCH_END = 165;
				enginesound->EmitAmbientSound("tutorial/hitsound.wav", flVol, bEndBeep ? PITCH_END : PITCH_NORM);
				m_iBeepSecsTotal = secsTotal;
			}
			// Otherwise don't beep/alter m_iBeepSecsTotal
		}
	}

	m_ePrevRoundStatus = NEORules()->GetRoundStatus();
}

ConVar cl_neo_squad_hud_sort_players_by_class_alive_and_star("cl_neo_squad_hud_sort_players_by_class_alive_and_star", "1", FCVAR_ARCHIVE, "whether to sort the top element by squad, then within the squad by alive status and then within the two status by class", true, 0, true, 1);
void CNEOHud_RoundState::DrawNeoHudElement()
{
	CheckActiveStar();

	if (!ShouldDraw())
	{
		return;
	}

	DrawActiveStar();

	int fontWidth, fontHeight;

	// Draw Box
	DrawNeoHudRoundedBox(m_iLeftOffset, Y_POS, m_iRightOffset, m_iBoxYEnd, box_color, top_left_corner, top_right_corner, bottom_left_corner, bottom_right_corner);

	// Draw round
	surface()->DrawSetTextColor((NEORules()->GetRoundStatus() == NeoRoundStatus::Pause) ? COLOR_RED : COLOR_DASH_ROUND);
	NeoHud::DrawTextCentered(m_hRoundFont, m_iXpos, m_iRoundBaseline, m_wszRoundUnicode);

	// Draw round status
	surface()->DrawSetTextColor(COLOR_WHITE);
	surface()->DrawSetTextFont(m_hStatusFont);
	surface()->GetTextSize(m_hStatusFont, m_pWszStatusUnicode, fontWidth, fontHeight);
	surface()->DrawSetTextPos(m_iXpos - (fontWidth / 2), m_iBoxYEnd);
	NeoHud::DrawPrintText(m_pWszStatusUnicode, m_iStatusUnicodeSize);

	const int localPlayerTeam = GetLocalPlayerTeam();
	const bool localPlayerSpecOrNoTeam = !NEORules()->IsTeamplay() || !(localPlayerTeam == TEAM_JINRAI || localPlayerTeam == TEAM_NSF);

	bool swapTeamSides = cl_neo_hud_team_swap_sides.GetBool();
	const int leftTeam = swapTeamSides ? (localPlayerSpecOrNoTeam ? TEAM_JINRAI : localPlayerTeam) : TEAM_JINRAI;
	const int rightTeam = swapTeamSides ? ((leftTeam == TEAM_JINRAI) ? TEAM_NSF : TEAM_JINRAI) : TEAM_NSF;
	const auto leftTeamInfo = m_teamLogoColors[leftTeam];
	const auto rightTeamInfo = m_teamLogoColors[rightTeam];

	// Draw total players alive (or score)
	DrawPlayersAlive();

	// Draw time
	const bool bClockRed = NEORules()->GetRoundStatus() == NeoRoundStatus::PreRoundFreeze || NEORules()->GetRoundStatus() == NeoRoundStatus::Countdown || NEORules()->GetGameType() == NEO_GAME_TYPE_CTG ?
		(NEORules()->GetRoundStatus() == NeoRoundStatus::Overtime && (NEORules()->GetRoundRemainingTime() < sv_neo_ctg_ghost_overtime_grace.GetFloat())) : NEORules()->GetRoundStatus() == NeoRoundStatus::Overtime;
	DrawRoundClock(bClockRed ? COLOR_RED : COLOR_WHITE);

	if (NEORules()->IsTeamplay())
	{
		// Draw score logo
		surface()->DrawSetTexture(leftTeamInfo.totalLogo);
		surface()->DrawSetColor(COLOR_FADED_DARK);
		surface()->DrawTexturedRect(m_rectLeftTeamTotalLogo.x0,
									m_rectLeftTeamTotalLogo.y0,
									m_rectLeftTeamTotalLogo.x1,
									m_rectLeftTeamTotalLogo.y1);

		surface()->DrawSetTexture(rightTeamInfo.totalLogo);
		surface()->DrawTexturedRect(m_rectRightTeamTotalLogo.x0,
									m_rectRightTeamTotalLogo.y0,
									m_rectRightTeamTotalLogo.x1,
									m_rectRightTeamTotalLogo.y1);

		// Draw score
		surface()->DrawSetTextColor(leftTeamInfo.color);
		const float flScoreTracking = NeoHud::GetReadoutTracking(m_hScoreFont);
		NeoHud::DrawNumberCentered(m_hScoreFont, m_posLeftTeamScore.x, m_posLeftTeamScore.y, m_wszLeftTeamScore, flScoreTracking);

		surface()->DrawSetTextColor(rightTeamInfo.color);
		NeoHud::DrawNumberCentered(m_hScoreFont, m_posRightTeamScore.x, m_posRightTeamScore.y, m_wszRightTeamScore, flScoreTracking);
	}

	m_iLeftPlayersTotal = m_iRightPlayersTotal = 0;

	if (!g_PR)
		return;

	if (localPlayerSpecOrNoTeam && cl_neo_hud_spectator_overlay_enabled.GetBool())
	{
		return;
	}

	if (NEORules()->IsTeamplay() && (!cl_neo_squad_hud_original.GetBool() || localPlayerSpecOrNoTeam))
	{ // Sort player list even if not drawing new hud so spectators can use commands
		for (int i = 0; i < MAX_PLAYERS; i++)
		{ // First pass update player values, and count them while we're doing it
			constexpr const int INDEX_SHIFT = 6;
			constexpr const int CLASS_SHIFT = 3;
			constexpr const int ALIVE_SHIFT = 1;
			constexpr const int STAR_SHIFT = 3;
			COMPILE_TIME_ASSERT(MAX_PLAYERS < (1 << INDEX_SHIFT));
			COMPILE_TIME_ASSERT(NEO_CLASS_ENUM_COUNT < (1 << CLASS_SHIFT));
			COMPILE_TIME_ASSERT(STAR__TOTAL < (1 << STAR_SHIFT));
			COMPILE_TIME_ASSERT(INT_MAX > ((TEAM__TOTAL - FIRST_GAME_TEAM) + 1) << (INDEX_SHIFT + CLASS_SHIFT + ALIVE_SHIFT + STAR_SHIFT));

			// m_nPlayerList[x].playerValue is used to sort players by team, within that team by star, within that star by isAlive, between the two isAlive options by class, and within each class by playerIndex
			//              unused, somewhere left of team      star   class
			//                                       /|\          /|  /|
			// m_nPlayerList[x].playerValue = 10000000000000000001110111000000
			//							      |                \|   |       \|
			//	                           sign              team   alive    playerIndex			

			int playerTeam = g_PR->GetTeam(m_nPlayerList[i].playerIndex);
			if (playerTeam >= FIRST_GAME_TEAM)
			{ // reorder the teams in the list, jinrai appear before nsf even though they have a lower index
				playerTeam = TEAM__TOTAL - 1 - (playerTeam - FIRST_GAME_TEAM);
			}
			m_nPlayerList[i].playerValue = m_nPlayerList[i].playerIndex + 
					((playerTeam - FIRST_GAME_TEAM) << (INDEX_SHIFT + CLASS_SHIFT + ALIVE_SHIFT + STAR_SHIFT));
			if (cl_neo_squad_hud_sort_players_by_class_alive_and_star.GetBool())
			{
				m_nPlayerList[i].playerValue +=
					(g_PR->GetClass(m_nPlayerList[i].playerIndex) << INDEX_SHIFT) +
					(g_PR->IsAlive(m_nPlayerList[i].playerIndex) << (INDEX_SHIFT + CLASS_SHIFT)) + 
					((STAR__TOTAL - g_PR->GetStar(m_nPlayerList[i].playerIndex)) << (INDEX_SHIFT + CLASS_SHIFT + ALIVE_SHIFT));
			}

			if (!g_PR->IsConnected(m_nPlayerList[i].playerIndex) || !g_PR->IsValid( m_nPlayerList[i].playerIndex ) || playerTeam < FIRST_GAME_TEAM)
			{
				m_nPlayerList[i].playerValue = -1;
				continue;
			}

			if (playerTeam == TEAM__TOTAL - 1 - (leftTeam - FIRST_GAME_TEAM))
			{
				m_iLeftPlayersTotal++;
			}
			else if (playerTeam == TEAM__TOTAL - 1 - (rightTeam - FIRST_GAME_TEAM))
			{
				m_iRightPlayersTotal++;
			}
		}

		m_nPlayerList.Sort([](const playerIndexAndTheirValue *first, const playerIndexAndTheirValue *second)->int{return second->playerValue - first->playerValue;});
	}

	if (cl_neo_squad_hud_original.GetBool())
	{
		DrawPlayerList();
		return;
	}

	// Draw players on top
	m_iAvatarsDrawnFrame = gpGlobals->framecount;
	int leftCount = 0;
	int rightCount = 0;
	if (NEORules()->IsTeamplay())
	{
		// Fade background to make names easier to see
		const int iFadeSide = m_iAvatarDashGap + m_iAvatarFadePad - m_iAvatarGap;
		surface()->DrawSetColor(COLOR_DARK);
		surface()->DrawFilledRectFade(m_iLeftOffset - iFadeSide - (m_iLeftPlayersTotal * (m_ilogoSize + m_iAvatarGap)), 0,
										m_iLeftOffset, m_iAvatarsYEnd, 255, 0, false);
		surface()->DrawFilledRectFade(m_iRightOffset, 0,
			m_iRightOffset + iFadeSide + (m_iRightPlayersTotal * (m_ilogoSize + m_iAvatarGap)), m_iAvatarsYEnd, 255, 0, false);

		int selectedPlayerIndexInList = localPlayerSpecOrNoTeam ? m_iSelectedPlayer : 0;

		// The list is sorted by team, so could just iterate through one team and then the other, but cl_neo_hud_team_swap_sides and whether the local player is a spectator or not makes this
		// complicated, NEO TODO (Adam) optimize the if (playerTeam == leftTeam or rightTeam) and if (!g_PR->IsConnected(m_nPlayerList[i].first)) checks away
		for (int i = 0; i < gpGlobals->maxClients; i++)
		{
			if (!g_PR->IsConnected(m_nPlayerList[i].playerIndex))
				return; // List is sorted, no more connected players after first not connected player

			const int playerTeam = g_PR->GetTeam(m_nPlayerList[i].playerIndex);
			if (playerTeam == leftTeam)
			{
				const int xOffset = (m_iLeftOffset - m_iAvatarDashGap) - ((leftCount + 1) * m_ilogoSize) - (leftCount * m_iAvatarGap);
				DrawPlayer(m_nPlayerList[i].playerIndex, leftCount, leftTeamInfo, xOffset, localPlayerTeam == playerTeam || localPlayerSpecOrNoTeam, -(leftCount+1) == selectedPlayerIndexInList);
				leftCount++;
			}
			else if (playerTeam == rightTeam)
			{
				const int xOffset = (m_iRightOffset + m_iAvatarDashGap) + (rightCount * (m_ilogoSize + m_iAvatarGap));
				DrawPlayer(m_nPlayerList[i].playerIndex, rightCount, rightTeamInfo, xOffset, localPlayerTeam == playerTeam || localPlayerSpecOrNoTeam, (rightCount+1) == selectedPlayerIndexInList);
				rightCount++;
			}
		}
	}
	else
	{
		PlayerXPInfo playersOrder[MAX_PLAYERS + 1] = {};
		int iTotalPlayers = 0;
		DMClSortedPlayers(&playersOrder, &iTotalPlayers);

		// Second pass: Render the players in this order
		const int iLTRSwitch = Ceil2Int(iTotalPlayers / 2.0f);
		leftCount = (iLTRSwitch - 1); // Start from furthest leftCount index from the center
		rightCount = 0;
		bool bOnLeft = true;
		for (int i = 0; i < iTotalPlayers; ++i)
		{
			if (i == iLTRSwitch)
			{
				bOnLeft = false;
			}

			const int iPlayerIdx = playersOrder[i].idx;
			const bool bPlayerLocal = g_PR->IsLocalPlayer(iPlayerIdx);

			// NEO NOTE (nullsystem): Even though they can be Jinrai/NSF, at most it's just a skin and different
			// color in non-teamplay deathmatch mode.
			const auto lrTeamInfo = (g_PR->GetTeam(iPlayerIdx) == leftTeam) ? leftTeamInfo : rightTeamInfo;
			if (bOnLeft)
			{
				const int xOffset = (m_iLeftOffset - m_iAvatarDashGap) - ((leftCount + 1) * m_ilogoSize) - (leftCount * m_iAvatarGap);
				DrawPlayer(iPlayerIdx, leftCount, lrTeamInfo, xOffset, bPlayerLocal);
				--leftCount;
			}
			else
			{
				const int xOffset = (m_iRightOffset + m_iAvatarDashGap) + (rightCount * (m_ilogoSize + m_iAvatarGap));
				DrawPlayer(iPlayerIdx, rightCount, lrTeamInfo, xOffset, bPlayerLocal);
				rightCount++;
			}
		}
	}
}

void CNEOHud_RoundState::DrawRoundClock(const Color &color) const
{
	if (!m_wszTime[0])
	{
		return;
	}

	// The colon holds still at screen center, with minutes and seconds hanging off either side of it
	wchar_t wszMinutes[ARRAYSIZE(m_wszTime)] = {};
	V_wcsncpy(wszMinutes, m_wszTime, sizeof(wszMinutes));
	const wchar_t *pwszSeconds = L"";
	if (wchar_t *pwchColon = wcschr(wszMinutes, L':'))
	{
		*pwchColon = L'\0';
		pwszSeconds = pwchColon + 1;
	}

	const int iColonX0 = m_iXpos - (m_iClockColonWide / 2);
	const int iColonX1 = iColonX0 + m_iClockColonWide;

	// The digits keep the font's own widths and spacing, so the minutes' ink ends and the seconds' ink starts a gap
	// from the colon whatever the digits are
	surface()->DrawSetTextColor(color);
	NeoHud::DrawTextRight(m_hClockFont, iColonX0 - m_iClockColonGap, m_iClockBaseline, wszMinutes);
	NeoHud::DrawTextLeft(m_hClockFont, iColonX1 + m_iClockColonGap, m_iClockBaseline, pwszSeconds);

	// Two square blocks half a block apart, centered on the digits' height above the baseline. The font's ascent
	// runs well above the digits, and a colon centered in it stood high, its top block level with the digits' tops.
	// Neuropol2's digits stand this share of its ascent tall.
	static constexpr float CLOCK_NUMERAL_TALL = 0.73f;
	const int iNumeralTall = RoundFloatToInt(surface()->GetFontAscent(m_hClockFont, L'0') * CLOCK_NUMERAL_TALL);
	const int iColonInset = (iNumeralTall - (m_iClockColonWide * 2) - (m_iClockColonWide / 2)) / 2;
	surface()->DrawSetColor(color);
	const int iTop = m_iClockBaseline - iNumeralTall + iColonInset;
	surface()->DrawFilledRect(iColonX0, iTop, iColonX1, iTop + m_iClockColonWide);
	const int iBottom = m_iClockBaseline - iColonInset;
	surface()->DrawFilledRect(iColonX0, iBottom - m_iClockColonWide, iColonX1, iBottom);
}

void CNEOHud_RoundState::DrawPlayersAlive() const
{
	if (!m_bPlayersAliveAreCounts)
	{
		// Scores and deathmatch's lead text stay as one string
		surface()->DrawSetTextColor(COLOR_DASH_ROUND);
		NeoHud::DrawTextCentered(m_hRoundFont, m_iXpos, m_iAliveBaseline, m_wszPlayersAliveUnicode);
		return;
	}

	// The group is centered and its gaps measured by ink, so the side bearings of the digits and the v and s don't shift it
	constexpr const wchar_t *WSZ_VS = L"vs";
	const auto InkWide = [](const NeoHud::InkRange &ink) { return ink.iRight - ink.iLeft; };
	const int iVsWide = InkWide(NeoHud::GetTextInk(m_hAliveVsFont, WSZ_VS));
	const int iLeftWide = InkWide(NeoHud::GetNumberInk(m_hAliveFont, m_wszLeftPlayersAlive));
	const int iRightWide = InkWide(NeoHud::GetNumberInk(m_hAliveFont, m_wszRightPlayersAlive));

	const int x = m_iXpos - ((iLeftWide + iVsWide + iRightWide + (m_iAliveVsGap * 2)) / 2);
	surface()->DrawSetTextColor(COLOR_DASH_ALIVE);
	const NeoHud::InkRange inkLeft = NeoHud::DrawNumberLeft(m_hAliveFont, x, m_iAliveBaseline, m_wszLeftPlayersAlive);
	surface()->DrawSetTextColor(COLOR_DASH_VS);
	const NeoHud::InkRange inkVs = NeoHud::DrawTextLeft(m_hAliveVsFont, inkLeft.iRight + m_iAliveVsGap, m_iAliveBaseline, WSZ_VS);
	surface()->DrawSetTextColor(COLOR_DASH_ALIVE);
	NeoHud::DrawNumberLeft(m_hAliveFont, inkVs.iRight + m_iAliveVsGap, m_iAliveBaseline, m_wszRightPlayersAlive);
}

void CNEOHud_RoundState::DrawActiveStar() const
{
	if (m_iStarToDraw < 0)
	{
		return;
	}

	const auto Scaled = [this](const int iReferencePx) -> int { return static_cast<int>(iReferencePx * m_flStarScale); };
	const int iIconX = Scaled(STAR_ICON_X);
	const int iIconSize = Scaled(STAR_ICON_SIZE);
	const int iNameY = Scaled(STAR_NAME_Y);

	surface()->DrawSetTexture(m_iStarTextures[m_iStarToDraw]);
	surface()->DrawSetColor(m_colorStar);
	surface()->DrawTexturedSubRect(iIconX, Y_POS, iIconX + iIconSize, Y_POS + iIconSize,
		0.0f, 0.0f, STAR_ICON_TEXS1, 1.0f);
	surface()->DrawTexturedSubRect(iIconX + iIconSize, Y_POS + iNameY, iIconX + iIconSize + Scaled(STAR_NAME_WIDE), Y_POS + iNameY + Scaled(STAR_NAME_TALL),
		STAR_ICON_TEXS1, STAR_NAME_TEXT0, 1.0f, STAR_NAME_TEXT1);
}

const char *CNEOHud_RoundState::GetSquadRowName(const int playerIndex) const
{
	C_NEO_Player *pNeoPlayer = ToNEOPlayer(UTIL_PlayerByIndex(playerIndex));
	if (g_PR->IsAlive(playerIndex))
	{
		return pNeoPlayer ? pNeoPlayer->GetPlayerNameWithTakeoverContext(playerIndex) : g_PR->GetPlayerName(playerIndex);
	}
	C_NEO_Player *pImpersonator = pNeoPlayer ? pNeoPlayer->GetSpectatorTakeoverPlayerImpersonatingMe() : nullptr;
	return pImpersonator ? pImpersonator->GetPlayerName() : g_PR->GetPlayerName(playerIndex);
}

void CNEOHud_RoundState::DrawPlayerList()
{
	if (!NEORules()->IsTeamplay())
	{
		return;
	}

	if (sv_neo_bot_cmdr_enable.GetBool())
	{
		DrawPlayerList_BotCmdr();
		return;
	}

	if (g_PR)
	{
		// Draw members of players squad in an old style list
		const int localPlayerTeam = GetLocalPlayerTeam();
		const int localPlayerIndex = GetLocalPlayerIndex();
		const bool localPlayerSpec = !(localPlayerTeam == TEAM_JINRAI || localPlayerTeam == TEAM_NSF);

		int offset = SQUAD_LIST_Y;
		if (cl_neo_squad_hud_star_scale.GetFloat() > 0)
		{
			IntDim res = {};
			surface()->GetScreenSize(res.w, res.h);
			offset *= cl_neo_squad_hud_star_scale.GetFloat() * res.h / 1080.0f;
		}

		const bool hideDueToScoreboard = cl_neo_hud_scoreboard_hide_others.GetBool() && g_pNeoScoreBoard->IsVisible();

		// Draw squad mates
		if (!localPlayerSpec && g_PR->GetStar(localPlayerIndex) != STAR_NONE && !hideDueToScoreboard)
		{
			bool squadMateFound = false;

			for (int i = 1; i <= gpGlobals->maxClients; i++)
			{
				if (i == localPlayerIndex)
				{
					continue;
				}
				if (!g_PR->IsConnected(i))
				{
					continue;
				}
				const int playerTeam = g_PR->GetTeam(i);
				if (playerTeam != localPlayerTeam)
				{
					continue;
				}
				const bool isSameSquad = g_PR->GetStar(i) == g_PR->GetStar(localPlayerIndex);
				if (!isSameSquad)
				{
					continue;
				}

				offset = DrawPlayerRow(i, offset);
				squadMateFound = true;
			}

			if (squadMateFound)
			{
				offset += SQUAD_LIST_GROUP_GAP;
			}
		}

		// Draw other team mates
		for (int i = 1; i <= gpGlobals->maxClients; i++)
		{
			if (!g_PR->IsConnected(i))
			{
				continue;
			}
			const int playerTeam = g_PR->GetTeam(i);
			if (playerTeam != localPlayerTeam)
			{
				continue;
			}
			if (i == localPlayerIndex || localPlayerSpec || hideDueToScoreboard)
			{
				continue;
			}
			const bool isSameSquad = g_PR->GetStar(i) == g_PR->GetStar(localPlayerIndex);
			if (isSameSquad)
			{
				continue;
			}

			offset = DrawPlayerRow(i, offset, true);
		}
	}
}

void CNEOHud_RoundState::DrawPlayerList_BotCmdr()
{
	if (!g_PR)
	{
		return;
	}

	// Draw members of players squad in an old style list
	const int localPlayerTeam = GetLocalPlayerTeam();
	const int localPlayerIndex = GetLocalPlayerIndex();
	const bool localPlayerSpec = !(localPlayerTeam == TEAM_JINRAI || localPlayerTeam == TEAM_NSF);

	int offset = SQUAD_LIST_Y;
	if (cl_neo_squad_hud_star_scale.GetFloat() > 0)
	{
		IntDim res = {};
		surface()->GetScreenSize(res.w, res.h);
		offset *= cl_neo_squad_hud_star_scale.GetFloat() * res.h / 1080.0f;
	}

	const bool hideDueToScoreboard = cl_neo_hud_scoreboard_hide_others.GetBool() && g_pNeoScoreBoard->IsVisible();

	// Draw squad mates
	m_commandedList.RemoveAll();
	m_nonCommandedList.RemoveAll();
	m_nonSquadList.RemoveAll();

	bool squadMateFound = false;
	const int localStar = g_PR->GetStar(localPlayerIndex);

	// Single pass to collect and categorize players
	for (int i = 1; i <= gpGlobals->maxClients; i++)
	{
		if (!g_PR->IsConnected(i))
		{
			continue;
		}
		const int playerTeam = g_PR->GetTeam(i);
		if (playerTeam != localPlayerTeam)
		{
			continue;
		}
		if (i == localPlayerIndex || localPlayerSpec || hideDueToScoreboard)
		{
			continue;
		}

		// Only consider players in the same squad star as the local player
		const bool isSameSquadStar = (localStar != STAR_NONE) && (g_PR->GetStar(i) == localStar);
		if (isSameSquadStar)
		{
			C_NEO_Player* pPlayer = ToNEOPlayer(UTIL_PlayerByIndex(i));
			bool isCommanded = (pPlayer && pPlayer->m_hCommandingPlayer.Get() == C_NEO_Player::GetLocalNEOPlayer());
			if (isCommanded)
			{
				m_commandedList.AddToTail(i);
			}
			else
			{
				m_nonCommandedList.AddToTail(i);
			}
			squadMateFound = true;
		}
		else
		{
			m_nonSquadList.AddToTail(i);
		}
	}

	// Draw commanded players first
	const auto player = C_NEO_Player::GetLocalNEOPlayer();
	TeamLogoColor* pTeamLogoColor = player ? &m_teamLogoColors[player->GetTeamNumber()] : nullptr;
	const Color* colorOverride = pTeamLogoColor ? &pTeamLogoColor->color : nullptr;
	for (int i = 0; i < m_commandedList.Count(); ++i)
	{
		offset = DrawPlayerRow(m_commandedList[i], offset, false, colorOverride);
	}

	// Draw non-commanded players (who are also in the same squad star)
	for (int i = 0; i < m_nonCommandedList.Count(); ++i)
	{
		offset = DrawPlayerRow(m_nonCommandedList[i], offset, false);
	}

	if (squadMateFound)
	{
		offset += SQUAD_LIST_GROUP_GAP;
	}

	// Draw other team mates
	for (int i = 0; i < m_nonSquadList.Count(); ++i)
	{
		offset = DrawPlayerRow(m_nonSquadList[i], offset, true);
	}
}

int CNEOHud_RoundState::DrawPlayerRow(int playerIndex, const int yOffset, bool small, const Color* colorOverride)
{
	const bool isAlive = g_PR->IsAlive(playerIndex);

	C_NEO_Player* pNeoPlayer = ToNEOPlayer(UTIL_PlayerByIndex(playerIndex));
	const char* pPlayerDisplayName = GetSquadRowName(playerIndex);
	const wchar_t* pwszRank = GetRankNameW(g_PR->GetXP(playerIndex), true);
	const wchar_t* pwszClass = GetSquadClassNameW(g_PR->GetClass(playerIndex));
	char playerHealth[7] = "DEAD"; // 4 digits + 2 letters
	if (isAlive)
	{
		const int healthMode = cl_neo_hud_health_mode.GetInt();
		V_snprintf(playerHealth, sizeof(playerHealth), healthMode ? "%dhp" : "%d%%", g_PR->GetDisplayedHealth(playerIndex, healthMode));
	}
	else
	{
		auto* pImpersonator = pNeoPlayer ? pNeoPlayer->m_hSpectatorTakeoverPlayerImpersonatingMe.Get() : nullptr;
		if (pImpersonator)
		{
			pwszClass = GetSquadClassNameW(pImpersonator->m_iClassBeforeTakeover);
			pwszRank = GetRankNameW(g_PR->GetXP(pImpersonator->entindex()), true);
		}
	}

	wchar_t wszName[MAX_PLAYER_NAME_LENGTH + 1];
	wchar_t wszHealth[ARRAYSIZE(playerHealth)];
	g_pVGuiLocalize->ConvertANSIToUnicode(pPlayerDisplayName, wszName, sizeof(wszName));
	g_pVGuiLocalize->ConvertANSIToUnicode(playerHealth, wszHealth, sizeof(wszHealth));

	// A teammate outside the squad has the smaller font, as on master
	const vgui::HFont hRowFont = small ? m_hOCRSmallerFont : m_hOCRSmallFont;
	const int iAliveAlpha = small ? SQUAD_ROW_SMALL_ALPHA : SQUAD_ROW_ALPHA;
	const int iAlpha = isAlive ? iAliveAlpha : static_cast<int>(iAliveAlpha * SQUAD_ROW_DEAD_ALPHA_SCALE);
	const Color colorRow = Color(255, 255, 255, iAlpha);
	const Color colorDetail = Color(255, 255, 255, iAlpha * SQUAD_DETAIL_ALPHA_SCALE);

	// The health, class and rank have fixed columns, so the name comes last and shows in full
	const int iHealthWide = (cl_neo_hud_health_mode.GetInt() != 0) ? m_iSquadHealthHpWide : m_iSquadHealthPercentWide;
	const int iHealthRight = m_iSquadRowX + iHealthWide;
	const int iClassX = iHealthRight + m_iSquadColumnGap;
	const int iRankX = iClassX + m_iSquadClassWide + m_iSquadColumnGap;
	const int iNameX = iRankX + m_iSquadRankWide + m_iSquadColumnGap;
	const int yBaseline = yOffset + surface()->GetFontAscent(hRowFont, L'0');

	// The health ends on the column's right edge by its ink
	surface()->DrawSetTextColor(colorOverride ? *colorOverride : colorRow);
	NeoHud::DrawTextRight(hRowFont, iHealthRight, yBaseline, wszHealth);

	surface()->DrawSetTextColor(colorDetail);
	NeoHud::DrawTextLeft(hRowFont, iClassX, yBaseline, pwszClass);
	NeoHud::DrawTextLeft(hRowFont, iRankX, yBaseline, pwszRank);

	surface()->DrawSetTextColor(colorOverride ? *colorOverride : colorRow);
	NeoHud::DrawTextLeft(hRowFont, iNameX, yBaseline, wszName);

	return yOffset + surface()->GetFontTall(hRowFont) + SQUAD_ROW_GAP;
}

void CNEOHud_RoundState::DrawPlayer(int playerIndex, int teamIndex, const TeamLogoColor &teamLogoColor,
									const int xOffset, const bool drawHealthClass, const bool isSelected)
{
	C_NEO_Player* pLocalNeoPlayer = C_NEO_Player::GetLocalNEOPlayer();
	if (!pLocalNeoPlayer)
		return;

	// Draw Name
	{
		wchar_t text[1 + MAX_PLAYER_NAME_LENGTH + 1] = {};
		g_pVGuiLocalize->ConvertANSIToUnicode( g_PR->GetPlayerName(playerIndex), &text[1], sizeof(text) - (2 * sizeof(text[0])));
		// A name can use the gaps beside its avatar, as long as it stays clear of the next one's name
		const int iNameWideMax = m_ilogoSize + m_iAvatarGap - 2;
		float textLengthPixels = 0.f;
		int textLength = 1;
		for (textLength; textLength < sizeof(text)/sizeof(text[0]) - 1; textLength++)
		{
			if (text[textLength] == '\0')
				break;

			float wide, abcA;
			surface()->GetKernedCharWidth(m_hTinyText, text[textLength], text[textLength - 1], text[textLength + 1], wide, abcA);
			if (textLengthPixels + wide > iNameWideMax)
				break;

			textLengthPixels += wide;
		}

		surface()->DrawSetTextFont(m_hTinyText);
		surface()->DrawSetTextColor(g_PR->IsAlive(playerIndex) ? COLOR_AVATAR_NAME : COLOR_AVATAR_NAME_DEAD);
		surface()->DrawSetTextPos(xOffset + ((m_ilogoSize - textLengthPixels) * 0.5), NeoHud::GetTextY(m_hTinyText, m_iAvatarNameBaseline));
		NeoHud::DrawPrintText( &text[1], textLength - 1);
	}

	// Draw Avatar
	{
		// There are 4 rows, the first has generic colour icons, second has jinrai colour icons, third has nsf colour icons and last has dead colour icons with a skull on top
		const float TEXTURE_HEIGHT = 1 / 4.f;
		// There are 8 columns. The first column contains the icon for team jinrai, the second for team nsf (for the team colour rows instead two versions of that teams icon are in column 0 and 1)
		// column 2 onwards contains class specific icons, the last column is unused since vtfs need to have power of 2 dimensions
		const float TEXTURE_WIDTH = 1 / 8.f;
		float textureYOffset = 3.f * TEXTURE_HEIGHT;
		if (g_PR->IsAlive(playerIndex))
		{
			if (pLocalNeoPlayer->GetTeamNumber() == g_PR->GetTeam(playerIndex) && pLocalNeoPlayer->GetStar() == g_PR->GetStar(playerIndex) && g_PR->GetStar(playerIndex) != STAR_NONE)
			{
				textureYOffset = (g_PR->GetTeam(playerIndex) == TEAM_JINRAI ? 1 : 2) * TEXTURE_HEIGHT;
			}
			else
			{
				textureYOffset = 0.f;
			}
		}

		float textureXOffset = g_PR->GetTeam(playerIndex) == TEAM_JINRAI ? 0 : TEXTURE_WIDTH;
		if (drawHealthClass)
		{
			textureXOffset = (2 + g_PR->GetClass(playerIndex)) * TEXTURE_WIDTH;
		}

		surface()->DrawSetTexture(m_iClassIcons);
		surface()->DrawSetColor(COLOR_WHITE);
		surface()->DrawTexturedSubRect(xOffset, Y_POS + m_iAvatarTop, xOffset + m_ilogoSize, Y_POS + m_iAvatarTop + m_ilogoSize,
										textureXOffset, textureYOffset, textureXOffset + 1.f/8.f, textureYOffset + 1.f/4.f);
	}

	const int TEXTURE_BORDER_WIDTH = floor((4.f / 128.f) * m_ilogoSize);
	int highlightPlayerIndex = pLocalNeoPlayer->entindex();
	// Draw Avatar Highlight
	{
		if (pLocalNeoPlayer->IsObserver())
		{
			int observerMode = pLocalNeoPlayer->GetObserverMode();
			if (observerMode != OBS_MODE_DEATHCAM)
			{
				highlightPlayerIndex = -1;
			}
			if (observerMode == OBS_MODE_IN_EYE || observerMode == OBS_MODE_CHASE)
			{
				CBaseEntity* pObserverTarget = pLocalNeoPlayer->GetObserverTarget();
				if (pObserverTarget)
				{
					highlightPlayerIndex = pObserverTarget->entindex();
				}
			}

			if (isSelected)
			{
				surface()->DrawSetColor(COLOR_NEO_ORANGE);
				const int alpha = (g_PR->IsAlive(playerIndex) ? 200 : 50) * Max(0.f, 1.f - (gpGlobals->curtime - m_flSelectedPlayerChangeTime));
				surface()->DrawFilledRectFade(xOffset + TEXTURE_BORDER_WIDTH, Y_POS + m_iAvatarTop + TEXTURE_BORDER_WIDTH, xOffset + m_ilogoSize - TEXTURE_BORDER_WIDTH, Y_POS + m_iAvatarTop + m_ilogoSize - TEXTURE_BORDER_WIDTH, 0, alpha, false);
			}
		}
		if (highlightPlayerIndex == playerIndex)
		{
			int alpha = 200;
			if (!g_PR->IsAlive(playerIndex))
			{
				alpha = 50;
				surface()->DrawSetColor(COLOR_RED);
			}
			surface()->DrawSetColor(COLOR_WHITE);
			surface()->DrawFilledRectFade(xOffset + TEXTURE_BORDER_WIDTH, Y_POS + m_iAvatarTop + TEXTURE_BORDER_WIDTH, xOffset + m_ilogoSize - TEXTURE_BORDER_WIDTH, Y_POS + m_iAvatarTop + m_ilogoSize - TEXTURE_BORDER_WIDTH, g_PR->IsAlive(playerIndex) ? 200 : 50, 0, false);
		}
	}

	// Draw commander selected highlight
	if (sv_neo_bot_cmdr_enable.GetBool())
	{
		C_NEO_Player* pPlayer = static_cast<C_NEO_Player*>(UTIL_PlayerByIndex(playerIndex));
		if (pPlayer)
		{
			C_NEO_Player* pCommandingPlayer = static_cast<C_NEO_Player*>(pPlayer->m_hCommandingPlayer.Get());
			if (pCommandingPlayer && pCommandingPlayer->entindex() == highlightPlayerIndex)
			{
				surface()->DrawSetColor(COLOR_YELLOW);
				surface()->DrawFilledRectFade(xOffset + TEXTURE_BORDER_WIDTH, Y_POS + m_iAvatarTop + TEXTURE_BORDER_WIDTH, xOffset + m_ilogoSize - TEXTURE_BORDER_WIDTH, Y_POS + m_iAvatarTop + m_ilogoSize - TEXTURE_BORDER_WIDTH, 200, 0, false);
			}
		}
	}

	// Draw Health
	if (drawHealthClass && g_PR->IsAlive(playerIndex))
	{
		const int health = g_PR->GetDisplayedHealth(playerIndex, 0);
		if (health_monochrome) {
			const int greenBlueValue = (health / 100.0f) * 255;
			surface()->DrawSetColor(Color(255, greenBlueValue, greenBlueValue, 255));
		}
		else {
			if (health <= 20)
				surface()->DrawSetColor(COLOR_RED);
			else if (health <= 80)
				surface()->DrawSetColor(COLOR_YELLOW);
			else
				surface()->DrawSetColor(COLOR_WHITE);
		}
		const int iHealthY = Y_POS + m_iAvatarTop + m_ilogoSize + m_iAvatarHealthGap;
		surface()->DrawFilledRect(xOffset, iHealthY, xOffset + ceil(health / 100.0f * m_ilogoSize), iHealthY + m_iAvatarHealthTall);
	}

	// Deathmatch only: Draw XP on everyone
	if (!NEORules()->IsTeamplay())
	{
		wchar_t wszXP[9];
		const int iWszLen = V_swprintf_safe(wszXP, L"%d", g_PR->GetXP(playerIndex));
		int fontWidth, fontHeight;
		surface()->DrawSetTextFont(m_hOCRSmallFont);
		surface()->GetTextSize(m_hOCRSmallFont, wszXP, fontWidth, fontHeight);
		const int iYExtra = drawHealthClass ? m_ilogoSize : fontHeight;
		surface()->DrawSetTextPos(xOffset + ((m_ilogoSize / 2) - (fontWidth / 2)),
								  Y_POS + m_iAvatarTop + m_ilogoSize - (fontHeight / 2) + iYExtra);
		surface()->DrawSetTextColor(COLOR_WHITE);
		NeoHud::DrawPrintText(wszXP, iWszLen);
	}
}

void CNEOHud_RoundState::CheckActiveStar()
{
	auto player = C_NEO_Player::GetLocalNEOPlayer();
	Assert(player);

	int currentStar;
	if (!ShouldDraw())
	{
		currentStar = STAR_NONE;
	}
	else
	{
		currentStar = player->GetStar();
	}
	const int currentTeam = player->GetTeamNumber();

	if (m_iPreviouslyActiveStar == currentStar && m_iPreviouslyActiveTeam == currentTeam)
	{
		return;
	}

	m_iPreviouslyActiveStar = currentStar;
	m_iPreviouslyActiveTeam = currentTeam;

	m_iStarToDraw = -1;

	if (currentTeam != TEAM_JINRAI && currentTeam != TEAM_NSF)
	{
		return;
	}

	// NEO NOTE (nullsystem): Unlikely, but sanity checking
	Assert(currentStar >= 0 && currentStar < STAR__TOTAL);
	if (currentStar < 0 || currentStar >= STAR__TOTAL)
	{
		currentStar = STAR_NONE;
	}

	m_iStarToDraw = currentStar;
	m_colorStar = currentStar == STAR_NONE ? COLOR_NEO_WHITE : currentTeam == TEAM_NSF ? COLOR_NSF : COLOR_JINRAI;
}

void CNEOHud_RoundState::Paint()
{
	BaseClass::Paint();
	PaintNeoElement();
}

int CNEOHud_RoundState::GetEntityIndexAtPositionInHud(int position, bool positionIsZeroIndexed)
{
	if (positionIsZeroIndexed)
	{
		position -= m_iLeftPlayersTotal;
		if (position >= 0)
		{
			position++;
		}
	}
	const int listPosition = position > 0 ? m_iLeftPlayersTotal + position - 1 : -position - 1;
	if (listPosition < 0 || listPosition > MAX_PLAYERS)
	{
		return -1;
	}
	return m_nPlayerList[listPosition].playerIndex;
}

int CNEOHud_RoundState::GetMinusIndexedPositionOfPlayerInHud(int entindex)
{
	bool found = false;
	int i = 0;
	for (i; i < gpGlobals->maxClients; i++)
	{
		if (m_nPlayerList[i].playerIndex == entindex)
		{
			found = true;
			break;
		}
	}
	if (found)
	{
		if (i < m_iLeftPlayersTotal)
		{
			i = (- i) - 1;
		}
		else
		{
			i -= m_iLeftPlayersTotal;
			i++;
		}
		return i;
	}
	return 0;
}

int CNEOHud_RoundState::GetNextAlivePlayerInHud(int minusIndexedPosition, bool reverse)
{
	bool found = false;
	int i = 0;
	int current = minusIndexedPosition == 0 ? m_iSelectedPlayer : minusIndexedPosition;
	while (!found && i < m_iLeftPlayersTotal + m_iRightPlayersTotal)
	{
		i++;
		current+= reverse ? -1 : 1;
		if (current == 0)
		{
			current = reverse ? -1 : 1;
		}
		else if (!reverse && current > m_iRightPlayersTotal)
		{
			current = -m_iLeftPlayersTotal;
		}
		else if (reverse && current < -m_iLeftPlayersTotal)
		{
			current = m_iRightPlayersTotal;
		}

		const int entityIndex = GetEntityIndexAtPositionInHud(current);
		if (entityIndex > 0 && entityIndex < MAX_PLAYERS && g_PR->IsAlive(entityIndex))
		{
			found = true;
		}
	}
	return found ? current : 0;
}

void CNEOHud_RoundState::SelectNextAlivePlayerInHud()
{
	if (!g_PR)
		return;

	int index = GetNextAlivePlayerInHud(m_iSelectedPlayer, false);
	if (index != 0)
	{
		m_flSelectedPlayerChangeTime = gpGlobals->curtime;
		m_iSelectedPlayer = index;
	}
}

void CNEOHud_RoundState::SelectPreviousAlivePlayerInHud()
{
	if (!g_PR)
		return;
	
	int index = GetNextAlivePlayerInHud(m_iSelectedPlayer, true);
	if (index != 0)
	{
		m_flSelectedPlayerChangeTime = gpGlobals->curtime;
		m_iSelectedPlayer = index;
	}
}

int CNEOHud_RoundState::GetSelectedPlayerInHud()
{
	const int entityIndex = GetEntityIndexAtPositionInHud(m_iSelectedPlayer);
	if (entityIndex > 0 && entityIndex < MAX_PLAYERS)
	{
		return entityIndex;
	}
	return -1;
}

