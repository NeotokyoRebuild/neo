#include "cbase.h"
#include "bot/neo_bot_ctg_carry_block.h"
#include "bot/neo_bot.h"
#include "bot/neo_bot_route_predict.h"
#include "GameEventListener.h"
#include "nav_mesh.h"
#include "neo_gamerules.h"
#include "neo_player.h"
#include "team.h"
#include "tier0/fasttimer.h"
#include "utlhashtable.h"

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

namespace NEOBotCarryBlock
{
	// The verdict costs a path search and a few floods, so the carrier re-evaluates it at this interval
	static constexpr float CARRY_BLOCK_EVAL_INTERVAL = 1.0f;

	// Once blocked, an enemy keeps the block while it is within this much travel of reaching the route
	// in time, about 1.5 s of running, so the verdict does not flip each evaluation
	static constexpr float CARRY_BLOCK_SLACK_BAND = 384.0f;

	// How recently an enemy must have been known to start a block, and to keep one;
	// the longer one is NextBot's own bound for obsolete knowledge
	static constexpr float CARRY_BLOCK_ENTRY_FRESHNESS = 5.0f;
	static constexpr float CARRY_BLOCK_STAY_FRESHNESS = 10.0f;

	// Only the enemies nearest the carrier are flooded, which bounds the cost of one evaluation
	static constexpr int CARRY_BLOCK_MAX_ENEMIES = 3;

	// Round time kept in hand beyond the carrier's run to the zone:
	// ATK attackers lose when time runs out, while a CTG round that runs out is a tie
	static constexpr float CARRY_BLOCK_CLOCK_RESERVE_ATK = 30.0f;
	static constexpr float CARRY_BLOCK_CLOCK_RESERVE_CTG = 15.0f;

	// Only the next stretch of the route is judged, about 8 s of running:
	// a prediction further out is not worth trusting, and the team re-evaluates on the way
	static constexpr float CARRY_BLOCK_HORIZON = 2000.0f;

	// Bounds on one enemy's flood, so a long route on a big map stays cheap;
	// an enemy not proven an interceptor within them counts as not intercepting
	static constexpr float CARRY_BLOCK_MAX_FLOOD_RANGE = 2400.0f;
	static constexpr int CARRY_BLOCK_MAX_FLOOD_AREAS = 1500;

	// A numbers lead this large sends the carrier for the zone whatever the enemies do
	static constexpr int CARRY_BLOCK_BREAK_LEAD = 2;

	// After a break the carrier commits to the run this long before it may hold again
	static constexpr float CARRY_BLOCK_BREAK_COMMIT = 4.0f;

	// The predicted route is reused while the carrier is still on its first few areas and it is this fresh
	static constexpr int CARRY_BLOCK_ROUTE_REUSE_AREAS = 4;
	static constexpr float CARRY_BLOCK_ROUTE_MAX_AGE = 5.0f;

	// Below these moves, and with no death in the kill feed, the last interceptor count still holds
	static constexpr float CARRY_BLOCK_CARRIER_MOVE_REUSE = 64.0f;
	static constexpr float CARRY_BLOCK_ENEMY_MOVE_REUSE = 128.0f;

	struct CarryBlockEnemy_t
	{
		int iEnemy;
		CNavArea *pArea;
		Vector vecKnownPos;
		float flDistSq;
		float flSinceKnown;
	};

	struct CarryBlockState_t
	{
		bool bBlocked;
		int iCarrier;
		int iBlocker;
		float flNextEval;
		float flBlockedSince;

		// When the last block of this carry ended, or a negative time when none has
		float flBreakTime;

		// The predicted route to the zone, with each area's route index
		CNEOBotPredictedRoute route;
		CUtlHashtable< unsigned int, int > routeIndex;
		Vector vecRouteGoal;
		float flRouteTime;

		// The evaluation in progress: one enemy is flooded per tick
		bool bEvaluating;
		CUtlVector< CarryBlockEnemy_t > enemies;
		int iCursor;
		int iStart;
		int iHorizon;
		float flMaxSlack;
		int nInterceptors;
		int iFirstInterceptor;
		int nMicroseconds;
		int nMaxTickMicroseconds;
		int nAreas;
		bool bCapped;

