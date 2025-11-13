#include "neo_hud_player_ping.h"

#include "c_neo_player.h"
#include "neo_gamerules.h"
#include "neo_hud_game_event.h"
#include "c_playerresource.h"

#include "iclientmode.h"
#include <vgui/ILocalize.h>
#include <vgui/ISurface.h>
#include "engine/IEngineSound.h"
#include "voice_status.h"
#include "hud_chat.h"
#include "engine/IEngineSound.h"

#include "ienginevgui.h"

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

DECLARE_NAMED_HUDELEMENT(CNEOHud_PlayerPing, NHudPlayerPing);

NEO_HUD_ELEMENT_DECLARE_FREQ_CVAR(PlayerPing, 0.05) // distances to pings and thus ping offset update at 20Hz looks nice enough

CNEOHud_PlayerPing* playerPingHudElement;

ConVar cl_neo_player_pings("cl_neo_player_pings", "1", FCVAR_ARCHIVE | FCVAR_USERINFO, "Show player pings.", true, 0, true, 1, 
	[](IConVar* var [[maybe_unused]], const char* pOldValue [[maybe_unused]], float flOldValue [[maybe_unused]])->void {
		if (!cl_neo_player_pings.GetBool())
		{
			playerPingHudElement->HideAllPings();
		}
	});

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

	/*pingSoundHandle = CBaseEntity::PrecacheScriptSound("HUD.Ping");
	Assert(pingSoundHandle > -1);*/

	SetVisible(true);

	playerPingHudElement = this;
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
	auto *localPlayer = C_NEO_Player::GetLocalNEOPlayer();
	if (!localPlayer) return;

	const int playerTeam = localPlayer->GetTeamNumber();
	for (int playerSlot = 0; playerSlot < gpGlobals->maxClients; playerSlot++)
	{
		if (gpGlobals->curtime < m_iPlayerPings[playerSlot].deathTime && (playerTeam == TEAM_SPECTATOR || m_iPlayerPings[playerSlot].team == playerTeam))
		{
			UpdateDistanceToPlayer(localPlayer, playerSlot, false);
		}

		if (gpGlobals->curtime < m_iEnemyHitPings[playerSlot].deathTime && (playerTeam == TEAM_SPECTATOR || m_iEnemyHitPings[playerSlot].team == playerTeam))
		{
			UpdateDistanceToPlayer(localPlayer, playerSlot, true);
		}
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
	auto eventName = event->GetName();
	if (!Q_stricmp(eventName, "player_ping"))
	{
		if (!cl_neo_player_pings.GetBool())
		{
			return;
		}

		const int localTeam = GetLocalPlayerTeam();
		const int playerTeam = event->GetInt("playerteam");
		if (localTeam != TEAM_SPECTATOR && playerTeam != localTeam)
		{
			return;
		}

		const int userID = event->GetInt("userid");
		const int playerIndex = engine->GetPlayerForUserID(userID);

		bool isShotPing = event->GetBool("shotping");
		if (!isShotPing && GetClientVoiceMgr()->IsPlayerBlocked(playerIndex))
		{
			return;
		}

		const Vector worldpos = Vector(event->GetInt("pingx"), event->GetInt("pingy"), event->GetInt("pingz"));
		bool ghosterPing = event->GetBool("ghosterping");
		SetPos(playerIndex - 1, playerTeam, worldpos, ghosterPing, isShotPing);
	}
	else if (!Q_stricmp(eventName, "round_start"))
	{
		HideAllPings();
	}
	else if (!Q_stricmp(eventName, "player_team"))
	{
		auto player = UTIL_PlayerByUserId(event->GetInt("userid"));
		if (player && player->IsLocalPlayer())
		{
			HideAllPings();
		}
	}

}

void CNEOHud_PlayerPing::LevelShutdown(void)
{
	HideAllPings();
	m_flNextPingSoundTime = 0.f;
}

enum NeoPlayerPingsInSpectate
{
	NEO_SPECTATE_PINGS_DISABLED	=		0,
	NEO_SPECTATE_PINGS_TARGET,
	NEO_SPECTATE_PINGS_ALL,

