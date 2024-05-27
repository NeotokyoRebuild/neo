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
#include "c_team.h"
#include "tier0/memdbgon.h"

using vgui::surface;

ConVar neo_friendly_marker_hud_scale_factor("neo_friendly_marker_hud_scale_factor", "0.5", FCVAR_USERINFO,
	"Friendly player marker HUD element scaling factor", true, 0.01, false, 0);

NEO_HUD_ELEMENT_DECLARE_FREQ_CVAR(FriendlyMarker, 0.01)

void CNEOHud_FriendlyMarker::UpdateStateForNeoHudElementDraw()
{
}

CNEOHud_FriendlyMarker::CNEOHud_FriendlyMarker(const char* pElemName, vgui::Panel* parent)
	: CNEOHud_WorldPosMarker(pElemName, parent)
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

	SetFgColor(Color(0, 0, 0, 0));
	SetBgColor(Color(0, 0, 0, 0));

	SetVisible(true);
}
void CNEOHud_FriendlyMarker::DrawNeoHudElement()
{
	if (!ShouldDraw())
	{
		return;
	}

	SetFgColor(Color(0, 0, 0, 0));
	SetBgColor(Color(0, 0, 0, 0));

	const float scale = neo_friendly_marker_hud_scale_factor.GetFloat();

	m_iMarkerWidth = (m_iMarkerTexWidth * 0.5f) * scale;
	m_iMarkerHeight = (m_iMarkerTexHeight * 0.5f) * scale;	

	auto localPlayer = C_NEO_Player::GetLocalNEOPlayer();
	auto team = localPlayer->GetTeam();
	m_IsSpectator = team->GetTeamNumber() == TEAM_SPECTATOR;
	
	
	if(m_IsSpectator)
	{
		auto nsf = GetGlobalTeam(TEAM_NSF);
		DrawPlayerForTeam(nsf, localPlayer);
		
		auto jinrai = GetGlobalTeam(TEAM_JINRAI);
		DrawPlayerForTeam(jinrai, localPlayer);
	} else
	{
		DrawPlayerForTeam(team, localPlayer);
	}
}

void CNEOHud_FriendlyMarker::DrawPlayerForTeam(C_Team* team, const C_NEO_Player* localPlayer) const
{
	auto memberCount = team->GetNumPlayers();
	for (int i = 0; i < memberCount; ++i)
	{
		auto player = team->GetPlayer(i);
		auto teamColour = GetTeamColour(team->GetTeamNumber());
		if(player && localPlayer->entindex() != player->entindex() && player->IsAlive())
		{
			DrawPlayer(teamColour, player);
		}		
	}
}

void CNEOHud_FriendlyMarker::DrawPlayer(Color teamColor, C_BasePlayer* player) const
{
	int x, y;
	static const float heightOffset = 48.0f;
	auto pPos = player->GetAbsOrigin();
	auto pos = Vector(
		pPos.x,
		pPos.y,
		pPos.z + heightOffset);

	if (GetVectorInScreenSpace(pos, x, y))
	{
		auto n = dynamic_cast<C_NEO_Player*>(player);
		auto a = n->m_rvFriendlyPlayerPositions;
		static const int maxNameLenght = 32 + 1;		
		auto playerName = player->GetPlayerName();

		wchar_t playerNameUnicode[maxNameLenght];
		char playerNameTrimmed[maxNameLenght];
		snprintf(playerNameTrimmed, maxNameLenght, "%s", playerName);
		auto textLen = V_strlen(playerNameTrimmed);
		g_pVGuiLocalize->ConvertANSIToUnicode(playerNameTrimmed, playerNameUnicode, sizeof(playerNameUnicode));

		auto fadeTextMultiplier = GetFadeValueTowardsScreenCentre(x, y);
		if(fadeTextMultiplier > 0.001)
		{
			surface()->DrawSetTextFont(m_hFont);
			surface()->DrawSetTextColor(FadeColour(teamColor, fadeTextMultiplier));
			int textWidth, textHeight;
			surface()->GetTextSize(m_hFont, playerNameUnicode, textWidth, textHeight);
			surface()->DrawSetTextPos(x - (textWidth / 2), y + m_iMarkerHeight);
			surface()->DrawPrintText(playerNameUnicode, textLen);
		}

		surface()->DrawSetTexture(m_hTex);
		surface()->DrawSetColor(teamColor);
		surface()->DrawTexturedRect(
			x - m_iMarkerWidth,
			y - m_iMarkerHeight,
			x + m_iMarkerWidth,
			y + m_iMarkerHeight
		);
	}
}

Color CNEOHud_FriendlyMarker::GetTeamColour(int team)
{
	return (team == TEAM_NSF) ? COLOR_NSF : COLOR_JINRAI;
}

void CNEOHud_FriendlyMarker::Paint()
{
	BaseClass::Paint();
	PaintNeoElement();
}