		// The last completed evaluation, reused while nothing moved
		bool bHasLast;
		bool bLastBlocked;
		Vector vecLastCarrierPos;
		CUtlVector< CarryBlockEnemy_t > lastEnemies;
		int nLastInterceptors;
		int iLastFirstInterceptor;
		int iLastDeathSerial;
	};

	static CarryBlockState_t s_state[TEAM__TOTAL];

	//---------------------------------------------------------------------------------------------
	// The kill feed: a death notice every client receives. A named enemy is dead until it is seen
	// alive again, so IsAlive is read only for enemies the feed has named.
	static bool s_bNamedDead[MAX_PLAYERS + 1];
	static int s_iDeathSerial = 0;

	class CCarryBlockDeathFeed : public CGameEventListener
	{
	public:
		virtual void FireGameEvent( IGameEvent *event ) override
		{
			CBasePlayer *pVictim = UTIL_PlayerByUserId( event->GetInt( "userid" ) );
			if ( !pVictim )
			{
				return;
			}

			s_bNamedDead[pVictim->entindex()] = true;
			++s_iDeathSerial;
		}
	};

	static CCarryBlockDeathFeed s_deathFeed;

	static bool IsNamedDead( CBasePlayer *pPlayer )
	{
		const int iIndex = pPlayer->entindex();
		if ( !s_bNamedDead[iIndex] )
		{
			return false;
		}

		// A new round respawned it
		if ( pPlayer->IsAlive() )
		{
			s_bNamedDead[iIndex] = false;
			return false;
		}

		return true;
	}

	// The enemy roster is public; the dead among it come from the kill feed only
	static int CountEnemiesAlive( int iEnemyTeam )
	{
		CTeam *pTeam = GetGlobalTeam( iEnemyTeam );
		if ( !pTeam )
		{
			return 0;
		}

		int nAlive = 0;
		for ( int i = 0; i < pTeam->GetNumPlayers(); ++i )
		{
			CBasePlayer *pPlayer = pTeam->GetPlayer( i );
			if ( pPlayer && !IsNamedDead( pPlayer ) )
			{
				++nAlive;
			}
		}

		return nAlive;
	}

	static int CountTeamAlive( int iTeam )
	{
		int nAlive = 0;
		for ( int i = 1; i <= gpGlobals->maxClients; ++i )
		{
			CBasePlayer *pPlayer = UTIL_PlayerByIndex( i );
			if ( pPlayer && pPlayer->IsAlive() && ( pPlayer->GetTeamNumber() == iTeam ) )
			{
				++nAlive;
			}
		}

		return nAlive;
	}

	//---------------------------------------------------------------------------------------------
	static CarryBlockState_t *GetState( int iTeam )
	{
		if ( ( iTeam < 0 ) || ( iTeam >= TEAM__TOTAL ) )
		{
			return nullptr;
		}

		return &s_state[iTeam];
	}

	// Forget everything tied to one carry: the route, the evaluation in progress and the last one
	static void ResetCarry( CarryBlockState_t &state )
	{
		state.flBreakTime = -1.0f;

		state.route.areas.RemoveAll();
		state.route.travel.RemoveAll();
		state.routeIndex.RemoveAll();
		state.flRouteTime = -1.0f;

		state.bEvaluating = false;
		state.enemies.RemoveAll();

		state.bHasLast = false;
		state.lastEnemies.RemoveAll();
		state.iLastDeathSerial = s_iDeathSerial;
	}

	static void ResetState( CarryBlockState_t &state )
	{
		ResetCarry( state );

		state.bBlocked = false;
		state.iCarrier = 0;
		state.iBlocker = 0;
		state.flNextEval = 0.0f;
		state.flBlockedSince = 0.0f;
	}

	static void ClearState( CarryBlockState_t &state, int iTeam, const char *pszReason, int nMicroseconds )
	{
		if ( state.bBlocked )
		{
			// NEO-CARRY-TEMP
			Msg( "NEO_CARRY clear team=%d carrier=%d reason=%s held=%.1f us=%d\n", iTeam, state.iCarrier, pszReason,
				gpGlobals->curtime - state.flBlockedSince, nMicroseconds );

			state.flBreakTime = gpGlobals->curtime;
		}

		state.bBlocked = false;
		state.iBlocker = 0;
	}

