#include "cbase.h"
#include "neo_hud_childelement.h"

#include "neo_hud_elements.h"
#include "c_neo_player.h"

#include "clientmode_hl2mpnormal.h"

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

using vgui::surface;

#define NEO_HUDBOX_COLOR Color(116, 116, 116, 178)
#define NEO_HUDBOX_CORNER_SCALE 1.0

// NEO TODO (Rain): this should be expanded into two margin_width/height cvars, so players can tweak their HUD position if they wish to.
ConVar neo_cl_hud_margin("neo_cl_hud_margin", "2", FCVAR_USERINFO, "Neo HUD margin, in pixels.", true, 0.0, false, 0.0);

CNEOHud_ChildElement::CNEOHud_ChildElement()
{
	m_pNeoHud = dynamic_cast<CNeoHudElements*>(GetClientModeNormal()->GetViewport()->FindChildByName(PANEL_NEO_HUD));
	Assert(m_pNeoHud);

	m_flLastUpdateTime = 0;

	m_hTex_Rounded_NW = surface()->DrawGetTextureId("vgui/hud/800corner1");
	m_hTex_Rounded_NE = surface()->DrawGetTextureId("vgui/hud/800corner2");
	m_hTex_Rounded_SW = surface()->DrawGetTextureId("vgui/hud/800corner3");
	m_hTex_Rounded_SE = surface()->DrawGetTextureId("vgui/hud/800corner4");
	Assert(m_hTex_Rounded_NE > 0);
	Assert(m_hTex_Rounded_NW > 0);
	Assert(m_hTex_Rounded_SE > 0);
	Assert(m_hTex_Rounded_SW > 0);

	// We assume identical dimensions for all 4 corners.
	int width_ne, width_nw, width_se, width_sw;
	int height_ne, height_nw, height_se, height_sw;
	surface()->DrawGetTextureSize(m_hTex_Rounded_NE, width_ne, height_ne);
	surface()->DrawGetTextureSize(m_hTex_Rounded_NW, width_nw, height_nw);
	surface()->DrawGetTextureSize(m_hTex_Rounded_SE, width_se, height_se);
	surface()->DrawGetTextureSize(m_hTex_Rounded_SW, width_sw, height_sw);
	Assert(width_ne == width_nw && width_ne == width_se && width_ne == width_sw);
	Assert(height_ne == height_nw && height_ne == height_se && height_ne == height_sw);

	// Only need to store one width/height for identically shaped corner pieces.
	m_rounded_width = width_ne * NEO_HUDBOX_CORNER_SCALE;
	m_rounded_height = height_ne * NEO_HUDBOX_CORNER_SCALE;
	Assert(m_rounded_width > 0 && m_rounded_height > 0);
}

void CNEOHud_ChildElement::DrawNeoHudRoundedBox(const int x0, const int y0, const int x1, const int y1) const
{
	const int x0w = x0 + m_rounded_width;
	const int x1w = x1 - m_rounded_width;
	const int y0h = y0 + m_rounded_height;
	const int y1h = y1 - m_rounded_height;

	surface()->DrawSetColor(NEO_HUDBOX_COLOR);

	// Rounded corner pieces
	surface()->DrawSetTexture(m_hTex_Rounded_NW);
	surface()->DrawTexturedRect(x0, y0, x0w, y0h);
	surface()->DrawSetTexture(m_hTex_Rounded_NE);
	surface()->DrawTexturedRect(x1w, y0, x1, y0h);
	surface()->DrawSetTexture(m_hTex_Rounded_SE);
	surface()->DrawTexturedRect(x0, y1h, x0w, y1);
	surface()->DrawSetTexture(m_hTex_Rounded_SW);
	surface()->DrawTexturedRect(x1w, y1h, x1, y1);

	// Large middle rectangle
	surface()->DrawFilledRect(x0w, y0, x1w, y1);

	// Small side rectangles
	surface()->DrawFilledRect(x0, y0h, x0w, y1h);
	surface()->DrawFilledRect(x1w, y0h, x1, y1h);
}

int CNEOHud_ChildElement::GetMargin()
{
	return neo_cl_hud_margin.GetInt();
}

