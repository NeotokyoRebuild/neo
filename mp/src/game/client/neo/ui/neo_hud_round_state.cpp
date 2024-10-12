#include "cbase.h"

#include <vgui_controls/ImagePanel.h>
#include <vgui_controls/ImageList.h>
#include "neo_hud_round_state.h"

#include "iclientmode.h"
#include "ienginevgui.h"

#include <vgui/ILocalize.h>
#include <vgui/ISurface.h>

#include <initializer_list>

#include <vgui_controls/ImagePanel.h>

#include "c_neo_player.h"
#include "c_team.h"
#include "c_playerresource.h"
#include "vgui_avatarimage.h"

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

using vgui::surface;

DECLARE_NAMED_HUDELEMENT(CNEOHud_RoundState, NRoundState);

NEO_HUD_ELEMENT_DECLARE_FREQ_CVAR(RoundState, 0.1)

ConVar neo_cl_squad_hud_original("neo_cl_squad_hud_original", "0", FCVAR_ARCHIVE, "Use the old squad HUD", true, 0.0, true, 1.0);

ConVar neo_cl_squad_hud_star_scale("neo_cl_squad_hud_star_scale", "0", FCVAR_ARCHIVE, "Scaling to apply from 1080p, 0 disables scaling");

namespace {
constexpr int Y_POS = 2;
constexpr bool STARS_HW_FILTERED = false;
}

CNEOHud_RoundState::CNEOHud_RoundState(const char *pElementName, vgui::Panel *parent)
	: CHudElement(pElementName)
	, Panel(parent, pElementName)
{
	m_pWszStatusUnicode = L"";
	SetAutoDelete(true);

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
			"vgui/reconSmall", "vgui/assaultSmall", "vgui/supportSmall", "vgui/vipSmall"
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

	m_mapAvatarsToImageList.SetLessFunc(DefLessFunc(CSteamID));
	m_mapAvatarsToImageList.RemoveAll();
	m_iNextAvatarUpdate = gpGlobals->curtime;
}

CNEOHud_RoundState::~CNEOHud_RoundState()
{
	delete m_pImageList;
}

