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

	// Screen dimensions
	surface()->GetScreenSize(m_resX, m_resY);
	m_iXpos = (m_resX / 2);
	m_iYpos = (m_resY * 0.01);

	// Box dimensions
	surface()->GetTextSize(m_hFont, L"00:00:00", m_iBoxWidth, m_iBoxHeight);
	m_iBoxHeight = (m_iBoxHeight * 2);
	m_iBoxWidth += 2;
	m_iBoxX0 = m_iXpos - (m_iBoxWidth / 2);
	m_iBoxY0 = m_iYpos - (m_resY * 0.01);
	m_iBoxX1 = m_iXpos + (m_iBoxWidth / 2);
	m_iBoxY1 = m_iYpos - (m_resY * 0.005) + m_iBoxHeight;

	// Logo dimensions
	m_ilogoSize = m_iBoxHeight;
	m_ilogoTotalSize = m_iBoxY1;

	// Time positions
	m_iTimeYpos = m_iYpos - (m_iBoxHeight / 4);

	// Score positions
	m_iFriendlyScoreXpos = m_iXpos - (m_iBoxWidth / 2) + 2;
	m_iFriendlyScoreYpos = m_iYpos + (m_iBoxHeight / 4);
	m_iEnemyScoreXpos = m_iXpos + (m_iBoxWidth / 2) - 2;
	m_iEnemyScoreYpos = m_iYpos + (m_iBoxHeight / 4);
	m_iRoundXpos = m_iXpos;
	m_iRoundYpos = m_iYpos + (m_iBoxHeight / 4);

	// Round status position
	m_iRoundStatusYpos = m_iBoxY1;

	// Total positions
	m_iFriendlyTotalLogoX0 = m_iXpos - (m_iBoxWidth / 2) - 2 - m_ilogoTotalSize;
	m_iFriendlyTotalLogoX1 = m_iXpos - (m_iBoxWidth / 2) - 2;
	m_iFriendlyTotalLogoY0 = 1;
	m_iFriendlyTotalLogoY1 = 1 + m_ilogoTotalSize;

	m_iEnemyTotalLogoX0 = m_iXpos + (m_iBoxWidth / 2) + 2;
	m_iEnemyTotalLogoX1 = m_iXpos + (m_iBoxWidth / 2) + 2 + m_ilogoTotalSize;
	m_iEnemyTotalLogoY0 = 1;
	m_iEnemyTotalLogoY1 = 1 + m_ilogoTotalSize;

	// Total num positions
	m_iFriendlyNumXpos = m_iFriendlyTotalLogoX0 + (m_ilogoTotalSize / 2);
	m_iFriendlyNumYpos = m_iFriendlyTotalLogoY0 + (m_ilogoTotalSize / 2) - (m_iBoxHeight / 4);

	m_iEnemyNumXpos = m_iEnemyTotalLogoX0 + (m_ilogoTotalSize / 2);
	m_iEnemyNumYpos = m_iEnemyTotalLogoY0 + (m_ilogoTotalSize / 2) - (m_iBoxHeight / 4);

	// Logos offsets
	m_iFriendlyLogoXOffset = m_iXpos - (m_iBoxWidth / 2) - 4 - m_ilogoTotalSize;
	m_iEnemyLogoXOffset = m_iXpos + (m_iBoxWidth / 2) + 4 + m_ilogoTotalSize;

	SetBounds(0, 0, m_resX, m_iBoxY1 + (m_iBoxHeight / 2));

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

	// Set initial logos
	m_iFriendlyLogo = m_iJinraiID;
	m_iFriendlyTotalLogo = m_iJinraiTotalID;
	m_iEnemyLogo = m_iNSFID;
	m_iEnemyTotalLogo = m_iNSFTotalID;
	friendlyColor = COLOR_JINRAI;
	enemyColor = COLOR_NSF;

	m_iPreviouslyActiveStar = m_iPreviouslyActiveTeam  = -1;

	// register for events as client listener
	ListenForGameEvent("player_team");
	//ListenForGameEvent("player_star");
	//ListenForGameEvent("player_spawn");
	//ListenForGameEvent("player_hurt");
	//ListenForGameEvent("player_death");
	//ListenForGameEvent("player_disconnect");
	ListenForGameEvent("round_start");
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
	const bool inSuddenDeath = NEORules()->RoundIsInSuddenDeath();
	const bool inMatchPoint = NEORules()->RoundIsMatchPoint();

	const char *prefixStr = (roundStatus == NeoRoundStatus::Warmup) ? "(Warmup)" : "";
	if (roundStatus == NeoRoundStatus::Idle) {
		prefixStr = "(Waiting for players)";
	}
	else if (roundStatus == NeoRoundStatus::PreRoundFreeze) {
		prefixStr = "(Pre-round freeze)";
	}
	else if (inSuddenDeath)
	{
		prefixStr = "(Sudden death)";
	}
	else if (inMatchPoint)
	{
		prefixStr = "(Match point)";
	}
	V_sprintf_safe(m_szStatusANSI, "%s", prefixStr);
	memset(m_wszStatusUnicode, 0, sizeof(m_wszStatusUnicode)); // NOTE (nullsystem): Clear it or get junk after warmup ends
	g_pVGuiLocalize->ConvertANSIToUnicode(m_szStatusANSI, m_wszStatusUnicode, sizeof(m_wszStatusUnicode));

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

	const int secsTotal = RoundFloatToInt(roundTimeLeft);
	const int secsRemainder = secsTotal % 60;
	const int minutes = (secsTotal - secsRemainder) / 60;
	V_snwprintf(m_wszTime, 6, L"%02d:%02d", minutes, secsRemainder);
	int localPlayerTeam = GetLocalPlayerTeam();
	if (localPlayerTeam == TEAM_JINRAI || localPlayerTeam == TEAM_NSF) {
		V_snwprintf(m_wszFriendlyScore, 3, L"%i", GetGlobalTeam(GetLocalPlayerTeam())->GetRoundsWon());
		V_snwprintf(m_wszEnemyScore, 3, L"%i", GetGlobalTeam(NEORules()->GetOpposingTeam(GetLocalPlayerTeam()))->GetRoundsWon());
	}
	else {
		V_snwprintf(m_wszFriendlyScore, 3, L"%i", GetGlobalTeam(TEAM_JINRAI)->GetRoundsWon());
		V_snwprintf(m_wszEnemyScore, 3, L"%i", GetGlobalTeam(TEAM_NSF)->GetRoundsWon());
	}
}