	static int GetElapsedMicroseconds( CFastTimer &timer )
	{
		timer.End();
		return (int)timer.GetDuration().GetMicroseconds();
	}

	// NEO-CARRY-TEMP: one line per evaluation, so the cost distribution is complete
	static void LogEval( const CarryBlockState_t &state, int iTeam, const char *pszGate, int nLead, bool bReused )
	{
		Msg( "NEO_CARRY eval team=%d carrier=%d blocked=%d gate=%s n=%d lead=%d enemies=%d us=%d maxtick=%d areas=%d capped=%d reused=%d\n",
			iTeam, state.iCarrier, state.bBlocked ? 1 : 0, pszGate, state.nInterceptors, nLead, state.enemies.Count(),
			state.nMicroseconds, state.nMaxTickMicroseconds, state.nAreas, state.bCapped ? 1 : 0, bReused ? 1 : 0 );
	}

	static void EndGate( CarryBlockState_t &state, int iTeam, const char *pszReason, CFastTimer &timer )
	{
		const int nMicroseconds = GetElapsedMicroseconds( timer );
		state.nMicroseconds = nMicroseconds;
		state.nMaxTickMicroseconds = nMicroseconds;
		ClearState( state, iTeam, pszReason, nMicroseconds );
		LogEval( state, iTeam, pszReason, 0, false );
	}

	static bool HasLivingTeammate( const CNEOBot *pCarrier )
	{
		for ( int i = 1; i <= gpGlobals->maxClients; ++i )
		{
			CBasePlayer *pPlayer = UTIL_PlayerByIndex( i );
			if ( pPlayer && ( pPlayer != pCarrier ) && pPlayer->IsAlive() && pCarrier->InSameTeam( pPlayer ) )
			{
				return true;
			}
		}

		return false;
	}

	// The carrier's own knowledge of enemies, which includes beacon reveals and teammates' callouts,
	// nearest first and cut to the few the evaluation floods
	static void CollectCountedEnemies( CNEOBot *pCarrier, float flFreshness, CUtlVector< CarryBlockEnemy_t > &enemies )
	{
		enemies.RemoveAll();

		CUtlVector< CKnownEntity > knownVector;
		pCarrier->GetVisionInterface()->CollectKnownEntities( &knownVector );

		for ( int i = 0; i < knownVector.Count(); ++i )
		{
			const CKnownEntity &known = knownVector[i];
			CNEO_Player *pEnemy = ToNEOPlayer( known.GetEntity() );
			if ( !pEnemy || pCarrier->InSameTeam( pEnemy ) || IsNamedDead( pEnemy ) )
			{
				continue;
			}

			if ( known.GetTimeSinceLastKnown() > flFreshness )
			{
				continue;
			}

			// The searches only write the areas' search state, so the const area is safe to flood from
			CNavArea *pArea = const_cast< CNavArea * >( known.GetLastKnownArea() );
			if ( !pArea )
			{
				pArea = TheNavMesh->GetNearestNavArea( known.GetLastKnownPosition() );
			}

			if ( !pArea )
			{
				continue;
			}

			CarryBlockEnemy_t &enemy = enemies[enemies.AddToTail()];
			enemy.iEnemy = pEnemy->entindex();
			enemy.pArea = pArea;
			enemy.vecKnownPos = known.GetLastKnownPosition();
			enemy.flDistSq = pCarrier->GetAbsOrigin().DistToSqr( known.GetLastKnownPosition() );
			enemy.flSinceKnown = known.GetTimeSinceLastKnown();
		}

		enemies.SortPredicate( []( const CarryBlockEnemy_t &a, const CarryBlockEnemy_t &b )
		{
			return a.flDistSq < b.flDistSq;
		} );

		if ( enemies.Count() > CARRY_BLOCK_MAX_ENEMIES )
		{
			enemies.RemoveMultipleFromTail( enemies.Count() - CARRY_BLOCK_MAX_ENEMIES );
		}
	}

