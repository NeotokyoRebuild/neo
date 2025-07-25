#include "cbase.h"
#include "neo_hud_ghost_beacons.h"

#include "iclientmode.h"
#include <vgui/ILocalize.h>
#include <vgui/ISurface.h>
#include <vgui_controls/Controls.h>
#include <vgui_controls/ImagePanel.h>

#include "ienginevgui.h"


#include "c_neo_npc_dummy.h"
#include "c_neo_player.h"
#include "neo_player_shared.h"
#include "c_team.h"
#include "neo_gamerules.h"
#include "weapon_ghost.h"


// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

using vgui::surface;

ConVar neo_ghost_beacon_scale("neo_ghost_beacon_scale", "1", FCVAR_ARCHIVE,
	"Ghost beacons scale.", true, 0.01, false, 0);

ConVar neo_ghost_beacon_scale_with_distance("neo_ghost_beacon_scale_with_distance", "0", FCVAR_ARCHIVE,
	"Toggles the scaling of ghost beacons with distance.", true, 0, true, 1);

ConVar neo_ghost_beacon_alpha("neo_ghost_beacon_alpha", "150", FCVAR_ARCHIVE,
	"Alpha channel transparency of HUD ghost beacons.", true, 0, true, 255);

ConVar neo_ghost_delay_secs("neo_ghost_delay_secs", "3.3", FCVAR_CHEAT | FCVAR_REPLICATED,
	"The delay in seconds until the ghost shows up after pick up.", true, 0.0, false, 0.0);

DECLARE_NAMED_HUDELEMENT(CNEOHud_GhostBeacons, neo_ghost_beacons);

NEO_HUD_ELEMENT_DECLARE_FREQ_CVAR(GhostBeacons, 0.01)

CNEOHud_GhostBeacons::CNEOHud_GhostBeacons(const char *pElementName, vgui::Panel *parent)
	: CHudElement(pElementName), EditablePanel(parent, pElementName)
{
	SetAutoDelete(true);
	m_iHideHudElementNumber = NEO_HUD_ELEMENT_GHOST_BEACONS;

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

extern ConVar neo_ctg_ghost_beacons_when_inactive;
void CNEOHud_GhostBeacons::DrawNeoHudElement()
{
	if (!ShouldDraw())
	{
		return;
	}

	auto spectateTarget = IsLocalPlayerSpectator() ? ToNEOPlayer(UTIL_PlayerByIndex(GetSpectatorTarget())) : C_NEO_Player::GetLocalNEOPlayer();
	if (!spectateTarget || spectateTarget->GetTeamNumber() < FIRST_GAME_TEAM || !spectateTarget->IsAlive() || NEORules()->IsRoundOver())
	{
		// NEO NOTE (nullsystem): Spectator and dead players even in spec shouldn't see beacons
		// Post-round also should switch off beacon
		return;
	}

	if (!spectateTarget->m_bCarryingGhost)
	{ // Saves iterating through all the weapons when neo_ctg_ghost_beacons_when_inactive is set to 1
		return;
	}

	C_WeaponGhost* ghost;
	if (neo_ctg_ghost_beacons_when_inactive.GetBool())
	{
		ghost = static_cast<C_WeaponGhost*>(GetNeoWepWithBits(spectateTarget, NEO_WEP_GHOST));
	}
	else
	{
		auto weapon = static_cast<C_NEOBaseCombatWeapon*>(spectateTarget->GetActiveWeapon());
		ghost = (weapon && weapon->IsGhost()) ? static_cast<C_WeaponGhost*>(weapon) : nullptr;
	}

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

	auto enemyTeamId = spectateTarget->GetTeamNumber() == TEAM_JINRAI ? TEAM_NSF : TEAM_JINRAI;
	auto enemyTeam = GetGlobalTeam(enemyTeamId);
	auto enemyCount = enemyTeam->GetNumPlayers();
	float closestEnemy = FLT_MAX;

	// Human and bot enemies
	for(int i = 0; i < enemyCount; ++i)
	{
		auto enemyToShow = enemyTeam->GetPlayer(i);
		if(enemyToShow)
		{
			auto enemyPos = enemyToShow->GetAbsOrigin();
			float distance = FLT_MAX;
			if(enemyToShow->IsAlive() && !enemyToShow->IsDormant() && ghost->IsPosWithinViewDistance(enemyPos, distance) && showGhost)
			{
				DrawPlayer(enemyPos);
			}
			closestEnemy = Min(distance, closestEnemy);
		}
	}
	// Dummy entity enemies
	for (auto dummy = C_NEO_NPCDummy::GetList(); dummy; dummy = dummy->m_pNext)
	{
		Assert(dummy);
		auto dummyPos = dummy->GetAbsOrigin();
		float distance = FLT_MAX;
		if (dummy->IsAlive() && !dummy->IsDormant() && ghost->IsPosWithinViewDistance(dummyPos, distance) && showGhost)
		{
			DrawPlayer(dummyPos);
		}
		closestEnemy = Min(distance, closestEnemy);
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

	float scale = neo_ghost_beacon_scale.GetFloat();
	constexpr float HALF_BASE_TEX_LENGTH = 32;
	float halfBeaconLength = HALF_BASE_TEX_LENGTH * scale;

	int posX, posY;
	Vector ghostBeaconOffset = Vector(0, 0, neo_ghost_beacon_scale_with_distance.GetBool() ? 32 : 48); // The center of the player, or raise slightly if not scaling
	GetVectorInScreenSpace(playerPos, posX, posY, &ghostBeaconOffset);
	if (neo_ghost_beacon_scale_with_distance.GetBool())
	{
		int pos2X, pos2Y;
		Vector ghostBeaconOffset2 = Vector(0, 0, 64); // The top of the player
		GetVectorInScreenSpace(playerPos, pos2X, pos2Y, &ghostBeaconOffset2);
		halfBeaconLength = (posY - pos2Y) * 0.5 * scale;
	}
	
	wchar_t m_wszBeaconTextUnicode[4 + 1];
	V_snwprintf(m_wszBeaconTextUnicode, ARRAYSIZE(m_wszBeaconTextUnicode), L"%02d m", FastFloatToSmallInt(dist * METERS_PER_INCH));

	int alpha = distInMeters < 35 ? neo_ghost_beacon_alpha.GetInt() : neo_ghost_beacon_alpha.GetInt() * ((45 - distInMeters) / 10);
	surface()->DrawSetTextColor(255, 255, 255, alpha);
	surface()->DrawSetTextFont(m_hFont);
	int textWidth, textHeight;
	surface()->GetTextSize(m_hFont, m_wszBeaconTextUnicode, textWidth, textHeight);
	surface()->DrawSetTextPos(posX - (textWidth / 2), posY + halfBeaconLength);
	surface()->DrawPrintText(m_wszBeaconTextUnicode, V_wcslen(m_wszBeaconTextUnicode));

	surface()->DrawSetColor(255, 20, 20, alpha);
	surface()->DrawSetTexture(m_hTex);

	// End coordinates according to art size (and our distance scaling)
	surface()->DrawTexturedRect(
		posX - halfBeaconLength,
		posY - halfBeaconLength,
		posX + halfBeaconLength,
		posY + halfBeaconLength);
}