void CNEOHud_RoundState::ApplySchemeSettings(vgui::IScheme* pScheme)
{
	BaseClass::ApplySchemeSettings(pScheme);

	m_hOCRSmallFont = pScheme->GetFont("NHudOCRSmall");
	m_hOCRFont = pScheme->GetFont("NHudOCR");

	SetFgColor(COLOR_TRANSPARENT);
	SetBgColor(COLOR_TRANSPARENT);

	// Screen dimensions
	IntDim res = {};
	surface()->GetScreenSize(res.w, res.h);
	m_iXpos = (res.w / 2);

	if (neo_cl_squad_hud_star_scale.GetFloat())
	{
		const float scale = neo_cl_squad_hud_star_scale.GetFloat() * (res.h / 1080.0);
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
	int iSmallFontWidth = 0;
	int iFontHeight = 0;
	[[maybe_unused]] int iFontWidth = 0;
	surface()->GetTextSize(m_hOCRSmallFont, L"ROUND 99", iSmallFontWidth, m_iSmallFontHeight);
	surface()->GetTextSize(m_hOCRFont, L"ROUND 99", iFontWidth, iFontHeight);
	m_iSmallFontHeight *= 0.85;
	iFontHeight *= 0.85;

	const int iBoxHeight = (m_iSmallFontHeight * 2) + iFontHeight;
	const int iBoxHeightHalf = (iBoxHeight / 2);
	const int iBoxWidth = iSmallFontWidth + (iBoxHeight * 2) + 2;
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

	m_posLeftTeamScore = IntPos{
		.x = m_rectLeftTeamTotalLogo.x0 + iBoxHeightHalf,
		.y = static_cast<int>(Y_POS + iBoxHeightHalf - ((iFontHeight / 0.85) / 2)),
	};

	m_posRightTeamScore = IntPos{
		.x = m_rectRightTeamTotalLogo.x0 + iBoxHeightHalf,
		.y = static_cast<int>(Y_POS + iBoxHeightHalf - ((iFontHeight / 0.85) / 2)),
	};

	// Clear player avatars

	if (m_pImageList)
		delete m_pImageList;
	m_pImageList = new vgui::ImageList( false );

	m_mapAvatarsToImageList.RemoveAll();

	SetBounds(0, Y_POS, res.w, res.h);
}

extern ConVar neo_sv_readyup_lobby;

void CNEOHud_RoundState::UpdateStateForNeoHudElementDraw()
{
	float roundTimeLeft = NEORules()->GetRoundRemainingTime();
	const NeoRoundStatus roundStatus = NEORules()->GetRoundStatus();
	const bool inSuddenDeath = NEORules()->RoundIsInSuddenDeath();
	const bool inMatchPoint = NEORules()->RoundIsMatchPoint();

	m_pWszStatusUnicode = (roundStatus == NeoRoundStatus::Warmup) ? L"Warmup" : L"";
	if (roundStatus == NeoRoundStatus::Idle) {
		m_pWszStatusUnicode = neo_sv_readyup_lobby.GetBool() ? L"Waiting for players to ready up" : L"Waiting for players";
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

	// Update steam images
	if (gpGlobals->curtime > m_iNextAvatarUpdate) {
		m_iNextAvatarUpdate = gpGlobals->curtime + 1;
		IGameResources* gr = GameResources();
		if (gr)
		{
			for (int i = 1; i <= gpGlobals->maxClients; ++i)
			{
				if (gr->IsConnected(i)) {
					UpdatePlayerAvatar(i);
				}
			}
		}
	}

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

	char szRoundANSI[9] = {};
	V_sprintf_safe(szRoundANSI, "ROUND %i", NEORules()->roundNumber());
	g_pVGuiLocalize->ConvertANSIToUnicode(szRoundANSI, m_wszRoundUnicode, sizeof(m_wszRoundUnicode));

	if (roundStatus == NeoRoundStatus::PreRoundFreeze)
		roundTimeLeft = NEORules()->GetRemainingPreRoundFreezeTime(true);

	const int secsTotal = RoundFloatToInt(roundTimeLeft);
	const int secsRemainder = secsTotal % 60;
	const int minutes = (secsTotal - secsRemainder) / 60;
	V_snwprintf(m_wszTime, 6, L"%02d:%02d", minutes, secsRemainder);

	const int localPlayerTeam = GetLocalPlayerTeam();
	if (localPlayerTeam == TEAM_JINRAI || localPlayerTeam == TEAM_NSF) {
		V_snwprintf(m_wszLeftTeamScore, 3, L"%i", GetGlobalTeam(localPlayerTeam)->GetRoundsWon());
		V_snwprintf(m_wszRightTeamScore, 3, L"%i", GetGlobalTeam(NEORules()->GetOpposingTeam(localPlayerTeam))->GetRoundsWon());
	}
	else {
		V_snwprintf(m_wszLeftTeamScore, 3, L"%i", GetGlobalTeam(TEAM_JINRAI)->GetRoundsWon());
		V_snwprintf(m_wszRightTeamScore, 3, L"%i", GetGlobalTeam(TEAM_NSF)->GetRoundsWon());
	}

	char szPlayersAliveANSI[9] = {};
	if (NEORules()->GetGameType() == NEO_GAME_TYPE_TDM)
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
	default:
		V_sprintf_safe(szGameTypeDescription, "Await further orders\n");
		break;
	}

	C_NEO_Player* localPlayer = C_NEO_Player::GetLocalNEOPlayer();
	if (localPlayer);
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
	}
}

