#include "cbase.h"
#include "neo_ads.h"
#include "neo_player_shared.h"
#include "neo_weapon_parse.h"

#ifdef CLIENT_DLL
#include "c_neo_player.h"
#include "weapon_neobasecombatweapon.h"
#include "filesystem.h"
#include "studio.h"
#include "bone_setup.h"
#include "vguicenterprint.h"
#endif

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

// The server's say: with 0, nobody aims down the sights and every weapon keeps the traditional NT aim.
ConVar sv_neo_ads("sv_neo_ads", "1", FCVAR_REPLICATED,
	"Allow aiming down the sights (ADS) on the weapons set up for it. 0 = everyone uses the traditional NT aim.", true, 0.0f, true, 1.0f);

#ifdef CLIENT_DLL
ConVar cl_neo_ads("cl_neo_ads", "0", FCVAR_ARCHIVE,
	"Aim down the weapon's sights instead of the traditional NT aim pose, on the weapons set up for it.", true, 0, true, 1);
ConVar cl_neo_ads_crosshair("cl_neo_ads_crosshair", "0", FCVAR_ARCHIVE,
	"Keep the crosshair visible while aiming down the sights, for players with a crosshair of their own.", true, 0, true, 1);

// Everything below is for tuning and testing, not for players: cheat-only, hidden, not saved to the config.
ConVar cl_neo_ads_time("cl_neo_ads_time", "0.2", FCVAR_CHEAT | FCVAR_HIDDEN,
	"Seconds for the viewmodel to move between hip and ADS.", true, 0.01f, true, 1.0f);

ConVar cl_neo_ads_bob("cl_neo_ads_bob", "0.1", FCVAR_CHEAT | FCVAR_HIDDEN,
	"Movement bob scale while on the sights (1 = same as hip).", true, 0, true, 1);
ConVar cl_neo_ads_idle("cl_neo_ads_idle", "0.02", FCVAR_CHEAT | FCVAR_HIDDEN,
	"Idle animation sway scale while on the sights (1 = same as hip).", true, 0, true, 1);
ConVar cl_neo_ads_recoil_vertical("cl_neo_ads_recoil_vertical", "0.05", FCVAR_CHEAT | FCVAR_HIDDEN,
	"Fire animation vertical kick scale on the sights: rise and pitch (1 = same as hip).", true, 0, true, 1);
ConVar cl_neo_ads_recoil_side("cl_neo_ads_recoil_side", "1", FCVAR_CHEAT | FCVAR_HIDDEN,
	"Fire animation sideways kick scale on the sights: drift, yaw and roll (1 = same as hip).", true, 0, true, 1);
ConVar cl_neo_ads_recoil_back("cl_neo_ads_recoil_back", "0.4", FCVAR_CHEAT | FCVAR_HIDDEN,
	"Fire animation pushback scale on the sights (1 = same as hip).", true, 0, true, 1);
ConVar cl_neo_ads_recoil_max_dist("cl_neo_ads_recoil_max_dist", "1", FCVAR_CHEAT | FCVAR_HIDDEN,
	"Leash on the sights: furthest the gun may move from its idle position, in units.", true, 0, false, 0);
ConVar cl_neo_ads_recoil_max_angle("cl_neo_ads_recoil_max_angle", "2", FCVAR_CHEAT | FCVAR_HIDDEN,
	"Leash on the sights: furthest the gun may rotate from its idle angle on each axis, in degrees.", true, 0, false, 0);

