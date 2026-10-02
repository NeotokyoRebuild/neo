#include "cbase.h"
#include "neo_ads_optic_disc.h"
#include "neo_ads_lens.h"
#include "neo_ads_optic.h"
#include "neo_ads.h"
#include "c_neo_player.h"
#include "weapon_neobasecombatweapon.h"
#include "view.h"
#include "materialsystem/imaterialsystem.h"
#include "materialsystem/imesh.h"
#include "materialsystem/MaterialSystemUtil.h"

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

// The glass's art ("reticle"), drawn by us where the gun's own glass doesn't show it: hidden (one pane), left out
// of the gun (cloak, thermals) or a scope's lens on the sights.

// The lens outline around its centre, for a superellipse of this exponent: LENS_SEGMENTS points at radius
// 1 (round at 2, squarer above). Worked out once per exponent.
static constexpr int LENS_SEGMENTS = 32;
static constexpr int LENS_RINGS = 6;	// enough for the rim fade
static const Vector2D *LensOutline(float shape)
{
	static float s_shape = -1.0f;
	static Vector2D s_outline[LENS_SEGMENTS];
	if (shape != s_shape)
	{
		s_shape = shape;
		for (int i = 0; i < LENS_SEGMENTS; ++i)
		{
			const float angle = 2.0f * M_PI_F * i / LENS_SEGMENTS;
			const float c = cosf(angle), s = sinf(angle);
			const float scale = powf(powf(fabsf(c), shape) + powf(fabsf(s), shape), -1.0f / shape);
			s_outline[i].Init(c * scale, s * scale);
		}
	}
	return s_outline;
}

// An area of the lens surface in UV to draw over: the lens ("lens_circle", "lens_shape") or the whole glass
// ("window_glass", else the lens). Exact, since a grown outline cut real parts of the gun out past the glass.
struct LensArea
{
	float centreU, centreV, scaleU, scaleV;
	const Vector2D *pOutline;
	int points;
};

static LensArea LensAreaOf(const CNEOWeaponInfo &data, bool bWholeGlass)
{
	if (bWholeGlass && data.m_iAdsOpticWindowGlassPoints >= 3)
	{
		const Vector2D &centre = data.m_vecAdsOpticWindowCentre;
		return { centre.x, centre.y, 1.0f, 1.0f, data.m_vecAdsOpticWindowGlass, data.m_iAdsOpticWindowGlassPoints };
	}
	const Vector &circle = data.m_vecAdsOpticLensCircle;
	return { circle.x, circle.y, circle.z, data.m_flAdsOpticLensRadiusV,
		LensOutline(data.m_flAdsOpticLensShape), LENS_SEGMENTS };
}

// An area of the lens (LensAreaOf) in rings of shared vertices, textured with the lens's UVs: centreAlpha
// inside the fade radius, easing to zero at the rim.
static void DrawLensShape(IMaterial *pMaterial, const NeoLensPane &pane, const LensArea &area, float centreAlpha,
	float fadeStart)
{
	const Vector2D *pOutline = area.pOutline;
	const int segments = area.points;
	const Vector eye = CurrentViewOrigin();

	CMatRenderContextPtr pRenderContext(materials);
	pRenderContext->Bind(pMaterial);
	IMesh *pMesh = pRenderContext->GetDynamicMesh();
	CMeshBuilder meshBuilder;
	// The centre, then LENS_RINGS rings of the outline's points; a fan to the first ring, quads between rings.
	const int vertices = 1 + LENS_RINGS * segments;
	const int indices = segments * 3 + (LENS_RINGS - 1) * segments * 6;
	meshBuilder.Begin(pMesh, MATERIAL_TRIANGLES, vertices, indices);
	const auto vertex = [&](float x, float y, float fraction) {
		const float u = area.centreU + area.scaleU * x;
		const float v = area.centreV + area.scaleV * y;
		const Vector world = pane.At(u, v);
		// Lifted a hair toward the eye so it sits on the lens rather than in it.
		Vector lift = eye - world;
		VectorNormalize(lift);
		const Vector position = world + lift * 0.01f;
		const float fade = NeoSmoothStep((fraction - fadeStart) / Max(1.0f - fadeStart, 0.001f));
		meshBuilder.Color4ub(255, 255, 255, static_cast<unsigned char>(255.0f * centreAlpha * (1.0f - fade)));
		meshBuilder.TexCoord2f(0, u, v);
		meshBuilder.Position3fv(position.Base());
		meshBuilder.AdvanceVertex();
	};
	vertex(0.0f, 0.0f, 0.0f);
	for (int ring = 1; ring <= LENS_RINGS; ++ring)
	{
		const float fraction = static_cast<float>(ring) / LENS_RINGS;
		for (int seg = 0; seg < segments; ++seg)
		{
			vertex(pOutline[seg].x * fraction, pOutline[seg].y * fraction, fraction);
		}
	}
	// Vertex index of ring r (1-based), segment s (wrapping).
	const auto at = [segments](int ring, int seg) { return 1 + (ring - 1) * segments + (seg % segments); };
	for (int seg = 0; seg < segments; ++seg)
	{
		meshBuilder.FastIndex(0);
		meshBuilder.FastIndex(at(1, seg));
		meshBuilder.FastIndex(at(1, seg + 1));
		for (int ring = 1; ring < LENS_RINGS; ++ring)
		{
			meshBuilder.FastIndex(at(ring, seg));
			meshBuilder.FastIndex(at(ring + 1, seg));
			meshBuilder.FastIndex(at(ring + 1, seg + 1));
			meshBuilder.FastIndex(at(ring, seg));
			meshBuilder.FastIndex(at(ring + 1, seg + 1));
			meshBuilder.FastIndex(at(ring, seg + 1));
		}
	}
	meshBuilder.End();
	pMesh->Draw();
}

