//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose:
//
//=============================================================================//

#include <KeyValues.h>
#include "neo_weapon_parse.h"


FileWeaponInfo_t* CreateWeaponInfo()
{
	return new CNEOWeaponInfo;
}


CNEOWeaponInfo::CNEOWeaponInfo()
{
	m_iBullets = 0;
	m_flCycleTime = 0.f;
	szViewModel2[0] = 0;
	szBulletCharacter[0] = 0;
	szDeathIcon[0] = 0;
	m_flPenetration = 0.f;
	m_bDropOnDeath = true;
	iAimFOV = 0;

	m_flVMFov = m_flVMAimFov = m_flVMAdsFov = 0.f;
	m_bHasAds = false;
	m_flAdsRecoilVertical = m_flAdsRecoilSide = m_flAdsRecoilBack = 1.f;
	m_flAdsRecoilMaxDist = m_flAdsRecoilMaxAngle = 0.f;
	m_vecVMPosOffset = m_vecVMAimPosOffset = m_vecVMAdsPosOffset = vec3_origin;
	m_angVMAngOffset = m_angVMAimAngOffset = m_angVMAdsAngOffset = vec3_angle;
}


void CNEOWeaponInfo::Parse( KeyValues *pKeyValuesData, const char *szWeaponName )
{
	BaseClass::Parse( pKeyValuesData, szWeaponName );

	m_iPlayerDamage = pKeyValuesData->GetInt( "Damage", 42 ); // Douglas Adams 1952 - 2001
	m_iBullets = pKeyValuesData->GetInt( "Bullets", 1 );
	m_flCycleTime = pKeyValuesData->GetFloat( "CycleTime", 0.15 );

	const char *notFoundStr = "notfound";
	Q_strncpy(szViewModel2, pKeyValuesData->GetString("team2viewmodel", notFoundStr), MAX_WEAPON_STRING);
	// If there was no NSF viewmodel specified, fall back to Source's default "viewmodel" to ensure we have something sensible available.
	// This might happen when attempting to equip a non-NT weapon.
	if (Q_strcmp(szViewModel2, notFoundStr) == 0)
	{
		Q_strncpy(szViewModel2, pKeyValuesData->GetString("viewmodel"), MAX_WEAPON_STRING);
	}

	Q_strncpy( szBulletCharacter, pKeyValuesData->GetString("BulletCharacter", "a"), MAX_BULLET_CHARACTER);
	Q_strncpy( szDeathIcon, pKeyValuesData->GetString("iDeathIcon", ""), MAX_BULLET_CHARACTER);
	m_flPenetration = pKeyValuesData->GetFloat("Penetration", 0);
	m_bDropOnDeath = pKeyValuesData->GetBool("DropOnDeath", true);
	iAimFOV = pKeyValuesData->GetInt("AimFov", 45);

	KeyValues *pViewModel = pKeyValuesData->FindKey("ViewModelOffset");
	if (pViewModel)
	{
		m_flVMFov = pKeyValuesData->GetFloat("VMFov", 60);

		m_vecVMPosOffset.x = pViewModel->GetFloat("forward", 0);
		m_vecVMPosOffset.y = pViewModel->GetFloat("right", 0);
		m_vecVMPosOffset.z = pViewModel->GetFloat("up", 0);

		m_angVMAngOffset[PITCH] = pViewModel->GetFloat("pitch", 0);
		m_angVMAngOffset[YAW] = pViewModel->GetFloat("yaw", 0);
		m_angVMAngOffset[ROLL] = pViewModel->GetFloat("roll", 0);
	}

	// ZoomOffset = Traditional NT aim offset
	// AimOffset = ADS offset, used instead when cl_neo_ads is enabled (see neo_ads.h). Many scripts carry an
	// untuned AimOffset, so a weapon only takes part when its block says "enabled" "1".
	if (KeyValues* pZoomOffset = pKeyValuesData->FindKey("ZoomOffset"))
	{
		m_flVMAimFov = pZoomOffset->GetFloat("fov", 55);

		m_vecVMAimPosOffset.x = pZoomOffset->GetFloat("forward", 0);
		m_vecVMAimPosOffset.y = pZoomOffset->GetFloat("right", 0);
		m_vecVMAimPosOffset.z = pZoomOffset->GetFloat("up", 0);

		m_angVMAimAngOffset[PITCH] = pZoomOffset->GetFloat("pitch", 0);
		m_angVMAimAngOffset[YAW] = pZoomOffset->GetFloat("yaw", 0);
		m_angVMAimAngOffset[ROLL] = pZoomOffset->GetFloat("roll", 0);
	}

	m_bHasAds = false;
	if (KeyValues* pAdsOffset = pKeyValuesData->FindKey("AimOffset"))
	{
		m_bHasAds = pAdsOffset->GetBool("enabled", false);
		m_flVMAdsFov = pAdsOffset->GetFloat("fov", 55);

		m_vecVMAdsPosOffset.x = pAdsOffset->GetFloat("forward", 0);
		m_vecVMAdsPosOffset.y = pAdsOffset->GetFloat("right", 0);
		m_vecVMAdsPosOffset.z = pAdsOffset->GetFloat("up", 0);

		m_angVMAdsAngOffset[PITCH] = pAdsOffset->GetFloat("pitch", 0);
		m_angVMAdsAngOffset[YAW] = pAdsOffset->GetFloat("yaw", 0);
		m_angVMAdsAngOffset[ROLL] = pAdsOffset->GetFloat("roll", 0);
	}

	// Optional per-weapon multipliers for the fire animation's kick while on the sights.
	KeyValues* pAdsRecoil = pKeyValuesData->FindKey("AdsRecoil");
	m_flAdsRecoilVertical = pAdsRecoil ? pAdsRecoil->GetFloat("vertical", 1) : 1.f;
	m_flAdsRecoilSide = pAdsRecoil ? pAdsRecoil->GetFloat("side", 1) : 1.f;
	m_flAdsRecoilBack = pAdsRecoil ? pAdsRecoil->GetFloat("back", 1) : 1.f;
	m_flAdsRecoilMaxDist = pAdsRecoil ? pAdsRecoil->GetFloat("max_dist", 0) : 0.f;
	m_flAdsRecoilMaxAngle = pAdsRecoil ? pAdsRecoil->GetFloat("max_angle", 0) : 0.f;

	// Optional glass on the sights (the "AdsOptic" block, neo_ads_optic_info.h).
	ParseAdsOptic(pKeyValuesData);
}


