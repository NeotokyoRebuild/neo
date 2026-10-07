#pragma once

// Aim down sights (ADS): where the viewmodel sits and how it moves while aiming. Gameplay aim (spread, camera
// FOV, speed) is unchanged. A weapon opts in with "enabled" "1" in the "AimOffset" block of its script; without
// it, with cl_neo_ads 0 or with sv_neo_ads 0, the traditional NT "ZoomOffset" pose is used.

#include "mathlib/vector.h"

class CNEOWeaponInfo;

struct NeoAimPose
{
	Vector pos;
	QAngle ang;
	float fov;
};

// True when this client should show the weapon's ADS pose: the server allows it, cl_neo_ads is on and the
// weapon has opted in. Always false on the server.
bool NeoAdsActive(const CNEOWeaponInfo &data);

// The viewmodel pose at full aim: ADS (or the live-tuned pose) or the traditional zoom.
NeoAimPose NeoGetAimPose(const CNEOWeaponInfo &data);

// Seconds for the hip <-> aim viewmodel transition.
float NeoAimTransitionTime(const CNEOWeaponInfo &data);

// Shapes the linear transition fraction (0..1) into the viewmodel's motion curve.
float NeoAimTransitionCurve(const CNEOWeaponInfo &data, float fraction);

// The smoothstep ease, 0 to 1 over t in [0, 1] (clamped), flat at both ends.
inline float NeoSmoothStep(float t)
{
	t = clamp(t, 0.0f, 1.0f);
	return t * t * (3.0f - 2.0f * t);
}

// Scale for movement bob at the given ADS blend (0 = hip, 1 = fully on the sights).
float NeoAdsBobScale(float adsBlend);

// How much of the idle animation's motion to show at the given ADS blend (1 = full idle). On the sights the
// sway shrinks to cl_neo_ads_idle of its size so the sight picture holds.
float NeoAdsIdleScale(float adsBlend);

// Fire and recoil animations whose kick is damped on the sights.
bool NeoAdsIsRecoilActivity(int activity);

#ifdef CLIENT_DLL
#include "studio.h"

// A cached viewmodel pose: one sequence at one cycle. The damping uses the idle's first frame (the pose the
// sights are tuned in) and the fire animation's settled last frame.
struct NeoAdsRestPose
{
	const studiohdr_t *pModel = nullptr;
	int sequence = -1;
	float cycle = -1.0f;
	Vector pos[MAXSTUDIOBONES];
	QuaternionAligned q[MAXSTUDIOBONES];

	// Rebuilds the pose if the model, sequence or cycle changed since the last call.
	void Update(CStudioHdr *hdr, int poseSequence, float poseCycle, const float poseparam[]);
};

// Damps the fire animation on the sights: only the gun's (refBone) kick relative to the animation's settled
// last frame (settledPos/settledQ) shrinks, by the cl_neo_ads_recoil_* scales and leash, which the script's
// "AdsRecoil" block adjusts per weapon. Applied rigidly at the root bones.
void NeoAdsDampRecoil(CStudioHdr *hdr, Vector pos[], Quaternion q[],
	const Vector settledPos[], const Quaternion settledQ[], int refBone, const CNEOWeaponInfo &data, float adsBlend);

// True when the crosshair should be hidden: only while aiming down the sights of a weapon using its ADS pose,
// unless the player keeps it (cl_neo_ads_crosshair). Everything else follows the player's crosshair settings.
// Aiming while cloaked keeps it too, since the cloaked sights are hard to see, unless the weapon has an optic:
// its glass stays see-through while cloaked.
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
