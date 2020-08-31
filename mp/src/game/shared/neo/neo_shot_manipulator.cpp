#include "cbase.h"
#include "neo_shot_manipulator.h"
#include "weapon_neobasecombatweapon.h"

#ifdef LINUX
#include <initializer_list>
#endif

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

ConVar sv_neo_recoil_capbullets("sv_neo_recoil_capbullets", "10", FCVAR_CHEAT | FCVAR_REPLICATED, "Generic fallback cvar for at how many bullets should weapons reach max recoil.", true, 0.0f, false, 0.0f);
ConVar sv_neo_recoil_capscale("sv_neo_recoil_capscale", "1", FCVAR_CHEAT | FCVAR_REPLICATED, "Generic fallback scaler for max recoil cap.", true, 0.0f, false, 0.0f);
ConVar sv_neo_recoil_viewfollow_scale("sv_neo_recoil_viewfollow_scale", "0.45", FCVAR_CHEAT | FCVAR_REPLICATED, "Scaler for how much should player eyeangle follow the current recoil.", true, 0.0f, false, 0.0f);

// NEO HACK/FIXME (Rain): Doing some temporary recoil cvars here for testing.
// Need to clean these up later once we have good values!!
#define TEMP_WEP_STR(name) #name
#define MAKE_TEMP_WEP_RECOIL_SCALER(weapon, defval) ConVar sv_neo_##weapon##_recoil_scale(TEMP_WEP_STR(sv_neo_##weapon##_recoil_scale), #defval, FCVAR_REPLICATED, TEMP_WEP_STR(Temporary recoil scaler for #weapon), true, 0.0, true, 10.0)
#define MAKE_TEMP_WEP_RECOIL_CAPBULLETS(weapon, defval) ConVar sv_neo_##weapon##_recoil_capbullets(TEMP_WEP_STR(sv_neo_##weapon##_recoil_capbullets), #defval, FCVAR_REPLICATED, TEMP_WEP_STR(At how many bullets should #weapon reach max recoil.), true, 0.01, true, 9999.0)

MAKE_TEMP_WEP_RECOIL_SCALER(weapon_jitte,			0);
MAKE_TEMP_WEP_RECOIL_SCALER(weapon_jittescoped,		0);
MAKE_TEMP_WEP_RECOIL_SCALER(weapon_kyla,			0);
MAKE_TEMP_WEP_RECOIL_SCALER(weapon_m41,				0);
MAKE_TEMP_WEP_RECOIL_SCALER(weapon_m41l,			0);
MAKE_TEMP_WEP_RECOIL_SCALER(weapon_m41s,			0);
MAKE_TEMP_WEP_RECOIL_SCALER(weapon_milso,			0);
MAKE_TEMP_WEP_RECOIL_SCALER(weapon_mpn,				0.75);
MAKE_TEMP_WEP_RECOIL_SCALER(weapon_mpn_unsilenced,	1.25);
MAKE_TEMP_WEP_RECOIL_SCALER(weapon_mx,				0);
MAKE_TEMP_WEP_RECOIL_SCALER(weapon_mx_silenced,		0);
MAKE_TEMP_WEP_RECOIL_SCALER(weapon_pz,				0);
MAKE_TEMP_WEP_RECOIL_SCALER(weapon_smac,			0);
MAKE_TEMP_WEP_RECOIL_SCALER(weapon_srm,				0);
MAKE_TEMP_WEP_RECOIL_SCALER(weapon_srm_s,			0);
MAKE_TEMP_WEP_RECOIL_SCALER(weapon_tachi,			0);
MAKE_TEMP_WEP_RECOIL_SCALER(weapon_zr68c,			0);
MAKE_TEMP_WEP_RECOIL_SCALER(weapon_zr68s,			0);
#ifdef INCLUDE_WEP_PBK
MAKE_TEMP_WEP_RECOIL_SCALER(weapon_pbk56s,			0);
#endif

MAKE_TEMP_WEP_RECOIL_CAPBULLETS(weapon_jitte,			30);
MAKE_TEMP_WEP_RECOIL_CAPBULLETS(weapon_jittescoped,		30);
MAKE_TEMP_WEP_RECOIL_CAPBULLETS(weapon_kyla,			30);
MAKE_TEMP_WEP_RECOIL_CAPBULLETS(weapon_m41,				30);
MAKE_TEMP_WEP_RECOIL_CAPBULLETS(weapon_m41l,			30);
MAKE_TEMP_WEP_RECOIL_CAPBULLETS(weapon_m41s,			30);
MAKE_TEMP_WEP_RECOIL_CAPBULLETS(weapon_milso,			30);
MAKE_TEMP_WEP_RECOIL_CAPBULLETS(weapon_mpn,				15);
MAKE_TEMP_WEP_RECOIL_CAPBULLETS(weapon_mpn_unsilenced,	30);
MAKE_TEMP_WEP_RECOIL_CAPBULLETS(weapon_mx,				30);
MAKE_TEMP_WEP_RECOIL_CAPBULLETS(weapon_mx_silenced,		30);
MAKE_TEMP_WEP_RECOIL_CAPBULLETS(weapon_pz,				30);
MAKE_TEMP_WEP_RECOIL_CAPBULLETS(weapon_smac,			30);
MAKE_TEMP_WEP_RECOIL_CAPBULLETS(weapon_srm,				30);
MAKE_TEMP_WEP_RECOIL_CAPBULLETS(weapon_srm_s,			30);
MAKE_TEMP_WEP_RECOIL_CAPBULLETS(weapon_tachi,			30);
MAKE_TEMP_WEP_RECOIL_CAPBULLETS(weapon_zr68c,			30);
MAKE_TEMP_WEP_RECOIL_CAPBULLETS(weapon_zr68s,			30);
#ifdef INCLUDE_WEP_PBK
MAKE_TEMP_WEP_RECOIL_CAPBULLETS(weapon_pbk56s,			30);
#endif

