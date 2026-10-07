#pragma once

// Sight glass the gun is seen through, from the "AdsOptic" block of a weapon script. Nothing is rendered for it:
// the world is already on screen when the gun is drawn, so the glass only has to be left out where the gun
// would cover it, which is under the cloak or thermals (the override material draws the glass solid) and, for a
// scope, its housing on the sights. Weapons without the block, or with ADS off, are untouched. Client only.

class CNEOWeaponInfo;
class C_BaseAnimating;
class C_NEO_Player;

// True while this player sees in thermals (a support in vision mode): the opaque thermal material covers the
// glass just as the cloak does.
bool NeoAdsInThermals(const C_NEO_Player *pPlayer);

// Draws the glass's art (the weapon's "reticle") where the gun's own glass doesn't show it: hidden (one pane)
// or left out (cloak, thermals). Call after the gun.
void NeoAdsDrawGlassArt(C_BaseAnimating *pViewModel, const CNEOWeaponInfo &data, bool bCloaked, bool bThermal,
	float adsBlend);
