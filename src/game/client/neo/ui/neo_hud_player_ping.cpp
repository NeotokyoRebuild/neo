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

NEO_HUD_ELEMENT_DECLARE_FREQ_CVAR(PlayerPing, 0.1)

CNEOHud_PlayerPing::CNEOHud_PlayerPing(const char* pElementName, vgui::Panel* parent)
	: CHudElement(pElementName), Panel(parent, pElementName)
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

	m_hTexture = vgui::surface()->CreateNewTextureID();
	Assert(m_hTexture > 0);

	SetVisible(true);
}

void CNEOHud_PlayerPing::ApplySchemeSettings(vgui::IScheme *pScheme)
{
	BaseClass::ApplySchemeSettings(pScheme);

	m_hFont = pScheme->GetFont("NHudOCRSmall");
	m_hFontSmall = pScheme->GetFont("NHudOCRSmaller");
	m_iFontTall = vgui::surface()->GetFontTall(m_hFont);
	m_iTexTall = m_iFontTall * 2;

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
	const bool playerIsPlaying = (playerTeam == TEAM_JINRAI || playerTeam == TEAM_NSF);
	if (!playerIsPlaying)
	{
		return;
	}

	for (int i = 0; i < gpGlobals->maxClients; i++)
	{
		if (gpGlobals->curtime >= m_iPlayerPings[i].deathTime || m_iPlayerPings[i].team != playerTeam)
		{
			continue;
		}

		m_iPlayerPings[i].distance = METERS_PER_INCH * player->GetAbsOrigin().DistTo(m_iPlayerPings[i].worldPos);
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CNEOHud_PlayerPing::Init(void)
{
	ListenForGameEvent("player_ping");
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
		if (gpGlobals->curtime >= m_iPlayerPings[i].deathTime || m_iPlayerPings[i].team != playerTeam)
		{
			continue;
		}

		GetVectorInScreenSpace(m_iPlayerPings[i].worldPos, x, y);

		constexpr float PING_MAX_OPACITY = 200;
		constexpr float PING_MIN_OPACITY = 25;
		float opacity = PING_MAX_OPACITY;
		float distanceToCenter = static_cast<float>(sqrt(pow((m_iPosX * 0.5) - x, 2) + pow(y - (m_iPosY * 0.5), 2)));
		float FADE_FROM_CENTER_START = 120 * (m_iPosY / 480);
		float FADE_FROM_CENTER_END = 60 * (m_iPosY / 480);
		if (distanceToCenter < FADE_FROM_CENTER_END)
		{
			opacity = PING_MIN_OPACITY;
		}
		else if (distanceToCenter < FADE_FROM_CENTER_START)
		{
			opacity = PING_MIN_OPACITY + ((PING_MAX_OPACITY - PING_MIN_OPACITY) * (distanceToCenter - FADE_FROM_CENTER_END) / (FADE_FROM_CENTER_START - FADE_FROM_CENTER_END));
		}

		constexpr float PING_FADE_START = 1.f;
		float deathTimeDelta = m_iPlayerPings[i].deathTime - gpGlobals->curtime;
		if (deathTimeDelta < PING_FADE_START)
		{
			opacity *= deathTimeDelta / PING_FADE_START;
		}

		int offsetTexture = m_iPlayerPings[i].ghosterPing ? m_iTexTall * 0.7 : m_iTexTall * 0.5;
		Color color = m_iPlayerPings[i].ghosterPing ? COLOR_GREY : (playerTeam == TEAM_JINRAI) ? COLOR_JINRAI : COLOR_NSF;
		color.SetColor(color.r(), color.g(), color.b(), opacity);
		vgui::surface()->DrawSetColor(color);
		vgui::surface()->DrawTexturedRect(
			x - offsetTexture,
			y - offsetTexture,
			x + offsetTexture,
			y + offsetTexture);

		char character = i >= 10 ? 'A' + i - 10 : '0' + i; // NEO TODO (Adam) Index in team instead of in server?
		int xWide = vgui::surface()->GetCharacterWidth(m_hFont, character);

		color = COLOR_WHITE;
		color.SetColor(color.r(), color.g(), color.b(), opacity);

		vgui::surface()->DrawSetTextColor(color);
		vgui::surface()->DrawSetTextFont(m_hFont);
		vgui::surface()->DrawSetTextPos(x - (xWide / 2), y - (m_iFontTall / 2));
		vgui::surface()->DrawUnicodeChar(character);

		vgui::surface()->DrawSetTextFont(m_hFontSmall);
		char m_szMarkerText[4 + 1] = {};
		wchar_t m_wszMarkerTextUnicode[4 + 1] = {};
		V_snprintf(m_szMarkerText, sizeof(m_szMarkerText), "%im", m_iPlayerPings[i].distance);
		g_pVGuiLocalize->ConvertANSIToUnicode(m_szMarkerText, m_wszMarkerTextUnicode, sizeof(m_wszMarkerTextUnicode));
		xWide = GetStringPixelWidth(m_wszMarkerTextUnicode, m_hFontSmall);

		vgui::surface()->DrawSetTextPos(x - (xWide / 2), y - ((3*m_iFontTall) / 2));
		vgui::surface()->DrawPrintText(m_wszMarkerTextUnicode, 5);
	}
}

int CNEOHud_PlayerPing::GetStringPixelWidth(wchar_t* pString, vgui::HFont hFont)
{
	int iLength = 0;

	for (wchar_t* wch = pString; *wch != 0; wch++)
	{
		iLength += vgui::surface()->GetCharacterWidth(hFont, *wch);
	}

	return iLength;
}

void CNEOHud_PlayerPing::Paint()
{
	BaseClass::Paint();
	PaintNeoElement();
}