	static float GetCarrierTravel( const CarryBlockState_t &state, int iRouteIndex )
	{
		return state.route.travel[iRouteIndex] - state.route.travel[state.iStart];
	}

	// The carrier's route to the zone, reused while the carrier is still on its first areas.
	// Sets iStart to the carrier's index on the route.
	static bool UpdateRoute( CNEOBot *pCarrier, const Vector &vecGoal, CarryBlockState_t &state )
	{
		CNavArea *pCarrierArea = pCarrier->GetLastKnownArea();
		if ( !pCarrierArea )
		{
			return false;
		}

		const bool bFresh = ( state.flRouteTime >= 0.0f )
			&& ( gpGlobals->curtime - state.flRouteTime < CARRY_BLOCK_ROUTE_MAX_AGE )
			&& state.vecRouteGoal.DistToSqr( vecGoal ) < 1.0f;
		if ( bFresh )
		{
			const int nReuse = Min( CARRY_BLOCK_ROUTE_REUSE_AREAS, state.route.areas.Count() );
			for ( int i = 0; i < nReuse; ++i )
			{
				if ( state.route.areas[i] == pCarrierArea )
				{
					state.iStart = i;
					return true;
				}
			}
		}

		// A new route makes the last interceptor count meaningless
		state.bHasLast = false;
		state.route.areas.RemoveAll();
		state.route.travel.RemoveAll();
		state.routeIndex.RemoveAll();
		state.flRouteTime = -1.0f;

		if ( !PredictRoute( pCarrierArea, vecGoal, state.route ) )
		{
			return false;
		}

		for ( int i = 0; i < state.route.areas.Count(); ++i )
		{
			state.routeIndex.Insert( state.route.areas[i]->GetID(), i );
		}

		state.vecRouteGoal = vecGoal;
		state.flRouteTime = gpGlobals->curtime;
		state.iStart = 0;
		return true;
	}

	//---------------------------------------------------------------------------------------------
	// One enemy's flood toward the route horizon. It stops on the first horizon route area the enemy
	// reaches within the allowed slack, which proves it an interceptor, or at the area cap.
	class CCarryBlockFlood : public CNEOBotTraversableFlood
	{
	public:
		CCarryBlockFlood( CNEOBot *me, const CarryBlockState_t &state )
			: CNEOBotTraversableFlood( me ), m_state( state ) {}

		virtual bool operator()( CNavArea *area, CNavArea *priorArea, float travelDistanceSoFar ) override
		{
			++m_nAreas;

			const int *pIndex = m_state.routeIndex.GetPtr( area->GetID() );
			if ( pIndex && ( *pIndex > m_state.iStart ) && ( *pIndex <= m_state.iHorizon )
				&& ( travelDistanceSoFar - GetCarrierTravel( m_state, *pIndex ) <= m_state.flMaxSlack ) )
			{
				m_bIntercepts = true;
				return false;
			}

			if ( m_nAreas >= CARRY_BLOCK_MAX_FLOOD_AREAS )
			{
				m_bCapped = true;
				return false;
			}

			return true;
		}

		bool m_bIntercepts = false;
		bool m_bCapped = false;
		int m_nAreas = 0;

	private:
		const CarryBlockState_t &m_state;
	};

	//---------------------------------------------------------------------------------------------
	// The only place the verdict is decided, so entry and exit cannot disagree
	static bool DecideBlocked( bool bBlocked, int nInterceptors, int nLead, float flSinceBreak )
	{
		if ( ( nInterceptors < 1 ) || ( nLead >= CARRY_BLOCK_BREAK_LEAD ) )
		{
			return false;
		}

		if ( bBlocked )
		{
			return true;
		}

		// No break yet in this carry
		if ( flSinceBreak < 0.0f )
		{
			return true;
		}

		// One player of hysteresis against the lead that broke the block, after the run has had its commit
		return ( nLead <= 0 ) && ( flSinceBreak >= CARRY_BLOCK_BREAK_COMMIT );
	}