	NEO_SPECTATE_PINGS_LAST_VALUE =		NEO_SPECTATE_PINGS_ALL
};
ConVar cl_neo_player_pings_in_spectate("cl_neo_player_pings_in_spectate", "1", FCVAR_ARCHIVE, "See pings of players playing the game. 0 = disabled, 1 = spectate target team, 2 = all", true, NEO_SPECTATE_PINGS_DISABLED, true, NEO_SPECTATE_PINGS_LAST_VALUE);
void CNEOHud_PlayerPing::DrawPings(playerPing* pings, int localPlayerTeam, int spectateTargetTeam, int playerPingsInSpectate, bool isEnemyPings)
{
	int x, y, x2, y2;
	for (int i = 0; i < gpGlobals->maxClients; i++)
	{
		Vector offset = Vector(0, 0, 32);
		if (gpGlobals->curtime >= pings[i].deathTime || !GetVectorInScreenSpace(pings[i].worldPos, x, y) || !GetVectorInScreenSpace(pings[i].worldPos, x2, y2, &offset))
		{
			continue;
		}

		if (localPlayerTeam == TEAM_SPECTATOR && playerPingsInSpectate == 1 && pings[i].team != spectateTargetTeam)
		{
			continue;
		}

		constexpr float PING_NORMAL_OPACITY = 200;
		constexpr float PING_OBSTRUCTED_OPACITY = 80;
		float opacity = pings[i].noLineOfSight ? PING_OBSTRUCTED_OPACITY : PING_NORMAL_OPACITY;

		constexpr float PING_FADE_START = 1.f;
		const float timeTillDeath = pings[i].deathTime - gpGlobals->curtime;
		if (timeTillDeath < PING_FADE_START)
		{
			opacity *= timeTillDeath / PING_FADE_START;
		}

		// Land animation
		constexpr float PLAYER_PING_LIFETIME = 8;
		constexpr float PING_PLACE_ANIMATION_DURATION = 0.5;
		if (timeTillDeath > PLAYER_PING_LIFETIME - PING_PLACE_ANIMATION_DURATION)
		{
			constexpr float PING_PLACE_ANIMATION_MAX_OFFSET = 0.25;
			y2 += ((y - y2) * PING_PLACE_ANIMATION_MAX_OFFSET) * sin(M_PI * (timeTillDeath - (PLAYER_PING_LIFETIME - PING_PLACE_ANIMATION_DURATION)) * (1 / PING_PLACE_ANIMATION_DURATION));
		}

		// Draw Ping Shape
		const int halfTexture = pings[i].ghosterPing ? m_iTexTall * 0.8 : m_iTexTall * 0.5;
		Color color = COLOR_RED;
		if (!isEnemyPings)
		{
			color = pings[i].ghosterPing ? COLOR_GREY : (pings[i].team == TEAM_JINRAI) ? COLOR_JINRAI : COLOR_NSF;
		}
		color.SetColor(color.r(), color.g(), color.b(), opacity);
		vgui::surface()->DrawSetColor(color);

		int drawX = x2;
		int drawY = y2;

		if (isEnemyPings)
		{
			drawX = x;
			drawY = y;
		}

		vgui::surface()->DrawTexturedRect(
			drawX - halfTexture,
			drawY - halfTexture,
			drawX + halfTexture,
			drawY + halfTexture);

		if (!isEnemyPings)
		{
			// Draw Line to root of Ping
			const Vector2D directionToOffset = Vector2D(x2 - x, y2 - y);
			const float offsetLengthOnScreen = Vector2DLength(directionToOffset);
			if (offsetLengthOnScreen > m_iTexTall)
			{
				// Offset the end position of the line so it doesn't draw over the ping icon
				const int lineX2 = x + directionToOffset.x * ((offsetLengthOnScreen - m_iTexTall) / offsetLengthOnScreen);
				const int lineY2 = y + directionToOffset.y * ((offsetLengthOnScreen - m_iTexTall) / offsetLengthOnScreen);

				vgui::surface()->DrawLine(x - 1, y, lineX2 - 1, lineY2);
				color.SetColor(COLOR_BLACK.r(), COLOR_BLACK.g(), COLOR_BLACK.b(), opacity);
				vgui::surface()->DrawSetColor(color);
				vgui::surface()->DrawLine(x, y + 1, lineX2, lineY2 + 1);
			}
		}

		// Draw Distance to Ping in meters
		vgui::surface()->DrawSetTextFont(pings[i].ghosterPing ? m_hFont : m_hFontSmall);
		char m_szMarkerText[4 + 1] = {};
		wchar_t m_wszMarkerTextUnicode[4 + 1] = {};
		V_snprintf(m_szMarkerText, sizeof(m_szMarkerText), "%im", (int)pings[i].distance);
		g_pVGuiLocalize->ConvertANSIToUnicode(m_szMarkerText, m_wszMarkerTextUnicode, sizeof(m_wszMarkerTextUnicode));
		const int halfTextWidth = 0.5 * GetStringPixelWidth(m_wszMarkerTextUnicode, pings[i].ghosterPing ? m_hFont : m_hFontSmall);

		// Adjust text position based on whether it's an enemy ping
		int textDrawX = drawX;
		int textDrawY = drawY;

		color.SetColor(COLOR_BLACK.r(), COLOR_BLACK.g(), COLOR_BLACK.b(), opacity);
		vgui::surface()->DrawSetTextColor(color);
		vgui::surface()->DrawSetTextPos(textDrawX - halfTextWidth, textDrawY + 1 - m_iTexTall - m_iFontTall);
		vgui::surface()->DrawPrintText(m_wszMarkerTextUnicode, 5);

		color.SetColor(COLOR_WHITE.r(), COLOR_WHITE.g(), COLOR_WHITE.b(), opacity);
		vgui::surface()->DrawSetTextColor(color);
		vgui::surface()->DrawSetTextPos(textDrawX - 1 - halfTextWidth, textDrawY - m_iTexTall - m_iFontTall);
		vgui::surface()->DrawPrintText(m_wszMarkerTextUnicode, 5);
	}
}

