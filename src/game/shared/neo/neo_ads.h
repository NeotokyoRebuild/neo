#pragma once

// Aim down sights (ADS): the viewmodel's pose and motion while aiming. Presentation only, spread, FOV and speed are
// unchanged. A weapon opts in with "enabled" "1" in its script's "AimOffset" block, else it keeps "ZoomOffset".

#include "mathlib/vector.h"

class CNEOWeaponInfo;

struct NeoAimPose
{
	Vector pos;
	QAngle ang;
	float fov;
};

// Always false on the server, and while sv_neo_ads is 0.
bool NeoAdsActive(const CNEOWeaponInfo &data);

NeoAimPose NeoGetAimPose(const CNEOWeaponInfo &data);
float NeoAimTransitionTime(const CNEOWeaponInfo &data);
float NeoAimTransitionCurve(const CNEOWeaponInfo &data, float fraction);

inline float NeoSmoothStep(float t)
{
	t = clamp(t, 0.0f, 1.0f);
	return t * t * (3.0f - 2.0f * t);
}

// adsBlend: 0 at the hip, 1 on the sights.
float NeoAdsBobScale(float adsBlend);
float NeoAdsIdleScale(float adsBlend);

bool NeoAdsIsRecoilActivity(int activity);

#ifdef CLIENT_DLL
#include "studio.h"

// One sequence at one cycle, rebuilt only when one of them or the model changes.
struct NeoAdsRestPose
{
	const studiohdr_t *pModel = nullptr;
	int sequence = -1;
	float cycle = -1.0f;
	Vector pos[MAXSTUDIOBONES];
	QuaternionAligned q[MAXSTUDIOBONES];

	void Update(CStudioHdr *hdr, int poseSequence, float poseCycle, const float poseparam[]);
};

// Shrinks the gun's kick relative to the fire animation's settled last frame, applied rigidly at the root bones.
void NeoAdsDampRecoil(CStudioHdr *hdr, Vector pos[], Quaternion q[],
	const Vector settledPos[], const Quaternion settledQ[], int refBone, const CNEOWeaponInfo &data, float adsBlend);

// Hidden only while aiming down an ADS weapon's sights, and not while cloaked (the cloaked sights are hard to see).
bool NeoAdsHideCrosshair(const CNEOWeaponInfo &data, bool bAiming, bool bCloaked);

class IMaterial;

// How far onto the sights (the viewmodel's ADS blend) the gun counts as on them: a scope's housing behind the
// glass stops drawing from here.
constexpr float NEO_ADS_ON_SIGHTS = 0.5f;

// Hides an optic's glass ("one_pane" in its "AdsOptic" block) for its lifetime, since the optic draws that
// glass's art itself (neo_ads_optic.h). Wrap the viewmodel draw in one; the material is restored when it goes
// out of scope.
class NeoAdsHiddenMaterials
{
public:
	NeoAdsHiddenMaterials(const CNEOWeaponInfo *pData);
	~NeoAdsHiddenMaterials();
private:
	IMaterial *m_pLens = nullptr;
};
#endif
