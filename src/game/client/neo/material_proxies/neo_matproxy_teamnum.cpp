#include "cbase.h"
#include "functionproxy.h"
#include "toolframework_client.h"

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

// forward declarations
void ToolFramework_RecordMaterialParams(IMaterial* pMaterial);

//-----------------------------------------------------------------------------
// Returns the team number of the entity the material (proxy) is attached to
//-----------------------------------------------------------------------------

class CNEOEntityTeamProxy : public CResultProxy
{
public:
    bool Init( IMaterial *pMaterial, KeyValues *pKeyValues );
    void OnBind( void *pC_BaseEntity );
};

bool CNEOEntityTeamProxy::Init( IMaterial *pMaterial, KeyValues *pKeyValues )
{
    if (!CResultProxy::Init( pMaterial, pKeyValues ))
        return false;

    return true;
}

void CNEOEntityTeamProxy::OnBind( void *pC_BaseEntity )
{
    if (!pC_BaseEntity)
        return;

    C_BaseEntity *pEntity = BindArgToEntity( pC_BaseEntity );
    if (!pEntity)
        return;

    Assert(m_pResult);

    // Shaders like floats, apparently
    SetFloatResult(static_cast<float>(pEntity->GetTeamNumber()));

    if ( ToolsEnabled() )
    {
        ToolFramework_RecordMaterialParams( GetMaterial() );
    }
}

EXPOSE_INTERFACE( CNEOEntityTeamProxy, IMaterialProxy, "EntityTeam" IMATERIAL_PROXY_INTERFACE_VERSION );