void CNEOHud_RoundState::DrawNeoHudElement()
{
	if (!ShouldDraw())
	{
		return;
	}

	surface()->DrawSetTextFont(m_hFont);

	// Draw Box
	Color box_color = Color(box_color_r, box_color_g, box_color_b, box_color_a);
	DrawNeoHudRoundedBox(m_iBoxX0, m_iBoxY0, m_iBoxX1, m_iBoxY1, box_color, false, false, true, true);

	// Draw time
	int fontWidth, fontHeight;
	surface()->GetTextSize(m_hFont, m_wszTime, fontWidth, fontHeight);
	surface()->DrawSetTextColor(whiteColor);
	surface()->DrawSetTextPos(m_iXpos - (fontWidth / 2), m_iTimeYpos);
	surface()->DrawPrintText(m_wszTime, 6);

	// Draw round
	surface()->GetTextSize(m_hFont, m_wszRound, fontWidth, fontHeight);
	surface()->DrawSetTextPos(m_iRoundXpos - (fontWidth / 2), m_iRoundYpos);
	surface()->DrawPrintText(m_wszRound, 2);

	// Draw score
	surface()->DrawSetTextPos(m_iFriendlyScoreXpos, m_iFriendlyScoreYpos);
	surface()->DrawSetTextColor(friendlyColor);
	surface()->DrawPrintText(m_wszFriendlyScore, 2);

	surface()->GetTextSize(m_hFont, m_wszEnemyScore, fontWidth, fontHeight);
	surface()->DrawSetTextPos(m_iEnemyScoreXpos - fontWidth, m_iEnemyScoreYpos);
	surface()->DrawSetTextColor(enemyColor);
	surface()->DrawPrintText(m_wszEnemyScore, 2);

	// Draw round status
	surface()->GetTextSize(m_hFont, m_wszStatusUnicode, fontWidth, fontHeight);
	surface()->DrawSetTextPos(m_iXpos - (fontWidth / 2), m_iRoundStatusYpos);
	surface()->DrawSetTextColor(whiteColor);
	surface()->DrawPrintText(m_wszStatusUnicode, 24);

	const int localPlayerTeam = GetLocalPlayerTeam();
	const int localPlayerIndex = GetLocalPlayerIndex();
	const int enemyTeam = GetLocalPlayerTeam() == TEAM_JINRAI ? TEAM_NSF : TEAM_JINRAI;

	// Draw players
	if (!g_PR)
		return;

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
					DrawFriend(i, friendCount);
					friendCount++;
				}

				if (g_PR->IsAlive(i))
					m_iFriendsAlive++;
				m_iFriendsTotal++;
			}
			else if (g_PR->GetTeam(i) == enemyTeam) {
				DrawEnemy(i, enemyCount);
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
	surface()->DrawSetColor(fadedDarkColor);
	surface()->DrawFilledRect(m_iFriendlyTotalLogoX0, m_iFriendlyTotalLogoY0, m_iFriendlyTotalLogoX1, m_iFriendlyTotalLogoY1);
	surface()->DrawTexturedRect(m_iFriendlyTotalLogoX0, m_iFriendlyTotalLogoY0, m_iFriendlyTotalLogoX1, m_iFriendlyTotalLogoY1);

	surface()->DrawSetTexture(m_iEnemyTotalLogo);
	surface()->DrawFilledRect(m_iEnemyTotalLogoX0, m_iEnemyTotalLogoY0, m_iEnemyTotalLogoX1, m_iEnemyTotalLogoY1);
	surface()->DrawTexturedRect(m_iEnemyTotalLogoX0, m_iEnemyTotalLogoY0, m_iEnemyTotalLogoX1, m_iEnemyTotalLogoY1);
	
	// Draw total players alive numbers
	V_snwprintf(m_wszFriendsAlive, 4, L"%i", m_iFriendsAlive);
	surface()->GetTextSize(m_hFont, m_wszFriendsAlive, fontWidth, fontHeight);
	surface()->DrawSetTextColor(whiteColor);
	surface()->DrawSetTextPos(m_iFriendlyNumXpos - (fontWidth / 2), m_iFriendlyNumYpos);
	surface()->DrawPrintText(m_wszFriendsAlive, 2);

	V_snwprintf(m_wszEnemiesAlive, 4, L"%i", m_iEnemiesAlive);
	surface()->GetTextSize(m_hFont, m_wszEnemiesAlive, fontWidth, fontHeight);
	surface()->DrawSetTextPos(m_iEnemyNumXpos - (fontWidth / 2), m_iEnemyNumYpos);
	surface()->DrawPrintText(m_wszEnemiesAlive, 2);

	CheckActiveStar();
}

