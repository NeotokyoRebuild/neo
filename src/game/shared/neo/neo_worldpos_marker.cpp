#include "neo_worldpos_marker.h"

#ifdef CLIENT_DLL
#include "ui/neo_hud_worldpos_marker_generic.h"
#endif

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

LINK_ENTITY_TO_CLASS( neo_worldpos_marker, CNEOWorldPosMarkerEnt );
#ifdef GAME_DLL
LINK_ENTITY_TO_CLASS( neo_controlpoint, CNEOWorldPosMarkerEnt );
#endif

#ifdef GAME_DLL
extern void SendProxy_StringT_To_String( const SendProp *pProp, const void *pStruct, const void *pData, DVariant *pOut, int iElement, int objectID );

IMPLEMENT_SERVERCLASS_ST( CNEOWorldPosMarkerEnt, DT_NEOWorldPosMarkerEnt )
	SendPropArray( SendPropString( SENDINFO_ARRAY( m_iszSpriteNames ), 0, SendProxy_StringT_To_String ), m_iszSpriteNames ),
	SendPropInt(SENDINFO( m_iCurrentSprite ), NumBitsForCount( MAX_SCREEN_OVERLAYS ), SPROP_UNSIGNED ),
	SendPropString(SENDINFO( m_szText )),
	SendPropFloat(SENDINFO( m_flScale )),
	SendPropBool(SENDINFO( m_bShowDistance )),
	SendPropBool(SENDINFO( m_bCapzoneEffect )),
	SendPropBool(SENDINFO( m_bEnabled ))
END_SEND_TABLE()
#else
#ifdef CNEOWorldPosMarkerEnt
#undef CNEOWorldPosMarkerEnt
#endif
IMPLEMENT_CLIENTCLASS_DT( C_NEOWorldPosMarkerEnt, DT_NEOWorldPosMarkerEnt, CNEOWorldPosMarkerEnt )
	RecvPropArray( RecvPropString( RECVINFO( m_iszSpriteNames[0]) ), m_iszSpriteNames ),
	RecvPropInt(RECVINFO( m_iCurrentSprite )),
	RecvPropString(RECVINFO( m_szText )),
	RecvPropFloat(RECVINFO( m_flScale )),
	RecvPropBool(RECVINFO( m_bShowDistance )),
	RecvPropBool(RECVINFO( m_bCapzoneEffect )),
	RecvPropBool(RECVINFO( m_bEnabled ))
END_RECV_TABLE()
#define CNEOWorldPosMarkerEnt C_NEOWorldPosMarkerEnt
#endif

BEGIN_DATADESC( CNEOWorldPosMarkerEnt )
#ifdef GAME_DLL
	DEFINE_KEYFIELD( m_iszSpriteNames[0], FIELD_STRING, "SpriteName1" ),
	DEFINE_KEYFIELD( m_iszSpriteNames[1], FIELD_STRING, "SpriteName2" ),
	DEFINE_KEYFIELD( m_iszSpriteNames[2], FIELD_STRING, "SpriteName3" ),
	DEFINE_KEYFIELD( m_iszSpriteNames[3], FIELD_STRING, "SpriteName4" ),
	DEFINE_KEYFIELD( m_iszSpriteNames[4], FIELD_STRING, "SpriteName5" ),
	DEFINE_KEYFIELD( m_iszSpriteNames[5], FIELD_STRING, "SpriteName6" ),
	DEFINE_KEYFIELD( m_iszSpriteNames[6], FIELD_STRING, "SpriteName7" ),
	DEFINE_KEYFIELD( m_iszSpriteNames[7], FIELD_STRING, "SpriteName8" ),
	DEFINE_KEYFIELD( m_iszSpriteNames[8], FIELD_STRING, "SpriteName9" ),
	DEFINE_KEYFIELD( m_iszSpriteNames[9], FIELD_STRING, "SpriteName10" ),
	DEFINE_KEYFIELD( m_flScale, FIELD_FLOAT, "scale" ),
	DEFINE_KEYFIELD( m_szKVText, FIELD_STRING, "Name" ),
	DEFINE_KEYFIELD( m_iAlpha, FIELD_INTEGER, "alpha" ),
	DEFINE_KEYFIELD( m_bStartDisabled, FIELD_BOOLEAN, "StartDisabled" ),
	DEFINE_KEYFIELD( m_bShowDistance, FIELD_BOOLEAN, "showdistance" ),
	DEFINE_KEYFIELD( m_bCapzoneEffect, FIELD_BOOLEAN, "capzoneeffect" ),

	// Legacy, for neo_controlpoint
	DEFINE_KEYFIELD( m_iIcon, FIELD_INTEGER, "Icon" ),

	// Inputs
	DEFINE_INPUTFUNC( FIELD_INTEGER, "SetSprite", InputSetSprite ),
	DEFINE_INPUTFUNC( FIELD_STRING, "SetText", InputSetText ),
	DEFINE_INPUTFUNC( FIELD_VOID, "Enable", InputEnable ),
	DEFINE_INPUTFUNC( FIELD_VOID, "Disable", InputDisable ),
