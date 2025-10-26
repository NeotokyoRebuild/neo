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

#include <utility>

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

using vgui::surface;

ConVar cl_neo_ghost_beacon_scale("cl_neo_ghost_beacon_scale", "1", FCVAR_ARCHIVE,
	"Ghost beacons scale.", true, 0.01, false, 0);

ConVar cl_neo_ghost_beacon_scale_with_distance("cl_neo_ghost_beacon_scale_with_distance", "0", FCVAR_ARCHIVE,
	"Toggles the scaling of ghost beacons with distance.", true, 0, true, 1);

ConVar cl_neo_ghost_beacon_alpha("cl_neo_ghost_beacon_alpha", "150", FCVAR_ARCHIVE,
	"Alpha channel transparency of HUD ghost beacons.", true, 0, true, 255);



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

extern ConVar sv_neo_ctg_ghost_beacons_when_inactive;
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
	{ // Saves iterating through all the weapons when sv_neo_ctg_ghost_beacons_when_inactive is set to 1
		return;
	}

	C_WeaponGhost* ghost;
	float ghostActivatedTime;
	if (sv_neo_ctg_ghost_beacons_when_inactive.GetBool())
	{
		ghost = assert_cast<C_WeaponGhost*>(GetNeoWepWithBits(spectateTarget, NEO_WEP_GHOST));
		if (!ghost)
			return;
		ghostActivatedTime = ghost->GetPickupTime();
	}
	else
	{
		auto weapon = assert_cast<C_NEOBaseCombatWeapon*>(spectateTarget->GetActiveWeapon());
		ghost = (weapon && weapon->IsGhost()) ? static_cast<C_WeaponGhost*>(weapon) : nullptr;
		if (!ghost)
			return;
		ghostActivatedTime = ghost->GetDeployTime();
	}

	const auto dtGhostActivated = gpGlobals->curtime - ghostActivatedTime;
	const bool showGhost = dtGhostActivated >= sv_neo_ghost_delay_secs.GetFloat();

	if (!showGhost)
	{
#if(0)
		if (!sv_neo_serverside_beacons.GetBool())
			DevMsg("CLI ghost beacons are activating... %f\n", dtGhostActivated);
#endif
		return;
	}

	auto enemyTeamId = spectateTarget->GetTeamNumber() == TEAM_JINRAI ? TEAM_NSF : TEAM_JINRAI;
	auto enemyTeam = GetGlobalTeam(enemyTeamId);
	auto enemyCount = enemyTeam->GetNumPlayers();
	[[maybe_unused]] float closestEnemy = FLT_MAX;

	const auto processBeacon = [this,
			&closestEnemy,
			&showGhost = std::as_const(showGhost),
			&spectateTarget = std::as_const(spectateTarget)]
			(auto* enemy)->void {
		if (!enemy || !enemy->IsAlive() || enemy->IsDormant())
			return;
		const auto& enemyPos = enemy->GetAbsOrigin();
		const float distTo = spectateTarget->GetAbsOrigin().DistTo(enemyPos);
		const float maxDist = CWeaponGhost::GetGhostRangeInHammerUnits();
		if (distTo <= maxDist)
		{
			closestEnemy = Min(closestEnemy, distTo);
			if (showGhost)
				DrawPlayer(distTo, enemyPos);
		}
	};

	// Human and bot enemies.
	for (int i = 0; i < enemyCount; ++i)
	{
		processBeacon(enemyTeam->GetPlayer(i));
	}
	// Dummy entity enemies
	for (auto dummy = C_NEO_NPCDummy::GetList(); dummy; dummy = dummy->m_pNext)
	{
		processBeacon(dummy);
	}

	// The server may restrict ghoster access to off-PVS enemy data, and in such a case
	// we can't play these ghost ping sounds client-side (because we don't know how close they are).
	// But if that's not the case, then we can handle the sounds here.
	if (!sv_neo_serverside_beacons.GetBool())
		ghost->TryGhostPing(closestEnemy * METERS_PER_INCH);
}

void CNEOHud_GhostBeacons::Paint()
{
	BaseClass::Paint();
	PaintNeoElement();
}

void CNEOHud_GhostBeacons::DrawPlayer(float distance, const Vector& playerPos) const
{	
	const auto distInMeters = distance * METERS_PER_INCH;
	const float scale = cl_neo_ghost_beacon_scale.GetFloat();
	constexpr float HALF_BASE_TEX_LENGTH = 32;

	float halfBeaconLength = HALF_BASE_TEX_LENGTH * scale;
	int posX, posY;
	Vector ghostBeaconOffset = Vector(0, 0, cl_neo_ghost_beacon_scale_with_distance.GetBool() ? 32 : 48); // The center of the player, or raise slightly if not scaling
	GetVectorInScreenSpace(playerPos, posX, posY, &ghostBeaconOffset);
	if (cl_neo_ghost_beacon_scale_with_distance.GetBool())
	{
		int pos2X, pos2Y;
		Vector ghostBeaconOffset2 = Vector(0, 0, 64); // The top of the player
		GetVectorInScreenSpace(playerPos, pos2X, pos2Y, &ghostBeaconOffset2);
		halfBeaconLength = (posY - pos2Y) * 0.5 * scale;
	}
	
	wchar_t m_wszBeaconTextUnicode[4 + 1];
	V_snwprintf(m_wszBeaconTextUnicode, ARRAYSIZE(m_wszBeaconTextUnicode), L"%02d m", FastFloatToSmallInt(distInMeters));

	const auto ghostViewDist = sv_neo_ghost_view_distance.GetFloat();
	const float alphaMultiplier = (distInMeters < ghostViewDist * 35.f / 45) ? 1.0 : ((ghostViewDist - distInMeters) / 10);
	const int alpha = cl_neo_ghost_beacon_alpha.GetInt() * alphaMultiplier;
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
