#include "cbase.h"

#include <vgui_controls/ImagePanel.h>
#include <vgui_controls/ImageList.h>
#include "neo_hud_round_state.h"

#include "iclientmode.h"
#include "ienginevgui.h"
#include "engine/IEngineSound.h"

#include <vgui/ILocalize.h>
#include <vgui/ISurface.h>

#include <initializer_list>

#include <vgui_controls/ImagePanel.h>

#include "c_neo_player.h"
#include "c_team.h"
#include "c_playerresource.h"
#include "vgui_avatarimage.h"
#include "neoui_scoreboard.h"

#include "hltvcamera.h"

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

using vgui::surface;

DECLARE_NAMED_HUDELEMENT(CNEOHud_RoundState, NRoundState);

NEO_HUD_ELEMENT_DECLARE_FREQ_CVAR(RoundState, 0.1)

ConVar cl_neo_hud_team_swap_sides("cl_neo_hud_team_swap_sides", "1", FCVAR_ARCHIVE, "Make the team of the local player always appear on the left side of the round info and scoreboard", true, 0.0, true, 1.0,
	[]([[maybe_unused]] IConVar* var, [[maybe_unused]] const char* pOldValue, [[maybe_unused]] float flOldValue) {
		// TODO REMOVE? g_pNeoUIScoreBoard->UpdateTeamColumnsPosition(GetLocalPlayerTeam());
	});
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

namespace {
constexpr int Y_POS = 0;
constexpr bool STARS_HW_FILTERED = false;
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
	const bool starAutoDelete = true;
	COMPILE_TIME_ASSERT(starAutoDelete); // If not, we need to handle that memory on dtor manually

