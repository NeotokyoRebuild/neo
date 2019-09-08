//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: 
//
//=============================================================================//

#include "cbase.h"
#include <KeyValues.h>
#include "hl2mp_weapon_parse.h"
#include "ammodef.h"

FileWeaponInfo_t* CreateWeaponInfo()
{
	return new CHL2MPSWeaponInfo;
}



CHL2MPSWeaponInfo::CHL2MPSWeaponInfo()
{
	m_iPlayerDamage = 0;

#ifdef NEO
	m_flVMFov = m_flVMAimFov = 0;

	m_vecVMPosOffset = m_vecVMAimPosOffset = vec3_origin;

	m_angVMAngOffset = m_angVMAimAngOffset = vec3_angle;
#endif
}


void CHL2MPSWeaponInfo::Parse( KeyValues *pKeyValuesData, const char *szWeaponName )
{
	BaseClass::Parse( pKeyValuesData, szWeaponName );

	m_iPlayerDamage = pKeyValuesData->GetInt( "damage", 0 );

#ifdef NEO
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

	KeyValues *pAimOffset = pKeyValuesData->FindKey("AimOffset");
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
#endif
}