// Live tuning: with cl_neo_ads_tune 1 the pose below replaces the weapon's AimOffset, on any weapon. It loads
// from each weapon as you switch to it; cl_neo_ads_save appends it to ads_tuning.txt, ready for the scripts.
ConVar cl_neo_ads_tune("cl_neo_ads_tune", "0", FCVAR_CHEAT | FCVAR_HIDDEN, "Use the cl_neo_ads_* pose instead of the weapon script.", true, 0, true, 1);
ConVar cl_neo_ads_forward("cl_neo_ads_forward", "0", FCVAR_CHEAT | FCVAR_HIDDEN, "Tuning: ADS forward offset.");
ConVar cl_neo_ads_right("cl_neo_ads_right", "0", FCVAR_CHEAT | FCVAR_HIDDEN, "Tuning: ADS right offset.");
ConVar cl_neo_ads_up("cl_neo_ads_up", "0", FCVAR_CHEAT | FCVAR_HIDDEN, "Tuning: ADS up offset.");
ConVar cl_neo_ads_pitch("cl_neo_ads_pitch", "0", FCVAR_CHEAT | FCVAR_HIDDEN, "Tuning: ADS pitch offset.");
ConVar cl_neo_ads_yaw("cl_neo_ads_yaw", "0", FCVAR_CHEAT | FCVAR_HIDDEN, "Tuning: ADS yaw offset.");
ConVar cl_neo_ads_roll("cl_neo_ads_roll", "0", FCVAR_CHEAT | FCVAR_HIDDEN, "Tuning: ADS roll offset.");
ConVar cl_neo_ads_fov("cl_neo_ads_fov", "45", FCVAR_CHEAT | FCVAR_HIDDEN, "Tuning: ADS viewmodel FOV.");

// Which weapon the tuning cvars were loaded from. Compared by class name, since callers
// may pass a copy of the weapon info.
static char s_szTunedWeapon[MAX_WEAPON_STRING] = "";

static CNEOBaseCombatWeapon *LocalActiveWeapon()
{
	C_NEO_Player *pPlayer = C_NEO_Player::GetLocalNEOPlayer();
	return pPlayer ? dynamic_cast<CNEOBaseCombatWeapon *>(pPlayer->GetActiveWeapon()) : nullptr;
}

static void LoadTuningFrom(const CNEOWeaponInfo &data)
{
	const Vector &pos = data.m_bHasAds ? data.m_vecVMAdsPosOffset : data.m_vecVMAimPosOffset;
	const QAngle &ang = data.m_bHasAds ? data.m_angVMAdsAngOffset : data.m_angVMAimAngOffset;
	cl_neo_ads_forward.SetValue(pos.x);
	cl_neo_ads_right.SetValue(pos.y);
	cl_neo_ads_up.SetValue(pos.z);
	cl_neo_ads_pitch.SetValue(ang[PITCH]);
	cl_neo_ads_yaw.SetValue(ang[YAW]);
	cl_neo_ads_roll.SetValue(ang[ROLL]);
	cl_neo_ads_fov.SetValue(data.m_bHasAds ? data.m_flVMAdsFov : data.m_flVMAimFov);
	V_strncpy(s_szTunedWeapon, data.szClassName, sizeof(s_szTunedWeapon));
}

CON_COMMAND_F(cl_neo_ads_tune_load, "Reload the active weapon's script pose into the cl_neo_ads_* tuning cvars.", FCVAR_CHEAT | FCVAR_HIDDEN)
{
	if (CNEOBaseCombatWeapon *pWeapon = LocalActiveWeapon())
	{
		LoadTuningFrom(pWeapon->GetNEOWpnData());
		Msg("Loaded pose for %s.\n", pWeapon->GetClassname());
	}
}