	for (int i = 0; i < STAR__TOTAL; ++i)
	{
		static constexpr const char *IP_STAR_NAMES[STAR__TOTAL] = {
			"alpha", "bravo", "charlie", "delta", "echo", "foxtrot", "none"
		};
		const char *name = IP_STAR_NAMES[i];
		char ipName[32];
		char hudName[32];
		V_snprintf(ipName, sizeof(ipName), "star_%s", name);
		V_snprintf(hudName, sizeof(hudName), "hud/star_%s", name);

		m_ipStars[i] = new vgui::ImagePanel(this, ipName);
		auto *star = m_ipStars[i];
		star->SetImage(vgui::scheme()->GetImage(hudName, STARS_HW_FILTERED));
		star->SetDrawColor(COLOR_NSF); // This will get updated in the draw check as required
		star->SetAlpha(1.0f);
		star->SetShouldScaleImage(true);
		star->SetAutoDelete(starAutoDelete);
		star->SetVisible(false);
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
	for (int i = 0; i < STAR__TOTAL; ++i)
	{
		auto* star = m_ipStars[i];
		star->SetVisible(false);
	}

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
}

void CNEOHud_RoundState::UpdateStarSize()
{
	IntDim res = {};
	surface()->GetScreenSize(res.w, res.h);
	const float scale = cl_neo_squad_hud_star_scale.GetFloat() != 0 ? cl_neo_squad_hud_star_scale.GetFloat() * (res.h / 1080.0f)
																	: 1.f;

	for (auto* star : m_ipStars)
	{
		star->SetWide(192 * scale);
		star->SetTall(48 * scale);
	}
}

void CNEOHud_RoundState::ApplySchemeSettings(vgui::IScheme* pScheme)
{
	BaseClass::ApplySchemeSettings(pScheme);

	m_hOCRLargeFont = pScheme->GetFont("NHudOCRLarge");
	m_hOCRFont = pScheme->GetFont("NHudOCR");
	m_hOCRSmallFont = pScheme->GetFont("NHudOCRSmall");
	m_hOCRSmallerFont = pScheme->GetFont("NHudOCRSmaller");
	m_hTinyText = pScheme->GetFont("NHudTinyText");
	m_iTinyTextHeight = surface()->GetFontTall(m_hTinyText);

	SetFgColor(COLOR_TRANSPARENT);
	SetBgColor(COLOR_TRANSPARENT);

	// Screen dimensions
	IntDim res = {};
	surface()->GetScreenSize(res.w, res.h);
	m_iXpos = (res.w / 2);

	// Box dimensions
	[[maybe_unused]] int iSmallFontWidth = 0;
	int iFontHeight = 0;
	int iFontWidth = 0;
	surface()->GetTextSize(m_hOCRSmallFont, L"ROUND 99", iSmallFontWidth, m_iSmallFontHeight);
	surface()->GetTextSize(m_hOCRFont, L"ROUND 99", iFontWidth, iFontHeight);
	m_iSmallFontHeight *= 0.85;
	iFontHeight *= 0.85;

	const int iBoxHeight = iFontHeight + m_iSmallFontHeight * 2 + 1;
	const int iBoxHeightHalf = (iBoxHeight / 2);
	const int iBoxWidth = iFontWidth + iBoxHeight * 2;
	const int iBoxWidthHalf = (iBoxWidth / 2);
	m_iLeftOffset = m_iXpos - iBoxWidthHalf;
	m_iRightOffset = m_iXpos + iBoxWidthHalf;
	m_iBoxYEnd = Y_POS + iBoxHeight;

	UpdateAvatarSize();
	UpdateStarSize();

	m_rectLeftTeamTotalLogo = vgui::IntRect{
		.x0 = m_iLeftOffset,
		.y0 = Y_POS,
		.x1 = m_iLeftOffset + iBoxHeight,
		.y1 = Y_POS + iBoxHeight,
	};
	m_rectRightTeamTotalLogo = vgui::IntRect{
		.x0 = m_iRightOffset - iBoxHeight,
		.y0 = Y_POS,
		.x1 = m_iRightOffset,
		.y1 = Y_POS + iBoxHeight,
	};

	surface()->GetTextSize(m_hOCRLargeFont, L"ROUND 99", iFontWidth, iFontHeight);
	m_posLeftTeamScore = IntPos{
		.x = m_rectLeftTeamTotalLogo.x0 + iBoxHeightHalf,
		.y = static_cast<int>(Y_POS + iBoxHeightHalf - (iFontHeight / 2)),
	};
	m_posRightTeamScore = IntPos{
		.x = m_rectRightTeamTotalLogo.x0 + iBoxHeightHalf,
		.y = static_cast<int>(Y_POS + iBoxHeightHalf - (iFontHeight / 2)),
	};

	// Clear player avatars

	SetBounds(0, Y_POS, res.w, res.h);
	SetZPos(90);
}

extern ConVar sv_neo_readyup_lobby;

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
		m_pWszStatusUnicode = sv_neo_readyup_lobby.GetBool() ? L"Waiting for players to ready up" : L"Waiting for players";
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
	else if (NEORules()->GetRoundStatus() != NeoRoundStatus::Pause && (NEORules()->IsRoundPreRoundFreeze() || g_pNeoUIScoreBoard->IsVisible()))
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
		m_iWszRoundUCSize = V_swprintf_safe(m_wszRoundUnicode, L"PAUSED");
		roundTimeLeft = NEORules()->m_flPauseEnd.Get() - gpGlobals->curtime;
	}
	else if (NEORules()->GetRoundStatus() == NeoRoundStatus::Countdown)
	{
		m_iWszRoundUCSize = V_swprintf_safe(m_wszRoundUnicode, L"STARTING");
	}
	else if (NEORules()->GetRoundStatus() == NeoRoundStatus::Overtime)
	{
		m_iWszRoundUCSize = V_swprintf_safe(m_wszRoundUnicode, L"OVERTIME");
	}
	else
	{
		if (NEORules()->GetGameType() == NEO_GAME_TYPE_DM)
		{
			m_iWszRoundUCSize = V_swprintf_safe(m_wszRoundUnicode, L"DEATHMATCH");
		}
		else
		{
			m_iWszRoundUCSize = V_swprintf_safe(m_wszRoundUnicode, L"ROUND %i", NEORules()->roundNumber());
		}
	}

	if (roundStatus == NeoRoundStatus::PreRoundFreeze)
		roundTimeLeft = NEORules()->GetRemainingPreRoundFreezeTime(true);

	int secsTotal = 0.0f;
	if (roundStatus == NeoRoundStatus::Overtime && NEORules()->GetGameType() == NEO_GAME_TYPE_CTG)
	{
		secsTotal = RoundFloatToInt(NEORules()->GetCTGOverTime());
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
		V_sprintf_safe(szPlayersAliveANSI, "%i vs %i", m_iLeftPlayersAlive, m_iRightPlayersAlive);
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

	surface()->DrawSetTextFont(m_hOCRFont);
	int fontWidth, fontHeight;
	surface()->GetTextSize(m_hOCRFont, m_wszTime, fontWidth, fontHeight);

	// Draw Box
	DrawNeoHudRoundedBox(m_iLeftOffset, Y_POS, m_iRightOffset, m_iBoxYEnd, box_color, top_left_corner, top_right_corner, bottom_left_corner, bottom_right_corner);

	// Draw round
	surface()->DrawSetTextColor((NEORules()->GetRoundStatus() == NeoRoundStatus::Pause) ? COLOR_RED : COLOR_FADED_WHITE);
	surface()->DrawSetTextFont(m_hOCRSmallerFont);
	surface()->GetTextSize(m_hOCRSmallerFont, m_wszRoundUnicode, fontWidth, fontHeight);
	surface()->DrawSetTextPos(m_iXpos - (fontWidth / 2), 0);
	surface()->DrawPrintText(m_wszRoundUnicode, m_iWszRoundUCSize);

	// Draw round status
	surface()->DrawSetTextColor(COLOR_WHITE);
	surface()->DrawSetTextFont(m_hOCRSmallFont);
	surface()->GetTextSize(m_hOCRSmallFont, m_pWszStatusUnicode, fontWidth, fontHeight);
	surface()->DrawSetTextPos(m_iXpos - (fontWidth / 2), m_iBoxYEnd);
	surface()->DrawPrintText(m_pWszStatusUnicode, m_iStatusUnicodeSize);

	const int localPlayerTeam = GetLocalPlayerTeam();
	const bool localPlayerSpecOrNoTeam = !NEORules()->IsTeamplay() || !(localPlayerTeam == TEAM_JINRAI || localPlayerTeam == TEAM_NSF);

	bool swapTeamSides = cl_neo_hud_team_swap_sides.GetBool();
	const int leftTeam = swapTeamSides ? (localPlayerSpecOrNoTeam ? TEAM_JINRAI : localPlayerTeam) : TEAM_JINRAI;
	const int rightTeam = swapTeamSides ? ((leftTeam == TEAM_JINRAI) ? TEAM_NSF : TEAM_JINRAI) : TEAM_NSF;
	const auto leftTeamInfo = m_teamLogoColors[leftTeam];
	const auto rightTeamInfo = m_teamLogoColors[rightTeam];

	// Draw total players alive (or score)
	surface()->DrawSetTextFont(m_hOCRSmallerFont);
	surface()->GetTextSize(m_hOCRSmallerFont, m_wszPlayersAliveUnicode, fontWidth, fontHeight);
	surface()->DrawSetTextColor(COLOR_FADED_WHITE);
	surface()->DrawSetTextPos(m_iXpos - (fontWidth / 2), m_iBoxYEnd - fontHeight);
	surface()->DrawPrintText(m_wszPlayersAliveUnicode, ARRAYSIZE(m_wszPlayersAliveUnicode) - 1);

	// Draw time
	surface()->DrawSetTextFont(m_hOCRFont);
	surface()->GetTextSize(m_hOCRFont, m_wszTime, fontWidth, fontHeight);
	if (NEORules()->GetRoundStatus() == NeoRoundStatus::PreRoundFreeze || NEORules()->GetRoundStatus() == NeoRoundStatus::Countdown || NEORules()->GetGameType() == NEO_GAME_TYPE_CTG ?
		(NEORules()->GetRoundStatus() == NeoRoundStatus::Overtime && (NEORules()->GetRoundRemainingTime() < sv_neo_ctg_ghost_overtime_grace.GetFloat())) : NEORules()->GetRoundStatus() == NeoRoundStatus::Overtime)
	{
		surface()->DrawSetTextColor(COLOR_RED);
	}
	else
	{
		surface()->DrawSetTextColor(COLOR_WHITE);
	}
	surface()->DrawSetTextPos(m_iXpos - (fontWidth / 2), m_iBoxYEnd / 2 - fontHeight / 2);
	surface()->DrawPrintText(m_wszTime, 6);

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
		surface()->GetTextSize(m_hOCRLargeFont, m_wszLeftTeamScore, fontWidth, fontHeight);
		surface()->DrawSetTextFont(m_hOCRLargeFont);
		surface()->DrawSetTextPos(m_posLeftTeamScore.x - (fontWidth / 2), m_posLeftTeamScore.y);
		surface()->DrawSetTextColor(leftTeamInfo.color);
		surface()->DrawPrintText(m_wszLeftTeamScore, 2);

		surface()->GetTextSize(m_hOCRLargeFont, m_wszRightTeamScore, fontWidth, fontHeight);
		surface()->DrawSetTextPos(m_posRightTeamScore.x - (fontWidth / 2), m_posRightTeamScore.y);
		surface()->DrawSetTextColor(rightTeamInfo.color);
		surface()->DrawPrintText(m_wszRightTeamScore, 2);
	}

	m_iLeftPlayersAlive = m_iLeftPlayersTotal = m_iRightPlayersAlive = m_iRightPlayersTotal = 0;

	if (!g_PR)
		return;

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
				if (g_PR->IsAlive(m_nPlayerList[i].playerIndex))
					m_iLeftPlayersAlive++;
				m_iLeftPlayersTotal++;
			}
			else if (playerTeam == TEAM__TOTAL - 1 - (rightTeam - FIRST_GAME_TEAM))
			{
				if (g_PR->IsAlive(m_nPlayerList[i].playerIndex))
					m_iRightPlayersAlive++;
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
	int leftCount = 0;
	int rightCount = 0;
	if (NEORules()->IsTeamplay())
	{
		// Fade background to make names easier to see
		surface()->DrawSetColor(COLOR_DARK);
		surface()->DrawFilledRectFade((m_iLeftOffset - 2) - (m_iLeftPlayersTotal * m_ilogoSize) - (m_iLeftPlayersTotal * 2), 0,
										m_iLeftOffset, Y_POS + m_ilogoSize + 6, 255, 0, false);
		surface()->DrawFilledRectFade(m_iRightOffset, 0,
			(m_iRightOffset + 2) + (m_iRightPlayersTotal * m_ilogoSize) + (m_iRightPlayersTotal * 2), Y_POS + m_ilogoSize + 6, 255, 0, false);

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
				const int xOffset = (m_iLeftOffset - 2) - ((leftCount + 1) * m_ilogoSize) - (leftCount * 2);
				DrawPlayer(m_nPlayerList[i].playerIndex, leftCount, leftTeamInfo, xOffset, localPlayerTeam == playerTeam || localPlayerSpecOrNoTeam, -(leftCount+1) == selectedPlayerIndexInList);
				leftCount++;
			}
			else if (playerTeam == rightTeam)
			{
				const int xOffset = (m_iRightOffset + 2) + (rightCount * m_ilogoSize) + (rightCount * 2);
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
				const int xOffset = (m_iLeftOffset - 4) - ((leftCount + 1) * m_ilogoSize) - (leftCount * 2);
				DrawPlayer(iPlayerIdx, leftCount, lrTeamInfo, xOffset, bPlayerLocal);
				--leftCount;
			}
			else
			{
				const int xOffset = (m_iRightOffset + 4) + (rightCount * m_ilogoSize) + (rightCount * 2);
				DrawPlayer(iPlayerIdx, rightCount, lrTeamInfo, xOffset, bPlayerLocal);
				rightCount++;
			}
		}
	}
}

