#pragma once

#include "mathlib/vector.h"
#include "mathlib/vector2d.h"
#include "weapon_parse.h"

class KeyValues;

// A weapon's sight glass, from its script's "AdsOptic" block.
class CNEOAdsOpticInfo
{
public:
	CNEOAdsOpticInfo();
	void ParseAdsOptic(KeyValues *pKeyValuesData);

	bool	m_bHasAdsOptic = false;
	char	m_szAdsOpticLens[MAX_WEAPON_STRING] = "";	// the glass's material

	// "lens_map": the lens surface in "lens_bone" space, origin + u * uAxis + v * vAxis.
	bool	m_bHasAdsOpticLensMap = false;
	char	m_szAdsOpticLensBone[MAX_WEAPON_STRING] = "";
	Vector	m_vecAdsOpticLensOrigin = Vector(0.0f, 0.0f, 0.0f);
	Vector	m_vecAdsOpticLensU = Vector(0.0f, 0.0f, 0.0f);
	Vector	m_vecAdsOpticLensV = Vector(0.0f, 0.0f, 0.0f);
	// "lens_map2": a second pane of the same glass.
	bool	m_bHasAdsOpticLensMap2 = false;
	Vector	m_vecAdsOpticLens2Origin = Vector(0.0f, 0.0f, 0.0f);
	Vector	m_vecAdsOpticLens2U = Vector(0.0f, 0.0f, 0.0f);
	Vector	m_vecAdsOpticLens2V = Vector(0.0f, 0.0f, 0.0f);
	// "lens_circle" "u v radius [vradius]" in UV; "lens_shape" 2 is round, higher is squarer.
	Vector	m_vecAdsOpticLensCircle = Vector(0.5f, 0.5f, 0.5f);
	float	m_flAdsOpticLensRadiusV = 0.5f;
	float	m_flAdsOpticLensShape = 2.0f;

	// "window": left out of the gun under the cloak and thermals.
	bool	m_bAdsOpticWindow = false;
	// "scope": a housing behind the glass, left out on the sights.
	bool	m_bAdsOpticScope = false;
	// "eyepiece": seen through only on the sights.
	bool	m_bAdsOpticEyepiece = false;
	// "window_skip": how deep behind the glass the gun is hidden, so a front sight past it still shows. Negative
	// hides all of it.
	float	m_flAdsOpticWindowSkip = -1.0f;
	// "window_glass" "u v u v ...": the glass outline in UV, kept as a convex hull. Defaults to "lens_circle".
	static constexpr int ADS_OPTIC_WINDOW_GLASS_MAX = 32;
	Vector2D m_vecAdsOpticWindowCentre = Vector2D(0.5f, 0.5f);
	int		m_iAdsOpticWindowGlassPoints = 0;
	Vector2D m_vecAdsOpticWindowGlass[ADS_OPTIC_WINDOW_GLASS_MAX];
	// "one_pane": both panes carry the art, so the glass is hidden and the art drawn once.
	bool	m_bAdsOpticOnePane = false;
	// "reticle_in_lens": the art has a dark frame, so only its clear part is drawn.
	bool	m_bAdsOpticReticleInLens = false;
	// "reticle": the glass's art.
	char	m_szAdsOpticReticle[MAX_WEAPON_STRING] = "";

private:
	void ParseWindowGlass(const char *pszPoints);
};