float CNEOShotManipulator::GetVerticalRecoil() const
{
	// If we got a nullptr (ie. not NT gun), treat it as a non-recoiled weapon.
	if (!m_pWeapon)
	{
		return 0;
	}

	// NEO HACK/FIXME (Rain): Doing some temporary bloom accuracy scaling here for easier testing.
	// Need to clean this up later once we have good values!!
	// (These should probably live in weapon scripts, eventually.)
	const std::initializer_list<ConVar*> recoilScalers = {
		&sv_neo_weapon_jitte_recoil_scale,
		&sv_neo_weapon_jittescoped_recoil_scale,
		&sv_neo_weapon_kyla_recoil_scale,
		&sv_neo_weapon_m41_recoil_scale,
		&sv_neo_weapon_m41l_recoil_scale,
		&sv_neo_weapon_m41s_recoil_scale,
		&sv_neo_weapon_milso_recoil_scale,
		&sv_neo_weapon_mpn_recoil_scale,
		&sv_neo_weapon_mpn_unsilenced_recoil_scale,
		&sv_neo_weapon_mx_recoil_scale,
		&sv_neo_weapon_mx_silenced_recoil_scale,
		&sv_neo_weapon_pz_recoil_scale,
		&sv_neo_weapon_smac_recoil_scale,
		&sv_neo_weapon_srm_recoil_scale,
		&sv_neo_weapon_srm_s_recoil_scale,
		&sv_neo_weapon_tachi_recoil_scale,
		&sv_neo_weapon_zr68c_recoil_scale,
		&sv_neo_weapon_zr68s_recoil_scale,
#ifdef INCLUDE_WEP_PBK
		& sv_neo_weapon_pbk56s_recoil_scale,
#endif
	};
	float wepSpecificRecoilScale = sv_neo_recoil_capscale.GetFloat();
	for (ConVar* scaler : recoilScalers)
	{
		if (V_strstr(scaler->GetName(), m_pWeapon->GetName()))
		{
			wepSpecificRecoilScale = scaler->GetFloat();
			break;
		}
	}

	if (wepSpecificRecoilScale == 0)
	{
		return 0;
	}

	const std::initializer_list<ConVar*> recoilCapbulletLimits = {
		&sv_neo_weapon_jitte_recoil_capbullets,
		&sv_neo_weapon_jittescoped_recoil_capbullets,
		&sv_neo_weapon_kyla_recoil_capbullets,
		&sv_neo_weapon_m41_recoil_capbullets,
		&sv_neo_weapon_m41l_recoil_capbullets,
		&sv_neo_weapon_m41s_recoil_capbullets,
		&sv_neo_weapon_milso_recoil_capbullets,
		&sv_neo_weapon_mpn_recoil_capbullets,
		&sv_neo_weapon_mpn_unsilenced_recoil_capbullets,
		&sv_neo_weapon_mx_recoil_capbullets,
		&sv_neo_weapon_mx_silenced_recoil_capbullets,
		&sv_neo_weapon_pz_recoil_capbullets,
		&sv_neo_weapon_smac_recoil_capbullets,
		&sv_neo_weapon_srm_recoil_capbullets,
		&sv_neo_weapon_srm_s_recoil_capbullets,
		&sv_neo_weapon_tachi_recoil_capbullets,
		&sv_neo_weapon_zr68c_recoil_capbullets,
		&sv_neo_weapon_zr68s_recoil_capbullets,
#ifdef INCLUDE_WEP_PBK
		&sv_neo_weapon_pbk56s_recoil_capbullets,
#endif
	};
	ConVar* myCapBullets = &sv_neo_recoil_capbullets;
	for (ConVar* capbulletLimit : recoilCapbulletLimits)
	{
		if (V_strstr(capbulletLimit->GetName(), m_pWeapon->GetName()))
		{
			myCapBullets = capbulletLimit;
			break;
		}
	}
	Assert(myCapBullets);

	return (Min(m_iNumBullet, myCapBullets->GetInt()) / myCapBullets->GetFloat()) * wepSpecificRecoilScale;
}