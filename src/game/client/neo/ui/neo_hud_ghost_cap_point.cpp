#include "neo_hud_ghost_cap_point.h"

#include "ienginevgui.h"
#include "iclientmode.h"
#include <vgui/ISurface.h>
#include <vgui/ILocalize.h>

#include "c_neo_player.h"
#include "neo_gamerules.h"

ConVar neo_ghost_cap_point_hud_scale_factor("neo_ghost_cap_point_hud_scale_factor", "0.33", FCVAR_USERINFO,
	"Ghost cap HUD element scaling factor", true, 0.01, false, 0);
ConVar neo_cl_hud_center_ghost_cap_size("neo_cl_hud_center_ghost_cap_size", "12.5", FCVAR_USERINFO,
	"HUD center size in percentage to fade ghost cap point.", true, 1, false, 0);

NEO_HUD_ELEMENT_DECLARE_FREQ_CVAR(GhostCapPoint, 0.01)

CNEOHud_GhostCapPoint::CNEOHud_GhostCapPoint(const char *pElementName, vgui::Panel *parent)
	: CNEOHud_WorldPosMarker(pElementName, parent)
{
	SetAutoDelete(true);

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
	vgui::surface()->DrawGetTextureSize(m_hCapTex, m_iCapTexWidth, m_iCapTexHeight);

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
	m_viewCentreSize = widerAxis * (neo_cl_hud_center_ghost_cap_size.GetFloat() / 100.0f);
}

void CNEOHud_GhostCapPoint::UpdateStateForNeoHudElementDraw()
{
	auto *player = C_NEO_Player::GetLocalNEOPlayer();
	if (!player) return;

	const bool playerIsPlaying = (player->GetTeamNumber() == TEAM_JINRAI || player->GetTeamNumber() == TEAM_NSF);
	if (playerIsPlaying)
	{
		m_flDistance = METERS_PER_INCH * player->GetAbsOrigin().DistTo(m_vecMyPos);
		V_snprintf(m_szMarkerText, sizeof(m_szMarkerText), "RETRIEVAL ZONE DISTANCE: %.0f m", m_flDistance);
		g_pVGuiLocalize->ConvertANSIToUnicode(m_szMarkerText, m_wszMarkerTextUnicode, sizeof(m_wszMarkerTextUnicode));
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

	const int offset_X = x - ((m_iCapTexWidth / 2) * scale);
	const int offset_Y = y - ((m_iCapTexHeight / 2) * scale);

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
			vgui::surface()->DrawSetTextPos(x - (xWide / 2), offset_Y + (m_iCapTexHeight * scale) + (yTall / 2));
			vgui::surface()->DrawPrintText(m_wszMarkerTextUnicode, sizeof(m_szMarkerText));
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

		const int offset_X2 = x - ((m_iCapTexWidth / 2) * m_fMarkerScalesCurrent[i] * scale);
		const int offset_Y2 = y - ((m_iCapTexHeight / 2) * m_fMarkerScalesCurrent[i] * scale);

		vgui::surface()->DrawTexturedRect(
			offset_X2,
			offset_Y2,
			offset_X2 + (m_iCapTexWidth * m_fMarkerScalesCurrent[i] * scale),
			offset_Y2 + (m_iCapTexHeight * m_fMarkerScalesCurrent[i] * scale));
	}

	// The increase and decrease in alpha over 6 seconds
	float alpha6 = remainder(gpGlobals->curtime, 6) + 3;
	alpha6 = alpha6 / 6;
	if (alpha6 > 0.5)
		alpha6 = 1 - alpha6;
	alpha6 = 255 * alpha6;

	targetColor[3] = alpha6;

	// The main arrow that stays the same size but dissappears and reappears on a 6 second loop
	vgui::surface()->DrawSetColor(targetColor);
	vgui::surface()->DrawTexturedRect(
		offset_X,
		offset_Y,
		offset_X + (m_iCapTexWidth * scale),
		offset_Y + (m_iCapTexHeight * scale));
}

void CNEOHud_GhostCapPoint::Paint()
{
	SetFgColor(COLOR_TRANSPARENT);
	SetBgColor(COLOR_TRANSPARENT);
	BaseClass::Paint();
	PaintNeoElement();
}
