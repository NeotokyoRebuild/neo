#pragma once
#include "cbase.h"

#ifdef CLIENT_DLL
#define CNEOWorldPosMarkerEnt C_NEOWorldPosMarkerEnt
class CNEOHud_WorldPosMarker_Generic;
#endif

enum
{
	ICON_INVALID = -1,
	ICON_1 = 0,
	ICON_2,
	ICON_3,
	ICON_4,
	ICON_5,
	ICON_6,
	ICON_A,
	ICON_B,
	ICON_C,
	ICON_D,
	ICON_E,
	ICON_F,

	ICON_TOTAL
};

class CNEOWorldPosMarkerEnt : public CBaseEntity
{
	DECLARE_CLASS( CNEOWorldPosMarkerEnt, CBaseEntity );

public:
#ifdef CLIENT_DLL
	DECLARE_CLIENTCLASS();
#else
	DECLARE_SERVERCLASS();
#endif
	DECLARE_DATADESC();

	CNEOWorldPosMarkerEnt();
	virtual ~CNEOWorldPosMarkerEnt();

#ifdef GAME_DLL
	virtual void	Spawn();
	virtual int		UpdateTransmitState() override;
#else
	virtual void	PostDataUpdate( DataUpdateType_t updateType ) override;
#endif

private:
#ifdef GAME_DLL
	void InputSetSprite( inputdata_t &inputdata );
	void InputSetText( inputdata_t &inputdata );
	void InputEnable( inputdata_t &inputdata );
	void InputDisable( inputdata_t &inputdata );
#else
	friend class CNEOHud_WorldPosMarker_Generic;
#endif

#ifdef GAME_DLL
	CNetworkArray( string_t, m_iszSpriteNames, MAX_SCREEN_OVERLAYS );
	CNetworkString( m_szText, 64 );
#else
	char m_iszSpriteNames[ MAX_SCREEN_OVERLAYS ][ MAX_PATH ];
	char m_szText[64];
#endif
	CNetworkVar( int, m_iCurrentSprite );
	CNetworkVar( float, m_flScale );
	CNetworkVar( bool, m_bShowDistance );
	CNetworkVar( bool, m_bCapzoneEffect );
	CNetworkVar( bool, m_bEnabled );

#ifdef GAME_DLL
	string_t m_szKVText = NULL_STRING;
	int m_iAlpha = 100;
	int m_iIcon = ICON_INVALID;
	bool m_bStartDisabled = false;
#else
	CNEOHud_WorldPosMarker_Generic *m_pHUD_WorldPosMarker = nullptr;
#endif
};