	static void FinishEvaluation( CarryBlockState_t &state, int iTeam, bool bReused )
	{
		state.bEvaluating = false;

		const int nLead = CountTeamAlive( iTeam ) - CountEnemiesAlive( GetEnemyTeam( iTeam ) );
		const float flSinceBreak = ( state.flBreakTime < 0.0f ) ? -1.0f : gpGlobals->curtime - state.flBreakTime;
		const bool bWasBlocked = state.bBlocked;
		const bool bBlocked = DecideBlocked( bWasBlocked, state.nInterceptors, nLead, flSinceBreak );
		const bool bDeathSinceLast = ( s_iDeathSerial != state.iLastDeathSerial );
		const int iBlocker = ( state.iFirstInterceptor >= 0 ) ? state.iFirstInterceptor : 0;

		if ( bWasBlocked && !bBlocked )
		{
			const char *pszReason = "none";
			if ( nLead >= CARRY_BLOCK_BREAK_LEAD )
			{
				pszReason = "lead";
			}
			else if ( bDeathSinceLast )
			{
				pszReason = "dead";
			}

			ClearState( state, iTeam, pszReason, state.nMicroseconds );
		}
		else if ( !bWasBlocked && bBlocked )
		{
			state.bBlocked = true;
			state.flBlockedSince = gpGlobals->curtime;
			state.iBlocker = iBlocker;

			// NEO-CARRY-TEMP
			Msg( "NEO_CARRY block team=%d carrier=%d enemy=%d n=%d lead=%d route=%.0f us=%d\n",
				iTeam, state.iCarrier, iBlocker, state.nInterceptors, nLead,
				state.route.travel.Tail() - state.route.travel[state.iStart], state.nMicroseconds );
		}
		else if ( bBlocked && ( iBlocker != state.iBlocker ) )
		{
			// NEO-CARRY-TEMP: another enemy now holds the block
			Msg( "NEO_CARRY swap team=%d carrier=%d enemy=%d was=%d n=%d lead=%d\n",
				iTeam, state.iCarrier, iBlocker, state.iBlocker, state.nInterceptors, nLead );

			state.iBlocker = iBlocker;
		}

		LogEval( state, iTeam, "none", nLead, bReused );

		state.iLastDeathSerial = s_iDeathSerial;
		if ( bReused )
		{
			return;
		}

		// The count was taken with the threshold of the state it started in
		state.bHasLast = true;
		state.bLastBlocked = bWasBlocked;
		state.nLastInterceptors = state.nInterceptors;
		state.iLastFirstInterceptor = state.iFirstInterceptor;
		state.lastEnemies.RemoveAll();
		state.lastEnemies.AddVectorToTail( state.enemies );
	}

	// The last count still holds when the carrier, every counted enemy and the kill feed are unchanged
	static bool CanReuseLast( const CNEOBot *pCarrier, const CarryBlockState_t &state )
	{
		if ( !state.bHasLast || ( state.bLastBlocked != state.bBlocked ) || ( state.iLastDeathSerial != s_iDeathSerial ) )
		{
			return false;
		}

		if ( pCarrier->GetAbsOrigin().DistToSqr( state.vecLastCarrierPos ) > Square( CARRY_BLOCK_CARRIER_MOVE_REUSE ) )
		{
			return false;
		}

		if ( state.enemies.Count() != state.lastEnemies.Count() )
		{
			return false;
		}

		for ( int i = 0; i < state.enemies.Count(); ++i )
		{
			const CarryBlockEnemy_t &enemy = state.enemies[i];
			const CarryBlockEnemy_t &last = state.lastEnemies[i];
			if ( ( enemy.iEnemy != last.iEnemy )
				|| ( enemy.vecKnownPos.DistToSqr( last.vecKnownPos ) > Square( CARRY_BLOCK_ENEMY_MOVE_REUSE ) ) )
			{
				return false;
			}
		}

		return true;
	}

