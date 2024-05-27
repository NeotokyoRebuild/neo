#include "cbase.h"
#include "neo_hud_round_state.h"

#include "iclientmode.h"
#include "ienginevgui.h"

#include <vgui/ILocalize.h>
#include <vgui/ISurface.h>

#include "neo_gamerules.h"
#include <initializer_list>

#include <vgui_controls/ImagePanel.h>

#include "c_neo_player.h"
#include "c_team.h"
#include "c_playerresource.h"

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

using vgui::surface;

ConVar neo_cl_hud_roundstatus_pos_x("neo_cl_hud_roundstatus_pos_x", "0", FCVAR_ARCHIVE | FCVAR_USERINFO,
	"Offset in pixels.");
ConVar neo_cl_hud_roundstatus_pos_y("neo_cl_hud_roundstatus_pos_y", "0", FCVAR_ARCHIVE | FCVAR_USERINFO,
	"Offset in pixels.");

NEO_HUD_ELEMENT_DECLARE_FREQ_CVAR(RoundState, 0.1)

CNEOHud_RoundState::CNEOHud_RoundState(const char *pElementName, vgui::Panel *parent)
	: CHudElement(pElementName), Panel(parent, pElementName)
{
	SetAutoDelete(true);

	vgui::HScheme neoscheme = vgui::scheme()->LoadSchemeFromFileEx(
		enginevgui->GetPanel(PANEL_CLIENTDLL), "resource/ClientScheme_Neo.res", "ClientScheme_Neo");
	SetScheme(neoscheme);

	if (parent)
	{
		SetParent(parent);
	}
	else
	{
		SetParent(g_pClientMode->GetViewport());
	}

	surface()->GetScreenSize(m_resX, m_resY);
	SetBounds(0, 0, m_resX, 72);

	// NEO HACK (Rain): this is kind of awkward, we should get the handle on ApplySchemeSettings
	vgui::IScheme *scheme = vgui::scheme()->GetIScheme(neoscheme);
	Assert(scheme);

	m_hFont = scheme->GetFont("NHudOCRSmall");

	{
		int i;
		for (i = 0; i < sizeof(m_szStatusANSI) - 1; ++i)
		{
			m_szStatusANSI[i] = ' ';
		}
		m_szStatusANSI[i] = '\0';
	}
	g_pVGuiLocalize->ConvertANSIToUnicode(m_szStatusANSI, m_wszStatusUnicode, sizeof(m_wszStatusUnicode));

	// Initialize star textures
	const bool starsHwFiltered = false;
	starNone = new vgui::ImagePanel(this, "star_none");
	starNone->SetImage(vgui::scheme()->GetImage("hud/star_none", starsHwFiltered));

	starA = new vgui::ImagePanel(this, "star_alpha");
	starA->SetImage(vgui::scheme()->GetImage("hud/star_alpha", starsHwFiltered));

	starB = new vgui::ImagePanel(this, "star_bravo");
	starB->SetImage(vgui::scheme()->GetImage("hud/star_bravo", starsHwFiltered));

	starC = new vgui::ImagePanel(this, "star_charlie");
	starC->SetImage(vgui::scheme()->GetImage("hud/star_charlie", starsHwFiltered));

	starD = new vgui::ImagePanel(this, "star_delta");
	starD->SetImage(vgui::scheme()->GetImage("hud/star_delta", starsHwFiltered));

	starE = new vgui::ImagePanel(this, "star_echo");
	starE->SetImage(vgui::scheme()->GetImage("hud/star_echo", starsHwFiltered));

	starF = new vgui::ImagePanel(this, "star_foxtrot");
	starF->SetImage(vgui::scheme()->GetImage("hud/star_foxtrot", starsHwFiltered));

	vgui::ImagePanel *stars[] = { starNone, starA, starB, starC, starD, starE, starF };

	const bool starAutoDelete = true;
	COMPILE_TIME_ASSERT(starAutoDelete); // If not, we need to handle that memory on dtor manually

	for (auto star : stars) {
		star->SetDrawColor(COLOR_NSF); // This will get updated in the draw check as required
		star->SetAlpha(1.0f);
		star->SetWide(192 * m_resX / 1920.0); // The icons are downscaled to 192*48 on a 1080p res.
		star->SetTall(48 * m_resY / 1080.0);
		star->SetShouldScaleImage(true);
		star->SetAutoDelete(starAutoDelete);
		star->SetVisible(false);
	}

	box_color_r = box_color_g = box_color_b = 116;
	box_color_a = 178;

	m_iJinraiID = surface()->CreateNewTextureID();
	surface()->DrawSetTextureFile(m_iJinraiID, "vgui/jinrai_128tm", true, false);

	m_iNSFID = surface()->CreateNewTextureID();
	surface()->DrawSetTextureFile(m_iNSFID, "vgui/nsf_128tm", true, false);

	m_iSupportID = surface()->CreateNewTextureID();
	surface()->DrawSetTextureFile(m_iSupportID, "vgui/support", true, false);

	m_iAssaultID = surface()->CreateNewTextureID();
	surface()->DrawSetTextureFile(m_iAssaultID, "vgui/assault", true, false);

	m_iReconID = surface()->CreateNewTextureID();
	surface()->DrawSetTextureFile(m_iReconID, "vgui/recon", true, false);

	m_iVIPID = surface()->CreateNewTextureID();
	surface()->DrawSetTextureFile(m_iVIPID, "vgui/vip", true, false);

	m_iJinraiTotalID = surface()->CreateNewTextureID();
	surface()->DrawSetTextureFile(m_iJinraiTotalID, "vgui/ts_jinrai", true, false);

	m_iNSFTotalID = surface()->CreateNewTextureID();
	surface()->DrawSetTextureFile(m_iNSFTotalID, "vgui/ts_nsf", true, false);

	m_iPreviouslyActiveStar = m_iPreviouslyActiveTeam  = -1;
}