// "reticle_in_lens" art fades out from this fraction of the lens circle to its edge.
static constexpr float RETICLE_IN_LENS_FADE = 0.85f;

// What this frame's glass drawing does for the gun in view.
struct LensState
{
	IMaterial *pReticle = nullptr;
	bool bOverridden = false;		// drawn over (cloak, thermals): the split leaves the glass out of the gun
								// (an eyepiece only on the sights)
	bool bScopeOnSights = false;	// a scope on the sights: the hole leaves its lens out
	bool bReticle = false;
};

// The weapon's reticle material, or null; found by name only when the weapon changes.
static IMaterial *ReticleMaterial(const CNEOWeaponInfo &data)
{
	static const CNEOWeaponInfo *s_pData = nullptr;
	static IMaterial *s_pReticle = nullptr;
	if (&data != s_pData)
	{
		s_pData = &data;
		s_pReticle = data.m_szAdsOpticReticle[0]
			? materials->FindMaterial(data.m_szAdsOpticReticle, TEXTURE_GROUP_VGUI, false) : nullptr;
		if (s_pReticle && s_pReticle->IsErrorMaterial())
		{
			s_pReticle = nullptr;
		}
		if (s_pReticle && !s_pReticle->IsPrecached())
		{
			PrecacheMaterial(s_pReticle->GetName());
		}
	}
	return s_pReticle;
}

static LensState GetLensState(const CNEOWeaponInfo &data, bool bCloaked, bool bThermal, float adsBlend)
{
	LensState state;
	state.pReticle = ReticleMaterial(data);
	// Without art to draw back (no reticle material, no lens map) the glass is left as the gun draws it.
	const bool bActive = NeoAdsActive(data) && state.pReticle && data.m_bHasAdsOpticLensMap;
	state.bOverridden = (bCloaked || bThermal) && data.m_bAdsOpticWindow && bActive
		&& (!data.m_bAdsOpticEyepiece || adsBlend >= NEO_ADS_ON_SIGHTS);
	state.bScopeOnSights = data.m_bAdsOpticScope && !state.bOverridden && adsBlend >= NEO_ADS_ON_SIGHTS
		&& bActive;
	// Glass hidden on the gun (one pane) shows its art whatever else happens to it.
	const bool bHidden = data.m_bAdsOpticOnePane && bActive;
	state.bReticle = state.pReticle && (state.bOverridden || state.bScopeOnSights || bHidden);
	return state;
}

void NeoAdsDrawGlassArt(C_BaseAnimating *pViewModel, const CNEOWeaponInfo &data, bool bCloaked, bool bThermal,
	float adsBlend)
{
	if (!pViewModel || !data.m_bHasAdsOpticLensMap)
	{
		return;
	}
	const LensState state = GetLensState(data, bCloaked, bThermal, adsBlend);
	NeoLensPane pane;
	if (!state.bReticle || !NeoAdsLensPane(pViewModel, data, CurrentViewOrigin(), pane))
	{
		return;
	}
	// The art covers the whole glass, unless its frame is dark and the gun is drawn over: then only the clear
	// part, softened at the edge.
	const bool bInLens = data.m_bAdsOpticReticleInLens && state.bOverridden;
	const LensArea area = LensAreaOf(data, !bInLens);
	const float fadeStart = bInLens ? RETICLE_IN_LENS_FADE : 1.0f;
	DrawLensShape(state.pReticle, pane, area, 1.0f, fadeStart);
}

// Draws nothing itself: only its depth is written (see DrawDepthOnly).
static IMaterial *GlassDepthMaterial()
{
	static CMaterialReference s_material;
	if (!s_material.IsValid())
	{
		KeyValues *pVMT = new KeyValues("UnlitGeneric");
		pVMT->SetString("$basetexture", "white");
		pVMT->SetInt("$nocull", 1);
		s_material.Init("__neo_ads_glass_depth", TEXTURE_GROUP_OTHER, pVMT);
	}
	return s_material;
}

