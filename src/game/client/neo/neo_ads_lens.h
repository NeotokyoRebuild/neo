#pragma once

// Where a weapon's lens is ("lens_bone", "lens_map", "lens_map2" in its AdsOptic block).

class C_BaseAnimating;
class CNEOWeaponInfo;

// One pane of the lens in world space, addressed in lens UV.
struct NeoLensPane
{
	Vector origin, u, v;

	Vector At(float lensU, float lensV) const { return origin + u * lensU + v * lensV; }
	// "window_glass" centre, else "lens_circle".
	Vector Centre(const CNEOWeaponInfo &data) const;
};

// The pane nearer the eye, from the viewmodel's drawn pose, and the other pane in pFarPane if there are two
// (else pFarPane is left alone).
// False if the lens bone isn't on the model.
bool NeoAdsLensPane(C_BaseAnimating *pViewModel, const CNEOWeaponInfo &data, const Vector &eye, NeoLensPane &pane,
	NeoLensPane *pFarPane = nullptr);
