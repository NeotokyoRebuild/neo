#include "neo_hud_ghost_cap_point.h"

#include "ienginevgui.h"
#include "iclientmode.h"
#include <vgui/ISurface.h>
#include <vgui/ILocalize.h>

#include "c_neo_player.h"
#include "neo_gamerules.h"

ConVar neo_ghost_cap_point_hud_scale_factor("neo_ghost_cap_point_hud_scale_factor", "1", FCVAR_ARCHIVE,
	"Ghost cap HUD element scaling factor", true, 0.01, false, 0);
ConVar cl_neo_hud_center_ghost_cap_size("cl_neo_hud_center_ghost_cap_size", "12.5", FCVAR_ARCHIVE,
	"HUD center size in percentage to fade ghost cap point.", true, 1, false, 0);

NEO_HUD_ELEMENT_DECLARE_FREQ_CVAR(GhostCapPoint, 0.01)

CNEOHud_GhostCapPoint::CNEOHud_GhostCapPoint(const char *pElementName, vgui::Panel *parent)
	: CNEOHud_WorldPosMarker(pElementName, parent)
{
	SetAutoDelete(true);
	m_iHideHudElementNumber = NEO_HUD_ELEMENT_GHOST_CAP_POINTS;

	if (parent)
	{
		SetParent(parent);
	}
	else
	{
		SetParent(g_pClientMode->GetViewport());
	}

	vgui::surface()->GetScreenSize(m_iPosX, m_iPosY);
	SetBounds(0, 0, m_iPosX, m_iPosY);

	m_hCapTex = vgui::surface()->CreateNewTextureID();
	Assert(m_hCapTex > 0);
	vgui::surface()->DrawSetTextureFile(m_hCapTex, "vgui/hud/ctg/g_beacon_arrow_down", 1, false);

	SetVisible(false);
}

void CNEOHud_GhostCapPoint::ApplySchemeSettings(vgui::IScheme *pScheme)
{
	BaseClass::ApplySchemeSettings(pScheme);

	m_hFont = pScheme->GetFont("NHudOCRSmall");

	vgui::surface()->GetScreenSize(m_iPosX, m_iPosY);
	SetBounds(0, 0, m_iPosX, m_iPosY);

	// Override CNEOHud_WorldPosMarker's sizing with our own
	const int widerAxis = max(m_viewWidth, m_viewHeight);
	m_viewCentreSize = widerAxis * (cl_neo_hud_center_ghost_cap_size.GetFloat() / 100.0f);
}

extern ConVar cl_neo_hud_worldpos_verbose;
void CNEOHud_GhostCapPoint::UpdateStateForNeoHudElementDraw()
{
	auto *player = C_NEO_Player::GetLocalNEOPlayer();
	if (!player) return;

	m_flDistance = METERS_PER_INCH * player->GetAbsOrigin().DistTo(m_vecMyPos);
	if (cl_neo_hud_worldpos_verbose.GetBool())
	{
		V_snwprintf(m_wszMarkerTextUnicode, ARRAYSIZE(m_wszMarkerTextUnicode), L"RETRIEVAL ZONE DISTANCE: %.0f m", m_flDistance);
	}
	else
	{
		V_snwprintf(m_wszMarkerTextUnicode, ARRAYSIZE(m_wszMarkerTextUnicode), L"%.0f m", m_flDistance);
	}
}

void CNEOHud_GhostCapPoint::DrawNeoHudElement()
{
	if (!ShouldDraw() || NEORules()->IsRoundOver() || !NEORules()->IsTeamplay())
	{
		return;
	}

	auto *player = C_NEO_Player::GetLocalNEOPlayer();
	const int playerTeam = player->GetTeamNumber();

	Color targetColor = (m_iCapTeam == TEAM_JINRAI) ? COLOR_JINRAI : COLOR_NSF;

	const bool playerIsPlaying = (playerTeam == TEAM_JINRAI || playerTeam == TEAM_NSF);
	if (playerIsPlaying)
	{
		const bool targetIsToDefend = (playerTeam != m_iCapTeam);
		if (targetIsToDefend)
		{
			static const Color COLORCAP_RED(255, 0, 0, 255);
			targetColor = COLORCAP_RED;
		}
	}

	int x, y;
	GetVectorInScreenSpace(m_vecMyPos, x, y);

	const float scale = neo_ghost_cap_point_hud_scale_factor.GetFloat();

	constexpr float HALF_BASE_TEX_LENGTH = 64;
	if (playerIsPlaying)
	{
		const float fadeTextMultiplier = GetFadeValueTowardsScreenCentre(x, y);
		if (m_flDistance > 0.2f && fadeTextMultiplier > 0.001f)
		{
			int xWide = 0;
			int yTall = 0;
			vgui::surface()->GetTextSize(m_hFont, m_wszMarkerTextUnicode, xWide, yTall);
			vgui::surface()->DrawSetColor(COLOR_TRANSPARENT);
			vgui::surface()->DrawSetTextColor(FadeColour(COLOR_TINTGREY, fadeTextMultiplier));
			vgui::surface()->DrawSetTextFont(m_hFont);
			vgui::surface()->DrawSetTextPos(x - (xWide / 2), y + (HALF_BASE_TEX_LENGTH * scale));
			vgui::surface()->DrawPrintText(m_wszMarkerTextUnicode, V_wcslen(m_wszMarkerTextUnicode));
		}
	}

	// The 4 arrows that slowly expand and disappear
	vgui::surface()->DrawSetTexture(m_hCapTex);
	for (int i = 0; i < 4; i++) {
		m_fMarkerScalesCurrent[i] = (remainder(gpGlobals->curtime, 2) / 2) + 0.5 + m_fMarkerScalesStart[i];
		if (m_fMarkerScalesCurrent[i] > 1)
			m_fMarkerScalesCurrent[i] -= 1;

		int alpha = 32;
		if (m_fMarkerScalesCurrent[i] > 0.5)
			alpha *= ((0.5 - (m_fMarkerScalesCurrent[i] - 0.5)) * 2);

		targetColor[3] = alpha;
		vgui::surface()->DrawSetColor(targetColor);

		const float halfArrowLength = HALF_BASE_TEX_LENGTH * m_fMarkerScalesCurrent[i] * scale;
		vgui::surface()->DrawTexturedRect(
			x - halfArrowLength,
			y - halfArrowLength,
			x + halfArrowLength,
			y + halfArrowLength);
	}

	// The increase and decrease in alpha over 6 seconds
	float alpha6 = remainder(gpGlobals->curtime, 6) + 3;
	alpha6 = alpha6 / 6;
	if (alpha6 > 0.5)
		alpha6 = 1 - alpha6;
	alpha6 = 255 * alpha6;

	targetColor[3] = alpha6;

	// The main arrow that stays the same size but dissappears and reappears on a 6 second loop
	const float halfArrowLength = HALF_BASE_TEX_LENGTH * scale;
	vgui::surface()->DrawSetColor(targetColor);
	vgui::surface()->DrawTexturedRect(
		x - halfArrowLength,
		y - halfArrowLength,
		x + halfArrowLength,
		y + halfArrowLength);
}

void CNEOHud_GhostCapPoint::Paint()
{
	SetFgColor(COLOR_TRANSPARENT);
	SetBgColor(COLOR_TRANSPARENT);
	BaseClass::Paint();
	PaintNeoElement();
}
