#pragma once

#include <vgui/VGUI.h>

namespace vgui
{
class IScheme;
}

// Drawing mechanics shared by the HUD panels: scheme fonts with a fallback, sizes written at 1080p,
// and text placed by its baseline and visible edges.
namespace NeoHud
{
// The on-screen span of what a draw call put down, from its leftmost to its rightmost visible pixel
struct InkRange
{
	int iLeft;
	int iRight;
};

// A scheme font by name. When ClientScheme.res has no such font, this warns in the console and gives
// hFallback instead, so a custom scheme from before the font existed keeps drawing. A chain of lookups
// reads top-down: look up a fallback before the font that falls back to it.
vgui::HFont GetFont(vgui::IScheme *pScheme, const char *pszName, vgui::HFont hFallback = vgui::INVALID_FONT, bool bProportional = true);

// A size in pixels at the reference resolution of 1080p, scaled to the screen's height. A nonzero size stays at least one pixel.
int ScaledPx(int iReferencePx);

// Text placed by its baseline and visible edges, so text in different fonts lines up. The caller sets the color.
// flTracking adds that many pixels between characters. Each draw returns the ink it put on screen.
int GetTextY(vgui::HFont font, int yBaseline);
int GetTextWide(vgui::HFont font, const wchar_t *pwszText, float flTracking = 0.0f);
// The visible span the text would have, relative to its pen position, for laying out before drawing
InkRange GetTextInk(vgui::HFont font, const wchar_t *pwszText, float flTracking = 0.0f);
InkRange DrawTextLeft(vgui::HFont font, int xInkLeft, int yBaseline, const wchar_t *pwszText, float flTracking = 0.0f);
InkRange DrawTextRight(vgui::HFont font, int xInkRight, int yBaseline, const wchar_t *pwszText, float flTracking = 0.0f);
InkRange DrawTextCentered(vgui::HFont font, int xCenter, int yBaseline, const wchar_t *pwszText, float flTracking = 0.0f);
// One glyph with its visible pixels centered in a column
InkRange DrawGlyphInColumn(vgui::HFont font, int xColumn, int iColumnWide, int yBaseline, wchar_t wch);

// Numbers with each digit centered in a cell as wide as the widest digit, so they hold still as they change.
// Other characters, such as the x in "x4", keep their own width.
int GetNumberWide(vgui::HFont font, const wchar_t *pwszNumber, float flTracking = 0.0f);
InkRange GetNumberInk(vgui::HFont font, const wchar_t *pwszNumber, float flTracking = 0.0f);
InkRange DrawNumberLeft(vgui::HFont font, int xInkLeft, int yBaseline, const wchar_t *pwszNumber, float flTracking = 0.0f);
InkRange DrawNumberRight(vgui::HFont font, int xInkRight, int yBaseline, const wchar_t *pwszNumber, float flTracking = 0.0f);
InkRange DrawNumberCentered(vgui::HFont font, int xCenter, int yBaseline, const wchar_t *pwszNumber, float flTracking = 0.0f);

// The readout style for instrument text: its font is sized a little below the text around it in the scheme,
// and this gives it a little extra space between characters, in proportion to the font's ascent.
float GetReadoutTracking(vgui::HFont font);

}