void CNEOHud_RoundState::ApplySchemeSettings(vgui::IScheme* pScheme)
{
	BaseClass::ApplySchemeSettings(pScheme);

	SetFgColor(Color(0, 0, 0, 0));
	SetBgColor(Color(0, 0, 0, 0));
}

void CNEOHud_RoundState::UpdateStateForNeoHudElementDraw()
{
	float roundTimeLeft = NEORules()->GetRoundRemainingTime();
	const NeoRoundStatus roundStatus = NEORules()->GetRoundStatus();

	// Exactly zero means there's no time limit, so we don't need to draw anything.
	if (roundTimeLeft == 0)
	{
		{
			int i;
			for (i = 0; i < sizeof(m_szStatusANSI) - 1; ++i)
			{
				m_szStatusANSI[i] = ' ';
			}
			m_szStatusANSI[i] = '\0';
		}
		g_pVGuiLocalize->ConvertANSIToUnicode(m_szStatusANSI, m_wszStatusUnicode, sizeof(m_wszStatusUnicode));
		return;
	}
	// Less than 0 means round is over, but cap the timer to zero for nicer display.
	else if (roundTimeLeft < 0)
	{
		roundTimeLeft = 0;
	}

	const int secsTotal = RoundFloatToInt(roundTimeLeft);
	const int secsRemainder = secsTotal % 60;
	const int minutes = (secsTotal - secsRemainder) / 60;

	V_sprintf_safe(m_szStatusANSI, "%s%02d:%02d", (roundStatus == NeoRoundStatus::Warmup) ? "(Warmup) " : "", minutes, secsRemainder);
	memset(m_wszStatusUnicode, 0, sizeof(m_wszStatusUnicode)); // NOTE (nullsystem): Clear it or get junk after warmup ends
	g_pVGuiLocalize->ConvertANSIToUnicode(m_szStatusANSI, m_wszStatusUnicode, sizeof(m_wszStatusUnicode));

	V_snwprintf(m_wszJinraiScore, 3, L"%i", GetGlobalTeam(TEAM_JINRAI)->GetRoundsWon());
	V_snwprintf(m_wszNSFScore, 3, L"%i", GetGlobalTeam(TEAM_NSF)->GetRoundsWon());
	V_snwprintf(m_wszRound, 3, L"%i", NEORules()->roundNumber());
}

