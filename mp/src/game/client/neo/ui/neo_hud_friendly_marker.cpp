#include "cbase.h"
#include "neo_hud_friendly_marker.h"

#include "iclientmode.h"
#include <vgui/ILocalize.h>
#include <vgui/ISurface.h>
#include <vgui_controls/Controls.h>
#include <vgui_controls/ImagePanel.h>
#include <vgui/IScheme.h>

#include <engine/ivdebugoverlay.h>
#include "ienginevgui.h"

#include "neo_gamerules.h"

#include "c_neo_player.h"

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

using vgui::surface;

ConVar neo_friendly_marker_hud_scale_factor("neo_friendly_marker_hud_scale_factor", "0.5", FCVAR_USERINFO,
	"Friendly player marker HUD element scaling factor", true, 0.01, false, 0);

CNEOHud_FriendlyMarker::CNEOHud_FriendlyMarker(const char* pElemName, vgui::Panel* parent)
	: CHudElement(pElemName), Panel(parent, pElemName)
{
	m_pOwner = NULL;

	m_iPosX = m_iPosY = 0;

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
	surface()->DrawSetTextureFile(m_hTex, "vgui/hud/star", 1, false);

	surface()->DrawGetTextureSize(m_hTex, m_iMarkerTexWidth, m_iMarkerTexHeight);

	SetVisible(true);
}

void CNEOHud_FriendlyMarker::Paint()
{
	SetFgColor(Color(0, 0, 0, 0));
	SetBgColor(Color(0, 0, 0, 0));

	BaseClass::Paint();

	if (!m_pOwner)
	{
		return;
	}

	const Color teamColor = (m_pOwner->GetTeamNumber() == TEAM_NSF) ?
		Color(20, 40, 200, 200) : Color(20, 200, 40, 200);

	surface()->DrawSetTextFont(m_hFont);
	surface()->DrawSetTextColor(teamColor);

	surface()->DrawSetColor(teamColor);
	surface()->DrawSetTexture(m_hTex);

	const float scale = neo_friendly_marker_hud_scale_factor.GetFloat();

	for (int i = 0; i < ARRAYSIZE(m_pOwner->m_rvFriendlyPlayerPositions); i++)
	{
		Vector friendlyPos = m_pOwner->m_rvFriendlyPlayerPositions[i];

		if (friendlyPos == vec3_origin)
		{
			continue;
		}

		const Vector heightOffset(0, 0, 48);

		Vector absPos;
		VectorAbs(friendlyPos, absPos);

		if (absPos.Length() < 1)
		{
			continue;
		}

		friendlyPos += heightOffset;

		int x, y;
		GetVectorInScreenSpace(friendlyPos, x, y);

		const int offset_X = x - ((m_iMarkerTexWidth / 2) * scale);
		const int offset_Y = y - ((m_iMarkerTexHeight / 2) * scale);

		surface()->DrawTexturedRect(
			offset_X,
			offset_Y,
			offset_X + (m_iMarkerTexWidth * scale),
			offset_Y + (m_iMarkerTexHeight * scale));
	}
}

void CNEOHud_FriendlyMarker::SetOwner(C_NEO_Player* player)
{
	m_pOwner = player;
}

C_NEO_Player* CNEOHud_FriendlyMarker::GetOwner() const
{
	return m_pOwner;
}
