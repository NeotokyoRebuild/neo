#pragma once

// Seeing through a weapon's sight glass ("AdsOptic" in its script) in ADS. The world is already on screen when the
// gun is drawn, so the glass is left out where the gun would cover it (under the cloak or thermals, and a scope's
// housing on the sights) and its art is drawn back on top. Client only.

#include "neo_ads_lens.h"

class CNEOWeaponInfo;
class C_BaseAnimating;
class C_NEO_Player;

bool NeoAdsInThermals(const C_NEO_Player *pPlayer);

// A reticle material that loads and a lens map to draw it on. Without them the glass is left as the gun draws it.
bool NeoAdsHasGlassArt(const CNEOWeaponInfo &data);

// How to draw the gun round its clear glass: the glass outline into depth, then the gun. With bFarFirst
// ("window_skip") the gun far behind the glass is drawn first, clipped by farPlane, and the rest after the
// depth, clipped by nearPlane.
struct NeoAdsGlassClear
{
	NeoLensPane pane;
	bool bFarFirst = false;
	float farPlane[4] = {};
	float nearPlane[4] = {};
};

// False when nothing is cleared: no glass to see through, or an eyepiece off the sights.
bool NeoAdsGetGlassClear(C_BaseAnimating *pViewModel, const CNEOWeaponInfo &data, bool bCloaked, bool bThermal,
	float adsBlend, NeoAdsGlassClear &clear);
void NeoAdsDrawGlassClearDepth(const CNEOWeaponInfo &data, const NeoAdsGlassClear &clear);

// The glass's art where the gun's own glass doesn't show it. Call after the gun.
void NeoAdsDrawGlassArt(C_BaseAnimating *pViewModel, const CNEOWeaponInfo &data, bool bCloaked, bool bThermal,
	float adsBlend);