void CNEOHud_RoundState::DrawNeoHudElement()
{
	if (!ShouldDraw())
	{
		return;
	}

	surface()->DrawSetTextFont(m_hFont);

	const int xpos = (m_resX / 2) + neo_cl_hud_roundstatus_pos_x.GetInt();
	const int ypos = (m_resY * 0.01) + neo_cl_hud_roundstatus_pos_y.GetInt();

	// Draw Box
	int boxWidth, boxHeight;
	surface()->GetTextSize(m_hFont, L"00:00:00", boxWidth, boxHeight);
	Color box_color = Color(box_color_r, box_color_g, box_color_b, box_color_a);
	DrawNeoHudRoundedBox(xpos - 2 - (boxWidth/2), 0, xpos + 2 + (boxWidth / 2), (boxHeight*2) + 2, box_color, false, false, true, true);

	// Draw time
	int fontWidth, fontHeight;
	surface()->GetTextSize(m_hFont, m_wszStatusUnicode, fontWidth, fontHeight);
	surface()->DrawSetTextColor(Color(255, 255, 255, 255));
	surface()->DrawSetTextPos(xpos - (fontWidth / 2), ypos - (fontHeight / 2));
	surface()->DrawPrintText(m_wszStatusUnicode, sizeof(m_szStatusANSI));

	// Draw round
	surface()->GetTextSize(m_hFont, m_wszRound, fontWidth, fontHeight);
	surface()->DrawSetTextPos(xpos - (fontWidth / 2), (ypos - (fontHeight / 2)) + fontHeight + 2);
	surface()->DrawPrintText(m_wszRound, 2);

	const int localPlayerTeam = GetLocalPlayerTeam();
	const int localPlayerIndex = GetLocalPlayerIndex();
	const int enemyTeam = GetLocalPlayerTeam() == TEAM_JINRAI ? TEAM_NSF : TEAM_JINRAI;

	// Draw score
	surface()->DrawSetTextPos(xpos - (boxWidth/2) + 2, (ypos - (fontHeight / 2)) + fontHeight + 2);
	surface()->DrawSetTextColor(localPlayerTeam == TEAM_JINRAI ? COLOR_JINRAI : COLOR_NSF);
	surface()->DrawPrintText(localPlayerTeam == TEAM_JINRAI ? m_wszJinraiScore : m_wszNSFScore, 2);

	surface()->GetTextSize(m_hFont, localPlayerTeam == TEAM_JINRAI ? m_wszNSFScore : m_wszJinraiScore, fontWidth, fontHeight);
	surface()->DrawSetTextPos((xpos + (boxWidth / 2) - 2) - fontWidth, (ypos - (fontHeight / 2)) + fontHeight + 2);
	surface()->DrawSetTextColor(localPlayerTeam == TEAM_JINRAI ? COLOR_NSF : COLOR_JINRAI);
	surface()->DrawPrintText(localPlayerTeam == TEAM_JINRAI ? m_wszNSFScore : m_wszJinraiScore, 2);

	// Draw players
	if (!g_PR)
		return;

	m_iFriendlyLogo = localPlayerTeam == TEAM_JINRAI ? m_iJinraiID : m_iNSFID;
	m_iFriendlyTotalLogo = localPlayerTeam == TEAM_JINRAI ? m_iJinraiTotalID : m_iNSFTotalID;
	m_iEnemyLogo = localPlayerTeam == TEAM_JINRAI ? m_iNSFID : m_iJinraiID;
	m_iEnemyTotalLogo = localPlayerTeam == TEAM_JINRAI ? m_iNSFTotalID : m_iJinraiTotalID;
	m_ilogoSize = fontHeight * 2;
	m_ilogoTotalSize = m_ilogoSize * 1.25;

	m_iFriendsAlive = 0;
	m_iFriendsTotal = 0;
	m_iEnemiesAlive = 0;
	m_iEnemiesTotal = 0;
	int friendCount = 0;
	int enemyCount = 0;
	for (int i = 0; i < (MAX_PLAYERS + 1); i++) {
		if (g_PR->IsConnected(i)) {
			if (g_PR->GetTeam(i) == localPlayerTeam) {
				if (g_PR->GetStar(i) == g_PR->GetStar(localPlayerIndex)) {
					DrawFriend(i, friendCount, xpos, boxWidth);
					friendCount++;
				}

				if (g_PR->IsAlive(i))
					m_iFriendsAlive++;
				m_iFriendsTotal++;
			}
			else if (g_PR->GetTeam(i) == enemyTeam) {
				DrawEnemy(i, enemyCount, xpos, boxWidth);
				enemyCount++;

				if (g_PR->IsAlive(i))
					m_iEnemiesAlive++;
				m_iEnemiesTotal++;
			}
		}
	}
	g_PR->GetTeam(localPlayerIndex);

	// Draw total players alive
	surface()->DrawSetTexture(m_iFriendlyTotalLogo);
	int xOffset = xpos - (boxWidth / 2) - 4 - m_ilogoTotalSize;
	surface()->DrawSetColor(Color(55, 55, 55, 255));
	surface()->DrawFilledRect(xOffset, 1, xOffset + m_ilogoTotalSize, 1 + m_ilogoTotalSize);
	/*surface()->DrawSetColor(localPlayerTeam == TEAM_JINRAI ? COLOR_JINRAI : COLOR_NSF);
	surface()->DrawFilledRect(xOffset, m_iFriendsTotal > 0 ? 1.0f + ((1.0f - ((float)m_iFriendsAlive / m_iFriendsTotal)) * m_ilogoTotalSize) : 1 + m_ilogoTotalSize, xOffset + m_ilogoTotalSize, 1 + m_ilogoTotalSize);*/
	surface()->DrawTexturedRect(xOffset, 1, xOffset + m_ilogoTotalSize, 1 + m_ilogoTotalSize);

	surface()->DrawSetTexture(m_iEnemyTotalLogo);
	xOffset = xpos + (boxWidth / 2) + 4;
	surface()->DrawSetColor(Color(55, 55, 55, 255));
	surface()->DrawFilledRect(xOffset, 1, xOffset + m_ilogoTotalSize, 1 + m_ilogoTotalSize);
	/*surface()->DrawSetColor(localPlayerTeam == TEAM_JINRAI ? COLOR_NSF : COLOR_JINRAI);
	surface()->DrawFilledRect(xOffset, m_iEnemiesTotal > 0 ? 1.0f + ((1.0f - ((float)m_iEnemiesAlive / m_iEnemiesTotal)) * m_ilogoTotalSize) : 1 + m_ilogoTotalSize, xOffset + m_ilogoTotalSize, 1 + m_ilogoTotalSize);*/
	surface()->DrawTexturedRect(xOffset, 1, xOffset + m_ilogoTotalSize, 1 + m_ilogoTotalSize);
	
	// Draw total players alive numbers
	V_snwprintf(m_wszFriendsAlive, 4, L"%i", m_iFriendsAlive);
	V_snwprintf(m_wszEnemiesAlive, 4, L"%i", m_iEnemiesAlive);
	surface()->GetTextSize(m_hFont, m_wszFriendsAlive, fontWidth, fontHeight);

	surface()->DrawSetTextColor(Color(255, 255, 255, 255));
	surface()->DrawSetTextPos(xpos - (boxWidth / 2) - 4 - (m_ilogoTotalSize / 2) - (fontWidth / 2), 1 + (m_ilogoTotalSize / 2) - (fontHeight / 2));
	surface()->DrawPrintText(m_wszFriendsAlive, 2);
	surface()->GetTextSize(m_hFont, m_wszEnemiesAlive, fontWidth, fontHeight);
	surface()->DrawSetTextPos(xpos + (boxWidth / 2) + 4 + (m_ilogoTotalSize / 2) - (fontWidth / 2), 1 + (m_ilogoTotalSize / 2) - (fontHeight / 2));
	surface()->DrawPrintText(m_wszEnemiesAlive, 2);

	CheckActiveStar();
}

