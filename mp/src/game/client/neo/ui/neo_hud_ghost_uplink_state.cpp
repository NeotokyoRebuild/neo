#include "cbase.h"
#include "neo_hud_ghost_uplink_state.h"

#include "iclientmode.h"
#include <vgui/ISurface.h>

#include "c_neo_player.h"
#include "weapon_ghost.h"

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

using vgui::surface;

DECLARE_NAMED_HUDELEMENT(CNEOHud_GhostUplinkState, neo_ghost_uplink_state);

NEO_HUD_ELEMENT_DECLARE_FREQ_CVAR(GhostUplinkState, 0.1)

CNEOHud_GhostUplinkState::CNEOHud_GhostUplinkState(const char *pElementName, vgui::Panel *parent)
	: CHudElement(pElementName), Panel(parent, pElementName)
{
	SetAutoDelete(true);

	SetScheme("ClientScheme.res");

	if (parent) {
		SetParent(parent);
	}
	else {
		SetParent(g_pClientMode->GetViewport());
	}

	for (int i = 0; i < numUplinkTextures; i++) {
		m_pUplinkTextures[i] = surface()->CreateNewTextureID();
		Assert(m_pUplinkTextures[i] > 0);
		char textureFilePath[24];
		V_sprintf_safe(textureFilePath, "%s%i", "vgui/hud/ctg/g_gscan_0", i);
		surface()->DrawSetTextureFile(m_pUplinkTextures[i], textureFilePath, 1, false);
	}

	surface()->DrawGetTextureSize(m_pUplinkTextures[0], m_uplinkTexWidth, m_uplinkTexHeight);

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
	SetBounds(textureXPos, textureYPos, m_uplinkTexWidth, m_uplinkTexHeight);

	BaseClass::ApplySchemeSettings(pScheme);
}

void CNEOHud_GhostUplinkState::UpdateStateForNeoHudElementDraw()
{
}

void CNEOHud_GhostUplinkState::DrawNeoHudElement()
{
	if (!ShouldDraw()) {
		return;
	}

	auto localPlayer = C_NEO_Player::GetLocalNEOPlayer();
	auto ghost = dynamic_cast<C_WeaponGhost*>(localPlayer->GetActiveWeapon());
	if (!ghost) {
		m_flTimeGhostEquip = 0.f;
		currentTexture = 0;
		return;
	}

	if (m_flTimeGhostEquip == 0.f) {
		m_flTimeGhostEquip = gpGlobals->curtime;
	}

	if (currentTexture < (numTextureStartTimes - 1)) {
		if ((gpGlobals->curtime - m_flTimeGhostEquip) >= textureStartTimes[currentTexture + 1])
			currentTexture++;
	}

	surface()->DrawSetColor(COLOR_RED);
	surface()->DrawSetTexture(m_pUplinkTextures[textureOrder[currentTexture]]);
	surface()->DrawTexturedRect(0, 0, m_uplinkTexWidth, m_uplinkTexHeight);
}

void CNEOHud_GhostUplinkState::Paint()
{
	BaseClass::Paint();
	SetFgColor(COLOR_TRANSPARENT);
	SetBgColor(COLOR_TRANSPARENT);
	PaintNeoElement();
}