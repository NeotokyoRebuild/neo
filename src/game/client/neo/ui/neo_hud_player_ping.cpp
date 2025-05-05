#include "neo_hud_player_ping.h"

#include "c_neo_player.h"
#include "neo_gamerules.h"
#include "neo_hud_game_event.h"
#include "c_playerresource.h"

#include "iclientmode.h"
#include <vgui/ILocalize.h>
#include <vgui/ISurface.h>

#include "ienginevgui.h"

DECLARE_NAMED_HUDELEMENT(CNEOHud_PlayerPing, NHudPlayerPing);

NEO_HUD_ELEMENT_DECLARE_FREQ_CVAR(PlayerPing, 0.05) // distances to pings and thus ping offset update at 20Hz looks nice enough

CNEOHud_PlayerPing::CNEOHud_PlayerPing(const char* pElementName, vgui::Panel* parent)
	: CHudElement(pElementName), Panel(parent, pElementName)
{
	SetAutoDelete(true);
	m_iHideHudElementNumber = NEO_HUD_ELEMENT_PLAYER_PING;

	if (parent)
	{
		SetParent(parent);
	}
	else
	{
		SetParent(g_pClientMode->GetViewport());
	}

	m_hTexture = vgui::surface()->CreateNewTextureID();
	Assert(m_hTexture > 0);

	SetVisible(true);
}

void CNEOHud_PlayerPing::ApplySchemeSettings(vgui::IScheme *pScheme)
{
	BaseClass::ApplySchemeSettings(pScheme);

	m_hFont = pScheme->GetFont("NHudOCRSmall");
	m_hFontSmall = pScheme->GetFont("NHudOCRSmallerNoAdditive");
	m_iFontTall = vgui::surface()->GetFontTall(m_hFontSmall);
	m_iTexTall = m_iFontTall;

	vgui::surface()->DrawSetTextureFile(m_hTexture, "vgui/hud/ping/ping", 1, false);
	vgui::surface()->DrawGetTextureSize(m_hTexture, m_iTexWidth, m_iTexHeight);

	vgui::surface()->GetScreenSize(m_iPosX, m_iPosY);
	SetBounds(0, 0, m_iPosX, m_iPosY);

	SetFgColor(COLOR_TRANSPARENT);
	SetBgColor(COLOR_TRANSPARENT);
}

