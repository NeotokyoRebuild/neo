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

	m_flVMFov = m_flVMAimFov = 0.f;
	m_vecVMPosOffset = m_vecVMAimPosOffset = vec3_origin;
	m_angVMAngOffset = m_angVMAimAngOffset = vec3_angle;
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

	// NEO TODO (Rain): add optional ironsight offsets
	// in addition to "traditional" NT aim

	// AimOffset = Ironsight ADS offset (Disabled)
	// ZoomOffset = Traditional ADS offset
#if 0
	KeyValues *pAimOffset = pKeyValuesData->FindKey("AimOffset");
#else
	KeyValues* pAimOffset = pKeyValuesData->FindKey("ZoomOffset");
#endif
	if (pAimOffset)
	{
		m_flVMAimFov = pAimOffset->GetFloat("fov", 55);

		m_vecVMAimPosOffset.x = pAimOffset->GetFloat("forward", 0);
		m_vecVMAimPosOffset.y = pAimOffset->GetFloat("right", 0);
		m_vecVMAimPosOffset.z = pAimOffset->GetFloat("up", 0);

		m_angVMAimAngOffset[PITCH] = pAimOffset->GetFloat("pitch", 0);
		m_angVMAimAngOffset[YAW] = pAimOffset->GetFloat("yaw", 0);
		m_angVMAimAngOffset[ROLL] = pAimOffset->GetFloat("roll", 0);
	}
}


