//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: 
//
// $NoKeywords: $
//=============================================================================//
#include "cbase.h"
#include "materialsystem/imaterialproxy.h"
#include "materialsystem/imaterial.h"
#include "materialsystem/imaterialvar.h"
#include <KeyValues.h>
#include "functionproxy.h"
#include "neo/ui/neo_hud_ghost_beacons.h"
#include "toolframework_client.h"

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

// forward declarations
void ToolFramework_RecordMaterialParams( IMaterial *pMaterial );

class CGhostBeaconRotationMaterialProxy : public IMaterialProxy
{
public:
	CGhostBeaconRotationMaterialProxy();
	virtual ~CGhostBeaconRotationMaterialProxy();
	virtual bool Init( IMaterial *pMaterial, KeyValues *pKeyValues );
	virtual void OnBind( void *pC_BaseEntity );
	virtual void Release( void ) { delete this; }
	virtual IMaterial *GetMaterial();

private:
	IMaterialVar *m_pGhostBeaconRotationVar;
};

CGhostBeaconRotationMaterialProxy::CGhostBeaconRotationMaterialProxy()
{
	m_pGhostBeaconRotationVar = NULL;
}

CGhostBeaconRotationMaterialProxy::~CGhostBeaconRotationMaterialProxy()
{
}

bool CGhostBeaconRotationMaterialProxy::Init( IMaterial *pMaterial, KeyValues *pKeyValues )
{
	char const* pScrollVarName = pKeyValues->GetString( "ghostBeaconRotationVar" );
	if( !pScrollVarName )
		return false;

	bool foundVar;
	m_pGhostBeaconRotationVar = pMaterial->FindVar( pScrollVarName, &foundVar, false );
	if( !foundVar )
		return false;

	return true;
}

extern CNEOHud_GhostBeacons* gHudGhostBeacons;
void CGhostBeaconRotationMaterialProxy::OnBind( void *pC_BaseEntity )
{
	if( !m_pGhostBeaconRotationVar || !gHudGhostBeacons )
	{
		return;
	}

	m_pGhostBeaconRotationVar->SetFloatValue( gHudGhostBeacons->GetRotation() );

	if ( ToolsEnabled() )
	{
		ToolFramework_RecordMaterialParams( GetMaterial() );
	}
}

IMaterial *CGhostBeaconRotationMaterialProxy::GetMaterial()
{
	return m_pGhostBeaconRotationVar->GetOwningMaterial();
}

EXPOSE_INTERFACE( CGhostBeaconRotationMaterialProxy, IMaterialProxy, "GhostBeaconRotation" IMATERIAL_PROXY_INTERFACE_VERSION );