void CNEOHud_RoundState::DrawPlayerList()
{
	if (!NEORules()->IsTeamplay())
	{
		return;
	}

	ConVarRef cl_neo_bot_cmdr_enable_ref("sv_neo_bot_cmdr_enable");
	Assert(cl_neo_bot_cmdr_enable_ref.IsValid());
	if (cl_neo_bot_cmdr_enable_ref.IsValid() && cl_neo_bot_cmdr_enable_ref.GetBool())
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
		const int leftTeam = cl_neo_hud_team_swap_sides.GetBool() ? (localPlayerSpec ? TEAM_JINRAI : localPlayerTeam) : TEAM_JINRAI;

		int offset = 52;
		if (cl_neo_squad_hud_star_scale.GetFloat() > 0)
		{
			IntDim res = {};
			surface()->GetScreenSize(res.w, res.h);
			offset *= cl_neo_squad_hud_star_scale.GetFloat() * res.h / 1080.0f;
		}

		const bool hideDueToScoreboard = cl_neo_hud_scoreboard_hide_others.GetBool() && g_pNeoUIScoreBoard->IsVisible();

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
				offset += 12;
			}
		}

		m_iLeftPlayersAlive = 0;
		m_iRightPlayersAlive = 0;

		// Draw other team mates
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
					m_iRightPlayersAlive++;
				}
			}
			else {
				if (g_PR->IsAlive(i))
				{
					m_iLeftPlayersAlive++;
				}
			}
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
	const int leftTeam = cl_neo_hud_team_swap_sides.GetBool() ? (localPlayerSpec ? TEAM_JINRAI : localPlayerTeam) : TEAM_JINRAI;

	int offset = 52;
	if (cl_neo_squad_hud_star_scale.GetFloat() > 0)
	{
		IntDim res = {};
		surface()->GetScreenSize(res.w, res.h);
		offset *= cl_neo_squad_hud_star_scale.GetFloat() * res.h / 1080.0f;
	}

	const bool hideDueToScoreboard = cl_neo_hud_scoreboard_hide_others.GetBool() && g_pNeoUIScoreBoard->IsVisible();

	// Draw squad mates
	m_commandedList.RemoveAll();
	m_nonCommandedList.RemoveAll();
	m_nonSquadList.RemoveAll();

	bool squadMateFound = false;
	m_iLeftPlayersAlive = 0;
	m_iRightPlayersAlive = 0;
	const int localStar = g_PR->GetStar(localPlayerIndex);

	// Single pass to collect and categorize players
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
				m_iRightPlayersAlive++;
			}
		}
		else {
			if (g_PR->IsAlive(i))
			{
				m_iLeftPlayersAlive++;
			}
		}
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
		offset = DrawPlayerRow_BotCmdr(m_commandedList[i], offset, false, colorOverride);
	}

	// Draw non-commanded players (who are also in the same squad star)
	for (int i = 0; i < m_nonCommandedList.Count(); ++i)
	{
		offset = DrawPlayerRow_BotCmdr(m_nonCommandedList[i], offset, false);
	}

	if (squadMateFound)
	{
		offset += 12;
	}

	// Draw other team mates
	for (int i = 0; i < m_nonSquadList.Count(); ++i)
	{
		offset = DrawPlayerRow_BotCmdr(m_nonSquadList[i], offset, true);
	}
}

