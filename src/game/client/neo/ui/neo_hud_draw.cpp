#include "cbase.h"
#include "neo_hud_draw.h"

#include <vgui/IScheme.h>
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

vgui::HFont GetFont(vgui::IScheme *pScheme, const char *pszName, const vgui::HFont hFallback, const bool bProportional)
{
	const vgui::HFont hFont = pScheme->GetFont(pszName, bProportional);
	if (hFont == vgui::INVALID_FONT)
	{
		Warning("NeoHud: ClientScheme.res has no font %s%s\n", pszName,
				(hFallback == vgui::INVALID_FONT) ? ", and it has no fallback" : ", using its fallback");
		return hFallback;
	}
	return hFont;
}

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
