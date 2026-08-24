#include "neo_hud_place_name.h"

#include "iclientmode.h"
#include <vgui/ISurface.h>
#include "c_neo_player.h"

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

DECLARE_NAMED_HUDELEMENT(CNEOHud_PlaceName, neo_place_name);

NEO_HUD_ELEMENT_DECLARE_FREQ_CVAR(PlaceName, 0.1)

CNEOHud_PlaceName::CNEOHud_PlaceName(const char *pElementName, vgui::Panel *parent)
	: CHudElement(pElementName), Panel(parent, pElementName)
{
	SetAutoDelete(true);
	m_iHideHudElementNumber = NEO_HUD_ELEMENT_PLACE_NAME;

	if (parent) {
		SetParent(parent);
	}
	else
	{
		SetParent(g_pClientMode->GetViewport());
	}
	
    m_szPlaceName[0] = L'\0';

	SetVisible(true);
}

void CNEOHud_PlaceName::ApplySchemeSettings(vgui::IScheme* pScheme)
{
	BaseClass::ApplySchemeSettings(pScheme);

	const int tall = vgui::surface()->GetFontTall(textFont);
	wide = GetWide();
	SetBounds(xpos, ypos, wide, tall);

	SetFgColor(COLOR_TRANSPARENT);
	SetBgColor(COLOR_TRANSPARENT);
}

enum
{
	TEXTALIGN_LEFT = 0,
	TEXTALIGN_CENTER,
	TEXTALIGN_RIGHT
};
void CNEOHud_PlaceName::UpdateStateForNeoHudElementDraw()
{
	C_NEO_Player* pTargetPlayer = C_NEO_Player::GetLocalNEOPlayer();
	if (!pTargetPlayer)
	{
		return;
	}

	if (pTargetPlayer->IsPlayerDead())
	{
		if (const int observerMode = pTargetPlayer->GetObserverMode();
			observerMode == OBS_MODE_IN_EYE || observerMode == OBS_MODE_CHASE)
		{
			if (C_BaseEntity* pObserverTarget = pTargetPlayer->GetObserverTarget();
				pObserverTarget && pObserverTarget->IsPlayer())
			{
				pTargetPlayer = static_cast<C_NEO_Player*>(pObserverTarget);
			}
		}
	}

	V_snwprintf(m_szPlaceName, MAX_PLACE_NAME_LENGTH, L"%hs", pTargetPlayer->GetLastKnownPlaceName());
	switch (textXAlignment)
	{
		case TEXTALIGN_LEFT:
		default:
			textXOffset = 0;
			break;
		case TEXTALIGN_CENTER:
		case TEXTALIGN_RIGHT:
			int textWidth = 0, textHeight = 0;
			vgui::surface()->GetTextSize(textFont, m_szPlaceName, textWidth, textHeight);
			textXOffset = textXAlignment == TEXTALIGN_CENTER ? (wide / 2) - (textWidth / 2) : wide - textWidth;
			break;
	}
}

ConVar cl_neo_hud_place_name_draw("cl_neo_hud_place_name_draw", "1", FCVAR_ARCHIVE, "Draw the place name");
void CNEOHud_PlaceName::DrawNeoHudElement()
{
	if (!ShouldDraw())
		return;

	if (!cl_neo_hud_place_name_draw.GetBool())
		return;

	vgui::surface()->DrawSetTextFont(textFont);
	vgui::surface()->DrawSetTextColor(textColor);
	vgui::surface()->DrawSetTextPos(textXOffset, 0);
	vgui::surface()->DrawPrintText(m_szPlaceName, V_wcslen(m_szPlaceName));
}

void CNEOHud_PlaceName::Paint()
{
	BaseClass::Paint();
	PaintNeoElement();
}