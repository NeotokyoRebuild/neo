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

#ifdef GAME_DLL
// Every loadout weapon's sight glass art, at level start, so clients load it with the level.
void NeoAdsPrecacheReticles();
#endif

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

// Hidden only while aiming down an ADS weapon's sights. Cloaked it stays, since the cloaked sights are hard to see,
// unless the weapon's glass stays see-through.
bool NeoAdsHideCrosshair(const CNEOWeaponInfo &data, bool bAiming, bool bCloaked);

class IMaterial;

constexpr float NEO_ADS_ON_SIGHTS = 0.5f;

// Hides a "one_pane" optic's glass while ADS is on, since neo_ads_optic.cpp draws its art instead. Wrap the
// viewmodel draw in one; the glass comes back when it goes out of scope.
class NeoAdsHiddenMaterials
{
public:
	NeoAdsHiddenMaterials(const CNEOWeaponInfo *pData);
	~NeoAdsHiddenMaterials();
private:
	IMaterial *m_pLens = nullptr;
};
#endif
