#pragma once

#include <vgui/VGUI.h>

namespace vgui
{
class IScheme;
}

// Drawing mechanics shared by the HUD panels: scheme fonts with a fallback, and sizes written at 1080p
namespace NeoHud
{
// A scheme font by name. When ClientScheme.res has no such font, this warns in the console and gives
// hFallback instead, so a custom scheme from before the font existed keeps drawing. A chain of lookups
// reads top-down: look up a fallback before the font that falls back to it.
vgui::HFont GetFont(vgui::IScheme *pScheme, const char *pszName, vgui::HFont hFallback = vgui::INVALID_FONT, bool bProportional = true);

// A size in pixels at the reference resolution of 1080p, scaled to the screen's height. A nonzero size stays at least one pixel.
int ScaledPx(int iReferencePx);
}
