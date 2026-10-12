#pragma once

#include "mathlib/vector.h"
#include "mathlib/vector2d.h"
#include "weapon_parse.h"

class KeyValues;

// A weapon's sight glass from its script's "AdsOptic" block (see neo_ads_optic.h). CNEOWeaponInfo inherits it.
class CNEOAdsOpticInfo
{
public:
	void ParseAdsOptic(KeyValues *pKeyValuesData);

	bool	m_bHasAdsOptic = false;
	char	m_szAdsOpticLens[MAX_WEAPON_STRING] = "";	// the glass's material

	// The lens surface on its bone: point(u, v) = origin + u * uAxis + v * vAxis in "lens_bone" space ("lens_map").
	bool	m_bHasAdsOpticLensMap = false;
	char	m_szAdsOpticLensBone[MAX_WEAPON_STRING] = "";
	Vector	m_vecAdsOpticLensOrigin = Vector(0.0f, 0.0f, 0.0f);
	Vector	m_vecAdsOpticLensU = Vector(0.0f, 0.0f, 0.0f);
	Vector	m_vecAdsOpticLensV = Vector(0.0f, 0.0f, 0.0f);
	// A second pane of the same glass ("lens_map2"); the art is drawn on whichever is nearer the eye.
	bool	m_bHasAdsOpticLensMap2 = false;
	Vector	m_vecAdsOpticLens2Origin = Vector(0.0f, 0.0f, 0.0f);
	Vector	m_vecAdsOpticLens2U = Vector(0.0f, 0.0f, 0.0f);
	Vector	m_vecAdsOpticLens2V = Vector(0.0f, 0.0f, 0.0f);
	// The lens within that surface in UV ("lens_circle" "u v radius [vradius]"): centre (x, y), radius across (z)
	// and down (m_flAdsOpticLensRadiusV). "lens_shape" is a superellipse exponent: 2 = round, higher = squarer.
	Vector	m_vecAdsOpticLensCircle = Vector(0.5f, 0.5f, 0.5f);
	float	m_flAdsOpticLensRadiusV = 0.5f;
	float	m_flAdsOpticLensShape = 2.0f;

	// "window": the glass is left out of the gun while it is drawn over (cloak, thermals), so the world shows
	// through, with the glass's own art on top.
	bool	m_bAdsOpticWindow = false;
	// "scope": glass with a housing behind it; on the sights the gun behind the glass is hidden in every state.
	bool	m_bAdsOpticScope = false;
	// "eyepiece": a scope's eyepiece, seen through only on the sights; off them the gun draws whole.
	bool	m_bAdsOpticEyepiece = false;
	// "window_skip": how far behind the glass, in viewmodel units, the gun is hidden while drawn over. The
	// gun beyond it (a front sight) shows through. Negative: all of it is hidden. Tune with cl_neo_ads_window_skip.
	float	m_flAdsOpticWindowSkip = -1.0f;
	// "window_glass" "u v u v ...": the glass's outline in UV (their convex hull), kept as the bounding box's
	// centre and the hull's corners around it. Without it the lens ("lens_circle") is used.
	static constexpr int ADS_OPTIC_WINDOW_GLASS_MAX = 32;
	Vector2D m_vecAdsOpticWindowCentre = Vector2D(0.5f, 0.5f);
	int		m_iAdsOpticWindowGlassPoints = 0;
	Vector2D m_vecAdsOpticWindowGlass[ADS_OPTIC_WINDOW_GLASS_MAX];
	// "one_pane": both panes carry the glass's art, so the glass ("lens") is hidden and the art drawn once.
	bool	m_bAdsOpticOnePane = false;
	// "reticle_in_lens": the art has a dark frame, so it is drawn only in the clear part ("lens_circle"), softened.
	bool	m_bAdsOpticReticleInLens = false;
	// The glass's art ("reticle" material), drawn where the gun's own glass doesn't show it.
	char	m_szAdsOpticReticle[MAX_WEAPON_STRING] = "";

private:
	void ParseWindowGlass(const char *pszPoints);
};
