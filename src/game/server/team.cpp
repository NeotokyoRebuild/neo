//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: Team management class. Contains all the details for a specific team
//
// $NoKeywords: $
//=============================================================================//
#include "cbase.h"
#include "team.h"
#include "player.h"
#include "team_spawnpoint.h"

#ifdef NEO
#include "neo_player.h"
#endif

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

CUtlVector< CTeam * > g_Teams;

//-----------------------------------------------------------------------------
// Purpose: SendProxy that converts the Team's player UtlVector to entindexes
//-----------------------------------------------------------------------------
void SendProxy_PlayerList( const SendProp *pProp, const void *pStruct, const void *pData, DVariant *pOut, int iElement, int objectID )
{
	CTeam *pTeam = (CTeam*)pData;

	// If this assertion fails, then SendProxyArrayLength_PlayerArray must have failed.
	Assert( iElement < pTeam->m_aPlayers.Size() );

	CBasePlayer *pPlayer = pTeam->m_aPlayers[iElement];
	pOut->m_Int = pPlayer->entindex();
}


int SendProxyArrayLength_PlayerArray( const void *pStruct, int objectID )
{
	CTeam *pTeam = (CTeam*)pStruct;
	return pTeam->m_aPlayers.Count();
}


// Datatable
IMPLEMENT_SERVERCLASS_ST_NOBASE(CTeam, DT_Team)
	SendPropInt( SENDINFO(m_iTeamNum), 5 ),
	SendPropInt( SENDINFO(m_iScore), 0 ),
	SendPropInt( SENDINFO(m_iRoundsWon), 8 ),
	SendPropString( SENDINFO( m_szTeamname ) ),
#ifdef NEO
	SendPropInt( SENDINFO(m_iReconCount), NumBitsForCount(MAX_PLAYERS) ),
	SendPropInt( SENDINFO(m_iAssaultCount), NumBitsForCount(MAX_PLAYERS) ),
	SendPropInt( SENDINFO(m_iSupportCount), NumBitsForCount(MAX_PLAYERS) ),
#endif // NEO

	SendPropArray2( 
		SendProxyArrayLength_PlayerArray,
		SendPropInt("player_array_element", 0, 4, 10, SPROP_UNSIGNED, SendProxy_PlayerList), 
		MAX_PLAYERS, 
		0, 
		"player_array"
		)
END_SEND_TABLE()

LINK_ENTITY_TO_CLASS( team_manager, CTeam );

//-----------------------------------------------------------------------------
// Purpose: Get a pointer to the specified team manager
//-----------------------------------------------------------------------------
CTeam *GetGlobalTeam( int iIndex )
{
	if ( iIndex < 0 || iIndex >= GetNumberOfTeams() )
		return NULL;

	return g_Teams[ iIndex ];
}

//-----------------------------------------------------------------------------
// Purpose: Get the number of team managers
//-----------------------------------------------------------------------------
int GetNumberOfTeams( void )
{
	return g_Teams.Size();
}


