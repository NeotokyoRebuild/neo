#include "cbase.h"
#include "neo_hud_childelement.h"

#include "c_neo_player.h"

#include "clientmode_hl2mpnormal.h"

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

using vgui::surface;

#define NEO_HUDBOX_CORNER_SCALE 1.0

// NEO TODO (Rain): this should be expanded into two margin_width/height cvars, so players can tweak their HUD position if they wish to.
ConVar cl_neo_hud_margin("cl_neo_hud_margin", "2", FCVAR_USERINFO, "Neo HUD margin, in pixels.", true, 0.0, false, 0.0);

CNEOHud_ChildElement::CNEOHud_ChildElement()
{
	m_flLastUpdateTime = 0;

	m_hTex_Rounded_NW = surface()->DrawGetTextureId("vgui/hud/8x800corner1");
	m_hTex_Rounded_NE = surface()->DrawGetTextureId("vgui/hud/8x800corner2");
	m_hTex_Rounded_SW = surface()->DrawGetTextureId("vgui/hud/8x800corner3");
	m_hTex_Rounded_SE = surface()->DrawGetTextureId("vgui/hud/8x800corner4");
	Assert(m_hTex_Rounded_NE > 0);
	Assert(m_hTex_Rounded_NW > 0);
	Assert(m_hTex_Rounded_SE > 0);
	Assert(m_hTex_Rounded_SW > 0);

#ifdef DEBUG
	// We assume identical dimensions for all 4 corners.
	int width_ne, width_nw, width_se, width_sw;
	int height_ne, height_nw, height_se, height_sw;
	surface()->DrawGetTextureSize(m_hTex_Rounded_NE, width_ne, height_ne);
	surface()->DrawGetTextureSize(m_hTex_Rounded_NW, width_nw, height_nw);
	surface()->DrawGetTextureSize(m_hTex_Rounded_SE, width_se, height_se);
	surface()->DrawGetTextureSize(m_hTex_Rounded_SW, width_sw, height_sw);
	Assert(width_ne == width_nw && width_ne == width_se && width_ne == width_sw);
	Assert(height_ne == height_nw && height_ne == height_se && height_ne == height_sw);
#endif

	// Only need to store one width/height for identically shaped corner pieces.
	// NEO NOTE (nullsystem): See: Panel::GetCornerTextureSize, legacy non-8x was always w/h 8
	// NEO TODO (nullsystem): Should actually be scaling with higher res, so remove fixed
	// usage of LEGACY800CORNERWH eventually
	static constexpr int LEGACY800CORNERWH = 8;
	m_rounded_width = LEGACY800CORNERWH * NEO_HUDBOX_CORNER_SCALE;
	m_rounded_height = LEGACY800CORNERWH * NEO_HUDBOX_CORNER_SCALE;
	Assert(m_rounded_width > 0 && m_rounded_height > 0);
}

void CNEOHud_ChildElement::resetLastUpdateTime()
{
	m_flLastUpdateTime = 0.0f;
}

CNEOHud_ChildElement::XYHudPos CNEOHud_ChildElement::DrawNeoHudRoundedCommon(
	const int x0, const int y0, const int x1, const int y1, Color color,
	bool topLeft, bool topRight, bool bottomLeft, bool bottomRight) const
{
	const XYHudPos p{
		.x0w = x0 + m_rounded_width,
		.x1w = x1 - m_rounded_width,
		.y0h = y0 + m_rounded_height,
		.y1h = y1 - m_rounded_height,
	};

	surface()->DrawSetColor(color);

	// Rounded corner pieces
	if (topLeft)
	{
		surface()->DrawSetTexture(m_hTex_Rounded_NW);
		surface()->DrawTexturedRect(x0, y0, p.x0w, p.y0h);
	}
	if (topRight)
	{
		surface()->DrawSetTexture(m_hTex_Rounded_NE);
		surface()->DrawTexturedRect(p.x1w, y0, x1, p.y0h);
	}
	if (bottomLeft)
	{
		surface()->DrawSetTexture(m_hTex_Rounded_SE);
		surface()->DrawTexturedRect(x0, p.y1h, p.x0w, y1);
	}
	if (bottomRight)
	{
		surface()->DrawSetTexture(m_hTex_Rounded_SW);
		surface()->DrawTexturedRect(p.x1w, p.y1h, x1, y1);
	}

	return p;
}

void CNEOHud_ChildElement::DrawNeoHudRoundedBox(const int x0, const int y0, const int x1, const int y1, Color color,
	bool topLeft, bool topRight, bool bottomLeft, bool bottomRight) const
{
	const auto p = DrawNeoHudRoundedCommon(x0, y0, x1, y1, color, topLeft, topRight, bottomLeft, bottomRight);

	// Large middle rectangle
	surface()->DrawFilledRect(p.x0w, y0, p.x1w, y1);

	// Small side rectangles
	surface()->DrawFilledRect(x0, topLeft ? p.y0h : y0, p.x0w, bottomLeft ? p.y1h : y1);
	surface()->DrawFilledRect(p.x1w, topRight ? p.y0h : y0, x1, bottomRight ? p.y1h : y1);
}

void CNEOHud_ChildElement::DrawNeoHudRoundedBoxFaded(const int x0, const int y0, const int x1, const int y1, Color color,
	unsigned int alpha0, unsigned int alpha1, bool bHorizontal,
	bool topLeft, bool topRight, bool bottomLeft, bool bottomRight) const
{
	const auto p = DrawNeoHudRoundedCommon(x0, y0, x1, y1, color, topLeft, topRight, bottomLeft, bottomRight);

	// Large middle rectangle
	surface()->DrawFilledRectFade(p.x0w, y0, p.x1w, y1, alpha0, alpha1, bHorizontal);

	// Small side rectangles
	surface()->DrawFilledRectFade(x0, topLeft ? p.y0h : y0, p.x0w, bottomLeft ? p.y1h : y1, alpha0, alpha0, bHorizontal);
	surface()->DrawFilledRectFade(p.x1w, topRight ? p.y0h : y0, x1, bottomRight ? p.y1h : y1, alpha1, alpha1, bHorizontal);
}

int CNEOHud_ChildElement::GetMargin()
{
	return cl_neo_hud_margin.GetInt();
}

