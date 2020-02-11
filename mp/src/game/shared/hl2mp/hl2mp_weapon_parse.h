//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: 
//
//=============================================================================//

#ifndef HL2MP_WEAPON_PARSE_H
#define HL2MP_WEAPON_PARSE_H
#ifdef _WIN32
#pragma once
#endif


#include "weapon_parse.h"
#include "networkvar.h"


//--------------------------------------------------------------------------------------------------------
class CHL2MPSWeaponInfo : public FileWeaponInfo_t
{
public:
	DECLARE_CLASS_GAMEROOT( CHL2MPSWeaponInfo, FileWeaponInfo_t );
	
	CHL2MPSWeaponInfo();
	
	virtual void Parse( ::KeyValues *pKeyValuesData, const char *szWeaponName );


public:

	int m_iPlayerDamage;

#ifdef NEO
	float m_flVMFov;

	Vector m_vecVMPosOffset;
	QAngle m_angVMAngOffset;

	float m_flVMAimFov;
	Vector m_vecVMAimPosOffset;
	QAngle m_angVMAimAngOffset;
#endif
};


#endif // HL2MP_WEAPON_PARSE_H