//-----------------------------------------------------------------------------
// Purpose: Needed because this is an entity, but should never be used
//-----------------------------------------------------------------------------
CTeam::CTeam( void )
{
	memset( m_szTeamname.GetForModify(), 0, sizeof(m_szTeamname) );
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
CTeam::~CTeam( void )
{
	m_aSpawnPoints.Purge();
	m_aPlayers.Purge();
}

//-----------------------------------------------------------------------------
// Purpose: Called every frame
//-----------------------------------------------------------------------------
void CTeam::Think( void )
{
}

//-----------------------------------------------------------------------------
// Purpose: Teams are always transmitted to clients
//-----------------------------------------------------------------------------
int CTeam::UpdateTransmitState()
{
	return SetTransmitState( FL_EDICT_ALWAYS );
}

//-----------------------------------------------------------------------------
// Visibility/scanners
//-----------------------------------------------------------------------------
bool CTeam::ShouldTransmitToPlayer( CBasePlayer* pRecipient, CBaseEntity* pEntity ) const
{
	// Always transmit the observer target to players
	if ( pRecipient && pRecipient->IsObserver() && pRecipient->GetObserverTarget() == pEntity )
		return true;

	return false;
}

//-----------------------------------------------------------------------------
// Initialization
//-----------------------------------------------------------------------------
void CTeam::Init( const char *pName, int iNumber )
{
	InitializeSpawnpoints();
	InitializePlayers();

	m_iScore = 0;

	Q_strncpy( m_szTeamname.GetForModify(), pName, MAX_TEAM_NAME_LENGTH );
	m_iTeamNum = iNumber;
}

//-----------------------------------------------------------------------------
// DATA HANDLING
//-----------------------------------------------------------------------------
int CTeam::GetTeamNumber( void ) const
{
	return m_iTeamNum;
}

//-----------------------------------------------------------------------------
// Purpose: Get the team's name
//-----------------------------------------------------------------------------
const char *CTeam::GetName( void ) const
{
	return m_szTeamname;
}


//-----------------------------------------------------------------------------
// Purpose: Update the player's client data
//-----------------------------------------------------------------------------
void CTeam::UpdateClientData( CBasePlayer *pPlayer )
{
}

//------------------------------------------------------------------------------------------------------------------
// SPAWNPOINTS
//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTeam::InitializeSpawnpoints( void )
{
	m_iLastSpawn = 0;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTeam::AddSpawnpoint( CTeamSpawnPoint *pSpawnpoint )
{
	m_aSpawnPoints.AddToTail( pSpawnpoint );
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTeam::RemoveSpawnpoint( CTeamSpawnPoint *pSpawnpoint )
{
	for (int i = 0; i < m_aSpawnPoints.Size(); i++ )
	{
		if ( m_aSpawnPoints[i] == pSpawnpoint )
		{
			m_aSpawnPoints.Remove( i );
			return;
		}
	}
}

//-----------------------------------------------------------------------------
// Purpose: Spawn the player at one of this team's spawnpoints. Return true if successful.
//-----------------------------------------------------------------------------
CBaseEntity *CTeam::SpawnPlayer( CBasePlayer *pPlayer )
{
	if ( m_aSpawnPoints.Size() == 0 )
		return NULL;

	// Randomize the start spot
	int iSpawn = m_iLastSpawn + random->RandomInt( 1,3 );
	if ( iSpawn >= m_aSpawnPoints.Size() )
		iSpawn -= m_aSpawnPoints.Size();
	int iStartingSpawn = iSpawn;

	// Now loop through the spawnpoints and pick one
	int loopCount = 0;
	do 
	{
		if ( iSpawn >= m_aSpawnPoints.Size() )
		{
			++loopCount;
			iSpawn = 0;
		}

		// check if pSpot is valid, and that the player is on the right team
		if ( (loopCount > 3) || m_aSpawnPoints[iSpawn]->IsValid( pPlayer ) )
		{
			// DevMsg( 1, "player: spawning at (%s)\n", STRING(m_aSpawnPoints[iSpawn]->m_iName) );
			m_aSpawnPoints[iSpawn]->m_OnPlayerSpawn.FireOutput( pPlayer, m_aSpawnPoints[iSpawn] );

			m_iLastSpawn = iSpawn;
			return m_aSpawnPoints[iSpawn];
		}

		iSpawn++;
	} while ( iSpawn != iStartingSpawn ); // loop if we're not back to the start

	return NULL;
}

//------------------------------------------------------------------------------------------------------------------
// PLAYERS
//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTeam::InitializePlayers( void )
{
}

//-----------------------------------------------------------------------------
// Purpose: Add the specified player to this team. Remove them from their current team, if any.
//-----------------------------------------------------------------------------
void CTeam::AddPlayer( CBasePlayer *pPlayer )
{
	m_aPlayers.AddToTail( pPlayer );
#ifdef NEO
	UpdateClassCounts();
#endif // NEO
	NetworkStateChanged();
}

//-----------------------------------------------------------------------------
// Purpose: Remove this player from the team
//-----------------------------------------------------------------------------
void CTeam::RemovePlayer( CBasePlayer *pPlayer )
{
	m_aPlayers.FindAndRemove( pPlayer );
#ifdef NEO
	UpdateClassCounts();
#endif // NEO
	NetworkStateChanged();
}

//-----------------------------------------------------------------------------
// Purpose: Return the number of players in this team.
//-----------------------------------------------------------------------------
int CTeam::GetNumPlayers( void ) const
{
	return m_aPlayers.Size();
}

//-----------------------------------------------------------------------------
// Purpose: Get a specific player
//-----------------------------------------------------------------------------
CBasePlayer *CTeam::GetPlayer( int iIndex ) const
{
	Assert( iIndex >= 0 && iIndex < m_aPlayers.Size() );
	return m_aPlayers[ iIndex ];
}

//------------------------------------------------------------------------------------------------------------------
// SCORING
//-----------------------------------------------------------------------------
// Purpose: Add / Remove score for this team
//-----------------------------------------------------------------------------
void CTeam::AddScore( int iScore )
{
	m_iScore += iScore;
}

void CTeam::SetScore( int iScore )
{
	m_iScore = iScore;
}

//-----------------------------------------------------------------------------
// Purpose: Get this team's score
//-----------------------------------------------------------------------------
int CTeam::GetScore( void ) const
{
	return m_iScore;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTeam::ResetScores( void )
{
	SetScore(0);
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTeam::AwardAchievement( int iAchievement )
{
	Assert( iAchievement >= 0 && iAchievement < 255 );	// must fit in short 

	CRecipientFilter filter;

	int iNumPlayers = GetNumPlayers();

	for ( int i=0;i<iNumPlayers;i++ )
	{
		if ( GetPlayer(i) )
		{
			filter.AddRecipient( GetPlayer(i) );
		}
	}

	UserMessageBegin( filter, "AchievementEvent" );
		WRITE_SHORT( iAchievement );
	MessageEnd();
}

int CTeam::GetAliveMembers( void ) const
{
	int iAlive = 0;

	int iNumPlayers = GetNumPlayers();

	for ( int i=0;i<iNumPlayers;i++ )
	{
#ifdef NEO
		auto player = static_cast<CNEO_Player*>(GetPlayer(i));
		// Consider yet unspawned players as alive, too, if they're qualified to spawn for this round.
		if (player && (player->IsAlive() || player->m_bIsPendingSpawnForThisRound))
		{
			++iAlive;
		}
#else
		if (GetPlayer(i) && GetPlayer(i)->IsAlive())
		{
			iAlive++;
		}
#endif
	}

	return iAlive;
}

#ifdef NEO
int CTeam::GetClassCount(int neoClass) const
{
	int iNumClass = 0;

	const int iNumPlayers = GetNumPlayers();
	for (int i = 0; i < iNumPlayers; i++)
	{
		if (auto pNeoPlayer = static_cast<CNEO_Player*>(GetPlayer(i));
			pNeoPlayer && neoClass == pNeoPlayer->GetClass())
		{
			iNumClass++;
		}
	}

	return iNumClass;
}

extern ConVar sv_neo_class_limit_recon;
extern ConVar sv_neo_class_limit_assault;
extern ConVar sv_neo_class_limit_support;
bool CTeam::IsClassFull(int neoClass) const
{
	switch (neoClass)
	{
	case NEO_CLASS_RECON:
		return sv_neo_class_limit_recon.GetInt() == -1 ? false : m_iReconCount >= sv_neo_class_limit_recon.GetInt();
	case NEO_CLASS_ASSAULT:
		return sv_neo_class_limit_assault.GetInt() == -1 ? false : m_iAssaultCount >= sv_neo_class_limit_assault.GetInt();
	case NEO_CLASS_SUPPORT:
		return sv_neo_class_limit_support.GetInt() == -1 ? false : m_iSupportCount >= sv_neo_class_limit_support.GetInt();
	}

	return false;
}

bool CTeam::IsClassOverThreshold(int neoClass) const
{
	switch (neoClass)
	{
	case NEO_CLASS_RECON:
		return sv_neo_class_limit_recon.GetInt() == -1 ? false : m_iReconCount > sv_neo_class_limit_recon.GetInt();
	case NEO_CLASS_ASSAULT:
		return sv_neo_class_limit_assault.GetInt() == -1 ? false : m_iAssaultCount > sv_neo_class_limit_assault.GetInt();
	case NEO_CLASS_SUPPORT:
		return sv_neo_class_limit_support.GetInt() == -1 ? false : m_iSupportCount > sv_neo_class_limit_support.GetInt();
	}

	return false;
}

int CTeam::GetAppropriateClass(int neoClass) const
{
	switch (neoClass)
	{
		case NEO_CLASS_RANDOM:
			neoClass = RandomInt(NEO_CLASS_RECON, NEO_CLASS_SUPPORT);
			[[fallthrough]];
		case NEO_CLASS_RECON:
		case NEO_CLASS_ASSAULT:
		case NEO_CLASS_SUPPORT:
			if (IsClassFull(neoClass))
			{
				for (int c = NEO_CLASS_RECON; c <= NEO_CLASS_SUPPORT; c++)
				{
					if (c == neoClass)
					{
						continue; // Skip the class that was full/banned
					}

					if (!IsClassFull(c))
					{
						return c;
					}
				}

				// All classes full. Do not switch classes
				return -1;
			}
	}

	return neoClass;
}

void CTeam::UpdateClassCounts()
{
	const int iNumPlayers = GetNumPlayers();
	int iNumRecon = 0;
	int iNumAssault = 0;
	int iNumSupport = 0;
	for (int i = 0; i < iNumPlayers; i++)
	{
		if (auto pNeoPlayer = static_cast<CNEO_Player*>(GetPlayer(i));
			pNeoPlayer)
		{
			switch (pNeoPlayer->GetClass())
			{
			case NEO_CLASS_RECON:
				iNumRecon++;
				break;
			case NEO_CLASS_ASSAULT:
				iNumAssault++;
				break;
			case NEO_CLASS_SUPPORT:
				iNumSupport++;
				break;
			}
		}
	}
	m_iReconCount = iNumRecon;
	m_iAssaultCount = iNumAssault;
	m_iSupportCount = iNumSupport;
}
#endif // NEO
