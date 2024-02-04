#include "cbase.h"
#include "neo_hud_round_state.h"

#include "iclientmode.h"
#include "ienginevgui.h"

#include <vgui/ILocalize.h>
#include <vgui/ISurface.h>

#include "neo_gamerules.h"

#include <vgui_controls/ImagePanel.h>

#include "c_neo_player.h"

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
	SetBounds(0, 0, m_resX, m_resY);

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

	const vgui::ImagePanel *stars[] = { starNone, starA, starB, starC, starD, starE, starF };

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

	V_sprintf_safe(m_szStatusANSI, "%02d:%02d", minutes, secsRemainder);
	g_pVGuiLocalize->ConvertANSIToUnicode(m_szStatusANSI, m_wszStatusUnicode, sizeof(m_wszStatusUnicode));
}

void CNEOHud_RoundState::DrawNeoHudElement()
{
	if (!ShouldDraw())
	{
		return;
	}

	surface()->DrawSetTextFont(m_hFont);

	int fontWidth, fontHeight;
	surface()->GetTextSize(m_hFont, m_wszStatusUnicode, fontWidth, fontHeight);

	const int xpos = (m_resX / 2) + neo_cl_hud_roundstatus_pos_x.GetInt();
	const int ypos = (m_resY * 0.01) + neo_cl_hud_roundstatus_pos_y.GetInt();

	surface()->DrawSetTextColor(Color(255, 255, 255, 255));
	surface()->DrawSetTextPos(xpos - (fontWidth / 2), ypos - (fontHeight / 2));
	surface()->DrawPrintText(m_wszStatusUnicode, sizeof(m_szStatusANSI));

	CheckActiveStar();
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
