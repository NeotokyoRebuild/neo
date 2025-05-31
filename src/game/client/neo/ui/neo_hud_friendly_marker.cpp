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
ConVar cl_neo_clantag_friendly_marker_spec_only("cl_neo_clantag_friendly_marker_spec_only", "1", FCVAR_ARCHIVE,
												"Clantags only appear for spectators.", true, 0.0f, true, 1.0f);

DECLARE_NAMED_HUDELEMENT(CNEOHud_FriendlyMarker, neo_iff);

NEO_HUD_ELEMENT_DECLARE_FREQ_CVAR(FriendlyMarker, 0.01)

void CNEOHud_FriendlyMarker::UpdateStateForNeoHudElementDraw()
{
}

CNEOHud_FriendlyMarker::CNEOHud_FriendlyMarker(const char* pElemName, vgui::Panel* parent)
	: CNEOHud_WorldPosMarker(pElemName, parent)
{
	SetAutoDelete(true);
	m_iHideHudElementNumber = NEO_HUD_ELEMENT_FRIENDLY_MARKER;

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

	m_hTex = surface()->CreateNewTextureID();
	Assert(m_hTex > 0);
	surface()->DrawSetTextureFile(m_hTex, "vgui/hud/star", 1, false);
	surface()->DrawGetTextureSize(m_hTex, m_iMarkerTexWidth, m_iMarkerTexHeight);

	SetFgColor(Color(0, 0, 0, 0));
	SetBgColor(Color(0, 0, 0, 0));

	SetVisible(true);
}

void CNEOHud_FriendlyMarker::ApplySchemeSettings(vgui::IScheme *pScheme)
{
	BaseClass::ApplySchemeSettings(pScheme);

	m_hFont = pScheme->GetFont("NHudOCRSmall", true);
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
	const auto *pTargetPlayer = (localPlayer->GetObserverMode() == OBS_MODE_IN_EYE) ?
				dynamic_cast<C_NEO_Player *>(localPlayer->GetObserverTarget()) : nullptr;
	
	if (NEORules()->IsTeamplay())
	{
		if (m_IsSpectator)
		{
			auto nsf = GetGlobalTeam(TEAM_NSF);
			DrawPlayerForTeam(nsf, localPlayer, pTargetPlayer);

			auto jinrai = GetGlobalTeam(TEAM_JINRAI);
			DrawPlayerForTeam(jinrai, localPlayer, pTargetPlayer);
		}
		else
		{
			DrawPlayerForTeam(team, localPlayer, pTargetPlayer);
		}
	}
	else
	{
		if (m_IsSpectator)
		{
			// TODO: They're not really Jinrai/NSF?
			auto nsf = GetGlobalTeam(TEAM_NSF);
			DrawPlayerForTeam(nsf, localPlayer, pTargetPlayer);

			auto jinrai = GetGlobalTeam(TEAM_JINRAI);
			DrawPlayerForTeam(jinrai, localPlayer, pTargetPlayer);
		}
	}
}

void CNEOHud_FriendlyMarker::DrawPlayerForTeam(C_Team *team, const C_NEO_Player *localPlayer,
											   const C_NEO_Player *pTargetPlayer) const
{
	auto memberCount = team->GetNumPlayers();
	auto teamColour = GetTeamColour(team->GetTeamNumber());
	for (int i = 0; i < memberCount; ++i)
	{
		auto player = static_cast<C_NEO_Player *>(team->GetPlayer(i));
		if (player && localPlayer->entindex() != player->entindex() && player->IsAlive())
		{
			if (pTargetPlayer && player->entindex() == pTargetPlayer->entindex())
			{
				// NEO NOTE (nullsystem): Skip this if we're observing this player in first person
				continue;
			}
			DrawPlayer(teamColour, player, localPlayer);
		}		
	}
}

extern ConVar glow_outline_effect_enable;
void CNEOHud_FriendlyMarker::DrawPlayer(Color teamColor, C_NEO_Player *player, const C_NEO_Player *localPlayer) const
{
	int x, y;
	constexpr float HEIGHT_OFFSET = 56.0f;
	auto pos = player->GetAbsOrigin() + Vector(0, 0, HEIGHT_OFFSET);

	bool drawOutline = glow_outline_effect_enable.GetBool();
	Vector offset = drawOutline ? Vector(0, 0, 7) : vec3_origin;
	if (GetVectorInScreenSpace(pos, x, y, &offset))
	{
		const float fadeTextMultiplier = GetFadeValueTowardsScreenCentre(x, y);
		if (fadeTextMultiplier > 0.001f)
		{
			static constexpr int MAX_MARKER_STRLEN = 48 + NEO_MAX_CLANTAG_LENGTH + 1;
			const bool localPlayerAlive = const_cast<C_NEO_Player *>(localPlayer)->IsAlive();
			const bool localPlayerSpec = (localPlayer->GetTeamNumber() < FIRST_GAME_TEAM);

			surface()->DrawSetTextFont(m_hFont);
			surface()->DrawSetTextColor(FadeColour(teamColor, fadeTextMultiplier));
			int textYOffset = drawOutline ? vgui::surface()->GetFontTall(m_hFont) * -2 : 0;
			char textASCII[MAX_MARKER_STRLEN];

			auto DisplayText = [this, &textYOffset, x, y](const char *textASCII, bool drawOutline) {
				wchar_t textUTF[MAX_MARKER_STRLEN];
				g_pVGuiLocalize->ConvertANSIToUnicode(textASCII, textUTF, sizeof(textUTF));
				int textWidth, textHeight;
				surface()->GetTextSize(m_hFont, textUTF, textWidth, textHeight);
				surface()->DrawSetTextPos(x - (textWidth / 2), y + (drawOutline ? 0 : m_iMarkerHeight) + textYOffset);
				surface()->DrawPrintText(textUTF, V_wcslen(textUTF));
				textYOffset += textHeight;
			};

			// Draw player's name and health
			auto playerName = player->GetNeoPlayerName();

			// Only show clan tag if spectator/no playing and this player has a clantag
			const char *playerClantag = player->GetNeoClantag();
			if (playerClantag && playerClantag[0] &&
					(!cl_neo_clantag_friendly_marker_spec_only.GetBool() || localPlayerSpec))
			{
				V_sprintf_safe(textASCII, "[%s] %s", playerClantag, playerName);
			}
			else
			{
				V_strcpy_safe(textASCII, playerName);
			}
			DisplayText(textASCII, drawOutline);

			// Draw distance to player - Only if local player alive and same team
			// OR local not alive or spectator and this player has ghost
			if ((localPlayerAlive && localPlayer->GetTeamNumber() == player->GetTeamNumber()) ||
					((!localPlayerAlive || localPlayerSpec) && player->IsCarryingGhost()))
			{
				const float flDistance = METERS_PER_INCH * player->GetAbsOrigin().DistTo(localPlayer->GetAbsOrigin());
				V_snprintf(textASCII, MAX_MARKER_STRLEN, "%s: %.0fm",
						 player->IsCarryingGhost() ? "GHOST DISTANCE" : "DISTANCE",
						 flDistance);
				DisplayText(textASCII, drawOutline);
			}

			V_snprintf(textASCII, MAX_MARKER_STRLEN, "%d%%", player->GetHealth());
			DisplayText(textASCII, drawOutline);
		}

		if (!drawOutline)
		{
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
