#include <KeyValues.h>
#include "neo_ads_optic_info.h"

void CNEOAdsOpticInfo::ParseAdsOptic(KeyValues *pKeyValuesData)
{
	KeyValues* pOptic = pKeyValuesData->FindKey("AdsOptic");
	m_bHasAdsOptic = pOptic != nullptr;
	V_strncpy(m_szAdsOpticLens, pOptic ? pOptic->GetString("lens", "") : "", sizeof(m_szAdsOpticLens));
	if (pOptic)
	{
		V_strncpy(m_szAdsOpticLensBone, pOptic->GetString("lens_bone", ""), sizeof(m_szAdsOpticLensBone));
		Vector &o = m_vecAdsOpticLensOrigin, &u = m_vecAdsOpticLensU, &v = m_vecAdsOpticLensV;
		m_bHasAdsOpticLensMap = m_szAdsOpticLensBone[0] && sscanf(pOptic->GetString("lens_map", ""), "%f %f %f %f %f %f %f %f %f",
			&o.x, &o.y, &o.z, &u.x, &u.y, &u.z, &v.x, &v.y, &v.z) == 9;
	}
	m_bHasAdsOpticLensMap2 = false;
	m_vecAdsOpticLensCircle.Init(0.5f, 0.5f, 0.5f);
	m_flAdsOpticLensRadiusV = 0.5f;
	m_flAdsOpticLensShape = 2.0f;
	if (pOptic)
	{
		Vector &o = m_vecAdsOpticLens2Origin, &u = m_vecAdsOpticLens2U, &v = m_vecAdsOpticLens2V;
		m_bHasAdsOpticLensMap2 = m_bHasAdsOpticLensMap && sscanf(pOptic->GetString("lens_map2", ""), "%f %f %f %f %f %f %f %f %f",
			&o.x, &o.y, &o.z, &u.x, &u.y, &u.z, &v.x, &v.y, &v.z) == 9;
		Vector &circle = m_vecAdsOpticLensCircle;
		const int count = sscanf(pOptic->GetString("lens_circle", "0.5 0.5 0.5"), "%f %f %f %f",
			&circle.x, &circle.y, &circle.z, &m_flAdsOpticLensRadiusV);
		if (count < 4)
		{
			m_flAdsOpticLensRadiusV = circle.z;
		}
		m_flAdsOpticLensShape = Max(1.0f, pOptic->GetFloat("lens_shape", 2.0f));
	}
	m_bAdsOpticWindow = pOptic && pOptic->GetBool("window") && m_bHasAdsOpticLensMap;
	m_flAdsOpticWindowSkip = pOptic ? pOptic->GetFloat("window_skip", -1.0f) : -1.0f;
	ParseWindowGlass(pOptic ? pOptic->GetString("window_glass", "") : "");
	m_bAdsOpticScope = m_bAdsOpticWindow && pOptic->GetBool("scope");
	m_bAdsOpticEyepiece = m_bAdsOpticScope && pOptic->GetBool("eyepiece");
	m_bAdsOpticOnePane = pOptic && pOptic->GetBool("one_pane") && m_bHasAdsOpticLensMap2 && m_szAdsOpticLens[0];
	m_bAdsOpticReticleInLens = pOptic && pOptic->GetBool("reticle_in_lens");
	V_strncpy(m_szAdsOpticReticle, pOptic ? pOptic->GetString("reticle", "") : "", sizeof(m_szAdsOpticReticle));
}

void CNEOAdsOpticInfo::ParseWindowGlass(const char *pszPoints)
{
	m_vecAdsOpticWindowCentre.Init(m_vecAdsOpticLensCircle.x, m_vecAdsOpticLensCircle.y);
	m_iAdsOpticWindowGlassPoints = 0;

	// The points, sorted by u then v, for the hull (Andrew's monotone chain).
	Vector2D points[2 * ADS_OPTIC_WINDOW_GLASS_MAX];
	int count = 0;
	for (char *pszEnd = nullptr; count < ARRAYSIZE(points); pszPoints = pszEnd)
	{
		const float u = strtof(pszPoints, &pszEnd);
		if (pszEnd == pszPoints)
		{
			break;
		}
		pszPoints = pszEnd;
		const float v = strtof(pszPoints, &pszEnd);
		if (pszEnd == pszPoints)
		{
			break;
		}
		points[count++].Init(u, v);
	}
	if (count < 3)
	{
		return;
	}
	for (int i = 1; i < count; ++i)
	{
		const Vector2D point = points[i];
		int j = i;
		for (; j > 0 && (points[j - 1].x > point.x || (points[j - 1].x == point.x && points[j - 1].y > point.y)); --j)
		{
			points[j] = points[j - 1];
		}
		points[j] = point;
	}
	const auto cross = [](const Vector2D &o, const Vector2D &a, const Vector2D &b) {
		return (a.x - o.x) * (b.y - o.y) - (a.y - o.y) * (b.x - o.x);
	};
	Vector2D hull[2 * ARRAYSIZE(points)];
	int size = 0;
	for (int i = 0; i < count; ++i)	// lower hull
	{
		while (size >= 2 && cross(hull[size - 2], hull[size - 1], points[i]) <= 0.0f)
		{
			--size;
		}
		hull[size++] = points[i];
	}
	for (int i = count - 2, lower = size + 1; i >= 0; --i)	// upper hull
	{
		while (size >= lower && cross(hull[size - 2], hull[size - 1], points[i]) <= 0.0f)
		{
			--size;
		}
		hull[size++] = points[i];
	}
	--size;	// the last point repeats the first
	if (size < 3 || size > ADS_OPTIC_WINDOW_GLASS_MAX)
	{
		return;
	}

	Vector2D mins = hull[0], maxs = hull[0];
	for (int i = 1; i < size; ++i)
	{
		mins.Init(Min(mins.x, hull[i].x), Min(mins.y, hull[i].y));
		maxs.Init(Max(maxs.x, hull[i].x), Max(maxs.y, hull[i].y));
	}
	const Vector2D centre = (mins + maxs) * 0.5f;
	m_vecAdsOpticWindowCentre = centre;
	for (int i = 0; i < size; ++i)
	{
		m_vecAdsOpticWindowGlass[i] = hull[i] - centre;
	}
	m_iAdsOpticWindowGlassPoints = size;
}
