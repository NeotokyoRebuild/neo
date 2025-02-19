//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: Entity that propagates general data needed by clients for every player.
//
// $NoKeywords: $
//=============================================================================//
#include "cbase.h"
#include "c_playerresource.h"
#include "c_team.h"
#include "gamestringpool.h"

#ifdef HL2MP
#include "hl2mp_gamerules.h"
#endif

#ifdef NEO
#include "neo_gamerules.h"
#include "c_neo_player.h"
#endif

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

const float PLAYER_RESOURCE_THINK_INTERVAL = 0.2f;

IMPLEMENT_CLIENTCLASS_DT_NOBASE(C_PlayerResource, DT_PlayerResource, CPlayerResource)
	RecvPropArray3( RECVINFO_ARRAY(m_iPing), RecvPropInt( RECVINFO(m_iPing[0]))),
#ifdef NEO
	RecvPropArray3(RECVINFO_ARRAY(m_iXP), RecvPropInt(RECVINFO(m_iXP[0]))),
	RecvPropArray3(RECVINFO_ARRAY(m_iClass), RecvPropInt(RECVINFO(m_iClass[0]))),
	RecvPropArray3(RECVINFO_ARRAY(m_szNeoName), RecvPropString(RECVINFO(m_szNeoName[0]))),
	RecvPropArray3(RECVINFO_ARRAY(m_iNeoNameDupeIdx), RecvPropInt(RECVINFO(m_iNeoNameDupeIdx[0]))),
	RecvPropArray3(RECVINFO_ARRAY(m_iStar), RecvPropInt(RECVINFO(m_iStar[0]))),
	RecvPropArray3(RECVINFO_ARRAY(m_szNeoClantag), RecvPropString(RECVINFO(m_szNeoClantag[0]))),
#endif
	RecvPropArray3( RECVINFO_ARRAY(m_iScore), RecvPropInt( RECVINFO(m_iScore[0]))),
	RecvPropArray3( RECVINFO_ARRAY(m_iDeaths), RecvPropInt( RECVINFO(m_iDeaths[0]))),
	RecvPropArray3( RECVINFO_ARRAY(m_bConnected), RecvPropInt( RECVINFO(m_bConnected[0]))),
	RecvPropArray3( RECVINFO_ARRAY(m_iTeam), RecvPropInt( RECVINFO(m_iTeam[0]))),
	RecvPropArray3( RECVINFO_ARRAY(m_bAlive), RecvPropInt( RECVINFO(m_bAlive[0]))),
	RecvPropArray3( RECVINFO_ARRAY(m_iHealth), RecvPropInt( RECVINFO(m_iHealth[0]))),
	RecvPropArray3( RECVINFO_ARRAY(m_iAccountID), RecvPropInt( RECVINFO(m_iAccountID[0]))),
	RecvPropArray3( RECVINFO_ARRAY(m_bValid), RecvPropInt( RECVINFO(m_bValid[0]))),
	RecvPropArray3( RECVINFO_ARRAY( m_iUserID ), RecvPropInt( RECVINFO( m_iUserID[0] ) ) ),
END_RECV_TABLE()

BEGIN_PREDICTION_DATA( C_PlayerResource )

	DEFINE_PRED_ARRAY( m_szName, FIELD_STRING, MAX_PLAYERS_ARRAY_SAFE, FTYPEDESC_PRIVATE ),
	DEFINE_PRED_ARRAY( m_iPing, FIELD_INTEGER, MAX_PLAYERS_ARRAY_SAFE, FTYPEDESC_PRIVATE ),
#ifdef NEO
	DEFINE_PRED_ARRAY(m_iXP, FIELD_INTEGER, MAX_PLAYERS_ARRAY_SAFE, FTYPEDESC_PRIVATE),
	DEFINE_PRED_ARRAY(m_iClass, FIELD_INTEGER, MAX_PLAYERS_ARRAY_SAFE, FTYPEDESC_PRIVATE),
	DEFINE_PRED_ARRAY(m_szNeoName, FIELD_STRING, MAX_PLAYERS_ARRAY_SAFE, FTYPEDESC_PRIVATE),
	DEFINE_PRED_ARRAY(m_iNeoNameDupeIdx, FIELD_INTEGER, MAX_PLAYERS_ARRAY_SAFE, FTYPEDESC_PRIVATE),
	DEFINE_PRED_ARRAY(m_iStar, FIELD_INTEGER, MAX_PLAYERS_ARRAY_SAFE, FTYPEDESC_PRIVATE),
	DEFINE_PRED_ARRAY(m_szNeoClantag, FIELD_STRING, MAX_PLAYERS_ARRAY_SAFE, FTYPEDESC_PRIVATE),
