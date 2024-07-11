#include "cbase.h"
#include "neo_hud_ghost_uplink_state.h"

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

DECLARE_NAMED_HUDELEMENT(CNEOHud_GhostUplinkState, neo_ghost_uplink_state);

NEO_HUD_ELEMENT_DECLARE_FREQ_CVAR(GhostUplinkState, 0.1)

CNEOHud_GhostUplinkState::CNEOHud_GhostUplinkState(const char *pElementName, vgui::Panel *parent)
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

	m_hTex0 = surface()->CreateNewTextureID();
	m_hTex1 = surface()->CreateNewTextureID();
	m_hTex2 = surface()->CreateNewTextureID();
	m_hTex3 = surface()->CreateNewTextureID();
	Assert(m_hTex0 > 0);
	Assert(m_hTex1 > 0);
	Assert(m_hTex2 > 0);
	Assert(m_hTex3 > 0);
	surface()->DrawSetTextureFile(m_hTex0, "vgui/hud/ctg/g_gscan_00", 1, false);
	surface()->DrawSetTextureFile(m_hTex1, "vgui/hud/ctg/g_gscan_01", 1, false);
	surface()->DrawSetTextureFile(m_hTex2, "vgui/hud/ctg/g_gscan_02", 1, false);
	surface()->DrawSetTextureFile(m_hTex3, "vgui/hud/ctg/g_gscan_03", 1, false);
	m_pUplinkTextures[0] = m_hTex0;
	m_pUplinkTextures[1] = m_hTex1;
	m_pUplinkTextures[2] = m_hTex2;
	m_pUplinkTextures[3] = m_hTex3;

	surface()->DrawGetTextureSize(m_hTex0, m_uplinkTexWidth, m_uplinkTexHeight);

	SetVisible(true);
}

void CNEOHud_GhostUplinkState::ApplySchemeSettings(vgui::IScheme* pScheme)
{
	vgui::IScheme* scheme = vgui::scheme()->GetIScheme(vgui::scheme()->GetDefaultScheme());
	Assert(scheme);

	int wide, tall;
	surface()->GetScreenSize(wide, tall);
	int centerX = wide / 2;
	int centerY = tall / 2;

	textureXPos = centerX - (m_uplinkTexWidth / 2);
	constexpr int compassHeightPlusMargins = 30;
	textureYPos = tall - m_uplinkTexHeight - compassHeightPlusMargins;

	SetBounds(0, 0, wide, tall);

	BaseClass::ApplySchemeSettings(pScheme);
}

void CNEOHud_GhostUplinkState::UpdateStateForNeoHudElementDraw()
{
}

void CNEOHud_GhostUplinkState::DrawNeoHudElement()
{
	if (!ShouldDraw())
	{
		return;
	}

	auto localPlayer = C_NEO_Player::GetLocalNEOPlayer();
	auto ghost = dynamic_cast<C_WeaponGhost*>(localPlayer->GetActiveWeapon());
	if (!ghost)
	{
		m_flTimeGhostEquip = 0.f;
		currentTexture = 0;
		return;
	}

	if (m_flTimeGhostEquip == 0.f)
		m_flTimeGhostEquip = gpGlobals->curtime;

	if (currentTexture < (numTextureStartTimes - 1))
		if ((gpGlobals->curtime - m_flTimeGhostEquip) >= textureStartTimes[currentTexture + 1])
			currentTexture++;

	surface()->DrawSetColor(COLOR_RED);
	surface()->DrawSetTexture(m_pUplinkTextures[textureOrder[currentTexture]]);
	surface()->DrawTexturedRect(textureXPos, textureYPos, textureXPos + m_uplinkTexWidth, textureYPos + m_uplinkTexHeight);
}

void CNEOHud_GhostUplinkState::Paint()
{
	BaseClass::Paint();
	PaintNeoElement();
}