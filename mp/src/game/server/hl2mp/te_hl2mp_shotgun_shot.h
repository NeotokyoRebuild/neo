//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: 
//
//=============================================================================//

#ifndef TE_HL2MP_SHOTGUN_SHOT_H
#define TE_HL2MP_SHOTGUN_SHOT_H
#ifdef _WIN32
#pragma once
#endif


void TE_HL2MPFireBullets( 
	int	iPlayerIndex,
	const Vector &vOrigin,
	const Vector &vDir,
	int	iAmmoID,
	int iSeed,
	int iShots,
#ifdef NEO
	const Vector &vSpread,
#else
	float flSpread, 
#endif
	bool bDoTracers,
	bool bDoImpacts );


#endif // TE_HL2MP_SHOTGUN_SHOT_H
