#include "cbase.h"
#include "neo_bot_manager.h"

#include "Player/NextBotPlayer.h"
#include "team.h"
#include "neo_bot.h"
#include "neo_gamerules.h"


//----------------------------------------------------------------------------------------------------------------

// Creates and sets CNEOBotManager as the NextBotManager singleton
static CNEOBotManager sNEOBotManager;

ConVar neo_bot_difficulty( "neo_bot_difficulty", "2", FCVAR_NONE, "Defines the skill of bots joining the game.  Values are: 0=easy, 1=normal, 2=hard, 3=expert.", true, 0, true, CNEOBot::DifficultyType::NUM_DIFFICULTY_LEVELS - 1);
ConVar neo_bot_quota( "neo_bot_quota", "10", FCVAR_NONE, "Determines the total number of bots in the game." );
ConVar neo_bot_quota_mode( "neo_bot_quota_mode", "fill", FCVAR_NONE, "Determines the type of quota.\nAllowed values: 'normal', 'fill', and 'match'.\nIf 'fill', the server will adjust bots to keep N players in the game, where N is bot_quota.\nIf 'match', the server will maintain a 1:N ratio of humans to bots, where N is bot_quota." );
ConVar neo_bot_join_after_player( "neo_bot_join_after_player", "1", FCVAR_NONE, "If nonzero, bots wait until a player joins before entering the game." );
ConVar neo_bot_auto_vacate( "neo_bot_auto_vacate", "1", FCVAR_NONE, "If nonzero, bots will automatically leave to make room for human players." );
ConVar neo_bot_offline_practice( "neo_bot_offline_practice", "0", FCVAR_NONE, "Tells the server that it is in offline practice mode." );
ConVar neo_bot_melee_only( "neo_bot_melee_only", "0", FCVAR_GAMEDLL, "If nonzero, NEOBots will only use melee weapons" );

extern const char *GetRandomBotName( void );
extern void CreateBotName( CNEOBot::DifficultyType skill, char* pBuffer, int iBufferSize );

static bool UTIL_KickBotFromTeam( int kickTeam )
{
	int i;

	// try to kick a dead bot first
	for ( i = 1; i <= gpGlobals->maxClients; ++i )
	{
		CNEO_Player *pPlayer = ToNEOPlayer( UTIL_PlayerByIndex( i ) );
		CNEOBot* pBot = ToNEOBot(pPlayer);

		if (pBot == NULL)
			continue;

		if ( pBot->HasAttribute( CNEOBot::QUOTA_MANANGED ) == false )
			continue;

		if ( ( pPlayer->GetFlags() & FL_FAKECLIENT ) == 0 )
			continue;

		if ( !pPlayer->IsAlive() && pPlayer->GetTeamNumber() == kickTeam )
		{
			// its a bot on the right team - kick it
			engine->ServerCommand( UTIL_VarArgs( "kickid %d\n", pPlayer->GetUserID() ) );

			return true;
		}
	}

	// no dead bots, kick any bot on the given team
	for ( i = 1; i <= gpGlobals->maxClients; ++i )
	{
		CNEO_Player *pPlayer = ToNEOPlayer( UTIL_PlayerByIndex( i ) );
		CNEOBot* pBot = ToNEOBot(pPlayer);

		if (pBot == NULL)
			continue;

		if ( pBot->HasAttribute( CNEOBot::QUOTA_MANANGED ) == false )
			continue;

		if ( ( pPlayer->GetFlags() & FL_FAKECLIENT ) == 0 )
			continue;

		if (pPlayer->GetTeamNumber() == kickTeam)
		{
			// its a bot on the right team - kick it
			engine->ServerCommand( UTIL_VarArgs( "kickid %d\n", pPlayer->GetUserID() ) );

			return true;
		}
	}

	return false;
}

//----------------------------------------------------------------------------------------------------------------

CNEOBotManager::CNEOBotManager()
	: NextBotManager()
	, m_flNextPeriodicThink( 0 )
{
	NextBotManager::SetInstance( this );
}


//----------------------------------------------------------------------------------------------------------------
CNEOBotManager::~CNEOBotManager()
{
	NextBotManager::SetInstance( NULL );
}