void CNEOHud_PlayerPing::DrawNeoHudElement()
{
	if (!ShouldDraw() || !NEORules()->IsTeamplay())
	{
		return;
	}
	
	C_BasePlayer *pLocalPlayer = C_BasePlayer::GetLocalPlayer();
	if (!pLocalPlayer)
	{
		return;
	}

	const int localPlayerTeam = pLocalPlayer->GetTeamNumber();
	const int playerPingsInSpectate = cl_neo_player_pings_in_spectate.GetInt();
	if (localPlayerTeam == TEAM_SPECTATOR && playerPingsInSpectate == NEO_SPECTATE_PINGS_DISABLED)
	{
		return;
	}

	int spectateTargetTeam = TEAM_UNASSIGNED;
	if (auto observerTarget = pLocalPlayer->GetObserverTarget())
	{
		spectateTargetTeam = observerTarget->GetTeamNumber();
	};

	vgui::surface()->DrawSetTexture(m_hTexture);
	
	DrawPings(m_iPlayerPings, localPlayerTeam, spectateTargetTeam, playerPingsInSpectate, false);
	DrawPings(m_iEnemyHitPings, localPlayerTeam, spectateTargetTeam, playerPingsInSpectate, true);
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
	for (int i = 0; i < gpGlobals->maxClients; i++)
	{
		m_iPlayerPings[i].deathTime = 0;
		m_iEnemyHitPings[i].deathTime = 0;
	}
}

void CNEOHud_PlayerPing::UpdateDistanceToPlayer(C_BasePlayer* player, const int playerSlot, bool isShotPing)
{
	playerPing* pings = isShotPing ? m_iEnemyHitPings : m_iPlayerPings;

	pings[playerSlot].distance = METERS_PER_INCH * player->GetAbsOrigin().DistTo(pings[playerSlot].worldPos);

	trace_t tr;
	UTIL_TraceLine(player->EyePosition(), pings[playerSlot].worldPos, MASK_VISIBLE_AND_NPCS, player, COLLISION_GROUP_NONE, &tr);
	pings[playerSlot].noLineOfSight = tr.fraction < 0.999;
}

void CNEOHud_PlayerPing::SetPos(const int playerSlot, const int playerTeam, const Vector& pos, bool ghosterPing, bool isShotPing) {
	constexpr float PLAYER_PING_LIFETIME = 8;
	auto localPlayer = C_NEO_Player::GetLocalNEOPlayer();
	if (!localPlayer) { return; }

	playerPing* pings = isShotPing ? m_iEnemyHitPings : m_iPlayerPings;

	pings[playerSlot].worldPos = pos;
	pings[playerSlot].deathTime = gpGlobals->curtime + PLAYER_PING_LIFETIME;
	pings[playerSlot].team = playerTeam;
	pings[playerSlot].ghosterPing = ghosterPing;

	UpdateDistanceToPlayer(localPlayer, playerSlot, isShotPing);
	const int playerPingsInSpectate = cl_neo_player_pings_in_spectate.GetInt();
	if (GetLocalPlayerTeam() == TEAM_SPECTATOR && playerPingsInSpectate != NEO_SPECTATE_PINGS_ALL)
	{
		if (playerPingsInSpectate == NEO_SPECTATE_PINGS_DISABLED)
		{
			return;
		}

		int spectateTargetTeam = TEAM_UNASSIGNED;
		if (auto observerTarget = localPlayer->GetObserverTarget())
		{
			spectateTargetTeam = observerTarget->GetTeamNumber();
			if (playerTeam != spectateTargetTeam)
			{
				return;
			}
		}
		else
		{
			return;
		}
	}
	NotifyPing(playerSlot, isShotPing);
}