int CNEOHud_RoundState::DrawPlayerRow(int playerIndex, const int yOffset, bool small)
{
	ConVarRef cl_neo_bot_cmdr_enable_ref("sv_neo_bot_cmdr_enable");
	Assert(cl_neo_bot_cmdr_enable_ref.IsValid());
	if (cl_neo_bot_cmdr_enable_ref.IsValid() && cl_neo_bot_cmdr_enable_ref.GetBool())
	{
		return DrawPlayerRow_BotCmdr(playerIndex, yOffset, small);
	}

	// Draw player
	static constexpr int SQUAD_MATE_TEXT_LENGTH = 62; // 31 characters in name without end character max plus 3 in short rank name plus 7 max in class name plus 3 max in health plus other characters
	char squadMateText[SQUAD_MATE_TEXT_LENGTH];
	wchar_t wSquadMateText[SQUAD_MATE_TEXT_LENGTH];
	const char* squadMateRankName = GetRankName(g_PR->GetXP(playerIndex), true);
	const char* squadMateClass = GetNeoClassName(g_PR->GetClass(playerIndex));
	const bool isAlive = g_PR->IsAlive(playerIndex);

	C_NEO_Player* pNeoPlayer = ToNEOPlayer(UTIL_PlayerByIndex(playerIndex));
	if (isAlive)
	{
		const char* pPlayerDisplayName = pNeoPlayer ?
			pNeoPlayer->GetPlayerNameWithTakeoverContext(playerIndex)
			: g_PR->GetPlayerName(playerIndex);

		const char* displayRankName = squadMateRankName;
		if (pNeoPlayer)
		{
			C_NEO_Player* pTakeoverTarget = ToNEOPlayer(pNeoPlayer->m_hSpectatorTakeoverPlayerTarget.Get());
			if (pTakeoverTarget)
			{
				displayRankName = GetRankName(g_PR->GetXP(pTakeoverTarget->entindex()), true);
			}
		}

		const int healthMode = cl_neo_hud_health_mode.GetInt();
		char playerHealth[7]; // 4 digits + 2 letters
		V_snprintf(playerHealth, sizeof(playerHealth), healthMode ? "%dhp" : "%d%%", g_PR->GetDisplayedHealth(playerIndex, healthMode));
		V_snprintf(squadMateText, SQUAD_MATE_TEXT_LENGTH, "%s %s  [%s]  %s", pPlayerDisplayName, squadMateRankName, squadMateClass, playerHealth);
	}
	else
	{
		auto* pImpersonator = pNeoPlayer ? pNeoPlayer->m_hSpectatorTakeoverPlayerImpersonatingMe.Get() : nullptr;

		const char* pPlayerDisplayName = pImpersonator ?
			pImpersonator->GetPlayerName()
			: g_PR->GetPlayerName(playerIndex);

		const char* displayClass = pImpersonator ? GetNeoClassName(pImpersonator->m_iClassBeforeTakeover) : squadMateClass;
		V_snprintf(squadMateText, SQUAD_MATE_TEXT_LENGTH, "%s  [%s]  DEAD", pPlayerDisplayName, displayClass);
	}
	g_pVGuiLocalize->ConvertANSIToUnicode(squadMateText, wSquadMateText, sizeof(wSquadMateText));

	int fontWidth, fontHeight;
	surface()->DrawSetTextFont(small ? m_hOCRSmallerFont : m_hOCRSmallFont);
	surface()->GetTextSize(m_hOCRSmallFont, m_wszPlayersAliveUnicode, fontWidth, fontHeight);
	surface()->DrawSetTextColor(isAlive ? COLOR_FADED_WHITE : COLOR_DARK_FADED_WHITE);
	surface()->DrawSetTextPos(8, yOffset);
	surface()->DrawPrintText(wSquadMateText, V_wcslen(wSquadMateText));

	return yOffset + fontHeight;
}