	// Floods the next counted enemy, and completes the evaluation after the last one
	static void FloodNextEnemy( CNEOBot *pCarrier, CarryBlockState_t &state )
	{
		CFastTimer timer;
		timer.Start();

		const CarryBlockEnemy_t &enemy = state.enemies[state.iCursor];
		++state.iCursor;

		// The kill feed may have named it since the evaluation started
		CBasePlayer *pEnemy = UTIL_PlayerByIndex( enemy.iEnemy );
		if ( pEnemy && !IsNamedDead( pEnemy ) )
		{
			const float flLimit = Min( GetCarrierTravel( state, state.iHorizon ) + state.flMaxSlack, CARRY_BLOCK_MAX_FLOOD_RANGE );

			CCarryBlockFlood flood( pCarrier, state );
			SearchSurroundingAreas( enemy.pArea, flood, flLimit );

			state.nAreas += flood.m_nAreas;
			state.bCapped |= flood.m_bCapped;
			if ( flood.m_bIntercepts )
			{
				++state.nInterceptors;
				if ( state.iFirstInterceptor < 0 )
				{
					state.iFirstInterceptor = enemy.iEnemy;
				}
			}
		}

		const int nMicroseconds = GetElapsedMicroseconds( timer );
		state.nMicroseconds += nMicroseconds;
		state.nMaxTickMicroseconds = Max( state.nMaxTickMicroseconds, nMicroseconds );

		if ( state.iCursor >= state.enemies.Count() )
		{
			FinishEvaluation( state, pCarrier->GetTeamNumber(), false );
		}
	}

	static float GetClockReserve( int iTeam )
	{
		return IsAtkAttackingTeam( iTeam ) ? CARRY_BLOCK_CLOCK_RESERVE_ATK : CARRY_BLOCK_CLOCK_RESERVE_CTG;
	}

	// Checks the gates, then sets up the floods; the first one runs on this tick
	static void StartEvaluation( CNEOBot *pCarrier, bool bCapInReach, CarryBlockState_t &state )
	{
		CFastTimer timer;
		timer.Start();

		const int iTeam = pCarrier->GetTeamNumber();

		state.nInterceptors = 0;
		state.iFirstInterceptor = -1;
		state.nMicroseconds = 0;
		state.nMaxTickMicroseconds = 0;
		state.nAreas = 0;
		state.bCapped = false;
		state.enemies.RemoveAll();

		// A lone carrier has no one to hunt for it
		if ( !HasLivingTeammate( pCarrier ) )
		{
			EndGate( state, iTeam, "alone", timer );
			return;
		}

		if ( bCapInReach )
		{
			EndGate( state, iTeam, "cap", timer );
			return;
		}

		const Vector vecGoal = NEORules()->GetNearestGhostCapPoint( iTeam, pCarrier->GetAbsOrigin() );
		if ( vecGoal == CNEO_Player::VECTOR_INVALID_WAYPOINT )
		{
			EndGate( state, iTeam, "route", timer );
			return;
		}

		if ( !UpdateRoute( pCarrier, vecGoal, state ) )
		{
			EndGate( state, iTeam, "route", timer );
			return;
		}

		// Already in the zone's area
		if ( state.iStart >= state.route.areas.Count() - 1 )
		{
			EndGate( state, iTeam, "cap", timer );
			return;
		}

		// Holding is only worth it while the round has time for the run once the block lifts
		const float flRunSpeed = Max( 1.0f, pCarrier->GetNormSpeed_WithActiveWepEncumberment() );
		const float flRunTime = GetCarrierTravel( state, state.route.areas.Count() - 1 ) / flRunSpeed;
		if ( NEORules()->GetRoundRemainingTime() <= GetClockReserve( iTeam ) + flRunTime )
		{
			EndGate( state, iTeam, "clock", timer );
			return;
		}

		state.iHorizon = state.iStart + 1;
		while ( ( state.iHorizon + 1 < state.route.areas.Count() )
			&& ( GetCarrierTravel( state, state.iHorizon + 1 ) <= CARRY_BLOCK_HORIZON ) )
		{
			++state.iHorizon;
		}

		// A held block gets the band and longer-lived knowledge
		const float flFreshness = state.bBlocked ? CARRY_BLOCK_STAY_FRESHNESS : CARRY_BLOCK_ENTRY_FRESHNESS;
		state.flMaxSlack = state.bBlocked ? CARRY_BLOCK_SLACK_BAND : 0.0f;

		CollectCountedEnemies( pCarrier, flFreshness, state.enemies );

		// The reference positions move only with a real count, so slow drift cannot reuse one forever
		const bool bReuse = CanReuseLast( pCarrier, state );
		if ( !bReuse )
		{
			state.vecLastCarrierPos = pCarrier->GetAbsOrigin();
		}

		state.nMicroseconds = GetElapsedMicroseconds( timer );
		state.nMaxTickMicroseconds = state.nMicroseconds;

		if ( bReuse )
		{
			state.nInterceptors = state.nLastInterceptors;
			state.iFirstInterceptor = state.iLastFirstInterceptor;
			FinishEvaluation( state, iTeam, true );
			return;
		}

		if ( state.enemies.IsEmpty() )
		{
			FinishEvaluation( state, iTeam, false );
			return;
		}

		state.bEvaluating = true;
		state.iCursor = 0;
		FloodNextEnemy( pCarrier, state );
	}

