#pragma once

// Seeing through a weapon's glass where the gun would cover it. The glass's exact outline goes into depth before
// the gun, so neither the glass nor the gun behind it draws there: a "window" while the gun is drawn over (cloak,
// thermals), a "scope" on the sights. The glass's art goes back on after the gun (neo_ads_optic.h).

class CNEOWeaponInfo;
class C_BaseAnimating;

// How to draw the gun round its clear glass this frame: the depth, then the gun. With bFarFirst ("window_skip",
// drawn over) the gun far behind the glass is drawn first with farPlane pushed, then the depth, then the rest
// with nearPlane pushed, so that part shows through the glass.
struct NeoAdsGlassClear
{
	bool bFarFirst = false;
	float farPlane[4] = {};
	float nearPlane[4] = {};
};

// False when nothing is cleared (no glass to see through, or an eyepiece off the sights). Worked out once a frame.
bool NeoAdsBeginGlassClear(C_BaseAnimating *pViewModel, const CNEOWeaponInfo &data, bool bCloaked, bool bThermal,
	float adsBlend, NeoAdsGlassClear &clear);

// This frame's glass outline into depth only, a hair in front of the glass. Call before each draw of the gun.
void NeoAdsDrawGlassClearDepth(const CNEOWeaponInfo &data);
