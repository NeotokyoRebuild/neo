#include "cbase.h"
#include "neo_hud_ghost_beacons.h"

#include "iclientmode.h"
#include <vgui/ILocalize.h>
#include <vgui/ISurface.h>
#include <vgui_controls/Controls.h>
#include <vgui_controls/ImagePanel.h>

#include "ienginevgui.h"

// memdbgon must be the last include file in a .cpp file!!!
#include "c_neo_player.h"
#include "c_team.h"
#include "neo_gamerules.h"
#include "weapon_ghost.h"
#include "tier0/memdbgon.h"

using vgui::surface;

// NEO HACK (Rain): This is a sort of magic number to help with screenspace hud elements
// scaling in world space. There's most likely some nicer and less confusing way to do this.
// Check which files make reference to this, if you decide to tweak it.
ConVar neo_ghost_beacon_scale_baseline("neo_ghost_beacon_scale_baseline", "150", FCVAR_USERINFO,
	"Distance in HU where ghost marker is same size as player.", true, 0, true, 9999);

ConVar neo_ghost_beacon_scale_toggle("neo_ghost_beacon_scale_toggle", "0", FCVAR_USERINFO,
	"Toggles the scaling of ghost beacons.", true, 0, true, 1);

ConVar neo_ghost_beacon_alpha("neo_ghost_beacon_alpha", "150", FCVAR_USERINFO,
	"Alpha channel transparency of HUD ghost beacons.", true, 0, true, 255);

ConVar neo_ghost_delay_secs("neo_ghost_delay_secs", "3.3", FCVAR_CHEAT | FCVAR_REPLICATED,
	"The delay in seconds until the ghost shows up after pick up.", true, 0.0, false, 0.0);

DECLARE_NAMED_HUDELEMENT(CNEOHud_GhostBeacons, neo_ghost_beacons);

NEO_HUD_ELEMENT_DECLARE_FREQ_CVAR(GhostBeacons, 0.01)

CNEOHud_GhostBeacons::CNEOHud_GhostBeacons(const char *pElementName, vgui::Panel *parent)
	: CHudElement(pElementName), EditablePanel(parent, pElementName)
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

	int screenWidth, screenHeight;
	surface()->GetScreenSize(screenWidth, screenHeight);
	SetBounds(0, 0, screenWidth, screenHeight);
	
	m_hTex = surface()->CreateNewTextureID();
	Assert(m_hTex > 0);
	surface()->DrawSetTextureFile(m_hTex, "vgui/hud/ctg/g_beacon_enemy", 1, false);

	surface()->DrawGetTextureSize(m_hTex, m_iBeaconTextureWidth, m_iBeaconTextureHeight);

	SetVisible(true);
}

void CNEOHud_GhostBeacons::UpdateStateForNeoHudElementDraw()
{
}

void CNEOHud_GhostBeacons::ApplySchemeSettings(vgui::IScheme* pScheme)
{
	BaseClass::ApplySchemeSettings(pScheme);

	m_hFont = pScheme->GetFont("NHudOCRSmall", true);

	int screenWidth, screenHeight;
	surface()->GetScreenSize(screenWidth, screenHeight);
	SetBounds(0, 0, screenWidth, screenHeight);
	SetFgColor(COLOR_TRANSPARENT);
	SetBgColor(COLOR_TRANSPARENT);
}

