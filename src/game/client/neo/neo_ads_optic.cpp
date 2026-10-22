#include "cbase.h"
#include "neo_ads_optic.h"
#include "neo_ads.h"
#include "c_neo_player.h"
#include "weapon_neobasecombatweapon.h"
#include "view.h"
#include "igamesystem.h"
#include "materialsystem/imaterialsystem.h"
#include "materialsystem/imesh.h"
#include "materialsystem/MaterialSystemUtil.h"

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

static constexpr int LENS_SEGMENTS = 32;
static constexpr int LENS_RINGS = 6;	// enough for the rim fade
// How far the glass's depth and art sit in front of it, toward the eye. Near the coord limits (MAX_COORD_FLOAT) the
// gun's own depth gets coarse, and a smaller lift lets the gun behind the glass flicker through there. The gun less
// than this in front of the glass, inside the outline, is hidden too.
static constexpr float LENS_LIFT = 0.3f;
// "reticle_in_lens" art fades out from this fraction of the lens circle to its edge.
static constexpr float RETICLE_IN_LENS_FADE = 0.85f;

bool NeoAdsInThermals(const C_NEO_Player *pPlayer)
{
	return pPlayer && pPlayer->GetClass() == NEO_CLASS_SUPPORT && pPlayer->IsInVision();
}

// A superellipse outline at radius 1: round at shape 2, squarer above.
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

// An area of the lens in UV: the lens ("lens_circle", "lens_shape") or the whole glass ("window_glass"). Exact,
// since a grown outline cuts real parts of the gun out past the glass.
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

// The area in rings of shared vertices, textured with the lens's UVs, fading from centreAlpha to zero at the rim.
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
	const int vertices = 1 + LENS_RINGS * segments;
	const int indices = segments * 3 + (LENS_RINGS - 1) * segments * 6;
	meshBuilder.Begin(pMesh, MATERIAL_TRIANGLES, vertices, indices);
	const auto vertex = [&](float x, float y, float fraction) {
		const float u = area.centreU + area.scaleU * x;
		const float v = area.centreV + area.scaleV * y;
		const Vector world = pane.At(u, v);
		// Lifted along the line of sight, so it doesn't move on screen.
		Vector lift = eye - world;
		VectorNormalize(lift);
		const Vector position = world + lift * LENS_LIFT;
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

struct LensState
{
	IMaterial *pReticle = nullptr;
	bool bOverridden = false;		// cloak or thermals draw the gun over, so the glass is left out of it
	bool bScopeOnSights = false;	// a scope's lens is left out on the sights
	bool bReticle = false;
};

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
	}
	return s_pReticle;
}

bool NeoAdsHasGlassArt(const CNEOWeaponInfo &data)
{
	return data.m_bHasAdsOpticLensMap && ReticleMaterial(data);
}

static LensState GetLensState(const CNEOWeaponInfo &data, bool bCloaked, bool bThermal, float adsBlend)
{
	LensState state;
	if (!NeoAdsActive(data))
	{
		return state;
	}
	state.pReticle = ReticleMaterial(data);
	const bool bActive = NeoAdsHasGlassArt(data);
	state.bOverridden = (bCloaked || bThermal) && data.m_bAdsOpticWindow && bActive
		&& (!data.m_bAdsOpticEyepiece || adsBlend >= NEO_ADS_ON_SIGHTS);
	state.bScopeOnSights = data.m_bAdsOpticScope && !state.bOverridden && adsBlend >= NEO_ADS_ON_SIGHTS
		&& bActive;
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
	// Art with a dark frame covers only the clear part of the lens while the gun is drawn over, softened at the edge.
	const bool bInLens = data.m_bAdsOpticReticleInLens && state.bOverridden;
	const LensArea area = LensAreaOf(data, !bInLens);
	const float fadeStart = bInLens ? RETICLE_IN_LENS_FADE : 1.0f;
	DrawLensShape(state.pReticle, pane, area, 1.0f, fadeStart);
}

static IMaterial *GlassDepthMaterial()
{
	static CMaterialReference s_material;
	if (!s_material.IsValid())
	{
		KeyValues *pVMT = new KeyValues("UnlitGeneric");
		pVMT->SetInt("$nocull", 1);
		s_material.Init("__neo_ads_glass_depth", TEXTURE_GROUP_OTHER, pVMT);
	}
	return s_material;
}

// Made before the level loads, so it is cached with the level instead of at its first draw.
class CNeoAdsOpticGameSystem : public CAutoGameSystem
{
	void LevelInitPreEntity() override
	{
		GlassDepthMaterial();
	}
} g_NeoAdsOpticGameSystem;

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

ConVar cl_neo_ads_window_skip("cl_neo_ads_window_skip", "", FCVAR_CHEAT | FCVAR_HIDDEN,
	"Tuning: how deep behind sight glass the gun is hidden while cloaked or in thermals (the weapon's"
	" \"window_skip\"), in viewmodel units; negative = all of it; empty = the weapon's own.");

static void SetPlane(float plane[4], const Vector &normal, float dist)
{
	plane[0] = normal.x;
	plane[1] = normal.y;
	plane[2] = normal.z;
	plane[3] = dist;
}

// The cloak and thermals draw the whole gun with one override material, and a scope's housing shows behind its
// glass, so depth is the only way to leave the glass out: its outline goes into depth just in front of it before
// the gun, and the world already on screen shows through.
bool NeoAdsGetGlassClear(C_BaseAnimating *pViewModel, const CNEOWeaponInfo &data, bool bCloaked, bool bThermal,
	float adsBlend, NeoAdsGlassClear &clear)
{
	const LensState state = GetLensState(data, bCloaked, bThermal, adsBlend);
	if (!pViewModel || !(state.bOverridden || state.bScopeOnSights))
	{
		return false;
	}
	// This frame's pose, so the depth sits where the gun is about to be drawn.
	pViewModel->SetupBones(nullptr, -1, BONE_USED_BY_ANYTHING, gpGlobals->curtime);
	clear = NeoAdsGlassClear();
	NeoLensPane farPane;
	if (!NeoAdsLensPane(pViewModel, data, CurrentViewOrigin(), clear.pane, &farPane))
	{
		return false;
	}
	const NeoLensPane &pane = clear.pane;
	const char *pszTunedSkip = cl_neo_ads_window_skip.GetString();
	const float skip = pszTunedSkip[0] ? cl_neo_ads_window_skip.GetFloat() : data.m_flAdsOpticWindowSkip;
	// Clip planes can't change mid-scene under fast clipping, so all of the gun behind the glass stays hidden.
	if (!state.bOverridden || skip < 0.0f || materials->UsingFastClipping())
	{
		return true;
	}
	// The glass plane, normal pointing away from the eye; a plane (n, d) keeps the points with n.p >= d.
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

void NeoAdsDrawGlassClearDepth(const CNEOWeaponInfo &data, const NeoAdsGlassClear &clear)
{
	DrawDepthOnly(clear.pane, LensAreaOf(data, true));
}