CON_COMMAND_F(cl_neo_ads_nudge, "Nudge an ADS tuning value and switch tuning on. Usage: cl_neo_ads_nudge <forward|right|up|pitch|yaw|roll|fov> <delta>", FCVAR_CHEAT | FCVAR_HIDDEN)
{
	if (args.ArgC() != 3)
	{
		Msg("Usage: cl_neo_ads_nudge <forward|right|up|pitch|yaw|roll|fov> <delta>\n");
		return;
	}
	char cvarName[64];
	V_snprintf(cvarName, sizeof(cvarName), "cl_neo_ads_%s", args.Arg(1));
	ConVarRef cvar(cvarName);
	if (!cvar.IsValid())
	{
		Msg("Unknown tuning value: %s\n", args.Arg(1));
		return;
	}
	if (!cl_neo_ads_tune.GetBool())
	{
		if (CNEOBaseCombatWeapon *pWeapon = LocalActiveWeapon())
		{
			LoadTuningFrom(pWeapon->GetNEOWpnData());
		}
		cl_neo_ads_tune.SetValue(1);
	}
	cvar.SetValue(cvar.GetFloat() + V_atof(args.Arg(2)));
	Msg("ADS: forward %g  right %g  up %g  pitch %g  yaw %g  roll %g  fov %g\n",
		cl_neo_ads_forward.GetFloat(), cl_neo_ads_right.GetFloat(), cl_neo_ads_up.GetFloat(),
		cl_neo_ads_pitch.GetFloat(), cl_neo_ads_yaw.GetFloat(), cl_neo_ads_roll.GetFloat(),
		cl_neo_ads_fov.GetFloat());
}

CON_COMMAND_F(cl_neo_ads_save, "Append the tuned pose for the active weapon to ads_tuning.txt.", FCVAR_CHEAT | FCVAR_HIDDEN)
{
	CNEOBaseCombatWeapon *pWeapon = LocalActiveWeapon();
	if (!pWeapon)
	{
		Msg("No active NT weapon.\n");
		return;
	}
	// One line per save: script name, fov, forward, right, up, pitch, yaw, roll. The last line per weapon wins.
	char line[256];
	V_snprintf(line, sizeof(line), "%s %g %g %g %g %g %g %g\n", pWeapon->GetClassname(),
		cl_neo_ads_fov.GetFloat(), cl_neo_ads_forward.GetFloat(), cl_neo_ads_right.GetFloat(),
		cl_neo_ads_up.GetFloat(), cl_neo_ads_pitch.GetFloat(), cl_neo_ads_yaw.GetFloat(),
		cl_neo_ads_roll.GetFloat());
	FileHandle_t file = g_pFullFileSystem->Open("ads_tuning.txt", "a", "MOD");
	if (!file)
	{
		Warning("Could not open ads_tuning.txt for writing.\n");
		return;
	}
	g_pFullFileSystem->Write(line, V_strlen(line), file);
	g_pFullFileSystem->Close(file);
	Msg("Saved: %s", line);
}

// Bindable in Settings > Keys (kb_act.lst), unbound by default. Client-side only, so no usercmd button bit.
CON_COMMAND(cl_neo_ads_toggle, "Switch between aiming down the sights and the traditional NT aim.")
{
	char text[32];
	if (!sv_neo_ads.GetBool())
	{
		V_strncpy(text, "ADS is off on this server", sizeof(text));
	}
	else
	{
		cl_neo_ads.SetValue(!cl_neo_ads.GetBool());
		V_strncpy(text, cl_neo_ads.GetBool() ? "ADS" : "Standard aim", sizeof(text));
	}
	if (internalCenterPrint)
	{
		internalCenterPrint->Print(text);
	}
}
#endif // CLIENT_DLL

bool NeoAdsActive(const CNEOWeaponInfo &data)
{
#ifdef CLIENT_DLL
	return sv_neo_ads.GetBool() && cl_neo_ads.GetBool() && (data.m_bHasAds || cl_neo_ads_tune.GetBool());
#else
	return false;
#endif
}

NeoAimPose NeoGetAimPose(const CNEOWeaponInfo &data)
{
#ifdef CLIENT_DLL
	if (NeoAdsActive(data) && cl_neo_ads_tune.GetBool())
	{
		if (V_strcmp(data.szClassName, s_szTunedWeapon) != 0)
		{
			LoadTuningFrom(data); // Switched weapons: start from this weapon's current pose.
		}
		return {
			Vector(cl_neo_ads_forward.GetFloat(), cl_neo_ads_right.GetFloat(), cl_neo_ads_up.GetFloat()),
			QAngle(cl_neo_ads_pitch.GetFloat(), cl_neo_ads_yaw.GetFloat(), cl_neo_ads_roll.GetFloat()),
			cl_neo_ads_fov.GetFloat() };
	}
#endif
	if (NeoAdsActive(data))
	{
		return { data.m_vecVMAdsPosOffset, data.m_angVMAdsAngOffset, data.m_flVMAdsFov };
	}
	return { data.m_vecVMAimPosOffset, data.m_angVMAimAngOffset, data.m_flVMAimFov };
}