void CNEOHud_RoundState::DrawNeoHudElement()
{
	if (!ShouldDraw())
	{
		return;
	}

	surface()->DrawSetTextFont(m_hOCRFont);
	int fontWidth, fontHeight;
	surface()->GetTextSize(m_hOCRFont, m_wszTime, fontWidth, fontHeight);

	if (!neo_cl_squad_hud_original.GetBool())
	{
		// Draw Box
		DrawNeoHudRoundedBox(m_iLeftOffset, Y_POS, m_iRightOffset, m_iBoxYEnd, box_color, false, false, true, true);

		// Draw round
		surface()->DrawSetTextFont(m_hOCRSmallFont);
		surface()->GetTextSize(m_hOCRSmallFont, m_wszRoundUnicode, fontWidth, fontHeight);
		surface()->DrawSetTextPos(m_iXpos - (fontWidth / 2), 0);
		surface()->DrawSetTextColor(COLOR_WHITE);
		surface()->DrawPrintText(m_wszRoundUnicode, 9);

		// Draw round status
		surface()->GetTextSize(m_hOCRSmallFont, m_pWszStatusUnicode, fontWidth, fontHeight);
		surface()->DrawSetTextPos(m_iXpos - (fontWidth / 2), m_iBoxYEnd);
		surface()->DrawPrintText(m_pWszStatusUnicode, m_iStatusUnicodeSize);

		const int localPlayerTeam = GetLocalPlayerTeam();
		const int localPlayerIndex = GetLocalPlayerIndex();
		const bool localPlayerSpec = !(localPlayerTeam == TEAM_JINRAI || localPlayerTeam == TEAM_NSF);

		const int leftTeam = localPlayerSpec ? TEAM_JINRAI : localPlayerTeam;
		const int rightTeam = (leftTeam == TEAM_JINRAI) ? TEAM_NSF : TEAM_JINRAI;
		const auto leftTeamInfo = m_teamLogoColors[leftTeam];
		const auto rightTeamInfo = m_teamLogoColors[rightTeam];

		// Draw players
		if (!g_PR)
			return;

		m_iLeftPlayersAlive = 0;
		m_iLeftPlayersTotal = 0;
		m_iRightPlayersAlive = 0;
		m_iRightPlayersTotal = 0;
		int leftCount = 0;
		int rightCount = 0;
		for (int i = 0; i < (MAX_PLAYERS + 1); i++)
		{
			if (g_PR->IsConnected(i))
			{
				const int playerTeam = g_PR->GetTeam(i);
				if (playerTeam == leftTeam)
				{
					const bool isSameSquad = g_PR->GetStar(i) == g_PR->GetStar(localPlayerIndex);
					if (localPlayerSpec || isSameSquad)
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
					DrawPlayer(i, rightCount, rightTeamInfo, xOffset, localPlayerSpec);
					rightCount++;

					if (g_PR->IsAlive(i))
						m_iRightPlayersAlive++;
					m_iRightPlayersTotal++;
				}
			}
		}

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
		surface()->GetTextSize(m_hOCRFont, m_wszLeftTeamScore, fontWidth, fontHeight);
		surface()->DrawSetTextFont(m_hOCRFont);
		surface()->DrawSetTextPos(m_posLeftTeamScore.x - (fontWidth / 2), m_posLeftTeamScore.y);
		surface()->DrawSetTextColor(leftTeamInfo.color);
		surface()->DrawPrintText(m_wszLeftTeamScore, 2);

		surface()->GetTextSize(m_hOCRFont, m_wszRightTeamScore, fontWidth, fontHeight);
		surface()->DrawSetTextPos(m_posRightTeamScore.x - (fontWidth / 2), m_posRightTeamScore.y);
		surface()->DrawSetTextColor(rightTeamInfo.color);
		surface()->DrawPrintText(m_wszRightTeamScore, 2);
	
		// Draw total players alive
		surface()->DrawSetTextFont(m_hOCRSmallFont);
		surface()->GetTextSize(m_hOCRSmallFont, m_wszPlayersAliveUnicode, fontWidth, fontHeight);
		surface()->DrawSetTextColor(COLOR_WHITE);
		surface()->DrawSetTextPos(m_iXpos - (fontWidth / 2), m_ilogoSize);
		surface()->DrawPrintText(m_wszPlayersAliveUnicode, 9);

	}
	else
	{
		if (g_PR)
		{
			// Draw members of players squad in an old style list
			const int localPlayerTeam = GetLocalPlayerTeam();
			const int localPlayerIndex = GetLocalPlayerIndex();
			const bool localPlayerSpec = !(localPlayerTeam == TEAM_JINRAI || localPlayerTeam == TEAM_NSF);

			if (!localPlayerSpec && g_PR->GetStar(localPlayerIndex) != 0)
			{
				const int leftTeam = localPlayerSpec ? TEAM_JINRAI : localPlayerTeam;
				const auto leftTeamInfo = m_teamLogoColors[leftTeam];
				int leftCount = 0;

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

					// Draw player
					static constexpr int SQUAD_MATE_TEXT_LENGTH = 62; // 31 characters in name without end character max plus 3 in short rank name plus 7 max in class name plus 3 max in health plus other characters
					char squadMateText[SQUAD_MATE_TEXT_LENGTH];
					wchar_t wSquadMateText[SQUAD_MATE_TEXT_LENGTH];
					const char* squadMateRankName = GetRankName(g_PR->GetXP(i), true);
					const char* squadMateClass = GetNeoClassName(g_PR->GetClass(i));
					const int squadMateHealth = g_PR->IsAlive( i ) ? g_PR->GetHealth( i ) : 0;
					V_snprintf(squadMateText, SQUAD_MATE_TEXT_LENGTH, "%s %s  [%s]  Integrity %i\0", g_PR->GetPlayerName( i ), squadMateRankName, squadMateClass, squadMateHealth);
					g_pVGuiLocalize->ConvertANSIToUnicode(squadMateText, wSquadMateText, sizeof(wSquadMateText));

					surface()->DrawSetTextFont(m_hOCRSmallFont);
					surface()->GetTextSize(m_hOCRSmallFont, m_wszPlayersAliveUnicode, fontWidth, fontHeight);
					surface()->DrawSetTextColor(g_PR->IsAlive( i ) ? COLOR_FADED_WHITE : COLOR_DARK_FADED_WHITE);
					surface()->DrawSetTextPos(8, 48 + fontHeight * leftCount);
					surface()->DrawPrintText(wSquadMateText, Q_strlen(squadMateText));

					leftCount++;
				}
			}
		}
	}

	// Draw time
	surface()->DrawSetTextFont(m_hOCRFont);
	surface()->GetTextSize(m_hOCRFont, m_wszTime, fontWidth, fontHeight);
	surface()->DrawSetTextColor(neo_cl_squad_hud_original.GetBool() ? COLOR_FADED_WHITE : (NEORules()->GetRoundStatus() == NeoRoundStatus::PreRoundFreeze) ?
									COLOR_RED : COLOR_WHITE);
	surface()->DrawSetTextPos(m_iXpos - (fontWidth / 2), neo_cl_squad_hud_original.GetBool() ? Y_POS : m_iSmallFontHeight);
	surface()->DrawPrintText(m_wszTime, 6);

	// Draw Game Type Description
	if (m_iGameTypeDescriptionState)
	{
		surface()->DrawSetTextFont(m_hOCRFont);
		surface()->GetTextSize(m_hOCRFont, m_wszGameTypeDescription, fontWidth, fontHeight);
		surface()->DrawSetTextColor(COLOR_WHITE);
		surface()->DrawSetTextPos(m_iXpos - (fontWidth / 2), m_iBoxYEnd);
		surface()->DrawPrintText(m_wszGameTypeDescription, Q_UnicodeLength(m_wszGameTypeDescription));
	}

	CheckActiveStar();
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

	int currentStar = player->GetStar();
	const int currentTeam = player->GetTeamNumber();

	if (m_iPreviouslyActiveStar == currentStar && m_iPreviouslyActiveTeam == currentTeam)
	{
		return;
	}

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

	m_iPreviouslyActiveStar = currentStar;
	m_iPreviouslyActiveTeam = currentTeam;
}

