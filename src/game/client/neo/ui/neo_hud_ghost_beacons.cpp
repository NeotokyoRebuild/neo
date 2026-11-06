#include "neo_hud_ghost_beacons.h"

#include "iclientmode.h"

#include "c_neo_npc_dummy.h"
#include "c_neo_player.h"
#include "c_team.h"
#include "neo_gamerules.h"
#include "weapon_ghost.h"

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

ConVar cl_neo_hud_ghost_beacon_scale("cl_neo_hud_ghost_beacon_scale", "1", FCVAR_ARCHIVE,
	"Ghost beacons scale.", true, 0.01, false, 0);

static float ghostBeaconRotation = 0.f;
ConVar cl_neo_hud_ghost_beacon_scale_with_distance("cl_neo_hud_ghost_beacon_scale_with_distance", "0", FCVAR_ARCHIVE,
	"Toggles the scaling of ghost beacons with distance.", true, 0, true, 1, [](IConVar* var, const char* pOldValue, float flOldValue)->void{
		if (!cl_neo_hud_ghost_beacon_scale_with_distance.GetBool())
			ghostBeaconRotation = 0.f;
});

ConVar cl_neo_hud_ghost_beacon_draw_distance("cl_neo_hud_ghost_beacon_draw_distance", "1", FCVAR_ARCHIVE,
	"Draw the distance to the ghost beacons.", true, 0, true, 1);

CNEOHud_GhostBeacons* gHudGhostBeacons;

DECLARE_NAMED_HUDELEMENT(CNEOHud_GhostBeacons, neo_ghost_beacons);

NEO_HUD_ELEMENT_DECLARE_FREQ_CVAR(GhostBeacons, 0.01)

CNEOHud_GhostBeacons::CNEOHud_GhostBeacons(const char *pElementName, vgui::Panel *parent)
	: CHudElement(pElementName), EditablePanel(parent, pElementName)
{
	gHudGhostBeacons = this;

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

	SetVisible(true);
}

CNEOHud_GhostBeacons::~CNEOHud_GhostBeacons()
{
	gHudGhostBeacons = nullptr;
}

void CNEOHud_GhostBeacons::UpdateStateForNeoHudElementDraw()
{
}

void CNEOHud_GhostBeacons::ApplySchemeSettings(vgui::IScheme* pScheme)
{
	BaseClass::ApplySchemeSettings(pScheme);

	vgui::surface()->GetScreenSize(screenWidth, screenHeight);
	SetBounds(0, 0, screenWidth, screenHeight);
	doubleScreenWidth = 2 * screenWidth;
	doubleScreenHeight = 2 * screenHeight;

	SetFgColor(COLOR_TRANSPARENT);
	SetBgColor(COLOR_TRANSPARENT);
}

extern ConVar sv_neo_ctg_ghost_beacons_when_inactive;
void CNEOHud_GhostBeacons::DrawNeoHudElement()
{
	if (!ShouldDraw())
		return;

	auto localPlayer = C_NEO_Player::GetLocalNEOPlayer();
	if (!localPlayer)
		return;

	// Only consider drawing beacons if alive or dead, but not LIFE_DYING (the death animation)
	if (localPlayer->m_lifeState != LIFE_ALIVE &&
		localPlayer->m_lifeState != LIFE_DEAD)
	{
		return;
	}

	auto ghoster = localPlayer->IsObserver()
		? ToNEOPlayer(localPlayer->GetObserverTarget())
		: localPlayer;

	if (!ghoster || !ghoster->m_bCarryingGhost ||
		ghoster->GetTeamNumber() < FIRST_GAME_TEAM ||
		!ghoster->IsAlive() || NEORules()->IsRoundOver() ||
		(engine->IsHLTV() && (localPlayer->GetObserverMode() != OBS_MODE_IN_EYE && localPlayer->GetObserverMode() != OBS_MODE_CHASE)))
	{
		return;
	}
	Assert(ghoster->GetTeamNumber() < TEAM__TOTAL);

	const C_WeaponGhost* ghost;
	if (sv_neo_ctg_ghost_beacons_when_inactive.GetBool())
	{
		ghost = assert_cast<const C_WeaponGhost*>(GetNeoWepWithBits(ghoster, NEO_WEP_GHOST));
		AssertMsg(ghoster->m_bCarryingGhost == !!ghost,
			"ghost ptr and m_bCarryingGhost mismatch");
	}
	else
	{
		auto weapon = assert_cast<C_NEOBaseCombatWeapon*>(ghoster->GetActiveWeapon());
		ghost = (weapon && weapon->IsGhost()) ? static_cast<const C_WeaponGhost*>(weapon) : nullptr;
		AssertMsg(ghoster->m_bCarryingGhost ==
			!!assert_cast<const C_WeaponGhost*>(GetNeoWepWithBits(ghoster, NEO_WEP_GHOST)),
			"ghost ptr and m_bCarryingGhost mismatch");
	}

	if (!ghost || !ghost->IsBootupCompleted())
		return;

	Assert(ghoster->GetTeamNumber() == TEAM_JINRAI || ghoster->GetTeamNumber() == TEAM_NSF);
	auto enemyTeamId = ghoster->GetTeamNumber() == TEAM_JINRAI ? TEAM_NSF : TEAM_JINRAI;
	auto enemyTeam = GetGlobalTeam(enemyTeamId);
	auto enemyCount = enemyTeam->GetNumPlayers();

	// Human and bot enemies.
	for (int i = 0; i < enemyCount; ++i)
	{
		if (auto enemy = enemyTeam->GetPlayer(i))
		{
			float distTo;
			if (ghost->BeaconRange(enemy, distTo))
				DrawPlayer(distTo, enemy->GetAbsOrigin());
		}
	}
	// Dummies.
	for (auto dummy = C_NEO_NPCDummy::GetList(); dummy; dummy = dummy->m_pNext)
	{
		float distTo;
		if (ghost->BeaconRange(dummy, distTo))
			DrawPlayer(distTo, dummy->GetAbsOrigin());
	}
}