float NeoAimTransitionTime(const CNEOWeaponInfo &data)
{
#ifdef CLIENT_DLL
	if (NeoAdsActive(data))
	{
		return cl_neo_ads_time.GetFloat();
	}
#endif
	return NEO_ZOOM_SPEED;
}

float NeoAimTransitionCurve(const CNEOWeaponInfo &data, float fraction)
{
	if (!NeoAdsActive(data))
	{
		return fraction; // Traditional NT aim moves linearly.
	}
	// Smootherstep: the gun eases out of the hip and settles gently onto the sights.
	const float t = clamp(fraction, 0.0f, 1.0f);
	return t * t * t * (t * (t * 6.0f - 15.0f) + 10.0f);
}

float NeoAdsBobScale(float adsBlend)
{
#ifdef CLIENT_DLL
	return Lerp(clamp(adsBlend, 0.0f, 1.0f), 1.0f, cl_neo_ads_bob.GetFloat());
#else
	return 1.0f;
#endif
}

float NeoAdsIdleScale(float adsBlend)
{
#ifdef CLIENT_DLL
	return Lerp(clamp(adsBlend, 0.0f, 1.0f), 1.0f, cl_neo_ads_idle.GetFloat());
#else
	return 1.0f;
#endif
}

bool NeoAdsIsRecoilActivity(int activity)
{
	// The Supa 7 fires buckshot with its secondary attack animation (slugs use the primary).
	return activity == ACT_VM_PRIMARYATTACK || activity == ACT_VM_SECONDARYATTACK || activity == ACT_VM_DRYFIRE
		|| activity == ACT_VM_RECOIL1 || activity == ACT_VM_RECOIL2 || activity == ACT_VM_RECOIL3;
}

#ifdef CLIENT_DLL
// Model-space transform of a bone, built by walking its parent chain.
static void BoneToModel(CStudioHdr *hdr, int bone, const Vector pos[], const Quaternion q[], matrix3x4_t &out)
{
	matrix3x4_t local;
	QuaternionMatrix(q[bone], pos[bone], local);
	const int parent = hdr->pBone(bone)->parent;
	if (parent < 0)
	{
		MatrixCopy(local, out);
		return;
	}
	matrix3x4_t parentToModel;
	BoneToModel(hdr, parent, pos, q, parentToModel);
	ConcatTransforms(parentToModel, local, out);
}

void NeoAdsRestPose::Update(CStudioHdr *hdr, int poseSequence, float poseCycle, const float poseparam[])
{
	if (!hdr || (hdr->GetRenderHdr() == pModel && poseSequence == sequence && poseCycle == cycle))
	{
		return;
	}
	pModel = hdr->GetRenderHdr();
	sequence = poseSequence;
	cycle = poseCycle;
	IBoneSetup boneSetup(hdr, BONE_USED_BY_ANYTHING, poseparam);
	boneSetup.InitPose(pos, q);
	boneSetup.AccumulatePose(pos, q, poseSequence, poseCycle, 1.0f, 0.0f, nullptr);
}