//----------------------------------------------------------------------------------------------------------------
void CNEOBotManager::OnMapLoaded( void )
{
	NextBotManager::OnMapLoaded();

	m_flNextPeriodicThink = 0.f;

	ClearStuckBotData();
}


//----------------------------------------------------------------------------------------------------------------
void CNEOBotManager::Update()
{
	MaintainBotQuota();

	DrawStuckBotData();

	NextBotManager::Update();
}


//----------------------------------------------------------------------------------------------------------------
bool CNEOBotManager::RemoveBotFromTeamAndKick( int nTeam )
{
	CUtlVector< CNEO_Player* > vecCandidates;

	// Gather potential candidates
	for ( int i = 1; i <= gpGlobals->maxClients; ++i )
	{
		CNEO_Player *pPlayer = ToNEOPlayer( UTIL_PlayerByIndex( i ) );

		if ( pPlayer == NULL )
			continue;

		if ( FNullEnt( pPlayer->edict() ) )
			continue;

		if ( !pPlayer->IsConnected() )
			continue;

		CNEOBot* pBot = ToNEOBot( pPlayer );
		if ( pBot && pBot->HasAttribute( CNEOBot::QUOTA_MANANGED ) )
		{
			if ( pBot->GetTeamNumber() == nTeam )
			{
				vecCandidates.AddToTail( pPlayer );
			}
		}
	}
	
	CNEO_Player *pVictim = NULL;
	if ( vecCandidates.Count() > 0 )
	{
		// first look for bots that are currently dead
		FOR_EACH_VEC( vecCandidates, i )
		{
			CNEO_Player *pPlayer = vecCandidates[i];
			if ( pPlayer && !pPlayer->IsAlive() )
			{
				pVictim = pPlayer;
				break;
			}
		}

		// if we didn't fine one, try to kick anyone on the team
		if ( !pVictim )
		{
			FOR_EACH_VEC( vecCandidates, i )
			{
				CNEO_Player *pPlayer = vecCandidates[i];
				if ( pPlayer )
				{
					pVictim = pPlayer;
					break;
				}
			}
		}
	}

	if ( pVictim )
	{
		if ( pVictim->IsAlive() )
		{
			pVictim->CommitSuicide();
		}
		UTIL_KickBotFromTeam( TEAM_UNASSIGNED );
		return true;
	}

	return false;
}

