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
#include "tier0/memdbgon.h"

using vgui::surface;

ConVar neo_ghost_marker_hud_scale_factor("neo_ghost_marker_hud_scale_factor", "0.125", FCVAR_USERINFO,
	"Ghost marker HUD element scaling factor", true, 0.01, false, 0);

NEO_HUD_ELEMENT_DECLARE_FREQ_CVAR(GhostMarker, 0.01)

CNEOHud_GhostMarker::CNEOHud_GhostMarker(const char* pElemName, vgui::Panel* parent)
	: CHudElement(pElemName), Panel(parent, pElemName)
{
	m_flDistMeters = 0;

	m_iGhostingTeam = TEAM_UNASSIGNED;
	m_iPosX = m_iPosY = 0;

	m_szMarkerText[0] = '\0';
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
	V_snprintf(m_szMarkerText, sizeof(m_szMarkerText), "GHOST %.0f M", m_flDistMeters);
	g_pVGuiLocalize->ConvertANSIToUnicode(m_szMarkerText, m_wszMarkerTextUnicode, sizeof(m_wszMarkerTextUnicode));
}

void CNEOHud_GhostMarker::DrawNeoHudElement()
{
	if (!ShouldDraw())
	{
		return;
	}

	surface()->DrawSetTextColor(COLOR_GREY);
	surface()->DrawSetTextFont(m_hFont);
	surface()->DrawSetTextPos(m_iPosX, m_iPosY);
	surface()->DrawPrintText(m_wszMarkerTextUnicode, sizeof(m_szMarkerText));

	const float scale = neo_ghost_marker_hud_scale_factor.GetFloat();

	const int offset_X = m_iPosX - ((m_iMarkerTexWidth * 0.5f) * scale);
	const int offset_Y = m_iPosY - ((m_iMarkerTexHeight * 0.5f) * scale);

	surface()->DrawSetColor(m_iGhostingTeam == TEAM_JINRAI ? COLOR_JINRAI : (m_iGhostingTeam == TEAM_NSF ? COLOR_NSF : COLOR_GREY));
	surface()->DrawSetTexture(m_hTex);
	surface()->DrawTexturedRect(
		offset_X,
		offset_Y,
		offset_X + (m_iMarkerTexWidth * scale),
		offset_Y + (m_iMarkerTexHeight * scale));
}

void CNEOHud_GhostMarker::Paint()
{
	BaseClass::Paint();
	PaintNeoElement();
}

void CNEOHud_GhostMarker::SetGhostingTeam(int team)
{
	m_iGhostingTeam = team;
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