ConVar snd_ping_volume("snd_ping_volume", "0.33", FCVAR_ARCHIVE, "Player ping volume", true, 0.f, true, 1.f);
ConVar cl_neo_player_pings_chat_message("cl_neo_player_pings_chat_message", "1", FCVAR_ARCHIVE, "Show message in chat for player pings.", true, 0, true, 1); // NEO TODO (Adam) custom chat filter instead?
ConVar cl_neo_player_pings_time_between_sounds("cl_neo_player_pings_time_between_sounds", "0", FCVAR_ARCHIVE, "Minimum time between two ping sounds", true, 0, false, 0);
void CNEOHud_PlayerPing::NotifyPing(const int playerSlot, bool isShotPing)
{
	C_NEO_Player* pPlayer = static_cast<C_NEO_Player*>(UTIL_PlayerByIndex(playerSlot + 1));
	if (!pPlayer || pPlayer->IsLocalPlayer())
	{
		return;
	}

	if (!isShotPing && cl_neo_player_pings_chat_message.GetBool())
	{
		CBaseHudChat* hudChat = (CBaseHudChat*)GET_HUDELEMENT(CHudChat);
		if (hudChat)
		{
			char szText[256];
			V_strcpy_safe(szText, pPlayer->GetNeoPlayerName());
			V_strcat_safe(szText, " pinged a location\n");
			hudChat->ChatPrintf(0, CHAT_FILTER_NONE, szText);
		}
	}
	
	if (gpGlobals->curtime < m_flNextPingSoundTime)
	{
		return;
	}

	const float timeBetweenPings = cl_neo_player_pings_time_between_sounds.GetFloat();
	if (timeBetweenPings > 0)
	{ // Allow multiple pings in the same tick if timeBetweenPings == 0
		m_flNextPingSoundTime = gpGlobals->curtime + timeBetweenPings;
	}

	/*EmitSound_t et;
	if (pingSoundHandle == -1)
	{
		et.m_pSoundName = ")gameplay/ping.wav";
		et.m_SoundLevel = SNDLVL_GUNFIRE;
		et.m_nChannel = CHAN_STATIC;
	}
	else
	{
		et.m_hSoundScriptHandle = pingSoundHandle;
	}
	et.m_nFlags |= SND_CHANGE_VOL;
	et.m_flVolume = snd_ping_volume.GetFloat();
	et.m_pOrigin = const_cast<Vector *>(&m_iPlayerPings[playerSlot].worldPos);*/

	playerPing* pings = isShotPing ? m_iEnemyHitPings : m_iPlayerPings;
	const Vector* origin = const_cast<Vector *>(&pings[playerSlot].worldPos);

	CLocalPlayerFilter filter;
	//CBaseEntity::EmitSound(filter, SOUND_FROM_WORLD, et);

	// NEO TODO (Adam) CBaseEntity::EmitSound allows us to pass a handle to the sound and allows players to change sound parameters like the sound level (controls sound fall-off over distance) 
	// and attenuation by editing the relevant resource file without adding even more console variables, but this adds a noticeable delay after a ping sound is played before another can be played,
	// find a solution? (CBaseEntity::EmitSound eventually calls enginesound->EmitSound, even with identical parameters passed this unwanted behavior is still present)
	enginesound->EmitSound(	filter, SOUND_FROM_WORLD, CHAN_STATIC, ")gameplay/ping.wav", 
		snd_ping_volume.GetFloat(), SNDLVL_GUNFIRE, 0, PITCH_NORM, 0, origin, NULL, NULL);

	/*CUtlVector<SndInfo_t> sounds;
	enginesound->GetActiveSounds(sounds);
	engine->Con_NPrintf(sounds.Size(), "%i", sounds.Size());*/
}