void CNEOHud_RoundState::UpdatePlayerAvatar(int playerIndex)
{
	// Update their avatar
	if (!steamapicontext->SteamFriends() || !steamapicontext->SteamUtils())
		return;

	player_info_t pi;
	if (!engine->GetPlayerInfo(playerIndex, &pi))
		return;
	
	if (!pi.friendsID)
		return;
	
	CSteamID steamIDForPlayer(pi.friendsID, 1, steamapicontext->SteamUtils()->GetConnectedUniverse(), k_EAccountTypeIndividual);

	// See if we already have that avatar in our list
	const int iMapIndex = m_mapAvatarsToImageList.Find(steamIDForPlayer);
	if (iMapIndex != m_mapAvatarsToImageList.InvalidIndex())
		return;
	
	CAvatarImage* pImage = new CAvatarImage();
	pImage->SetAvatarSteamID(steamIDForPlayer, k_EAvatarSize64x64);
	pImage->SetAvatarSize(64, 64);	// Deliberately non scaling
	const int iImageIndex = m_pImageList->AddImage(pImage);

	m_mapAvatarsToImageList.Insert(steamIDForPlayer, iImageIndex);
}

void CNEOHud_RoundState::SetTextureToAvatar(int playerIndex)
{
	player_info_t pi;
	if (!engine->GetPlayerInfo(playerIndex, &pi))
		return;
	
	if (!pi.friendsID)
		return;
	
	CSteamID steamIDForPlayer(pi.friendsID, 1, steamapicontext->SteamUtils()->GetConnectedUniverse(), k_EAccountTypeIndividual);
	const int mapIndex = m_mapAvatarsToImageList.Find(steamIDForPlayer);
	if ((mapIndex == m_mapAvatarsToImageList.InvalidIndex()))
		return; 

	CAvatarImage* pAvIm = (CAvatarImage*)m_pImageList->GetImage(m_mapAvatarsToImageList[mapIndex]);
	surface()->DrawSetTexture(pAvIm->getTextureID());
	surface()->DrawSetColor(COLOR_WHITE);
}

void CNEOHud_RoundState::Paint()
{
	BaseClass::Paint();
	PaintNeoElement();
}
