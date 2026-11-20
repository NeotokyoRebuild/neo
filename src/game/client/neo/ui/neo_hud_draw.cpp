#include "cbase.h"
#include "neo_hud_draw.h"

#include <vgui/ISurface.h>
#include <vgui_controls/Controls.h>

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

using vgui::surface;

namespace NeoHud
{
namespace
{
// The reference resolution. Sizes in the HUD are written as they are at this screen height
constexpr int REFERENCE_SCREEN_TALL = 1080;
}  // namespace

int ScaledPx(const int iReferencePx)
{
	int iScreenWide = 0, iScreenTall = 0;
	surface()->GetScreenSize(iScreenWide, iScreenTall);
	const int iScaled = (iReferencePx * iScreenTall) / REFERENCE_SCREEN_TALL;
	if (iScaled == 0 && iReferencePx != 0)
	{
		// A hairline stays a pixel at any resolution
		return (iReferencePx > 0) ? 1 : -1;
	}
	return iScaled;
}
}  // namespace NeoHud