// An area of the glass into depth only, a hair in front of it, so the gun behind it fails the depth test there.
static void DrawDepthOnly(const NeoLensPane &pane, const LensArea &area)
{
	CMatRenderContextPtr pRenderContext(materials);
	pRenderContext->OverrideColorWriteEnable(true, false);
	pRenderContext->OverrideAlphaWriteEnable(true, false);
	pRenderContext->OverrideDepthEnable(true, true);
	DrawLensShape(GlassDepthMaterial(), pane, area, 1.0f, 1.0f);
	pRenderContext->OverrideDepthEnable(false, true);
	pRenderContext->OverrideAlphaWriteEnable(false, true);
	pRenderContext->OverrideColorWriteEnable(false, true);
}

// The glass's exact outline goes into depth a hair in front of it before the gun is drawn, so neither the glass
// nor the gun behind it draws inside the outline and the world already on screen shows through. The cloak and
// thermals draw the whole gun with one override material, and a scope's housing shows behind its glass, so
// this is the only way to leave them out. Depth only, so it goes down before each draw of the gun (a two-pass
// model is drawn twice a frame).
// "window_skip": the gun further behind the glass than that is drawn first, clipped to beyond it, and the rest
// after the outline, clipped to this side, so each part draws once.

// For tuning, not for players: cheat-only, hidden, and not saved to the config.
ConVar cl_neo_ads_window_skip("cl_neo_ads_window_skip", "", FCVAR_CHEAT | FCVAR_HIDDEN,
	"Tuning: how deep behind sight glass the gun is hidden while cloaked or in thermals (the weapon's"
	" \"window_skip\"), in viewmodel units; negative = all of it; empty = the weapon's own.");

// This frame's clear view, worked out once a frame (a two-pass model asks again for its translucent pass).
static struct
{
	int frame = -1;
	const CNEOWeaponInfo *pData = nullptr;
	bool bClear = false;
	NeoLensPane pane;
	NeoAdsGlassClear clear;
} s_clearView;

static void SetPlane(float plane[4], const Vector &normal, float dist)
{
	plane[0] = normal.x;
	plane[1] = normal.y;
	plane[2] = normal.z;
	plane[3] = dist;
}

static bool ComputeGlassClear(C_BaseAnimating *pViewModel, const CNEOWeaponInfo &data, bool bCloaked, bool bThermal,
	float adsBlend, NeoLensPane &pane, NeoAdsGlassClear &clear)
{
	const LensState state = GetLensState(data, bCloaked, bThermal, adsBlend);
	if (!pViewModel || !(state.bOverridden || state.bScopeOnSights))
	{
		return false;
	}
	// This frame's pose, before the gun sets it up itself, so the depth sits where the gun is drawn.
	pViewModel->SetupBones(nullptr, -1, BONE_USED_BY_ANYTHING, gpGlobals->curtime);
	NeoLensPane farPane;
	if (!NeoAdsLensPane(pViewModel, data, CurrentViewOrigin(), pane, &farPane))
	{
		return false;
	}
	clear = NeoAdsGlassClear();
	const char *pszTunedSkip = cl_neo_ads_window_skip.GetString();
	const float skip = pszTunedSkip[0] ? cl_neo_ads_window_skip.GetFloat() : data.m_flAdsOpticWindowSkip;
	// Custom clip planes can't change mid-scene under fast clipping (depth problems): all of the gun behind the
	// glass stays hidden then.
	if (!state.bOverridden || skip < 0.0f || materials->UsingFastClipping())
	{
		return true;
	}
	// The glass plane, the normal pointing away from the eye; a plane (n, d) keeps the points with n.p >= d.
	Vector normal = CrossProduct(pane.u, pane.v);
	if (VectorNormalize(normal) <= 0.0f)
	{
		return true;
	}
	if (DotProduct(normal, CurrentViewOrigin() - pane.origin) > 0.0f)
	{
		normal = -normal;
	}
	// Measured from the far pane of two (they are parallel).
	const float glassDist = DotProduct(normal, data.m_bHasAdsOpticLensMap2 ? farPane.origin : pane.origin);
	const float skipDist = Max(glassDist, DotProduct(normal, pane.origin)) + skip;
	clear.bFarFirst = true;
	SetPlane(clear.farPlane, normal, skipDist);
	SetPlane(clear.nearPlane, -normal, -skipDist);
	return true;
}

bool NeoAdsBeginGlassClear(C_BaseAnimating *pViewModel, const CNEOWeaponInfo &data, bool bCloaked, bool bThermal,
	float adsBlend, NeoAdsGlassClear &clear)
{
	if (s_clearView.frame != gpGlobals->framecount || s_clearView.pData != &data)
	{
		s_clearView.frame = gpGlobals->framecount;
		s_clearView.pData = &data;
		s_clearView.bClear = ComputeGlassClear(pViewModel, data, bCloaked, bThermal, adsBlend, s_clearView.pane,
			s_clearView.clear);
	}
	clear = s_clearView.clear;
	return s_clearView.bClear;
}

void NeoAdsDrawGlassClearDepth(const CNEOWeaponInfo &data)
{
	if (s_clearView.frame != gpGlobals->framecount || s_clearView.pData != &data || !s_clearView.bClear)
	{
		return;
	}
	DrawDepthOnly(s_clearView.pane, LensAreaOf(data, true));
}