void CNEOHud_GhostBeacons::DrawNeoHudElement()
{
	if (!ShouldDraw())
	{
		return;
	}

	auto localPlayer = C_NEO_Player::GetLocalNEOPlayer();
	if (localPlayer->GetTeamNumber() < FIRST_GAME_TEAM || !localPlayer->IsAlive() || NEORules()->IsRoundOver())
	{
		// NEO NOTE (nullsystem): Spectator and dead players even in spec shouldn't see beacons
		// Post-round also should switch off beacon
		return;
	}

	auto ghost = dynamic_cast<C_WeaponGhost*>(localPlayer->GetActiveWeapon());
	if (!ghost) //Check ghost ready here as players might be in PVS
	{
		m_bHoldingGhost = false;
		return;
	}

	if (!m_bHoldingGhost)
	{
		// Just changed to holding the ghost, start delay timer
		m_flNextAllowGhostShowTime = gpGlobals->curtime + neo_ghost_delay_secs.GetFloat();
	}
	m_bHoldingGhost = true;

	m_pGhost = ghost;
	const bool showGhost = (gpGlobals->curtime > m_flNextAllowGhostShowTime);

	auto enemyTeamId = localPlayer->GetTeamNumber() == TEAM_JINRAI ? TEAM_NSF : TEAM_JINRAI;
	auto enemyTeam = GetGlobalTeam(enemyTeamId);
	auto enemyTeamCount = enemyTeam->GetNumPlayers();
	float closestEnemy = FLT_MAX;
	for(int i = 0; i < enemyTeamCount; ++i)
	{
		auto enemyToShow = enemyTeam->GetPlayer(i);
		if(enemyToShow)
		{
			auto enemyPos = enemyToShow->GetAbsOrigin();
			float distance = FLT_MAX;
			if(enemyToShow->IsAlive() && ghost->IsPosWithinViewDistance(enemyPos, distance) && !enemyToShow->IsDormant() && showGhost)
			{
				DrawPlayer(enemyPos);
			}
			closestEnemy = Min(distance, closestEnemy);
		}
	}
	ghost->TryGhostPing(closestEnemy * METERS_PER_INCH);
}

void CNEOHud_GhostBeacons::Paint()
{
	BaseClass::Paint();
	PaintNeoElement();
}

void CNEOHud_GhostBeacons::DrawPlayer(const Vector& playerPos) const
{	
	auto dist = m_pGhost->DistanceToPos(playerPos);	//Move this to some util
	auto distInMeters = dist * METERS_PER_INCH;

	int posX, posY;
	if (neo_ghost_beacon_scale_toggle.GetInt() == 0)
	{
		static constexpr int DISTANCE_AT_WHICH_MARKER_ON_FLOOR = 25;
		static constexpr int MAX_DISTANCE_TO_RAISE_GHOST_BREACON = DISTANCE_AT_WHICH_MARKER_ON_FLOOR * 2;
		Vector ghostBeaconOffset = Vector(0, 0, (MAX_DISTANCE_TO_RAISE_GHOST_BREACON - (distInMeters * 2)));
		GetVectorInScreenSpace(playerPos, posX, posY, &ghostBeaconOffset);
	} 
	else
	{
		GetVectorInScreenSpace(playerPos, posX, posY);
	}
	
	char m_szBeaconTextANSI[4 + 1];
	wchar_t m_wszBeaconTextUnicode[4 + 1];

	V_snprintf(m_szBeaconTextANSI, sizeof(m_szBeaconTextANSI), "%02d m", FastFloatToSmallInt(dist * METERS_PER_INCH));
	g_pVGuiLocalize->ConvertANSIToUnicode(m_szBeaconTextANSI, m_wszBeaconTextUnicode, sizeof(m_wszBeaconTextUnicode));

	int alpha = distInMeters < 35 ? neo_ghost_beacon_alpha.GetInt() : neo_ghost_beacon_alpha.GetInt() * ((45 - distInMeters) / 10);
	surface()->DrawSetTextColor(255, 255, 255, alpha);
	surface()->DrawSetTextFont(m_hFont);
	int textWidth, textHeight;
	surface()->GetTextSize(m_hFont, m_wszBeaconTextUnicode, textWidth, textHeight);
	surface()->DrawSetTextPos(posX - (textWidth / 2), posY);
	surface()->DrawPrintText(m_wszBeaconTextUnicode, sizeof(m_szBeaconTextANSI));

	surface()->DrawSetColor(255, 20, 20, alpha);
	surface()->DrawSetTexture(m_hTex);

	float scale = neo_ghost_beacon_scale_toggle.GetBool() ? (neo_ghost_beacon_scale_baseline.GetInt() / dist) : 0.25;

	// Offset screen space starting positions by half of the texture x/y coords,
	// so it starts centered on target.
	const int posfix_X = posX - ((m_iBeaconTextureWidth / 2) * scale);
	const int posfix_Y = posY - (m_iBeaconTextureHeight * scale);

	// End coordinates according to art size (and our distance scaling)
	surface()->DrawTexturedRect(
		posfix_X,
		posfix_Y,
		posfix_X + (m_iBeaconTextureWidth * scale),
		posfix_Y + (m_iBeaconTextureHeight * scale));
}