//----------------------------------------------------------------------------------------------------------------
extern ConVar neo_bot_recon_ratio;
extern ConVar neo_bot_support_ratio;
void CNEOBotManager::MaintainBotQuota()
{
	if ( TheNavMesh->IsGenerating() )
		return;

	if ( g_fGameOver )
		return;

	// new players can't spawn immediately after the round has been going for some time
	if ( !NEORules() )
		return;

	NeoGameType gameType = (NeoGameType)NEORules()->GetGameType();
	if (gameType == NEO_GAME_TYPE_EMT || gameType == NEO_GAME_TYPE_TUT)
	{
		return;
	}

	// if it is not time to do anything...
	if ( gpGlobals->curtime < m_flNextPeriodicThink )
		return;

	// think every quarter second
	m_flNextPeriodicThink = gpGlobals->curtime + 0.25f;

	// don't add bots until local player has been registered, to make sure he's player ID #1
	if ( !engine->IsDedicatedServer() )
	{
		CBasePlayer *pPlayer = UTIL_GetListenServerHost();
		if ( !pPlayer )
			return;
	}

	// We want to balance based on who's playing on game teams not necessary who's on team spectator, etc.
	int nConnectedClients = 0;
	int nNEOBots = 0;
	int nNEOBotsOnGameTeams = 0;
	int nNonNEOBotsOnGameTeams = 0;
	int nSpectators = 0;
	for ( int i = 1; i <= gpGlobals->maxClients; ++i )
	{
		CNEO_Player *pPlayer = ToNEOPlayer( UTIL_PlayerByIndex( i ) );

		if ( pPlayer == NULL )
			continue;

		if ( FNullEnt( pPlayer->edict() ) )
			continue;

		if ( !pPlayer->IsConnected() )
			continue;

		CNEOBot* pBot = ToNEOBot( pPlayer );
		if ( pBot && pBot->HasAttribute( CNEOBot::QUOTA_MANANGED ) )
		{
			nNEOBots++;
			if ( pPlayer->GetTeamNumber() == TEAM_JINRAI || pPlayer->GetTeamNumber() == TEAM_NSF )
			{
				nNEOBotsOnGameTeams++;
			}
		}
		else
		{
			if ( pPlayer->GetTeamNumber() == TEAM_JINRAI || pPlayer->GetTeamNumber() == TEAM_NSF)
			{
				nNonNEOBotsOnGameTeams++;
			}
			else if ( pPlayer->GetTeamNumber() == TEAM_SPECTATOR )
			{
				nSpectators++;
			}
		}

		nConnectedClients++;
	}

	int desiredBotCount = neo_bot_quota.GetInt();
	int nTotalNonNEOBots = nConnectedClients - nNEOBots;

	if ( FStrEq( neo_bot_quota_mode.GetString(), "fill" ) )
	{
		desiredBotCount = Max( 0, desiredBotCount - nNonNEOBotsOnGameTeams );
	}
	else if ( FStrEq( neo_bot_quota_mode.GetString(), "match" ) )
	{
		// If bot_quota_mode is 'match', we want the number of bots to be bot_quota * total humans
		desiredBotCount = Max( 0, (int)neo_bot_quota.GetFloat() * nNonNEOBotsOnGameTeams );
	}

	// wait for a player to join, if necessary
	if ( neo_bot_join_after_player.GetBool() )
	{
		if ( ( nNonNEOBotsOnGameTeams == 0 ) && ( nSpectators == 0 ) )
		{
			desiredBotCount = 0;
		}
	}

	// if bots will auto-vacate, we need to keep one slot open to allow players to join
	if ( neo_bot_auto_vacate.GetBool() )
	{
		desiredBotCount = Min( desiredBotCount, gpGlobals->maxClients - nTotalNonNEOBots - 1 );
	}
	else
	{
		desiredBotCount = Min( desiredBotCount, gpGlobals->maxClients - nTotalNonNEOBots );
	}

	// add bots if necessary
	if ( desiredBotCount > nNEOBotsOnGameTeams )
	{
		CNEOBot::DifficultyType skill = Clamp((CNEOBot::DifficultyType)neo_bot_difficulty.GetInt(), CNEOBot::EASY, CNEOBot::EXPERT);
		char name[MAX_PLAYER_NAME_LENGTH];
		CreateBotName(skill, name, sizeof(name));

		CNEOBot *pBot = GetAvailableBotFromPool();
		if ( pBot == NULL )
		{
			pBot = NextBotCreatePlayerBot< CNEOBot >(name);
		}
		if ( pBot )
		{
			pBot->SetAttribute( CNEOBot::QUOTA_MANANGED );

			int iTeam = TEAM_UNASSIGNED;

			if ( NEORules()->IsTeamplay() && iTeam == TEAM_UNASSIGNED )
			{
				CTeam* pJinrai = GetGlobalTeam(TEAM_JINRAI);
				CTeam* pNSF = GetGlobalTeam(TEAM_NSF);
				const int numJinrai = pJinrai->GetNumPlayers();
				const int numNSF = pNSF->GetNumPlayers();

				iTeam = numJinrai < numNSF ? TEAM_JINRAI : numNSF < numJinrai ? TEAM_NSF : RandomInt(TEAM_JINRAI, TEAM_NSF);
			}

			float flDice = RandomFloat();
			if (flDice <= neo_bot_recon_ratio.GetFloat())
			{
				pBot->RequestSetClass(NEO_CLASS_RECON);
			}
			else if (flDice >= (1.0f - neo_bot_support_ratio.GetFloat()))
			{
				pBot->RequestSetClass(NEO_CLASS_SUPPORT);
			}
			else
			{
				pBot->RequestSetClass(NEO_CLASS_ASSAULT);
			}

			engine->SetFakeClientConVarValue( pBot->edict(), "name", name );
			pBot->RequestSetSkin(RandomInt(0, 2));
			pBot->HandleCommand_JoinTeam( iTeam );
		}
	}
	else if ( desiredBotCount < nNEOBotsOnGameTeams )
	{
		// kick a bot to maintain quota
		
		// first remove any unassigned bots
		if ( UTIL_KickBotFromTeam( TEAM_UNASSIGNED ) )
			return;

		int kickTeam;

		CTeam *pJinrai = GetGlobalTeam(TEAM_JINRAI);
		CTeam *pNSF = GetGlobalTeam(TEAM_NSF);

		// remove from the team that has more players
		if (pJinrai->GetNumPlayers() > pNSF->GetNumPlayers() )
		{
			kickTeam = TEAM_JINRAI;
		}
		else if (pJinrai->GetNumPlayers() < pNSF->GetNumPlayers() )
		{
			kickTeam = TEAM_NSF;
		}
		// remove from the team that's winning
		else if (pJinrai->GetScore() > pNSF->GetScore() )
		{
			kickTeam = TEAM_JINRAI;
		}
		else if (pJinrai->GetScore() < pNSF->GetScore() )
		{
			kickTeam = TEAM_NSF;
		}
		else
		{
			// teams and scores are equal, pick a team at random
			kickTeam = RandomInt(TEAM_JINRAI, TEAM_NSF);
		}

		// attempt to kick a bot from the given team
		if ( UTIL_KickBotFromTeam( kickTeam ) )
			return;

		// if there were no bots on the team, kick a bot from the other team
		UTIL_KickBotFromTeam(NEORules()->GetOpposingTeam(kickTeam));
	}
}


