#pragma once

#include <vgui/VGUI.h>

// A debug view of what the HUD draws, for checking text against the font's line box and the boxes around it.
// cl_neo_hud_debug_draw outlines every text drawn through neo_hud_draw.h and every box a panel reports, and
// cl_neo_hud_debug_dump writes the same rectangles to a text file, in screen pixels.
namespace NeoHud
{
struct InkRange;

// Whether the overlay is on or a dump is waiting, so a caller can skip work that only feeds the debug view
bool IsDebugActive();

// Names the HUD element whose draws follow and gives its panel's place on screen, since a panel draws in its
// own coordinates. A null name ends the element.
void SetDebugElement(const char *pszName, int xPanel = 0, int yPanel = 0);

// Remembers a scheme font's name, so the dump can say which font a handle is
void NameDebugFont(vgui::HFont font, const char *pszName);

// A run of text drawn from the pen position xPen on yBaseline. ink is the span the caller placed it by.
// iDigitCellWide is the width every digit is centered in, or zero for text that keeps its own advances.
void DebugText(vgui::HFont font, int xPen, int yBaseline, const wchar_t *pwszText, float flTracking, const InkRange &ink, int iDigitCellWide = 0);

// The surface's DrawPrintText and DrawUnicodeChar, for a draw that sets the surface's font and text position
// itself. They draw exactly what the surface would, and let the debug view see it.
void DrawPrintText(const wchar_t *pwszText, int iLength);
void DrawUnicodeChar(wchar_t wch);

// A glyph drawn from its rendered quad at flScale of the font's size, as the ammo rail draws its smaller rounds.
// Its line box is the font's, scaled about the baseline. ink is the span its visible pixels were placed in.
void DebugScaledGlyph(vgui::HFont font, wchar_t wch, float flScale, int xPen, int yBaseline, const InkRange &ink);

// A box that holds other draws, such as a panel's background or a column inside it
void DebugBox(const char *pszName, int x0, int y0, int x1, int y1);
}