	//---------------------------------------------------------------------------------------------
	void UpdateCarryBlock( CNEOBot *pCarrier, bool bCapInReach )
	{
		const int iTeam = pCarrier->GetTeamNumber();
		CarryBlockState_t *pState = GetState( iTeam );
		if ( !pState || !HasGhostCapZone( iTeam ) )
		{
			return;
		}

		CarryBlockState_t &state = *pState;
		if ( state.iCarrier != pCarrier->entindex() )
		{
			ClearState( state, iTeam, "carrier", 0 );
			ResetCarry( state );
			state.iCarrier = pCarrier->entindex();
			state.flNextEval = 0.0f;
		}

		// At most one enemy flood per tick
		if ( state.bEvaluating )
		{
			FloodNextEnemy( pCarrier, state );
			return;
		}

		if ( gpGlobals->curtime < state.flNextEval )
		{
			return;
		}

		state.flNextEval = gpGlobals->curtime + CARRY_BLOCK_EVAL_INTERVAL;
		StartEvaluation( pCarrier, bCapInReach, state );
	}

	//---------------------------------------------------------------------------------------------
	void Update()
	{
		const int iGhoster = NEORules()->GhostExists() ? NEORules()->GetGhosterPlayer() : 0;
		const bool bRoundOver = NEORules()->IsRoundOver();
		for ( int iTeam = 0; iTeam < TEAM__TOTAL; ++iTeam )
		{
			CarryBlockState_t &state = s_state[iTeam];
			if ( state.iCarrier == 0 )
			{
				continue;
			}

			if ( bRoundOver )
			{
				ClearState( state, iTeam, "round", 0 );
				ResetCarry( state );
				state.iCarrier = 0;
				continue;
			}

			CBasePlayer *pCarrier = UTIL_PlayerByIndex( state.iCarrier );
			if ( ( state.iCarrier != iGhoster ) || !pCarrier || !pCarrier->IsAlive() )
			{
				ClearState( state, iTeam, "carrier", 0 );
				ResetCarry( state );
				state.iCarrier = 0;
			}
		}
	}

	//---------------------------------------------------------------------------------------------
	void Reset()
	{
		for ( int iTeam = 0; iTeam < TEAM__TOTAL; ++iTeam )
		{
			ResetState( s_state[iTeam] );
		}

		V_memset( s_bNamedDead, 0, sizeof( s_bNamedDead ) );
		s_iDeathSerial = 0;

		// Map load also runs this; re-subscribing keeps one registration
		s_deathFeed.StopListeningForAllEvents();
		s_deathFeed.ListenForGameEvent( "player_death" );
	}

	//---------------------------------------------------------------------------------------------
	bool HasClearPathToGoal( int iTeam )
	{
		if ( !HasGhostCapZone( iTeam ) )
		{
			return false;
		}

		const CarryBlockState_t *pState = GetState( iTeam );
		return !pState || !pState->bBlocked;
	}

	//---------------------------------------------------------------------------------------------
	CNEO_Player *GetCarryBlocker( int iTeam )
	{
		const CarryBlockState_t *pState = GetState( iTeam );
		if ( !pState || !pState->bBlocked )
		{
			return nullptr;
		}

		return ToNEOPlayer( UTIL_PlayerByIndex( pState->iBlocker ) );
	}
}