void CNEOHud_PlayerPing::UpdateStateForNeoHudElementDraw()
{
	auto *player = C_NEO_Player::GetLocalNEOPlayer();
	if (!player) return;

	const int playerTeam = player->GetTeamNumber();
	if (playerTeam != TEAM_JINRAI && playerTeam != TEAM_NSF)
	{
		return;
	}

	for (int i = 0; i < gpGlobals->maxClients; i++)
	{
		if (gpGlobals->curtime >= m_iPlayerPings[i].deathTime || m_iPlayerPings[i].team != playerTeam)
		{
			continue;
		}

		UpdateDistanceToPlayer(player, i);
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CNEOHud_PlayerPing::Init(void)
{
	ListenForGameEvent("player_ping");
	ListenForGameEvent("round_start");
	ListenForGameEvent("player_team");
}

//-----------------------------------------------------------------------------
// Purpose: Player created a ping
//-----------------------------------------------------------------------------
void CNEOHud_PlayerPing::FireGameEvent(IGameEvent* event)
{
	if (!g_PR)
		return;

	auto eventName = event->GetName();
	if (!Q_stricmp(eventName, "player_ping"))
	{
		int playerIndex = event->GetInt("playerindex");
		Vector worldpos = Vector(event->GetInt("pingx"), event->GetInt("pingy"), event->GetInt("pingz"));
		bool ghosterPing = event->GetBool("ghosterping");
		SetPos(playerIndex, worldpos, ghosterPing);
	}
	else if (!Q_stricmp(eventName, "round_start"))
	{
		for (int i = 0; i < MAX_PLAYERS; i++)
		{
			m_iPlayerPings[i].deathTime = 0;
		}
	}
	else if (!Q_stricmp(eventName, "player_team"))
	{
		auto player = UTIL_PlayerByUserId(event->GetInt("userid"));
		if (player && player->IsLocalPlayer())
		{
			for (int i = 0; i < MAX_PLAYERS; i++)
			{
				m_iPlayerPings[i].deathTime = 0;
			}
		}
	}

}

void CNEOHud_PlayerPing::LevelShutdown(void)
{
	for (int i = 0; i < MAX_PLAYERS; i++)
	{
		m_iPlayerPings[i].deathTime = 0;
	}
}

void CNEOHud_PlayerPing::DrawNeoHudElement()
{
	if (!ShouldDraw() || !NEORules()->IsTeamplay())
	{
		return;
	}

	auto *player = C_NEO_Player::GetLocalNEOPlayer();
	if (!player) return;

	const int playerTeam = player->GetTeamNumber();
	const bool playerIsPlaying = (playerTeam == TEAM_JINRAI || playerTeam == TEAM_NSF);
	if (!playerIsPlaying)
	{
		return;
	}

	int x, y;
	vgui::surface()->DrawSetTexture(m_hTexture);
	for (int i = 0; i < gpGlobals->maxClients; i++)
	{
		if (gpGlobals->curtime >= m_iPlayerPings[i].deathTime || m_iPlayerPings[i].team != playerTeam || !GetVectorInScreenSpace(m_iPlayerPings[i].worldPos, x, y))
		{
			continue;
		}

		float y2 = y - m_iPlayerPings[i].distanceYOffset;

		constexpr float PING_MAX_OPACITY = 200;
		float opacity = PING_MAX_OPACITY;

		constexpr float PING_FADE_START = 1.f;
		const float timeTillDeath = m_iPlayerPings[i].deathTime - gpGlobals->curtime;
		if (timeTillDeath < PING_FADE_START)
		{
			opacity *= timeTillDeath / PING_FADE_START;
		}

		// Draw Ping Shape
		const int halfTexture = m_iPlayerPings[i].ghosterPing ? m_iTexTall * 0.8 : m_iTexTall * 0.5;
		Color color = m_iPlayerPings[i].ghosterPing ? COLOR_GREY : (playerTeam == TEAM_JINRAI) ? COLOR_JINRAI : COLOR_NSF;
		color.SetColor(color.r(), color.g(), color.b(), opacity);
		vgui::surface()->DrawSetColor(color);
		vgui::surface()->DrawTexturedRect(
			x - halfTexture,
			y2 - halfTexture,
			x + halfTexture,
			y2 + halfTexture);

		// Draw Line to root of Ping
		if (y - y2 > m_iTexTall)
		{
			vgui::surface()->DrawLine(x-1,y,x-1,y2 + m_iTexTall);
			color.SetColor(COLOR_BLACK.r(), COLOR_BLACK.g(), COLOR_BLACK.b(), opacity);
			vgui::surface()->DrawSetColor(color);
			vgui::surface()->DrawLine(x, y+1, x, y2+1 + m_iTexTall);
		}

		// Draw Distance to Ping in meters
		vgui::surface()->DrawSetTextFont(m_iPlayerPings[i].ghosterPing ? m_hFont : m_hFontSmall);
		char m_szMarkerText[4 + 1] = {};
		wchar_t m_wszMarkerTextUnicode[4 + 1] = {};
		V_snprintf(m_szMarkerText, sizeof(m_szMarkerText), "%im", (int)m_iPlayerPings[i].distance);
		g_pVGuiLocalize->ConvertANSIToUnicode(m_szMarkerText, m_wszMarkerTextUnicode, sizeof(m_wszMarkerTextUnicode));
		const int halfTextWidth = 0.5 * GetStringPixelWidth(m_wszMarkerTextUnicode, m_iPlayerPings[i].ghosterPing ? m_hFont : m_hFontSmall);

		color.SetColor(COLOR_BLACK.r(), COLOR_BLACK.g(), COLOR_BLACK.b(), opacity);
		vgui::surface()->DrawSetTextColor(color);
		vgui::surface()->DrawSetTextPos(x - halfTextWidth, y2 + 1 - m_iTexTall - m_iFontTall);
		vgui::surface()->DrawPrintText(m_wszMarkerTextUnicode, 5);

		color.SetColor(COLOR_WHITE.r(), COLOR_WHITE.g(), COLOR_WHITE.b(), opacity);
		vgui::surface()->DrawSetTextColor(color);
		vgui::surface()->DrawSetTextPos(x - 1 - halfTextWidth, y2 - m_iTexTall - m_iFontTall);
		vgui::surface()->DrawPrintText(m_wszMarkerTextUnicode, 5);
	}
}

void CNEOHud_PlayerPing::Paint()
{
	BaseClass::Paint();
	PaintNeoElement();
}

//-----------------------------------------------------------------------------
// Purpose: Compute length of string using given font, borrowed from hud_credits, maybe make this part of CNEOHud_ChildElement?
//-----------------------------------------------------------------------------
int CNEOHud_PlayerPing::GetStringPixelWidth(wchar_t* pString, vgui::HFont hFont)
{
	int iLength = 0;

	for (wchar_t* wch = pString; *wch != 0; wch++)
	{
		iLength += vgui::surface()->GetCharacterWidth(hFont, *wch);
	}

	return iLength;
}

void CNEOHud_PlayerPing::HideAllPings()
{
	for (int i = 0; i < MAX_PLAYERS; i++)
	{
		m_iPlayerPings[i].deathTime = 0;
	}
}

void CNEOHud_PlayerPing::UpdateDistanceToPlayer(C_BasePlayer* player, int pingIndex)
{
	m_iPlayerPings[pingIndex].distance = METERS_PER_INCH * player->GetAbsOrigin().DistTo(m_iPlayerPings[pingIndex].worldPos);

	constexpr float MAX_Y_DISTANCE_OFFSET_RATIO = 1.f / 8;
	m_iPlayerPings[pingIndex].distanceYOffset = min(m_iPosY * MAX_Y_DISTANCE_OFFSET_RATIO, m_iPlayerPings[pingIndex].distance * 2 * (m_iPosY / 480));
}

void CNEOHud_PlayerPing::SetPos(const int index, Vector& pos, bool ghosterPing) {
	constexpr float PLAYER_PING_LIFETIME = 8;
	auto localPlayer = UTIL_PlayerByIndex(GetLocalPlayerIndex());
	if (!localPlayer) { return; }
	auto pingPlayer = UTIL_PlayerByIndex(index);
	if (!pingPlayer) { return; }

	const int pingIndex = index - 1;
	m_iPlayerPings[pingIndex].worldPos = pos;
	m_iPlayerPings[pingIndex].deathTime = gpGlobals->curtime + PLAYER_PING_LIFETIME;
	m_iPlayerPings[pingIndex].team = pingPlayer->GetTeamNumber();
	m_iPlayerPings[pingIndex].ghosterPing = ghosterPing;

	UpdateDistanceToPlayer(localPlayer, pingIndex);
}