void CNEOHud_RoundState::DrawFriend(int playerIndex, int teamIndex) {
	surface()->DrawSetTexture(m_iFriendlyLogo);
	switch (g_PR->GetClass(playerIndex)) {
	case NEO_CLASS_SUPPORT:
		surface()->DrawSetTexture(m_iSupportID);
		break;
	case NEO_CLASS_ASSAULT:
		surface()->DrawSetTexture(m_iAssaultID);
		break;
	case NEO_CLASS_RECON:
		surface()->DrawSetTexture(m_iReconID);
		break;
	case NEO_CLASS_VIP:
		surface()->DrawSetTexture(m_iVIPID);
		break;
	default:
		surface()->DrawSetTexture(m_iFriendlyLogo);
		break;
	}

	int xOffset = m_iFriendlyLogoXOffset - ((teamIndex + 1) * m_ilogoSize) - (teamIndex * 2);
	surface()->DrawSetColor(darkColor);
	surface()->DrawFilledRect(xOffset, 1, xOffset + m_ilogoSize, 1 + m_ilogoSize);
	if (g_PR->IsAlive(playerIndex))
		surface()->DrawSetColor(friendlyColor);
		surface()->DrawFilledRect(xOffset, 1.0f + ((1.0f - (g_PR->GetHealth(playerIndex) / 100.0f)) * m_ilogoSize), xOffset + m_ilogoSize, 1 + m_ilogoSize);
	surface()->DrawSetColor(Color(0, 0, 0, 176));
	surface()->DrawTexturedRect(xOffset, 1, xOffset + m_ilogoSize, 1 + m_ilogoSize);
}

void CNEOHud_RoundState::DrawEnemy(int playerIndex, int teamIndex) {
	surface()->DrawSetTexture(m_iEnemyLogo);
	int xOffset = m_iEnemyLogoXOffset + (teamIndex * m_ilogoSize) + (teamIndex * 2);
	if (g_PR->IsAlive(playerIndex)) {
		surface()->DrawSetColor(enemyColor);
		surface()->DrawFilledRect(xOffset, 1, xOffset + m_ilogoSize, 1 + m_ilogoSize);
	}
	else {
		surface()->DrawSetColor(darkColor);
		surface()->DrawFilledRect(xOffset, 1, xOffset + m_ilogoSize, 1 + m_ilogoSize);
	}
	surface()->DrawTexturedRect(xOffset, 1, xOffset + m_ilogoSize, 1 + m_ilogoSize);
}

void CNEOHud_RoundState::FireGameEvent(IGameEvent* event)
{
	if (Q_strcmp(event->GetName(), "player_team") == 0)
	{ // A player switched teams. If local update logos
		if (event->GetInt("userid") == GetLocalPlayerIndex() + 1) {
			const int newTeam = event->GetInt("team");
			if (newTeam == TEAM_JINRAI) {
				m_iFriendlyLogo = m_iJinraiID;
				m_iFriendlyTotalLogo = m_iJinraiTotalID;
				m_iEnemyLogo = m_iNSFID;
				m_iEnemyTotalLogo = m_iNSFTotalID;
				friendlyColor = COLOR_JINRAI;
				enemyColor = COLOR_NSF;
			}
			else {
				m_iFriendlyLogo = m_iNSFID;
				m_iFriendlyTotalLogo = m_iNSFTotalID;
				m_iEnemyLogo = m_iJinraiID;
				m_iEnemyTotalLogo = m_iJinraiTotalID;
				friendlyColor = COLOR_NSF;
				enemyColor = COLOR_JINRAI;
			}
		}
	} else if (Q_strcmp(event->GetName(), "round_start") == 0)
	{ // New round started
		V_snwprintf(m_wszRound, 3, L"%i", NEORules()->roundNumber());
	}
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