void NeoAdsDampRecoil(CStudioHdr *hdr, Vector pos[], Quaternion q[],
	const Vector settledPos[], const Quaternion settledQ[], int refBone, const CNEOWeaponInfo &data, float adsBlend)
{
	const float blend = clamp(adsBlend, 0.0f, 1.0f);
	if (!hdr || refBone < 0 || refBone >= hdr->numbones() || blend <= 0.0f)
	{
		return;
	}
	const auto onSights = [blend](float scale, float weaponScale) {
		return Lerp(blend, 1.0f, clamp(scale * weaponScale, 0.0f, 1.0f));
	};
	const float vertScale = onSights(cl_neo_ads_recoil_vertical.GetFloat(), data.m_flAdsRecoilVertical);
	const float sideScale = onSights(cl_neo_ads_recoil_side.GetFloat(), data.m_flAdsRecoilSide);
	const float backScale = onSights(cl_neo_ads_recoil_back.GetFloat(), data.m_flAdsRecoilBack);

	// Gun (refBone) transforms in viewmodel space (x forward, y left, z up): live, and the fire
	// animation's settled last frame. Only the kick relative to the settled frame is damped; the
	// settled pose itself is left as animated. Some fire animations settle well off the idle pose and the
	// engine already compensates for that later, so correcting toward idle would double it.
	matrix3x4_t live, settled, settledInv, kick;
	BoneToModel(hdr, refBone, pos, q, live);
	BoneToModel(hdr, refBone, settledPos, settledQ, settled);
	MatrixInvert(settled, settledInv);
	ConcatTransforms(live, settledInv, kick);

	Vector liveOrigin, settledOrigin, unused;
	MatrixGetColumn(live, 3, liveOrigin);
	MatrixGetColumn(settled, 3, settledOrigin);
	QAngle kickAngles;
	MatrixAngles(kick, kickAngles, unused);

	// The kick with each axis scaled: pushback (x), sideways (y, yaw, roll), vertical (z, pitch).
	const Vector travel = liveOrigin - settledOrigin;
	Vector dampedTravel(travel.x * backScale, travel.y * sideScale, travel.z * vertScale);
	QAngle dampedAngles(kickAngles[PITCH] * vertScale, kickAngles[YAW] * sideScale, kickAngles[ROLL] * sideScale);

	// Leash: on the sights the gun never strays far from its settled pose, whatever the animation does.
	const float maxDist = ((data.m_flAdsRecoilMaxDist > 0.0f) ? data.m_flAdsRecoilMaxDist : cl_neo_ads_recoil_max_dist.GetFloat()) / blend;
	const float dist = dampedTravel.Length();
	if (dist > maxDist)
	{
		dampedTravel *= maxDist / dist;
	}
	const float maxAngle = ((data.m_flAdsRecoilMaxAngle > 0.0f) ? data.m_flAdsRecoilMaxAngle : cl_neo_ads_recoil_max_angle.GetFloat()) / blend;
	for (int axis = 0; axis < 3; ++axis)
	{
		dampedAngles[axis] = clamp(dampedAngles[axis], -maxAngle, maxAngle);
	}

	// Where the gun should be: the settled pose plus the damped kick.
	matrix3x4_t kickRot, target;
	AngleMatrix(dampedAngles, kickRot);
	ConcatTransforms(kickRot, settled, target);
	MatrixSetColumn(settledOrigin + dampedTravel, 3, target);

	// Correct the whole viewmodel rigidly at its root bones so arms and gun stay together.
	matrix3x4_t liveInv, correction;
	MatrixInvert(live, liveInv);
	ConcatTransforms(target, liveInv, correction);
	for (int i = 0; i < hdr->numbones(); ++i)
	{
		if (hdr->pBone(i)->parent >= 0)
		{
			continue;
		}
		matrix3x4_t local, corrected;
		QuaternionMatrix(q[i], pos[i], local);
		ConcatTransforms(correction, local, corrected);
		MatrixQuaternion(corrected, q[i]);
		MatrixGetColumn(corrected, 3, pos[i]);
	}
}

bool NeoAdsHideCrosshair(const CNEOWeaponInfo &data, bool bAiming, bool bCloaked)
{
	return NeoAdsActive(data) && bAiming && !cl_neo_ads_crosshair.GetBool() && !bCloaked;
}
#endif // CLIENT_DLL