#endif
END_DATADESC()

static const char *ICON_PATHS[] =
{
	"vgui/hud/cp/cp_1",
	"vgui/hud/cp/cp_2",
	"vgui/hud/cp/cp_3",
	"vgui/hud/cp/cp_4",
	"vgui/hud/cp/cp_5",
	"vgui/hud/cp/cp_6",
	"vgui/hud/cp/cp_a",
	"vgui/hud/cp/cp_b",
	"vgui/hud/cp/cp_c",
	"vgui/hud/cp/cp_d",
	"vgui/hud/cp/cp_e",
	"vgui/hud/cp/cp_f",
};

CNEOWorldPosMarkerEnt::CNEOWorldPosMarkerEnt()
{
#ifdef GAME_DLL
	m_szText.GetForModify()[0] = 0;
#endif
	m_iCurrentSprite = 0;
	m_flScale = 0.5f;
	m_bShowDistance = true;
	m_bCapzoneEffect = false;
	m_bEnabled = true;
}

CNEOWorldPosMarkerEnt::~CNEOWorldPosMarkerEnt()
{
#ifdef CLIENT_DLL
	if ( m_pHUD_WorldPosMarker )
	{
		m_pHUD_WorldPosMarker->DeletePanel();
		m_pHUD_WorldPosMarker = nullptr;
	}
#endif
}

#ifdef GAME_DLL
void CNEOWorldPosMarkerEnt::Spawn()
{
	if ( m_bStartDisabled )
	{
		m_bEnabled = false;
	}

	COMPILE_TIME_ASSERT( ICON_TOTAL == ARRAYSIZE(ICON_PATHS) );
	if ( m_iIcon > ICON_INVALID && m_iIcon < ICON_TOTAL )
	{
		KeyValue( "SpriteName1", ICON_PATHS[m_iIcon] );
	}

	V_strncpy( m_szText.GetForModify(), STRING( m_szKVText ), 64 );

	SetRenderColorA( clamp( m_iAlpha, 0, 255 ) );
}

int CNEOWorldPosMarkerEnt::UpdateTransmitState()
{
	return SetTransmitState( FL_EDICT_ALWAYS );
}

void CNEOWorldPosMarkerEnt::InputSetSprite( inputdata_t &inputdata )
{
	int iCorrectedIndex = inputdata.value.Int() - 1;
	iCorrectedIndex = abs( iCorrectedIndex );

	if ( m_iszSpriteNames[iCorrectedIndex] == NULL_STRING )
	{
		Warning("neo_worldpos_marker %s has no sprite to display at index %d.\n", STRING(GetEntityName()), inputdata.value.Int() );
		return;
	}

	m_iCurrentSprite = iCorrectedIndex;
}

void CNEOWorldPosMarkerEnt::InputSetText( inputdata_t &inputdata )
{
	V_strncpy( m_szText.GetForModify(), inputdata.value.String(), 64 );
}

void CNEOWorldPosMarkerEnt::InputEnable( inputdata_t &inputdata )
{
	m_bEnabled = true;
}

void CNEOWorldPosMarkerEnt::InputDisable( inputdata_t &inputdata )
{
	m_bEnabled = false;
}
#else
void CNEOWorldPosMarkerEnt::PostDataUpdate( DataUpdateType_t updateType )
{
	BaseClass::PostDataUpdate( updateType );

	if ( !m_pHUD_WorldPosMarker && updateType == DATA_UPDATE_DATATABLE_CHANGED )
	{
		m_pHUD_WorldPosMarker = new CNEOHud_WorldPosMarker_Generic( "hudWPMent", this );
		m_pHUD_WorldPosMarker->SetVisible( true );
	}
}
#endif
