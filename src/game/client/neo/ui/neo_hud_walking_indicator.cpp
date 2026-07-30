#include "cbase.h"
#include "neo_hud_walking_indicator.h"

#include "iclientmode.h"
#include <vgui/ISurface.h>

#include "c_neo_player.h"

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

DECLARE_NAMED_HUDELEMENT(CNEOHud_WalkingIndicator, neo_walking_indicator);

NEO_HUD_ELEMENT_DECLARE_FREQ_CVAR(WalkingIndicator, 0.1)

CNEOHud_WalkingIndicator::CNEOHud_WalkingIndicator(const char *pElementName, vgui::Panel *parent)
	: CHudElement(pElementName), Panel(parent, pElementName)
{
	SetAutoDelete(true);
	m_iHideHudElementNumber = NEO_HUD_ELEMENT_WALKING_INDICATOR;

	if (parent) {
		SetParent(parent);
	}
	else
	{
		SetParent(g_pClientMode->GetViewport());
	}

	SetVisible(true);
}

void CNEOHud_WalkingIndicator::ApplySchemeSettings(vgui::IScheme* pScheme)
{
	BaseClass::ApplySchemeSettings(pScheme);
	
	SetBounds(xpos, ypos, wide, tall);
	SetFgColor(COLOR_TRANSPARENT);
	SetBgColor(COLOR_TRANSPARENT);
}

void CNEOHud_WalkingIndicator::UpdateStateForNeoHudElementDraw()
{
}

void CNEOHud_WalkingIndicator::DrawNeoHudElement()
{
	if (!ShouldDraw() || !visible)
		return;

	C_NEO_Player* pLocalPlayer = C_NEO_Player::GetLocalNEOPlayer();
	if (!pLocalPlayer)
		return;

	C_NEO_Player* pTargetPlayer = pLocalPlayer->IsObserver() && (pLocalPlayer->GetObserverMode() == OBS_MODE_IN_EYE || pLocalPlayer->GetObserverMode() == OBS_MODE_CHASE) ? static_cast<C_NEO_Player *>(pLocalPlayer->GetObserverTarget()) : pLocalPlayer;
	if (!pTargetPlayer)
		return;

	if (!pTargetPlayer->IsAlive() || !pTargetPlayer->IsWalking())
		return;
	
	vgui::surface()->DrawSetTexture(texture);
	vgui::surface()->DrawSetColor(color);
	vgui::surface()->DrawTexturedRect(0, 0, wide, tall);

	if (barwidth)
	{
		const float percentageTowardsGraceThreshold = pTargetPlayer->SpeedFractionToSoundThreshold();
		const float colourGB = 255 - (255 * (percentageTowardsGraceThreshold < 1 ? percentageTowardsGraceThreshold * 0.5f : 1));
		vgui::surface()->DrawSetColor(255, colourGB, colourGB, 255);
		vgui::surface()->DrawFilledRect(0, tall - (tall * percentageTowardsGraceThreshold), barwidth, tall);
	}
}

void CNEOHud_WalkingIndicator::Paint()
{
	BaseClass::Paint();
	PaintNeoElement();
}