#endif
	DEFINE_PRED_ARRAY( m_iScore, FIELD_INTEGER, MAX_PLAYERS_ARRAY_SAFE, FTYPEDESC_PRIVATE ),
	DEFINE_PRED_ARRAY( m_iDeaths, FIELD_INTEGER, MAX_PLAYERS_ARRAY_SAFE, FTYPEDESC_PRIVATE ),
	DEFINE_PRED_ARRAY( m_bConnected, FIELD_BOOLEAN, MAX_PLAYERS_ARRAY_SAFE, FTYPEDESC_PRIVATE ),
	DEFINE_PRED_ARRAY( m_iTeam, FIELD_INTEGER, MAX_PLAYERS_ARRAY_SAFE, FTYPEDESC_PRIVATE ),
	DEFINE_PRED_ARRAY( m_bAlive, FIELD_BOOLEAN, MAX_PLAYERS_ARRAY_SAFE, FTYPEDESC_PRIVATE ),
	DEFINE_PRED_ARRAY( m_iHealth, FIELD_INTEGER, MAX_PLAYERS_ARRAY_SAFE, FTYPEDESC_PRIVATE ),
	DEFINE_PRED_ARRAY( m_iAccountID, FIELD_INTEGER, MAX_PLAYERS_ARRAY_SAFE, FTYPEDESC_PRIVATE ),
	DEFINE_PRED_ARRAY( m_bValid, FIELD_BOOLEAN, MAX_PLAYERS_ARRAY_SAFE, FTYPEDESC_PRIVATE ),
	DEFINE_PRED_ARRAY( m_iUserID, FIELD_INTEGER, MAX_PLAYERS_ARRAY_SAFE, FTYPEDESC_PRIVATE ),

END_PREDICTION_DATA()	

#ifdef NEO
extern ConVar neo_cl_streamermode;
#endif

C_PlayerResource *g_PR;