//----------------------------------------------------------------------------------------------------------------
bool CNEOBotManager::IsAllBotTeam( int iTeam )
{
	CTeam *pTeam = GetGlobalTeam( iTeam );
	if ( pTeam == NULL )
	{
		return false;
	}

	// check to see if any players on the team are humans
	for ( int i = 0, n = pTeam->GetNumPlayers(); i < n; ++i )
	{
		CNEO_Player *pPlayer = ToNEOPlayer( pTeam->GetPlayer( i ) );
		if ( pPlayer == NULL )
		{
			continue;
		}
		if ( pPlayer->IsBot() == false )
		{
			return false;
		}
	}

	// if we made it this far, then they must all be bots!
	if ( pTeam->GetNumPlayers() != 0 )
	{
		return true;
	}

	return true;
}


//----------------------------------------------------------------------------------------------------------------
void CNEOBotManager::SetIsInOfflinePractice(bool bIsInOfflinePractice)
{
	neo_bot_offline_practice.SetValue( bIsInOfflinePractice ? 1 : 0 );
}


//----------------------------------------------------------------------------------------------------------------
bool CNEOBotManager::IsInOfflinePractice() const
{
	return neo_bot_offline_practice.GetBool();
}


//----------------------------------------------------------------------------------------------------------------
bool CNEOBotManager::IsMeleeOnly() const
{
	return neo_bot_melee_only.GetBool();
}

//----------------------------------------------------------------------------------------------------------------
void CNEOBotManager::RevertOfflinePracticeConvars()
{
	neo_bot_quota.Revert();
	neo_bot_quota_mode.Revert();
	neo_bot_auto_vacate.Revert();
	neo_bot_difficulty.Revert();
	neo_bot_offline_practice.Revert();
}


//----------------------------------------------------------------------------------------------------------------
void CNEOBotManager::LevelShutdown()
{
	m_flNextPeriodicThink = 0.0f;
	if ( IsInOfflinePractice() )
	{
		RevertOfflinePracticeConvars();
		SetIsInOfflinePractice( false );
	}		
}


//----------------------------------------------------------------------------------------------------------------
CNEOBot* CNEOBotManager::GetAvailableBotFromPool()
{
	for ( int i = 1; i <= gpGlobals->maxClients; ++i )
	{
		CNEOBot* pBot = ToNEOBot(UTIL_PlayerByIndex(i));

		if (pBot == NULL)
			continue;

		if ( ( pBot->GetFlags() & FL_FAKECLIENT ) == 0 )
			continue;

		if ( pBot->GetTeamNumber() == TEAM_SPECTATOR || pBot->GetTeamNumber() == TEAM_UNASSIGNED )
		{
			pBot->ClearAttribute( CNEOBot::QUOTA_MANANGED );
			return pBot;
		}
	}
	return NULL;
}


