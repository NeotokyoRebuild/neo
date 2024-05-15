#include "cbase.h"
#include "neo_hud_ghost_beacons.h"

#include "iclientmode.h"
#include <vgui/ILocalize.h>
#include <vgui/ISurface.h>
#include <vgui_controls/Controls.h>
#include <vgui_controls/ImagePanel.h>

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

ConVar neo_ghost_beacon_alpha("neo_ghost_beacon_alpha", "150", FCVAR_USERINFO,
	"Alpha channel transparency of HUD ghost beacons.", true, 0, true, 255);

NEO_HUD_ELEMENT_DECLARE_FREQ_CVAR(GhostBeacons, 0.01)

CNEOHud_GhostBeacons::CNEOHud_GhostBeacons(const char *pElementName, vgui::Panel *parent)
	: CHudElement(pElementName), Panel(parent, pElementName)
{
	SetAutoDelete(true);

	SetScheme("ClientScheme.res");

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
	vgui::IScheme *scheme = vgui::scheme()->GetIScheme(vgui::scheme()->GetDefaultScheme());
	Assert(scheme);

	m_hFont = scheme->GetFont("Default", true);

	m_hTex = surface()->CreateNewTextureID();
	Assert(m_hTex > 0);
	surface()->DrawSetTextureFile(m_hTex, "vgui/hud/ctg/g_beacon_enemy", 1, false);

	surface()->DrawGetTextureSize(m_hTex, m_beaconTexWidth, m_beaconTexHeight);

	SetVisible(true);
}

static inline double GetColorPulse()
{
	const double startPulse = 0.5;
	static double colorPulse = startPulse;
	static double pulseStep = 0.001;
	colorPulse = clamp(colorPulse + pulseStep, startPulse, 1);
	if (colorPulse >= 1 || colorPulse <= startPulse)
	{
		pulseStep = -pulseStep;
	}

	return colorPulse;
}

void CNEOHud_GhostBeacons::UpdateStateForNeoHudElementDraw()
{
}

void CNEOHud_GhostBeacons::DrawNeoHudElement()
{
	if (!ShouldDraw())
	{
		return;
	}

	auto localPlayer = C_NEO_Player::GetLocalNEOPlayer();
	auto ghost = dynamic_cast<C_WeaponGhost*>(localPlayer->GetActiveWeapon());
	if(!ghost)
	{
		return;
	}

	m_pGhost = ghost;

	auto enemyTeamId = localPlayer->GetTeamNumber() == TEAM_JINRAI ? TEAM_NSF : TEAM_JINRAI;
	auto enemyTeam = GetGlobalTeam(enemyTeamId);
	auto enemyTeamCount = enemyTeam->GetNumPlayers();

	for(int i = 0; i < enemyTeamCount; ++i)
	{
		auto enemyToShow = dynamic_cast<C_NEO_Player*>(enemyTeam->GetPlayer(i));
		auto enemyPos = enemyToShow->GetAbsOrigin();
		float distance;
		if(ghost->IsPosWithinViewDistance(enemyPos, distance) && !enemyToShow->IsDormant())
		{
			DrawPlayer(enemyPos);
		}
	}

	
}

void CNEOHud_GhostBeacons::Paint()
{
	BaseClass::Paint();
	PaintNeoElement();
}

void CNEOHud_GhostBeacons::DrawPlayer(const Vector& playerPos) const
{
	int posX, posY;
	GetVectorInScreenSpace(playerPos, posX, posY);
			
	const Color textColor = Color(220, 180, 180, neo_ghost_beacon_alpha.GetInt());
	auto dist = m_pGhost->DistanceToPos(playerPos);	//Move this to some util

	char m_szBeaconTextANSI[4 + 1];
	wchar_t m_wszBeaconTextUnicode[4 + 1];

	V_snprintf(m_szBeaconTextANSI, sizeof(m_szBeaconTextANSI), "%02d M", FastFloatToSmallInt(dist * METERS_PER_INCH));
	g_pVGuiLocalize->ConvertANSIToUnicode(m_szBeaconTextANSI, m_wszBeaconTextUnicode, sizeof(m_wszBeaconTextUnicode));

	surface()->DrawSetTextColor(textColor);
	surface()->DrawSetTextFont(m_hFont);
	int textWidth, textHeight;
	surface()->GetTextSize(m_hFont, m_wszBeaconTextUnicode, textWidth, textHeight);
	surface()->DrawSetTextPos(posX - (textWidth / 2), posY);
	surface()->DrawPrintText(m_wszBeaconTextUnicode, sizeof(m_szBeaconTextANSI));

	const double colorPulse = GetColorPulse();

	const Color beaconColor = Color(
		colorPulse * 255,
		colorPulse * 20,
		colorPulse * 20,
		neo_ghost_beacon_alpha.GetInt());

	surface()->DrawSetColor(beaconColor);
	surface()->DrawSetTexture(m_hTex);

	float scale = neo_ghost_beacon_scale_baseline.GetInt() / dist;

	// Offset screen space starting positions by half of the texture x/y coords,
	// so it starts centered on target.
	const int posfix_X = posX - ((m_beaconTexWidth / 2) * scale);
	const int posfix_Y = posY - (m_beaconTexHeight * scale);

	// End coordinates according to art size (and our distance scaling)
	surface()->DrawTexturedRect(
		posfix_X,
		posfix_Y,
		posfix_X + (m_beaconTexWidth * scale),
		posfix_Y + (m_beaconTexHeight * scale));
}
