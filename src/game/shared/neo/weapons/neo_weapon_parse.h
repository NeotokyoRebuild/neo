//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose:
//
//=============================================================================//

#ifndef NEO_WEAPON_PARSE_H
#define NEO_WEAPON_PARSE_H
#ifdef _WIN32
#pragma once
#endif

#include "hl2mp_weapon_parse.h"

//--------------------------------------------------------------------------------------------------------
class CNEOWeaponInfo : public CHL2MPSWeaponInfo
{
public:
	DECLARE_CLASS_GAMEROOT( CNEOWeaponInfo, CHL2MPSWeaponInfo );

	CNEOWeaponInfo();

	virtual void Parse( ::KeyValues *pKeyValuesData, const char *szWeaponName );

	int		m_iBullets;
	float	m_flCycleTime;
	char	szViewModel2[MAX_WEAPON_STRING];		// Team 2 (NSF) wep vm
	char	szBulletCharacter[MAX_BULLET_CHARACTER];// character used to display ammunition in current clip
	char	szDeathIcon[MAX_BULLET_CHARACTER];
	float	m_flPenetration;
	bool	m_bDropOnDeath;
	int		iAimFOV;

	float	m_flVMFov;
	Vector	m_vecVMPosOffset;
	QAngle	m_angVMAngOffset;

	float	m_flVMAimFov;
	Vector	m_vecVMAimPosOffset;
	QAngle	m_angVMAimAngOffset;

	// The ADS pose ("AimOffset" block). m_bHasAds: the block says "enabled" "1", so the weapon takes part in ADS.
	bool	m_bHasAds;
	float	m_flVMAdsFov;
	Vector	m_vecVMAdsPosOffset;
	QAngle	m_angVMAdsAngOffset;

	// Per-weapon multipliers on the cl_neo_ads_recoil_* scales ("AdsRecoil" block).
	float	m_flAdsRecoilVertical;
	float	m_flAdsRecoilSide;
	float	m_flAdsRecoilBack;
	float	m_flAdsRecoilMaxDist;	// <= 0: use cl_neo_ads_recoil_max_dist
	float	m_flAdsRecoilMaxAngle;	// <= 0: use cl_neo_ads_recoil_max_angle
};


#endif // NEO_WEAPON_PARSE_H