//----------------------------------------------------------------------------------------------------------------
void CNEOBotManager::OnForceAddedBots( int iNumAdded )
{
	neo_bot_quota.SetValue( neo_bot_quota.GetInt() + iNumAdded );
	m_flNextPeriodicThink = gpGlobals->curtime + 1.0f;
}


//----------------------------------------------------------------------------------------------------------------
void CNEOBotManager::OnForceKickedBots( int iNumKicked )
{
	neo_bot_quota.SetValue( MAX( neo_bot_quota.GetInt() - iNumKicked, 0 ) );
	// allow time for the bots to be kicked
	m_flNextPeriodicThink = gpGlobals->curtime + 2.0f;
}


//----------------------------------------------------------------------------------------------------------------
CNEOBotManager &TheNEOBots( void )
{
	return static_cast<CNEOBotManager&>( TheNextBots() );
}



//----------------------------------------------------------------------------------------------------------------
//----------------------------------------------------------------------------------------------------------------
CON_COMMAND_F( neo_bot_debug_stuck_log, "Given a server logfile, visually display bot stuck locations.", FCVAR_GAMEDLL | FCVAR_CHEAT )
{
	// Listenserver host or rcon access only!
	if ( !UTIL_IsCommandIssuedByServerAdmin() )
		return;

	if ( args.ArgC() < 2 )
	{
		DevMsg( "%s <logfilename>\n", args.Arg(0) );
		return;
	}

	FileHandle_t file = filesystem->Open( args.Arg(1), "r", "GAME" );

	const int maxBufferSize = 1024;
	char buffer[ maxBufferSize ];

	char logMapName[ maxBufferSize ];
	logMapName[0] = '\000';

	TheNEOBots().ClearStuckBotData();

	if ( file )
	{
		int line = 0;
		while( !filesystem->EndOfFile( file ) )
		{
			filesystem->ReadLine( buffer, maxBufferSize, file );
			++line;

			strtok( buffer, ":" );
			strtok( NULL, ":" );
			strtok( NULL, ":" );
			char *first = strtok( NULL, " " );

			if ( !first )
				continue;

			if ( !strcmp( first, "Loading" ) )
			{
				// L 08/08/2012 - 15:10:47: Loading map "mvm_coaltown"
				strtok( NULL, " " );
				char *mapname = strtok( NULL, "\"" );

				if ( mapname )
				{
					strcpy( logMapName, mapname );
					Warning( "*** Log file from map '%s'\n", mapname );
				}
			}
			else if ( first[0] == '\"' )
			{
				// might be a player ID

				char *playerClassname = &first[1];

				char *nameEnd = playerClassname;
				while( *nameEnd != '\000' && *nameEnd != '<' )
					++nameEnd;
				*nameEnd = '\000';

				char *botIDString = ++nameEnd;
				char *IDEnd = botIDString;
				while( *IDEnd != '\000' && *IDEnd != '>' )
					++IDEnd;
				*IDEnd = '\000';

				int botID = atoi( botIDString );

				char *second = strtok( NULL, " " );
				if ( second && !strcmp( second, "stuck" ) )
				{
					CStuckBot *stuckBot = TheNEOBots().FindOrCreateStuckBot( botID, playerClassname );

					CStuckBotEvent *stuckEvent = new CStuckBotEvent;


					// L 08/08/2012 - 15:15:05: "Scout<53><BOT><Blue>" stuck (position "-180.61 2471.29 216.04") (duration "2.52") L 08/08/2012 - 15:15:05:    path_goal ( "-180.61 2471.29 216.04" )
					strtok( NULL, " (\"" );	// (position

					stuckEvent->m_stuckSpot.x = (float)atof( strtok( NULL, " )\"" ) );
					stuckEvent->m_stuckSpot.y = (float)atof( strtok( NULL, " )\"" ) );
					stuckEvent->m_stuckSpot.z = (float)atof( strtok( NULL, " )\"" ) );

					strtok( NULL, ") (\"" );
					stuckEvent->m_stuckDuration = (float)atof( strtok( NULL, "\"" ) );

					strtok( NULL, ") (\"-L0123456789/:" );	// path_goal

					char *goal = strtok( NULL, ") (\"" );

					if ( goal && strcmp( goal, "NULL" ) )
					{
						stuckEvent->m_isGoalValid = true;

						stuckEvent->m_goalSpot.x = (float)atof( goal );
						stuckEvent->m_goalSpot.y = (float)atof( strtok( NULL, ") (\"" ) );
						stuckEvent->m_goalSpot.z = (float)atof( strtok( NULL, ") (\"" ) );
					}
					else
					{
						stuckEvent->m_isGoalValid = false;
					}

					stuckBot->m_stuckEventVector.AddToTail( stuckEvent );
				}
			}
		}

		filesystem->Close( file );
	}
	else
	{
		Warning( "Can't open file '%s'\n", args.Arg(1) );
	}

	//TheNEOBots().DrawStuckBotData();
}


