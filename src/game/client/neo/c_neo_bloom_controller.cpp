//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: Neo Bloom Controller
//
// $NoKeywords: $
//===========================================================================//
#include "cbase.h"

extern bool g_bUseCustomBloomScale;
extern float g_flCustomBloomScale;
extern EHANDLE g_hTonemapControllerInUse;

EHANDLE g_hNeoBloomControllerInUse = NULL;

//-----------------------------------------------------------------------------
// Purpose: neo_bloom_controller entity for mapmakers to control bloom amount in maps
//-----------------------------------------------------------------------------
class C_NeoBloomController : public C_BaseEntity
{
	DECLARE_CLASS(C_NeoBloomController, C_BaseEntity);
public:
	DECLARE_CLIENTCLASS();

	C_NeoBloomController();
	~C_NeoBloomController();
	virtual void OnDataChanged(DataUpdateType_t updateType);
private:
	float m_flBloomAmountSetting;
};

IMPLEMENT_CLIENTCLASS_DT(C_NeoBloomController, DT_NeoBloomController, CNeoBloomController)
RecvPropFloat(RECVINFO(m_flBloomAmountSetting)),
END_RECV_TABLE()

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
C_NeoBloomController::C_NeoBloomController(void)
{
	m_flBloomAmountSetting = 1.0f;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
C_NeoBloomController::~C_NeoBloomController(void)
{
	if (g_hNeoBloomControllerInUse == this && g_hTonemapControllerInUse == NULL)
	{
		g_flCustomBloomScale = 1.0f; // Whatever the default bloom amount value is here
		g_bUseCustomBloomScale = false;
	}
}

void C_NeoBloomController::OnDataChanged(DataUpdateType_t updateType)
{
	BaseClass::OnDataChanged(updateType);

	if (g_hTonemapControllerInUse != NULL)
	{
		return;
	}

	g_flCustomBloomScale = m_flBloomAmountSetting;
	g_bUseCustomBloomScale = true;
	g_hNeoBloomControllerInUse = this;
}