void CNEOHud_RoundState::DrawFriend(int playerIndex, int teamIndex, int xpos, int boxWidth) {
	surface()->DrawSetTexture(m_iFriendlyLogo);
	if (g_PR->GetClass(playerIndex) == NEO_CLASS_SUPPORT)
		surface()->DrawSetTexture(m_iSupportID);
	else if (g_PR->GetClass(playerIndex) == NEO_CLASS_ASSAULT)
		surface()->DrawSetTexture(m_iAssaultID);
	else if (g_PR->GetClass(playerIndex) == NEO_CLASS_RECON)
		surface()->DrawSetTexture(m_iReconID);
	else if (g_PR->GetClass(playerIndex) == NEO_CLASS_VIP)
		surface()->DrawSetTexture(m_iVIPID);

	int xOffset = xpos - (boxWidth / 2) - 6 - (m_ilogoSize * 1.25) - ((teamIndex + 1) * m_ilogoSize) - (teamIndex * 2);
	surface()->DrawSetColor(Color(55, 55, 55, 255));
	surface()->DrawFilledRect(xOffset, 1, xOffset + m_ilogoSize, 1 + m_ilogoSize);
	if (g_PR->IsAlive(playerIndex))
		surface()->DrawSetColor(Color(255, 255, 255, 176));
		surface()->DrawFilledRect(xOffset, 1.0f + ((1.0f - (g_PR->GetHealth(playerIndex) / 100.0f)) * m_ilogoSize), xOffset + m_ilogoSize, 1 + m_ilogoSize);
	surface()->DrawSetColor(Color(0, 0, 0, 176));
	surface()->DrawTexturedRect(xOffset, 1, xOffset + m_ilogoSize, 1 + m_ilogoSize);
}