//----------------------------------------------------------------------------------------------------------------
//----------------------------------------------------------------------------------------------------------------
CON_COMMAND_F( neo_bot_debug_stuck_log_clear, "Clear currently loaded bot stuck data", FCVAR_GAMEDLL | FCVAR_CHEAT )
{
	// Listenserver host or rcon access only!
	if ( !UTIL_IsCommandIssuedByServerAdmin() )
		return;

	TheNEOBots().ClearStuckBotData();
}


//----------------------------------------------------------------------------------------------------------------
// for parsing and debugging stuck bot server logs
void CNEOBotManager::ClearStuckBotData()
{
	m_stuckBotVector.PurgeAndDeleteElements();
}


//----------------------------------------------------------------------------------------------------------------
// for parsing and debugging stuck bot server logs
CStuckBot *CNEOBotManager::FindOrCreateStuckBot( int id, const char *playerClass )
{
	for( int i=0; i<m_stuckBotVector.Count(); ++i )
	{
		CStuckBot *stuckBot = m_stuckBotVector[i];

		if ( stuckBot->IsMatch( id, playerClass ) )
		{
			return stuckBot;
		}
	}

	// new instance of a stuck bot
	CStuckBot *newStuckBot = new CStuckBot( id, playerClass );
	m_stuckBotVector.AddToHead( newStuckBot );

	return newStuckBot;
}


//----------------------------------------------------------------------------------------------------------------
void CNEOBotManager::DrawStuckBotData( float deltaT )
{
	if ( engine->IsDedicatedServer() )
		return;

	if ( !m_stuckDisplayTimer.IsElapsed() )
		return;

	m_stuckDisplayTimer.Start( deltaT );

	CBasePlayer *player = UTIL_GetListenServerHost();
	if ( player == NULL )
		return;

// 	Vector forward;
// 	AngleVectors( player->EyeAngles(), &forward );

	for( int i=0; i<m_stuckBotVector.Count(); ++i )
	{
		for( int j=0; j<m_stuckBotVector[i]->m_stuckEventVector.Count(); ++j )
		{
			m_stuckBotVector[i]->m_stuckEventVector[j]->Draw( deltaT );
		}

		for( int j=0; j<m_stuckBotVector[i]->m_stuckEventVector.Count()-1; ++j )
		{
			NDebugOverlay::HorzArrow( m_stuckBotVector[i]->m_stuckEventVector[j]->m_stuckSpot, 
									  m_stuckBotVector[i]->m_stuckEventVector[j+1]->m_stuckSpot,
									  3, 100, 0, 255, 255, true, deltaT );
		}

		NDebugOverlay::Text( m_stuckBotVector[i]->m_stuckEventVector[0]->m_stuckSpot, CFmtStr( "%s(#%d)", m_stuckBotVector[i]->m_name, m_stuckBotVector[i]->m_id ), false, deltaT );
	}
}