void CNEOHud_GhostBeacons::Paint()
{
	BaseClass::Paint();
	PaintNeoElement();
}

void CNEOHud_GhostBeacons::DrawPlayer(float distance, const Vector& playerPos) const
{	
	const float distInMeters = distance * METERS_PER_INCH;
	constexpr float BASE_TEX_LENGTH = 64;
	float halfBeaconLength = BASE_TEX_LENGTH * 0.5f * cl_neo_hud_ghost_beacon_scale.GetFloat();

	int posX, posY;
	Vector ghostBeaconOffset = Vector(0, 0, cl_neo_hud_ghost_beacon_scale_with_distance.GetBool() ? 0 : 48);
	if (!GetVectorInScreenSpace(playerPos, posX, posY, &ghostBeaconOffset))
	{
		return;
	}
	bool centerXClamped = clamp(&posX, -screenWidth, doubleScreenWidth);
	bool centerYClamped = clamp(&posY, -screenHeight, doubleScreenHeight);

	if (cl_neo_hud_ghost_beacon_scale_with_distance.GetBool())
	{
		if (centerXClamped && centerYClamped)
		{
			return;
		}

		int pos2X, pos2Y;
		Vector ghostBeaconEyeOffset = Vector(0, 0, 64);
		if (!GetVectorInScreenSpace(playerPos, pos2X, pos2Y, &ghostBeaconEyeOffset))
		{
			return;
		}

		if (clamp(&pos2X, -screenWidth, doubleScreenWidth) && clamp(&pos2Y, -screenHeight, doubleScreenHeight))
		{
			return;
		}

		Vector2D playerUpDirectionOnScreen = Vector2D(pos2X - posX, pos2Y - posY);
		const Vector2D screenUpDirection = Vector2D(0, 1);
		float rotation = acos((playerUpDirectionOnScreen.Dot(screenUpDirection)) / playerUpDirectionOnScreen.Length()) + M_PI;
		if (pos2X < posX)
		{
			rotation *= -1;
		}
		ghostBeaconRotation = RAD2DEG(rotation);

		halfBeaconLength = (posY - pos2Y) * 0.5f * cl_neo_hud_ghost_beacon_scale.GetFloat();
		posX = (posX + pos2X) / 2;
		posY = (posY + pos2Y) / 2;
	}

	const auto ghostViewDist = sv_neo_ghost_view_distance.GetFloat();
	const float alphaMultiplier = distInMeters < (ghostViewDist * 35.f / 45) ? 1.0f : (ghostViewDist - distInMeters) / 10;
	const int alpha = beaconColor.a() * alphaMultiplier;

	if (cl_neo_hud_ghost_beacon_draw_distance.GetBool())
	{
		wchar_t m_wszBeaconTextUnicode[4 + 1];
		V_snwprintf(m_wszBeaconTextUnicode, ARRAYSIZE(m_wszBeaconTextUnicode), L"%02d m", FastFloatToSmallInt(distInMeters));

		vgui::surface()->DrawSetTextColor(255, 255, 255, alpha);
		vgui::surface()->DrawSetTextFont(m_hFont);
		int textWidth, textHeight;
		vgui::surface()->GetTextSize(m_hFont, m_wszBeaconTextUnicode, textWidth, textHeight);
		vgui::surface()->DrawSetTextPos(posX - (textWidth / 2), posY + halfBeaconLength);
		vgui::surface()->DrawPrintText(m_wszBeaconTextUnicode, V_wcslen(m_wszBeaconTextUnicode));
	}

	vgui::surface()->DrawSetColor(beaconColor.r(), beaconColor.g(), beaconColor.b(), alpha);
	vgui::surface()->DrawSetTexture(m_hTex);
	vgui::surface()->DrawTexturedRect(
		posX - halfBeaconLength,
		posY - halfBeaconLength,
		posX + halfBeaconLength,
		posY + halfBeaconLength);
}

float CNEOHud_GhostBeacons::GetRotation()
{
	return ghostBeaconRotation;
}
