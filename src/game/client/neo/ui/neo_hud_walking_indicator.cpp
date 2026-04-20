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

	m_hWalkingIndicatorTexture = vgui::surface()->CreateNewTextureID();
	Assert(m_hWalkingIndicatorTexture > 0);
	vgui::surface()->DrawSetTextureFile(m_hWalkingIndicatorTexture, "vgui/hud/player/walkingIndicator", 1, false);

	SetVisible(true);
}

void CNEOHud_WalkingIndicator::ApplySchemeSettings(vgui::IScheme* pScheme)
{
	BaseClass::ApplySchemeSettings(pScheme);
	
	SetBounds(xpos, ypos-tall, wide, tall);
	SetFgColor(COLOR_TRANSPARENT);
	SetBgColor(COLOR_TRANSPARENT);
}

void CNEOHud_WalkingIndicator::UpdateStateForNeoHudElementDraw()
{
}

void CNEOHud_WalkingIndicator::DrawNeoHudElement()
{
	if (!ShouldDraw())
		return;

	C_NEO_Player* pLocalPlayer = C_NEO_Player::GetLocalNEOPlayer();
	if (!pLocalPlayer)
		return;

	C_NEO_Player* pTargetPlayer = pLocalPlayer->IsObserver() && (pLocalPlayer->GetObserverMode() == OBS_MODE_IN_EYE || pLocalPlayer->GetObserverMode() == OBS_MODE_CHASE) ? static_cast<C_NEO_Player *>(pLocalPlayer->GetObserverTarget()) : pLocalPlayer;
	if (!pTargetPlayer)
		return;

	if (!pTargetPlayer->IsAlive() || !pTargetPlayer->IsWalking())// && !pTargetPlayer->IsInAim()) player is also silent if aiming and not abusing the threshold, but its much harder to make noise when aiming, better for the indicator to strictly appear when walk is pressed
		return;

	// we don't want the subrectangle taken from the texture to vary in size when speed changes but the split between the top and bottom image remains in the same spot since texture can be larger than the area its drawn to
	const int pictureSplitDistanceInPixels = tall * (1 - Max(0.f, Min(1.f, pTargetPlayer->SpeedFractionToSoundThreshold())));
	const float pictureSplitDistanceFraction = (float)pictureSplitDistanceInPixels / tall;

	vgui::surface()->DrawSetTexture(m_hWalkingIndicatorTexture);
	vgui::surface()->DrawSetColor(COLOR_WHITE);
	const float ICON_WIDTH = 1 / 2.f;
	const float ICON_HEIGHT = 1 / 2.f;
	vgui::surface()->DrawTexturedSubRect(0, 0, wide, pictureSplitDistanceInPixels, 
		0, ICON_HEIGHT, ICON_WIDTH, ICON_HEIGHT + (ICON_HEIGHT * pictureSplitDistanceFraction));
	vgui::surface()->DrawSetColor(255, 255 * pictureSplitDistanceFraction, 255 * pictureSplitDistanceFraction, 255);
	vgui::surface()->DrawTexturedSubRect(0, pictureSplitDistanceInPixels, wide, tall, 
		0, ICON_HEIGHT * pictureSplitDistanceFraction, ICON_WIDTH, ICON_HEIGHT);
}

void CNEOHud_WalkingIndicator::Paint()
{
	BaseClass::Paint();
	PaintNeoElement();
}