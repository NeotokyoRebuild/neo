//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: Resource collection entity
//
// $NoKeywords: $
//=============================================================================//

#ifndef SKYCAMERA_H
#define SKYCAMERA_H

#ifdef _WIN32
#pragma once
#endif

class CSkyCamera;

//=============================================================================
//
// Sky Camera Class
//
class CSkyCamera : public CLogicalEntity
{
	DECLARE_CLASS( CSkyCamera, CLogicalEntity );

public:

	DECLARE_DATADESC();
	CSkyCamera();
	~CSkyCamera();
	virtual void Spawn( void );
	virtual void Activate();

public:
	sky3dparams_t	m_skyboxData;
	bool			m_bUseAngles;
	CSkyCamera		*m_pNext;

#ifdef NEO
private:
	string_t		m_strWaterLevelDesignator = NULL_STRING;
#endif
};


//-----------------------------------------------------------------------------
// Retrives the current skycamera
//-----------------------------------------------------------------------------
CSkyCamera*		GetCurrentSkyCamera();
CSkyCamera*		GetSkyCameraList();


#endif // SKYCAMERA_H
