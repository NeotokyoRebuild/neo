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
#include "neo_scoreboard.h"

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

using vgui::surface;

DECLARE_NAMED_HUDELEMENT(CNEOHud_RoundState, NRoundState);

NEO_HUD_ELEMENT_DECLARE_FREQ_CVAR(RoundState, 0.1)

ConVar cl_neo_squad_hud_original("cl_neo_squad_hud_original", "1", FCVAR_ARCHIVE, "Use the old squad HUD", true, 0.0, true, 1.0);
ConVar cl_neo_squad_hud_star_scale("cl_neo_squad_hud_star_scale", "0", FCVAR_ARCHIVE, "Scaling to apply from 1080p, 0 disables scaling");
extern ConVar sv_neo_dm_win_xp;
extern ConVar cl_neo_streamermode;
extern ConVar snd_victory_volume;
extern ConVar sv_neo_readyup_countdown;

namespace {
constexpr int Y_POS = 0;
constexpr bool STARS_HW_FILTERED = false;
}

CNEOHud_RoundState::CNEOHud_RoundState(const char *pElementName, vgui::Panel *parent)
	: CHudElement(pElementName)
	, Panel(parent, pElementName)
{
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
			"none", "alpha", "bravo", "charlie", "delta", "echo", "foxtrot"
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

	for (int i = 0; i < NEO_CLASS__ENUM_COUNT; ++i)
	{
		static constexpr const char *TEX_NAMES[NEO_CLASS__ENUM_COUNT] = {
			"vgui/reconSmall", "vgui/assaultSmall", "vgui/supportSmall", "vgui/vipSmall", "vgui/vipSmall"
		};
		m_iGraphicID[i] = surface()->CreateNewTextureID();
		surface()->DrawSetTextureFile(m_iGraphicID[i], TEX_NAMES[i], true, false);
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

void CNEOHud_RoundState::ApplySchemeSettings(vgui::IScheme* pScheme)
{
	BaseClass::ApplySchemeSettings(pScheme);

	m_hOCRLargeFont = pScheme->GetFont("NHudOCRLarge");
	m_hOCRFont = pScheme->GetFont("NHudOCR");
	m_hOCRSmallFont = pScheme->GetFont("NHudOCRSmall");
	m_hOCRSmallerFont = pScheme->GetFont("NHudOCRSmaller");

	SetFgColor(COLOR_TRANSPARENT);
	SetBgColor(COLOR_TRANSPARENT);

	// Screen dimensions
	IntDim res = {};
	surface()->GetScreenSize(res.w, res.h);
	m_iXpos = (res.w / 2);

	if (cl_neo_squad_hud_star_scale.GetFloat())
	{
		const float scale = cl_neo_squad_hud_star_scale.GetFloat() * (res.h / 1080.0);
		for (auto* star : m_ipStars)
		{
			star->SetWide(192 * scale);
			star->SetTall(48 * scale);
		}
	}
	else {
		for (auto* star : m_ipStars)
		{
			star->SetWide(192);
			star->SetTall(48);
		}
	}

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
	m_ilogoSize = m_iSmallFontHeight + iFontHeight;

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
	const bool inMatchPoint = NEORules()->RoundIsMatchPoint();

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
	else if (inMatchPoint)
	{
		m_pWszStatusUnicode = L"Match point";
	}
	m_iStatusUnicodeSize = V_wcslen(m_pWszStatusUnicode);

	// Clear the strings so zero roundTimeLeft also picks it up as to not draw
	memset(m_wszRoundUnicode, 0, sizeof(m_wszRoundUnicode));
	memset(m_wszPlayersAliveUnicode, 0, sizeof(m_wszPlayersAliveUnicode));
	memset(m_wszTime, 0, sizeof(m_wszTime));
	memset(m_wszLeftTeamScore, 0, sizeof(m_wszLeftTeamScore));
	memset(m_wszRightTeamScore, 0, sizeof(m_wszRightTeamScore));
	memset(m_wszGameTypeDescription, 0, sizeof(m_wszGameTypeDescription));

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

	const int secsTotal = RoundFloatToInt(roundTimeLeft);
	const int secsRemainder = secsTotal % 60;
	const int minutes = (secsTotal - secsRemainder) / 60;
	V_snwprintf(m_wszTime, 6, L"%02d:%02d", minutes, secsRemainder);

	int iDMHighestXP = 0;

	const int localPlayerTeam = GetLocalPlayerTeam();
	if (NEORules()->IsTeamplay())
	{
		if (localPlayerTeam == TEAM_JINRAI || localPlayerTeam == TEAM_NSF) {
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
		if (localPlayerTeam == TEAM_JINRAI || localPlayerTeam == TEAM_NSF) {
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

	// Update Objective
	switch (NEORules()->GetGameType()) {
	case NEO_GAME_TYPE_DM:
		// Don't print objective for deathmatch
		szGameTypeDescription[0] = '\0';
		break;
	case NEO_GAME_TYPE_TDM:
		V_sprintf_safe(szGameTypeDescription, "Score the most Points\n");
		break;
	case NEO_GAME_TYPE_CTG:
		V_sprintf_safe(szGameTypeDescription, "Capture the Ghost\n");
		break;
	case NEO_GAME_TYPE_VIP:
		if (localPlayerTeam == NEORules()->m_iEscortingTeam.Get())
		{
			if (NEORules()->GhostExists())
			{
				V_sprintf_safe(szGameTypeDescription, "VIP down, prevent Ghost capture\n");
			}
			else
			{
				V_sprintf_safe(szGameTypeDescription, "Escort the VIP\n");
			}
		}
		else
		{
			if (NEORules()->GhostExists())
			{
				V_sprintf_safe(szGameTypeDescription, "HVT down, secure the Ghost\n");
			}
			else
			{
				V_sprintf_safe(szGameTypeDescription, "Eliminate the HVT\n");
			}
		}
		break;
	case NEO_GAME_TYPE_JGR:
		V_sprintf_safe(szGameTypeDescription, "Control the Juggernaut\n");
		break;
	default:
		V_sprintf_safe(szGameTypeDescription, "Await further orders\n");
		break;
	}

	if (NEORules()->GetRoundStatus() == NeoRoundStatus::Pause ||
			NEORules()->GetRoundStatus() == NeoRoundStatus::Countdown)
	{
		szGameTypeDescription[0] = '\0';
	}

	C_NEO_Player* localPlayer = C_NEO_Player::GetLocalNEOPlayer();
	if (localPlayer)
	{
		if (NEORules()->IsRoundPreRoundFreeze() || localPlayer->m_nButtons & IN_SCORE)
		{
			m_iGameTypeDescriptionState = MIN(m_iGameTypeDescriptionState + 1, Q_UnicodeLength(szGameTypeDescription));
		}
		else
		{
			m_iGameTypeDescriptionState = MAX(m_iGameTypeDescriptionState - 1, 0);
		}

		g_pVGuiLocalize->ConvertANSIToUnicode(szGameTypeDescription, m_wszGameTypeDescription, m_iGameTypeDescriptionState * sizeof(wchar_t));

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
				const float flVol = (bEndBeep) ? (1.3f * snd_victory_volume.GetFloat()) : snd_victory_volume.GetFloat();
				static constexpr int PITCH_END = 165;
				enginesound->EmitAmbientSound("tutorial/hitsound.wav", flVol, bEndBeep ? PITCH_END : PITCH_NORM);
				m_iBeepSecsTotal = secsTotal;
			}
			// Otherwise don't beep/alter m_iBeepSecsTotal
		}
	}

	m_ePrevRoundStatus = NEORules()->GetRoundStatus();
}

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
	const int localPlayerIndex = GetLocalPlayerIndex();
	const bool localPlayerSpecOrNoTeam = !NEORules()->IsTeamplay() || !(localPlayerTeam == TEAM_JINRAI || localPlayerTeam == TEAM_NSF);

	const int leftTeam = localPlayerSpecOrNoTeam ? TEAM_JINRAI : localPlayerTeam;
	const int rightTeam = (leftTeam == TEAM_JINRAI) ? TEAM_NSF : TEAM_JINRAI;
	const auto leftTeamInfo = m_teamLogoColors[leftTeam];
	const auto rightTeamInfo = m_teamLogoColors[rightTeam];

	// Draw total players alive (or score)
	surface()->DrawSetTextFont(m_hOCRSmallerFont);
	surface()->GetTextSize(m_hOCRSmallerFont, m_wszPlayersAliveUnicode, fontWidth, fontHeight);
	surface()->DrawSetTextColor(COLOR_FADED_WHITE);
	surface()->DrawSetTextPos(m_iXpos - (fontWidth / 2), m_ilogoSize);
	surface()->DrawPrintText(m_wszPlayersAliveUnicode, ARRAYSIZE(m_wszPlayersAliveUnicode) - 1);

	// Draw time
	surface()->DrawSetTextFont(m_hOCRFont);
	surface()->GetTextSize(m_hOCRFont, m_wszTime, fontWidth, fontHeight);
	surface()->DrawSetTextColor((NEORules()->GetRoundStatus() == NeoRoundStatus::PreRoundFreeze ||
								 NEORules()->GetRoundStatus() == NeoRoundStatus::Countdown) ?
									COLOR_RED : COLOR_WHITE);
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

	// Draw Game Type Description
	if (m_iGameTypeDescriptionState)
	{
		surface()->DrawSetTextFont(m_hOCRFont);
		surface()->GetTextSize(m_hOCRFont, m_wszGameTypeDescription, fontWidth, fontHeight);
		surface()->DrawSetTextColor(COLOR_WHITE);
		surface()->DrawSetTextPos(m_iXpos - (fontWidth / 2), m_iBoxYEnd);
		surface()->DrawPrintText(m_wszGameTypeDescription, Q_UnicodeLength(m_wszGameTypeDescription));
	}

	if (!cl_neo_squad_hud_original.GetBool())
	{
		// Draw players
		if (!g_PR)
			return;

		m_iLeftPlayersAlive = 0;
		m_iLeftPlayersTotal = 0;
		m_iRightPlayersAlive = 0;
		m_iRightPlayersTotal = 0;
		int leftCount = 0;
		int rightCount = 0;
		bool bDMRightSide = false;
		if (NEORules()->IsTeamplay())
		{
			for (int i = 0; i < (MAX_PLAYERS + 1); i++)
			{
				if (g_PR->IsConnected(i))
				{
					const int playerTeam = g_PR->GetTeam(i);
					if (playerTeam == leftTeam)
					{
						const bool isSameSquad = g_PR->GetStar(i) == g_PR->GetStar(localPlayerIndex);
						if (localPlayerSpecOrNoTeam || isSameSquad)
						{
							const int xOffset = (m_iLeftOffset - 4) - ((leftCount + 1) * m_ilogoSize) - (leftCount * 2);
							DrawPlayer(i, leftCount, leftTeamInfo, xOffset, true);
							leftCount++;
						}

						if (g_PR->IsAlive(i))
							m_iLeftPlayersAlive++;
						m_iLeftPlayersTotal++;
					}
					else if (playerTeam == rightTeam)
					{
						const int xOffset = (m_iRightOffset + 4) + (rightCount * m_ilogoSize) + (rightCount * 2);
						DrawPlayer(i, rightCount, rightTeamInfo, xOffset, localPlayerSpecOrNoTeam);
						rightCount++;

						if (g_PR->IsAlive(i))
							m_iRightPlayersAlive++;
						m_iRightPlayersTotal++;
					}
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
	else
	{
		DrawPlayerList();
	}
}

void CNEOHud_RoundState::DrawPlayerList()
{
	if (g_PR)
	{
		// Draw members of players squad in an old style list
		const int localPlayerTeam = GetLocalPlayerTeam();
		const int localPlayerIndex = GetLocalPlayerIndex();
		const bool localPlayerSpec = !(localPlayerTeam == TEAM_JINRAI || localPlayerTeam == TEAM_NSF);
		const int leftTeam = localPlayerSpec ? TEAM_JINRAI : localPlayerTeam;

		if (localPlayerSpec)
		{
			return;
		}

		int offset = 52;
		if (cl_neo_squad_hud_star_scale.GetFloat() > 0)
		{
			IntDim res = {};
			surface()->GetScreenSize(res.w, res.h);
			offset *= cl_neo_squad_hud_star_scale.GetFloat() * res.h / 1080.0f;
		}

		// Draw squad mates
		if (g_PR->GetStar(localPlayerIndex) != 0)
		{
			bool squadMateFound = false;

			for (int i = 0; i < (MAX_PLAYERS + 1); i++)
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
				if (playerTeam != leftTeam)
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
		for (int i = 0; i < (MAX_PLAYERS + 1); i++)
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
				continue;
			}
			else {
				if (g_PR->IsAlive(i))
				{
					m_iLeftPlayersAlive++;
				}
			}
			if (i == localPlayerIndex)
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

int CNEOHud_RoundState::DrawPlayerRow(int playerIndex, const int yOffset, bool small)
{
	// Draw player
	static constexpr int SQUAD_MATE_TEXT_LENGTH = 62; // 31 characters in name without end character max plus 3 in short rank name plus 7 max in class name plus 3 max in health plus other characters
	char squadMateText[SQUAD_MATE_TEXT_LENGTH];
	wchar_t wSquadMateText[SQUAD_MATE_TEXT_LENGTH];
	const char* squadMateRankName = GetRankName(g_PR->GetXP(playerIndex), true);
	const char* squadMateClass = GetNeoClassName(g_PR->GetClass(playerIndex));
	const bool isAlive = g_PR->IsAlive(playerIndex);
	const int squadMateHealth = isAlive ? g_PR->GetHealth(playerIndex) : 0;

	if (isAlive)
	{
		V_snprintf(squadMateText, SQUAD_MATE_TEXT_LENGTH, "%s %s  [%s]  Integrity %i", g_PR->GetPlayerName(playerIndex), squadMateRankName, squadMateClass, squadMateHealth);
	}
	else
	{
		V_snprintf(squadMateText, SQUAD_MATE_TEXT_LENGTH, "%s  [%s]  DEAD", g_PR->GetPlayerName(playerIndex), squadMateClass);
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

void CNEOHud_RoundState::DrawPlayer(int playerIndex, int teamIndex, const TeamLogoColor &teamLogoColor,
									const int xOffset, const bool drawHealthClass)
{
	// Draw Outline
	surface()->DrawSetColor(box_color);
	surface()->DrawFilledRect(xOffset - 1, Y_POS, xOffset + m_ilogoSize + 1,
							  Y_POS + m_ilogoSize + 2 + (drawHealthClass ? 5 : 0));

	// Drawing Avatar
	surface()->DrawSetTexture(teamLogoColor.logo);

	if (g_PR->IsAlive(playerIndex)) {
		surface()->DrawSetColor(teamLogoColor.color);
		surface()->DrawFilledRect(xOffset, Y_POS + 1, xOffset + m_ilogoSize, Y_POS + m_ilogoSize + 1);
	}
	else {
		surface()->DrawSetColor(COLOR_DARK);
		surface()->DrawFilledRect(xOffset, Y_POS + 1, xOffset + m_ilogoSize, Y_POS + m_ilogoSize + 1);
	}

	SetTextureToAvatar(playerIndex);
	if (!g_PR->IsAlive(playerIndex))
		surface()->DrawSetColor(COLOR_DARK);
	surface()->DrawTexturedRect(xOffset, Y_POS + 1, xOffset + m_ilogoSize, Y_POS + m_ilogoSize + 1);

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

	// Return early to not draw healthbar and class icon
	if (!drawHealthClass)
	{
		return;
	}

	// Drawing Class Icon
	surface()->DrawSetTexture(m_iGraphicID[g_PR->GetClass(playerIndex)]);
	surface()->DrawSetColor(teamLogoColor.color);
	surface()->DrawTexturedRect(xOffset, Y_POS + m_ilogoSize + 7, xOffset + m_ilogoSize, Y_POS + m_ilogoSize + 71);

	// Drawing Healthbar
	if (!g_PR->IsAlive(playerIndex))
		return;

	if (health_monochrome) {
		const int greenBlueValue = (g_PR->GetHealth(playerIndex) / 100.0f) * 255;
		surface()->DrawSetColor(Color(255, greenBlueValue, greenBlueValue, 255));
	}
	else {
		if (g_PR->GetHealth(playerIndex) <= 20)
			surface()->DrawSetColor(COLOR_RED);
		else if (g_PR->GetHealth(playerIndex) <= 80)
			surface()->DrawSetColor(COLOR_YELLOW);
		else
			surface()->DrawSetColor(COLOR_WHITE);
	}
	surface()->DrawFilledRect(xOffset, Y_POS + m_ilogoSize + 2, xOffset + (g_PR->GetHealth(playerIndex) / 100.0f * m_ilogoSize), Y_POS + m_ilogoSize + 6);
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
	if (!g_pNeoScoreBoard)
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
	const int mapIndex = g_pNeoScoreBoard->m_mapAvatarsToImageList.Find(steamIDForPlayer);
	if ((mapIndex == g_pNeoScoreBoard->m_mapAvatarsToImageList.InvalidIndex()))
		return;

	CAvatarImage* pAvIm = (CAvatarImage*)g_pNeoScoreBoard->m_pImageList->GetImage(g_pNeoScoreBoard->m_mapAvatarsToImageList[mapIndex]);
	surface()->DrawSetTexture(pAvIm->getTextureID());
	surface()->DrawSetColor(COLOR_WHITE);
}

void CNEOHud_RoundState::Paint()
{
	BaseClass::Paint();
	PaintNeoElement();
}
