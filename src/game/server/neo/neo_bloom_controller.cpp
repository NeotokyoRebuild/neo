//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: 
//
//=============================================================================

#include "cbase.h"
#include "baseentity.h"
#include "entityoutput.h"
#include "convar.h"
// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

//-----------------------------------------------------------------------------
// Purpose: Entity that controls player's tonemap
//-----------------------------------------------------------------------------
class CNeoBloomController : public CPointEntity
{
	DECLARE_CLASS( CNeoBloomController, CPointEntity );
public:
	DECLARE_DATADESC();
	DECLARE_SERVERCLASS();

	void	Spawn( void );
	int		UpdateTransmitState( void );

	// Inputs
	void	InputSetBloomScale( inputdata_t &inputdata );

private:
	CNetworkVar( float, m_flBloomAmountSetting);
};

LINK_ENTITY_TO_CLASS( neo_bloom_controller, CNeoBloomController );

BEGIN_DATADESC( CNeoBloomController )
	DEFINE_KEYFIELD( m_flBloomAmountSetting, FIELD_FLOAT, "bloomamount" ),
	DEFINE_INPUTFUNC( FIELD_FLOAT, "SetBloomScale", InputSetBloomScale ),
END_DATADESC()


IMPLEMENT_SERVERCLASS_ST( CNeoBloomController, DT_NeoBloomController )
	SendPropFloat( SENDINFO(m_flBloomAmountSetting), 0, SPROP_NOSCALE),
END_SEND_TABLE()

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CNeoBloomController::Spawn( void )
{
	SetSolid( SOLID_NONE );
	SetMoveType( MOVETYPE_NONE );
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
int CNeoBloomController::UpdateTransmitState()
{
	return SetTransmitState( FL_EDICT_ALWAYS );
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CNeoBloomController::InputSetBloomScale( inputdata_t &inputdata )
{
	m_flBloomAmountSetting = inputdata.value.Float();
}