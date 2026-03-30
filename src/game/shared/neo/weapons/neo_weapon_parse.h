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

	// TODO: NT;RE hasn't been using this, idk if they're hardcoded somewhere else
	char m_szAnimExtension[16];		// string used to generate player animations with this weapon

	// int		m_iDamage;
	int		m_iBullets;
	float	m_flCycleTime = 0.0f;

	char	szViewModel2[MAX_WEAPON_STRING];		// Team 2 (NSF) wep vm

	char	szBulletCharacter[MAX_BULLET_CHARACTER];// character used to display ammunition in current clip
	float	m_flVMFov = 0.0f;
	Vector	m_vecVMPosOffset;
	QAngle	m_angVMAngOffset;

	float	m_flVMAimFov = 0.0f;
	Vector	m_vecVMAimPosOffset;
	QAngle	m_angVMAimAngOffset;

	Vector	vecVmOffset;

	char	szDeathIcon[MAX_BULLET_CHARACTER];
	int		iAimFOV;
	float	m_flPenetration;
	bool	m_bDropOnDeath;
};


#endif // NEO_WEAPON_PARSE_H