IGameResources * GameResources( void ) { return g_PR; }

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
C_PlayerResource::C_PlayerResource()
{
	memset( m_iPing, 0, sizeof( m_iPing ) );
//	memset( m_iPacketloss, 0, sizeof( m_iPacketloss ) );
#ifdef NEO
	memset(m_iXP, 0, sizeof(m_iXP));
	memset(m_iClass, 0, sizeof(m_iClass));
	memset(m_szNeoName, 0, sizeof(m_szNeoName));
	memset(m_iNeoNameDupeIdx, 0, sizeof(m_iNeoNameDupeIdx));
	memset(m_szDispNameWDupeIdx, 0, sizeof(m_szDispNameWDupeIdx));
	memset(m_iStar, 0, sizeof(m_iStar));
	memset(m_szNeoClantag, 0, sizeof(m_szNeoClantag));
#endif
	memset( m_iScore, 0, sizeof( m_iScore ) );
	memset( m_iDeaths, 0, sizeof( m_iDeaths ) );
	memset( m_bConnected, 0, sizeof( m_bConnected ) );
	memset( m_iTeam, 0, sizeof( m_iTeam ) );
	memset( m_bAlive, 0, sizeof( m_bAlive ) );
	memset( m_iHealth, 0, sizeof( m_iHealth ) );
	memset( m_iAccountID, 0, sizeof( m_iAccountID ) );
	memset( m_bValid, 0, sizeof( m_bValid ) );
	memset( m_iUserID, 0, sizeof( m_iUserID ) );
	m_szUnconnectedName = 0;
	
	for ( int i=0; i<MAX_TEAMS; i++ )
	{
		m_Colors[i] = COLOR_GREY;
	}

#ifdef NEO
	m_Colors[TEAM_UNASSIGNED] = COLOR_NEO_WHITE;
	m_Colors[TEAM_SPECTATOR] = COLOR_SPEC;
	m_Colors[TEAM_JINRAI] = COLOR_JINRAI;
	m_Colors[TEAM_NSF] = COLOR_NSF;
#else
#ifdef HL2MP
	m_Colors[TEAM_COMBINE] = COLOR_BLUE;
	m_Colors[TEAM_REBELS] = COLOR_RED;
	m_Colors[TEAM_UNASSIGNED] = COLOR_YELLOW;
#endif
#endif

	g_PR = this;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
C_PlayerResource::~C_PlayerResource()
{
	g_PR = NULL;
}

void C_PlayerResource::OnDataChanged(DataUpdateType_t updateType)
{
	BaseClass::OnDataChanged( updateType );
	if ( updateType == DATA_UPDATE_CREATED )
	{
		SetNextClientThink( gpGlobals->curtime + PLAYER_RESOURCE_THINK_INTERVAL );
	}
}

void C_PlayerResource::UpdatePlayerName( int slot )
{
	if ( slot < 1 || slot > MAX_PLAYERS )
	{
		Error( "UpdatePlayerName with bogus slot %d\n", slot );
		return;
	}

	if ( !m_szUnconnectedName )
	{
		m_szUnconnectedName = AllocPooledString( PLAYER_UNCONNECTED_NAME );
	}
	
	player_info_t sPlayerInfo;
	if ( IsConnected( slot ) && engine->GetPlayerInfo( slot, &sPlayerInfo ) )
	{
		m_szName[slot] = AllocPooledString( UTIL_GetFilteredPlayerName( slot, sPlayerInfo.name ) );
	}
	else 
	{
		if ( !IsValid( slot ) )
		{
			m_szName[slot] = m_szUnconnectedName;
		}
	}
}

void C_PlayerResource::ClientThink()
{
	BaseClass::ClientThink();

	for ( int i = 1; i <= MAX_PLAYERS; ++i )
	{
		UpdatePlayerName( i );
	}

	SetNextClientThink( gpGlobals->curtime + PLAYER_RESOURCE_THINK_INTERVAL );
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
const char *C_PlayerResource::GetPlayerName( int iIndex )
{
#ifdef NEO
	if (iIndex == 0)
	{
		return "world";
	}
#endif

	if ( iIndex < 1 || iIndex > MAX_PLAYERS )
	{
		Assert( false );
		return PLAYER_ERROR_NAME;
	}
	
	if ( !IsConnected( iIndex ) && !IsValid( iIndex ) )
		return PLAYER_UNCONNECTED_NAME;

	// X360TBD: Network - figure out why the name isn't set
	if ( !m_szName[ iIndex ] || !Q_stricmp( m_szName[ iIndex ], PLAYER_UNCONNECTED_NAME ) )
	{
		// If you get a full "reset" uncompressed update from server, then you can have NULLNAME show up in the scoreboard
		UpdatePlayerName( iIndex );
	}

#ifdef NEO
	if (neo_cl_streamermode.GetBool() && !IsLocalPlayer(iIndex))
	{
		[[maybe_unused]] uchar32 u32Out;
		bool bError = false;
		const int iSize = Q_UTF8ToUChar32(m_szName[iIndex], u32Out, bError);
		if (!bError) V_memcpy(gStreamerModeNames[iIndex], m_szName[iIndex], iSize);
		return gStreamerModeNames[iIndex];
	}

	const bool clientWantNeoName = C_NEO_Player::GetLocalNEOPlayer()->ClientWantNeoName();
	const int dupeIdx = m_iNeoNameDupeIdx[iIndex];

	if (clientWantNeoName && m_szNeoName[iIndex][0] != '\0')
	{
		if (dupeIdx > 0)
		{
			Q_snprintf(m_szDispNameWDupeIdx[iIndex], sizeof(m_szDispNameWDupeIdx[iIndex]), "%s (%d)", m_szNeoName[iIndex], dupeIdx);
			return m_szDispNameWDupeIdx[iIndex];
		}
		return m_szNeoName[iIndex];
	}

	if (clientWantNeoName && dupeIdx > 0)
	{
		Q_snprintf(m_szDispNameWDupeIdx[iIndex], sizeof(m_szDispNameWDupeIdx[iIndex]), "%s (%d)", m_szName[iIndex], dupeIdx);
		return m_szDispNameWDupeIdx[iIndex];
	}
#endif

	// This gets updated in ClientThink, so it could be up to 1 second out of date, oh well.
	return m_szName[iIndex];
}

#ifdef NEO
const char *C_PlayerResource::GetClanTag(int iIndex)
{
	return m_szNeoClantag[iIndex];
}
#endif

bool C_PlayerResource::IsAlive(int iIndex )
{
	return m_bAlive[iIndex];
}

int C_PlayerResource::GetTeam(int iIndex )
{
#ifdef NEO
	if (iIndex == 0)
	{
		return TEAM_UNASSIGNED;
	}
#endif

	if ( iIndex < 1 || iIndex > MAX_PLAYERS )
	{
		Assert( false );
		return 0;
	}
	else
	{
		return m_iTeam[iIndex];
	}
}

const char * C_PlayerResource::GetTeamName(int index_)
{
	C_Team *team = GetGlobalTeam( index_ );

	if ( !team )
		return "Unknown";

	return team->Get_Name();
}

int C_PlayerResource::GetTeamScore(int index_ )
{
	C_Team *team = GetGlobalTeam( index_ );

	if ( !team )
		return 0;

	return team->Get_Score();
}

int C_PlayerResource::GetXP(int index)
{
	if (!IsConnected(index))
	{
		return 0;
	}

	return m_iXP[index];
}

int C_PlayerResource::GetClass(int index)
{
	if (!IsConnected(index))
	{
		return -1;
	}

	return m_iClass[index];
}

int C_PlayerResource::GetStar(int index)
{
	if (!IsConnected(index))
	{
		return -1;
	}

	return m_iStar[index];
}

int C_PlayerResource::GetFrags(int index_ )
{
	return 666;
}

bool C_PlayerResource::IsLocalPlayer(int index_ )
{
	C_BasePlayer *pPlayer =	C_BasePlayer::GetLocalPlayer();

	if ( !pPlayer )
		return false;

	return (index_ == pPlayer->entindex() );
}


bool C_PlayerResource::IsHLTV(int index_ )
{
	if ( !IsConnected( index_ ) && !IsValid( index_ ) )
		return false;

	player_info_t sPlayerInfo;
	
	if ( engine->GetPlayerInfo( index_, &sPlayerInfo ) )
	{
		return sPlayerInfo.ishltv;
	}
	
	return false;
}

bool C_PlayerResource::IsReplay(int index_ )
{
#if defined( REPLAY_ENABLED )
	if ( !IsConnected( index_ ) && !IsValid( index_ ) )
		return false;

	player_info_t sPlayerInfo;

	if ( engine->GetPlayerInfo( index_, &sPlayerInfo ) )
	{
		return sPlayerInfo.isreplay;
	}
#endif

	return false;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
bool C_PlayerResource::IsFakePlayer( int iIndex )
{
	if ( !IsConnected( iIndex ) && !IsValid( iIndex ) )
		return false;

	// Yuck, make sure it's up to date
	player_info_t sPlayerInfo;
	if ( engine->GetPlayerInfo( iIndex, &sPlayerInfo ) )
	{
		return sPlayerInfo.fakeplayer;
	}
	
	return false;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
int	C_PlayerResource::GetPing( int iIndex )
{
	if ( !IsConnected( iIndex ) && !IsValid( iIndex ) )
		return 0;

	return m_iPing[iIndex];
}

//-----------------------------------------------------------------------------
// Purpose: 
/*-----------------------------------------------------------------------------
int	C_PlayerResource::GetPacketloss( int iIndex )
{
	if ( !IsConnected( iIndex ) && !IsPreservedData( iIndex ) )
		return 0;

	return m_iPacketloss[iIndex];
}*/

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
int	C_PlayerResource::GetPlayerScore( int iIndex )
{
	if ( !IsConnected( iIndex ) && !IsValid( iIndex ) )
		return 0;

	return m_iScore[iIndex];
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
int	C_PlayerResource::GetDeaths( int iIndex )
{
	if ( !IsConnected( iIndex ) && !IsValid( iIndex ) )
		return 0;

	return m_iDeaths[iIndex];
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
int	C_PlayerResource::GetHealth( int iIndex )
{
	if ( !IsConnected( iIndex ) && !IsValid( iIndex ) )
		return 0;

	return m_iHealth[iIndex];
}

const Color &C_PlayerResource::GetTeamColor(int index_ )
{
	if ( index_ < 0 || index_ >= MAX_TEAMS )
	{
		Assert( false );
		static Color blah;
		return blah;
	}
	else
	{
		return m_Colors[index_];
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
bool C_PlayerResource::IsConnected( int iIndex )
{
	if ( iIndex < 1 || iIndex > MAX_PLAYERS )
		return false;
	else
		return m_bConnected[iIndex];
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
uint32 C_PlayerResource::GetAccountID( int iIndex )
{
	if ( ( iIndex < 0 ) || ( iIndex >= ARRAYSIZE( m_iAccountID ) ) )
		return 0;

	if ( !IsConnected( iIndex ) && !IsValid( iIndex ) )
		return 0;

	return m_iAccountID[iIndex];
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
bool C_PlayerResource::IsValid( int iIndex )
{
	if ( ( iIndex < 0 ) || ( iIndex >= ARRAYSIZE( m_bValid ) ) )
		return false;

	return m_bValid[iIndex];
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
int C_PlayerResource::GetUserID( int iIndex )
{
	if ( ( iIndex < 0 ) || ( iIndex >= ARRAYSIZE( m_iUserID ) ) )
		return 0;

	if ( !IsConnected( iIndex ) )
		return 0;

	return m_iUserID[iIndex];
}
