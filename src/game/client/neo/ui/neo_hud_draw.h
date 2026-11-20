#pragma once

// Drawing mechanics shared by the HUD panels
namespace NeoHud
{
// A size in pixels at the reference resolution of 1080p, scaled to the screen's height. A nonzero size stays at least one pixel.
int ScaledPx(int iReferencePx);
}
