#include "cbase.h"
#include "neo_ads_lens.h"
#include "weapon_neobasecombatweapon.h"

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

Vector NeoLensPane::Centre(const CNEOWeaponInfo &data) const
{
	return At(data.m_vecAdsOpticWindowCentre.x, data.m_vecAdsOpticWindowCentre.y);
}

// The lens bone's index on the viewmodel, looked up by name only when the model or the weapon changes.
static int LensBone(C_BaseAnimating *pViewModel, const CNEOWeaponInfo &data)
{
	static const studiohdr_t *s_pModel = nullptr;
	static const CNEOWeaponInfo *s_pData = nullptr;
	static int s_bone = -1;
	const CStudioHdr *pHdr = pViewModel ? pViewModel->GetModelPtr() : nullptr;
	const studiohdr_t *pModel = pHdr ? pHdr->GetRenderHdr() : nullptr;
	if (!pModel)
	{
		return -1;
	}
	if (pModel != s_pModel || &data != s_pData)
	{
		s_pModel = pModel;
		s_pData = &data;
		s_bone = pViewModel->LookupBone(data.m_szAdsOpticLensBone);
	}
	return s_bone;
}

bool NeoAdsLensPane(C_BaseAnimating *pViewModel, const CNEOWeaponInfo &data, const Vector &eye, NeoLensPane &pane,
	NeoLensPane *pFarPane)
{
	const int bone = LensBone(pViewModel, data);
	if (bone < 0)
	{
		return false;
	}
	matrix3x4_t lensToWorld;
	// From the drawn pose, not GetBoneTransform: its cache holds only hitbox bones, so a lens bone that is not one
	// would come back as the viewmodel's origin.
	MatrixCopy(pViewModel->GetBone(bone), lensToWorld);
	const auto toWorld = [&](const Vector &origin, const Vector &u, const Vector &v, NeoLensPane &out) {
		VectorTransform(origin, lensToWorld, out.origin);
		VectorRotate(u, lensToWorld, out.u);
		VectorRotate(v, lensToWorld, out.v);
	};
	toWorld(data.m_vecAdsOpticLensOrigin, data.m_vecAdsOpticLensU, data.m_vecAdsOpticLensV, pane);
	if (data.m_bHasAdsOpticLensMap2)
	{
		NeoLensPane second;
		toWorld(data.m_vecAdsOpticLens2Origin, data.m_vecAdsOpticLens2U, data.m_vecAdsOpticLens2V, second);
		if (second.Centre(data).DistToSqr(eye) < pane.Centre(data).DistToSqr(eye))
		{
			V_swap(pane, second);
		}
		if (pFarPane)
		{
			*pFarPane = second;
		}
	}
	return true;
}