int CNEOHud_RoundState::DrawPlayerRow_BotCmdr(int playerIndex, const int yOffset, bool small, const Color* colorOverride)
{
	// Draw player
	static constexpr int SQUAD_MATE_TEXT_LENGTH = 62; // 31 characters in name without end character max plus 3 in short rank name plus 7 max in class name plus 3 max in health plus other characters
	char squadMateText[SQUAD_MATE_TEXT_LENGTH];
	wchar_t wSquadMateText[SQUAD_MATE_TEXT_LENGTH];
	const char* squadMateRankName = GetRankName(g_PR->GetXP(playerIndex), true);
	const char* squadMateClass = GetNeoClassName(g_PR->GetClass(playerIndex));
	const bool isAlive = g_PR->IsAlive(playerIndex);

	C_NEO_Player* pNeoPlayer = ToNEOPlayer(UTIL_PlayerByIndex(playerIndex));
	if (isAlive)
	{
		const char* pPlayerDisplayName = pNeoPlayer ?
			pNeoPlayer->GetPlayerNameWithTakeoverContext(playerIndex)
			: g_PR->GetPlayerName(playerIndex);

		const char* displayRankName = squadMateRankName;
		if (pNeoPlayer)
		{
			C_NEO_Player* pTakeoverTarget = ToNEOPlayer(pNeoPlayer->m_hSpectatorTakeoverPlayerTarget.Get());
			if (pTakeoverTarget)
			{
				displayRankName = GetRankName(g_PR->GetXP(pTakeoverTarget->entindex()), true);
			}
		}

		const int healthMode = cl_neo_hud_health_mode.GetInt();
		char playerHealth[7]; // 4 digits + 2 letters
		V_snprintf(playerHealth, sizeof(playerHealth), healthMode ? "%dhp" : "%d%%", g_PR->GetDisplayedHealth(playerIndex, healthMode));
		V_snprintf(squadMateText, SQUAD_MATE_TEXT_LENGTH, "%s %s  [%s]  %s", pPlayerDisplayName, squadMateRankName, squadMateClass, playerHealth);
	}
	else
	{
		auto* pImpersonator = pNeoPlayer ? pNeoPlayer->m_hSpectatorTakeoverPlayerImpersonatingMe.Get() : nullptr;

		const char* pPlayerDisplayName = pImpersonator ?
			pImpersonator->GetPlayerName()
			: g_PR->GetPlayerName(playerIndex);

		const char* displayClass = pImpersonator ? GetNeoClassName(pImpersonator->m_iClassBeforeTakeover) : squadMateClass;
		V_snprintf(squadMateText, SQUAD_MATE_TEXT_LENGTH, "%s  [%s]  DEAD", pPlayerDisplayName, displayClass);
	}
	int wSquadMateTextLen = g_pVGuiLocalize->ConvertANSIToUnicode(squadMateText, wSquadMateText, sizeof(wSquadMateText));

	int fontWidth, fontHeight;
	surface()->DrawSetTextFont(small ? m_hOCRSmallerFont : m_hOCRSmallFont);
	surface()->GetTextSize(m_hOCRSmallFont, m_wszPlayersAliveUnicode, fontWidth, fontHeight);

	surface()->DrawSetTextColor(colorOverride ? *colorOverride : (isAlive ? COLOR_FADED_WHITE : COLOR_DARK_FADED_WHITE));
	surface()->DrawSetTextPos(8, yOffset);
	surface()->DrawPrintText(wSquadMateText, wSquadMateTextLen - 1);

	return yOffset + fontHeight;
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
		float textLengthPixels = 0.f;
		int textLength = 1;
		for (textLength; textLength < sizeof(text)/sizeof(text[0]) - 1; textLength++)
		{
			if (text[textLength] == '\0')
				break;

			float wide, abcA;
			surface()->GetKernedCharWidth(m_hTinyText, text[textLength], text[textLength - 1], text[textLength + 1], wide, abcA);
			if (textLengthPixels + wide > m_ilogoSize)
				break;

			textLengthPixels += wide;
		}

		surface()->DrawSetTextFont(m_hTinyText);
		surface()->DrawSetTextColor(COLOR_WHITE);
		surface()->DrawSetTextPos( xOffset + ((m_ilogoSize - textLengthPixels) * 0.5), Y_POS );
		surface()->DrawPrintText( &text[1], textLength - 1);
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
		surface()->DrawTexturedSubRect(xOffset, Y_POS + 1 + m_iTinyTextHeight, xOffset + m_ilogoSize, Y_POS + 1 + m_iTinyTextHeight + m_ilogoSize,
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
				surface()->DrawFilledRectFade(xOffset + TEXTURE_BORDER_WIDTH, Y_POS + 1 + m_iTinyTextHeight + TEXTURE_BORDER_WIDTH, xOffset + m_ilogoSize - TEXTURE_BORDER_WIDTH, Y_POS + 1 + m_iTinyTextHeight + m_ilogoSize - TEXTURE_BORDER_WIDTH, 0, alpha, false);
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
			surface()->DrawFilledRectFade(xOffset + TEXTURE_BORDER_WIDTH, Y_POS + 1 + m_iTinyTextHeight + TEXTURE_BORDER_WIDTH, xOffset + m_ilogoSize - TEXTURE_BORDER_WIDTH, Y_POS + 1 + m_iTinyTextHeight + m_ilogoSize - TEXTURE_BORDER_WIDTH, g_PR->IsAlive(playerIndex) ? 200 : 50, 0, false);
		}
	}

	// Draw commander selected highlight
	ConVarRef cl_neo_bot_cmdr_enable_ref("sv_neo_bot_cmdr_enable");
	Assert(cl_neo_bot_cmdr_enable_ref.IsValid());
	if (cl_neo_bot_cmdr_enable_ref.IsValid() && cl_neo_bot_cmdr_enable_ref.GetBool())
	{
		C_NEO_Player* pPlayer = static_cast<C_NEO_Player*>(UTIL_PlayerByIndex(playerIndex));
		if (pPlayer)
		{
			C_NEO_Player* pCommandingPlayer = static_cast<C_NEO_Player*>(pPlayer->m_hCommandingPlayer.Get());
			if (pCommandingPlayer && pCommandingPlayer->entindex() == highlightPlayerIndex)
			{
				surface()->DrawSetColor(COLOR_YELLOW);
				surface()->DrawFilledRectFade(xOffset + TEXTURE_BORDER_WIDTH, Y_POS + m_iTinyTextHeight + 1 + TEXTURE_BORDER_WIDTH, xOffset + m_ilogoSize - TEXTURE_BORDER_WIDTH, Y_POS + m_iTinyTextHeight + m_ilogoSize + 1 - TEXTURE_BORDER_WIDTH, 200, 0, false);
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
		surface()->DrawFilledRect(xOffset, Y_POS + m_iTinyTextHeight + m_ilogoSize + 2, xOffset + ceil(health / 100.0f * m_ilogoSize), Y_POS + m_iTinyTextHeight + m_ilogoSize + 6);
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
								  Y_POS + m_ilogoSize + 2 - (fontHeight / 2) + iYExtra);
		surface()->DrawSetTextColor(COLOR_WHITE);
		surface()->DrawPrintText(wszXP, iWszLen);
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

	for (auto *ipStar : m_ipStars)
	{
		ipStar->SetVisible(false);
	}

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

	vgui::ImagePanel *target = m_ipStars[currentStar];
	target->SetVisible(true);
	target->SetDrawColor(currentStar == STAR_NONE ? COLOR_NEO_WHITE : currentTeam == TEAM_NSF ? COLOR_NSF : COLOR_JINRAI);
}

void CNEOHud_RoundState::SetTextureToAvatar(int playerIndex)
{
	if (!g_pNeoUIScoreBoard)
	{
		return;
	}

	if (cl_neo_streamermode.GetBool())
	{
		return;
	}

	player_info_t pi;
	if (!engine->GetPlayerInfo(playerIndex, &pi))
		return;

	if (!pi.friendsID)
		return;

	CSteamID steamIDForPlayer(pi.friendsID, 1, steamapicontext->SteamUtils()->GetConnectedUniverse(), k_EAccountTypeIndividual);
	const int mapIndex = g_pNeoUIScoreBoard->m_mapAvatarsToImageList.Find(steamIDForPlayer);
	if ((mapIndex == g_pNeoUIScoreBoard->m_mapAvatarsToImageList.InvalidIndex()))
		return;

	CAvatarImage* pAvIm = (CAvatarImage*)g_pNeoUIScoreBoard->m_pImageList->GetImage(g_pNeoUIScoreBoard->m_mapAvatarsToImageList[mapIndex].i32Idx);
	surface()->DrawSetTexture(pAvIm->getTextureID());
	surface()->DrawSetColor(COLOR_WHITE);
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

CON_COMMAND_F( spec_player_by_hud_position, "Spectate player by position in the top hud", FCVAR_CLIENTCMD_CAN_EXECUTE )
{
	if (engine->IsHLTV() && HLTVCamera()->IsPVSLocked())
	{
		ConMsg( "%s: HLTV Camera is PVS locked\n", __FUNCTION__ );
		return;
	}

	if ( args.ArgC() != 2 )
	{
		ConMsg( "Usage: spec_player_by_hud_position { player position in top hud, 0 indexed }\n" );
		return;
	}

	int positionInHud = atoi( args[1] );
	if (positionInHud < 0 || positionInHud > MAX_PLAYERS - 1)
	{
		ConMsg( "Usage: spec_player_by_hud_position { player position in top hud, 0 indexed }\n" );
		return;
	}

	if (!g_pNeoHudRoundState)
		return;
	
	C_NEO_Player *pNeoPlayer = C_NEO_Player::GetLocalNEOPlayer();
	if ( !pNeoPlayer || !pNeoPlayer->IsObserver() )
		return;

	const int entityIndex = g_pNeoHudRoundState->GetEntityIndexAtPositionInHud(positionInHud, true);
	if (entityIndex)
	{
		engine->IsHLTV() ? HLTVCamera()->SetPrimaryTarget(entityIndex) : engine->ClientCmd(VarArgs("spec_player_entity_number %d", entityIndex));
	}
}

CON_COMMAND_F( spec_next_entity_in_hud, "Spectate next valid player to the right of the current spectate target", FCVAR_CLIENTCMD_CAN_EXECUTE )
{
	if (engine->IsHLTV() && HLTVCamera()->IsPVSLocked())
	{
		ConMsg( "%s: HLTV Camera is PVS locked\n", __FUNCTION__ );
		return;
	}

	if (!g_pNeoHudRoundState)
		return;
	
	C_NEO_Player *pNeoPlayer = C_NEO_Player::GetLocalNEOPlayer();
	if ( !pNeoPlayer || !pNeoPlayer->IsObserver() )
		return;

	int spectateTargetMinusIndexedPositionInHud = 0;
	C_BaseEntity *pSpectateTarget = pNeoPlayer->GetObserverTarget();
	if (pSpectateTarget)
	{
		spectateTargetMinusIndexedPositionInHud = g_pNeoHudRoundState->GetMinusIndexedPositionOfPlayerInHud(pSpectateTarget->entindex());
	}

	const int playerIndex = g_pNeoHudRoundState->GetEntityIndexAtPositionInHud(g_pNeoHudRoundState->GetNextAlivePlayerInHud(spectateTargetMinusIndexedPositionInHud, false));
	if (playerIndex)
	{
		engine->IsHLTV() ? HLTVCamera()->SetPrimaryTarget(playerIndex) : engine->ClientCmd(VarArgs("spec_player_entity_number %d", playerIndex));
	}
}

CON_COMMAND_F( spec_previous_entity_in_hud, "Spectate next valid player to the left of the current spectate target", FCVAR_CLIENTCMD_CAN_EXECUTE )
{
	if (engine->IsHLTV() && HLTVCamera()->IsPVSLocked())
	{
		ConMsg( "%s: HLTV Camera is PVS locked\n", __FUNCTION__ );
		return;
	}

	if (!g_pNeoHudRoundState)
		return;
	
	C_NEO_Player *pNeoPlayer = C_NEO_Player::GetLocalNEOPlayer();
	if ( !pNeoPlayer || !pNeoPlayer->IsObserver() )
		return;
	
	int spectateTargetMinusIndexedPositionInHud = 0;
	C_BaseEntity *pSpectateTarget = pNeoPlayer->GetObserverTarget();
	if (pSpectateTarget)
	{
		spectateTargetMinusIndexedPositionInHud = g_pNeoHudRoundState->GetMinusIndexedPositionOfPlayerInHud(pSpectateTarget->entindex());
	}
	
	const int playerIndex = g_pNeoHudRoundState->GetEntityIndexAtPositionInHud(g_pNeoHudRoundState->GetNextAlivePlayerInHud(spectateTargetMinusIndexedPositionInHud, true));
	if (playerIndex)
	{
		engine->IsHLTV() ? HLTVCamera()->SetPrimaryTarget(playerIndex) : engine->ClientCmd(VarArgs("spec_player_entity_number %d", playerIndex));
	}
}

CON_COMMAND_F( select_next_alive_player_in_hud, "Select the next alive player in the top hud", FCVAR_CLIENTCMD_CAN_EXECUTE )
{
	if (engine->IsHLTV() && HLTVCamera()->IsPVSLocked())
	{
		ConMsg( "%s: Selection is used to switch observer target in spectate_player_selected_in_hud, but HLTV Camera is PVS locked\n", __FUNCTION__ );
		return;
	}

	if (!g_pNeoHudRoundState)
		return;
	
	C_NEO_Player *pNeoPlayer = C_NEO_Player::GetLocalNEOPlayer();
	if ( !pNeoPlayer || !pNeoPlayer->IsObserver() )
		return;

	g_pNeoHudRoundState->SelectNextAlivePlayerInHud();
}

CON_COMMAND_F( select_previous_alive_player_in_hud, "Select the previous alive player in the top hud", FCVAR_CLIENTCMD_CAN_EXECUTE )
{
	if (engine->IsHLTV() && HLTVCamera()->IsPVSLocked())
	{
		ConMsg( "%s: Selection is used to switch observer target in spectate_player_selected_in_hud, but HLTV Camera is PVS locked\n", __FUNCTION__ );
		return;
	}

	if (!g_pNeoHudRoundState)
		return;
	
	C_NEO_Player *pNeoPlayer = C_NEO_Player::GetLocalNEOPlayer();
	if ( !pNeoPlayer || !pNeoPlayer->IsObserver() )
		return;

	g_pNeoHudRoundState->SelectPreviousAlivePlayerInHud();
}

CON_COMMAND_F( spectate_player_selected_in_hud, "Spectate entity selected in the top hud", FCVAR_CLIENTCMD_CAN_EXECUTE )
{
	if (engine->IsHLTV() && HLTVCamera()->IsPVSLocked())
	{
		ConMsg( "%s: HLTV Camera is PVS locked\n", __FUNCTION__ );
		return;
	}

	if (!g_pNeoHudRoundState)
		return;
	
	C_NEO_Player *pNeoPlayer = C_NEO_Player::GetLocalNEOPlayer();
	if ( !pNeoPlayer || !pNeoPlayer->IsObserver() )
		return;

	const int entityIndex = g_pNeoHudRoundState->GetSelectedPlayerInHud();
	if (entityIndex)
	{
		engine->IsHLTV() ? HLTVCamera()->SetPrimaryTarget(entityIndex) : engine->ClientCmd(VarArgs("spec_player_entity_number %d", entityIndex));
	}
}
