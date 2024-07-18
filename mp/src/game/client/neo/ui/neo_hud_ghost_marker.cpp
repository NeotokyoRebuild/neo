#include "cbase.h"
#include "neo_hud_ghost_marker.h"

#include "neo_gamerules.h"
#include "neo_player_shared.h"

#include "iclientmode.h"
#include <vgui/ILocalize.h>
#include <vgui/ISurface.h>
#include <vgui_controls/Controls.h>
#include <vgui_controls/ImagePanel.h>
#include <vgui/IScheme.h>

#include <engine/ivdebugoverlay.h>
#include "ienginevgui.h"

// memdbgon must be the last include file in a .cpp file!!!
#include "neo_hud_worldpos_marker.h"
#include "tier0/memdbgon.h"

using vgui::surface;

ConVar neo_ghost_marker_hud_scale_factor("neo_ghost_marker_hud_scale_factor", "0.5", FCVAR_USERINFO,
	"Ghost marker HUD element scaling factor", true, 0.01, false, 0);

DECLARE_NAMED_HUDELEMENT(CNEOHud_GhostMarker, neo_ghost_marker);

NEO_HUD_ELEMENT_DECLARE_FREQ_CVAR(GhostMarker, 0.01)

CNEOHud_GhostMarker::CNEOHud_GhostMarker(const char* pElemName, vgui::Panel* parent)
	: CNEOHud_WorldPosMarker(pElemName, parent)
{
	m_flDistMeters = 0;

	m_iGhostingTeam = TEAM_UNASSIGNED;
	m_iClientTeam = TEAM_UNASSIGNED;
	m_iPosX = m_iPosY = 0;

	{
		int i;
		for (i = 0; i < sizeof(m_szMarkerText) - 1; ++i)
		{
			m_szMarkerText[i] = ' ';
		}
		m_szMarkerText[i] = '\0';
	}

	g_pVGuiLocalize->ConvertANSIToUnicode(m_szMarkerText, m_wszMarkerTextUnicode, sizeof(m_wszMarkerTextUnicode));

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

	int wide, tall;
	surface()->GetScreenSize(wide, tall);
	SetBounds(0, 0, wide, tall);

	// NEO HACK (Rain): this is kind of awkward, we should get the handle on ApplySchemeSettings
	vgui::IScheme *scheme = vgui::scheme()->GetIScheme(neoscheme);
	Assert(scheme);

	m_hFont = scheme->GetFont("NHudOCRSmall", true);

	m_hTex = surface()->CreateNewTextureID();
	Assert(m_hTex > 0);
	surface()->DrawSetTextureFile(m_hTex, "vgui/hud/ctg/g_beacon_circle", 1, false);

	surface()->DrawGetTextureSize(m_hTex, m_iMarkerTexWidth, m_iMarkerTexHeight);

	SetVisible(false);

	SetFgColor(Color(0, 0, 0, 0));
	SetBgColor(Color(0, 0, 0, 0));
}

void CNEOHud_GhostMarker::UpdateStateForNeoHudElementDraw()
{
	V_snprintf(m_szMarkerText, sizeof(m_szMarkerText), "GHOST DISTANCE: %.0fm", m_flDistMeters);
	g_pVGuiLocalize->ConvertANSIToUnicode(m_szMarkerText, m_wszMarkerTextUnicode, sizeof(m_wszMarkerTextUnicode));
}

void CNEOHud_GhostMarker::DrawNeoHudElement()
{
	if (!ShouldDraw() || NEORules()->IsRoundOver())
	{
		return;
	}

	bool hideText = false;
	Color ghostColor = COLOR_GREY;
	if (m_iGhostingTeam == TEAM_JINRAI || m_iGhostingTeam == TEAM_NSF)
	{
		if ((m_iClientTeam == TEAM_JINRAI || m_iClientTeam == TEAM_NSF) && (m_iClientTeam != m_iGhostingTeam))
		{
			// If viewing from playing player, but opposite of ghosting team, show red
			ghostColor = COLOR_RED;
		}
		else
		{
			// Otherwise show ghosting team color (if friendly or spec)
			ghostColor = (m_iGhostingTeam == TEAM_JINRAI) ? COLOR_JINRAI : COLOR_NSF;

			// Use the friendly HUD text for distance display instead if spectator team or same team
			hideText = (m_iClientTeam < FIRST_GAME_TEAM || m_iGhostingTeam == m_iClientTeam);
		}
	}

	const float fadeMultiplier = GetFadeValueTowardsScreenCentre(m_iPosX, m_iPosY);
	if (!hideText && fadeMultiplier > 0.001f)
	{
		auto adjustedGrey = Color(COLOR_GREY.r(), COLOR_GREY.b(), COLOR_GREY.g(), COLOR_GREY.a() * fadeMultiplier);
	
		surface()->DrawSetTextColor(adjustedGrey);
		surface()->DrawSetTextFont(m_hFont);
		int textSizeX, textSizeY;
		surface()->GetTextSize(m_hFont, m_wszMarkerTextUnicode, textSizeX, textSizeY);
		surface()->DrawSetTextPos(m_iPosX - (textSizeX / 2), m_iPosY + (2 * textSizeY));
		surface()->DrawPrintText(m_wszMarkerTextUnicode, sizeof(m_szMarkerText));
	}

	const float scale = neo_ghost_marker_hud_scale_factor.GetFloat();

	// The increase and decrease in alpha over 6 seconds
	float alpha6 = remainder(gpGlobals->curtime, 6) + 3;
	alpha6 = alpha6 / 6;
	if (alpha6 > 0.5)
		alpha6 = 1 - alpha6;
	alpha6 = 128 * alpha6;

	for (int i = 0; i < 4; i++) {
		m_fMarkerScalesCurrent[i] = (remainder(gpGlobals->curtime, 2) / 2) + 0.5 + m_fMarkerScalesStart[i];
		if (m_fMarkerScalesCurrent[i] > 1)
			m_fMarkerScalesCurrent[i] -= 1;

		const int offset_X = m_iPosX - ((m_iMarkerTexWidth * 0.5f * m_fMarkerScalesCurrent[i]) * scale);
		const int offset_Y = m_iPosY - ((m_iMarkerTexHeight * 0.5f * m_fMarkerScalesCurrent[i]) * scale);

		int alpha = 64 + alpha6;
		if (m_fMarkerScalesCurrent[i] > 0.5)
			alpha *= ((0.5 - (m_fMarkerScalesCurrent[i] - 0.5)) * 2);

		ghostColor[3] = alpha;

		surface()->DrawSetColor(ghostColor);
		surface()->DrawSetTexture(m_hTex);
		surface()->DrawTexturedRect(
			offset_X,
			offset_Y,
			offset_X + (m_iMarkerTexWidth * m_fMarkerScalesCurrent[i] * scale),
			offset_Y + (m_iMarkerTexHeight * m_fMarkerScalesCurrent[i] * scale));
	}

}

void CNEOHud_GhostMarker::Paint()
{
	SetFgColor(COLOR_TRANSPARENT);
	SetBgColor(COLOR_TRANSPARENT);
	BaseClass::Paint();
	PaintNeoElement();
}

void CNEOHud_GhostMarker::SetGhostingTeam(int team)
{
	m_iGhostingTeam = team;
}

void CNEOHud_GhostMarker::SetClientCurrentTeam(int team)
{
	m_iClientTeam = team;
}

void CNEOHud_GhostMarker::SetScreenPosition(int x, int y)
{
	m_iPosX = x;
	m_iPosY = y;
}

void CNEOHud_GhostMarker::SetGhostDistance(float distance)
{
	m_flDistMeters = distance;
}