void CNEOHud_RoundState::DrawEnemy(int playerIndex, int teamIndex, int xpos, int boxWidth) {
	surface()->DrawSetTexture(m_iEnemyLogo);
	int xOffset = xpos + (boxWidth / 2) + 6 + (m_ilogoSize * 1.25) + (teamIndex * m_ilogoSize) + (teamIndex * 2);
	if (g_PR->IsAlive(playerIndex)) {
		surface()->DrawSetColor(Color(255, 255, 255, 176));
		surface()->DrawFilledRect(xOffset, 1, xOffset + m_ilogoSize, 1 + m_ilogoSize);
	}
	else {
		surface()->DrawSetColor(Color(55, 55, 55, 255));
		surface()->DrawFilledRect(xOffset, 1, xOffset + m_ilogoSize, 1 + m_ilogoSize);
	}
	surface()->DrawTexturedRect(xOffset, 1, xOffset + m_ilogoSize, 1 + m_ilogoSize);
}

void CNEOHud_RoundState::CheckActiveStar()
{
	auto player = C_NEO_Player::GetLocalNEOPlayer();
	Assert(player);

	const int currentStar = player->GetStar();
	const int currentTeam = player->GetTeamNumber();

	if (m_iPreviouslyActiveStar == currentStar && m_iPreviouslyActiveTeam == currentTeam)
	{
		return;
	}

	starNone->SetVisible(false);
	starA->SetVisible(false);
	starB->SetVisible(false);
	starC->SetVisible(false);
	starD->SetVisible(false);
	starE->SetVisible(false);
	starF->SetVisible(false);

	if (currentTeam != TEAM_JINRAI && currentTeam != TEAM_NSF)
	{
		return;
	}

	vgui::ImagePanel* target;

	switch (currentStar) {
	case STAR_ALPHA:
		target = starA;
		break;
	case STAR_BRAVO:
		target = starB;
		break;
	case STAR_CHARLIE:
		target = starC;
		break;
	case STAR_DELTA:
		target = starD;
		break;
	case STAR_ECHO:
		target = starE;
		break;
	case STAR_FOXTROT:
		target = starF;
		break;
	default:
		target = starNone;
		break;
	}

	target->SetVisible(true);
	target->SetDrawColor(currentStar == STAR_NONE ? COLOR_NEO_WHITE : currentTeam == TEAM_NSF ? COLOR_NSF : COLOR_JINRAI);

	m_iPreviouslyActiveStar = currentStar;
	m_iPreviouslyActiveTeam = currentTeam;
}

void CNEOHud_RoundState::Paint()
{
	BaseClass::Paint();
	PaintNeoElement();
}
