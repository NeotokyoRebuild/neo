#include "cbase.h"
#include "bot/neo_bot_ctg_carry_block.h"
#include "bot/neo_bot.h"
#include "bot/neo_bot_route_predict.h"
#include "nav_mesh.h"
#include "neo_gamerules.h"
#include "neo_player.h"
#include "tier0/fasttimer.h"

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

	struct CarryBlockState_t
	{
		bool bBlocked;
		int iCarrier;
		int iBlocker;
		float flNextEval;
		float flBlockedSince;
	};

	static CarryBlockState_t s_state[TEAM__TOTAL];

	struct CarryBlockEnemy_t
	{
		CNEO_Player *pEnemy;
		CNavArea *pArea;
		float flDistSq;
		float flSinceKnown;
	};

	static CarryBlockState_t *GetState( int iTeam )
	{
		if ( ( iTeam < 0 ) || ( iTeam >= TEAM__TOTAL ) )
		{
			return nullptr;
		}

		return &s_state[iTeam];
	}

	static void ClearState( CarryBlockState_t &state, int iTeam, const char *pszReason, int nMicroseconds )
	{
		if ( state.bBlocked )
		{
			// NEO-CARRY-TEMP
			Msg( "NEO_CARRY clear team=%d carrier=%d reason=%s held=%.1f us=%d\n", iTeam, state.iCarrier, pszReason,
				gpGlobals->curtime - state.flBlockedSince, nMicroseconds );
		}

		state.bBlocked = false;
		state.iBlocker = 0;
	}

	static int GetElapsedMicroseconds( CFastTimer &timer )
	{
		timer.End();
		return (int)timer.GetDuration().GetMicroseconds();
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
		CUtlVector< CKnownEntity > knownVector;
		pCarrier->GetVisionInterface()->CollectKnownEntities( &knownVector );

		for ( int i = 0; i < knownVector.Count(); ++i )
		{
			const CKnownEntity &known = knownVector[i];
			CNEO_Player *pEnemy = ToNEOPlayer( known.GetEntity() );
			if ( !pEnemy || !pEnemy->IsAlive() || pCarrier->InSameTeam( pEnemy ) )
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
			enemy.pEnemy = pEnemy;
			enemy.pArea = pArea;
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

	// How much later than the carrier this enemy can stand on the carrier's route: the lowest
	// (enemy travel - carrier travel) over the route areas its flood reaches, past the carrier's own area.
	// FLT_MAX when the flood reaches none of them.
	static float ComputeSlack( CNEOBot *pCarrier, CNavArea *pEnemyArea, const CNEOBotPredictedRoute &route )
	{
		// An area left beyond the limit keeps a cost above it, so a slack within the band is still exact
		CNEOBotTraversableFlood flood( pCarrier );
		SearchSurroundingAreas( pEnemyArea, flood, route.travel.Tail() + CARRY_BLOCK_SLACK_BAND );

		// The marks are read now, before the next search reuses the areas' search state
		float flSlack = FLT_MAX;
		for ( int i = 1; i < route.areas.Count(); ++i )
		{
			const CNavArea *pArea = route.areas[i];
			if ( pArea->IsMarked() )
			{
				flSlack = Min( flSlack, pArea->GetCostSoFar() - route.travel[i] );
			}
		}

		return flSlack;
	}

	static float GetClockReserve( int iTeam )
	{
		return IsAtkAttackingTeam( iTeam ) ? CARRY_BLOCK_CLOCK_RESERVE_ATK : CARRY_BLOCK_CLOCK_RESERVE_CTG;
	}

	// Why a block ended when no counted enemy holds it any more
	static const char *GetReleaseReason( const CarryBlockState_t &state, const CUtlVector< CarryBlockEnemy_t > &enemies )
	{
		CBasePlayer *pBlocker = UTIL_PlayerByIndex( state.iBlocker );
		if ( !pBlocker || !pBlocker->IsAlive() )
		{
			return "dead";
		}

		for ( int i = 0; i < enemies.Count(); ++i )
		{
			if ( enemies[i].pEnemy == pBlocker )
			{
				return "route";
			}
		}

		return "lost";
	}

	// The verdict itself: sets the state and writes the log
	static void EvaluateCarryBlock( CNEOBot *pCarrier, bool bCapInReach, CarryBlockState_t &state, CFastTimer &timer )
	{
		const int iTeam = pCarrier->GetTeamNumber();

		// A lone carrier has no one to hunt for it
		if ( !HasLivingTeammate( pCarrier ) )
		{
			ClearState( state, iTeam, "alone", GetElapsedMicroseconds( timer ) );
			return;
		}

		if ( bCapInReach )
		{
			ClearState( state, iTeam, "cap", GetElapsedMicroseconds( timer ) );
			return;
		}

		const Vector vecGoal = NEORules()->GetNearestGhostCapPoint( iTeam, pCarrier->GetAbsOrigin() );
		if ( vecGoal == CNEO_Player::VECTOR_INVALID_WAYPOINT )
		{
			ClearState( state, iTeam, "route", GetElapsedMicroseconds( timer ) );
			return;
		}

		CNEOBotPredictedRoute route;
		if ( !PredictRoute( pCarrier->GetLastKnownArea(), vecGoal, route ) )
		{
			ClearState( state, iTeam, "route", GetElapsedMicroseconds( timer ) );
			return;
		}

		// Already in the zone's area
		if ( route.areas.Count() < 2 )
		{
			ClearState( state, iTeam, "cap", GetElapsedMicroseconds( timer ) );
			return;
		}

		// Holding is only worth it while the round has time for the run once the block lifts
		const float flRunSpeed = Max( 1.0f, pCarrier->GetNormSpeed_WithActiveWepEncumberment() );
		const float flRunTime = route.travel.Tail() / flRunSpeed;
		if ( NEORules()->GetRoundRemainingTime() <= GetClockReserve( iTeam ) + flRunTime )
		{
			ClearState( state, iTeam, "clock", GetElapsedMicroseconds( timer ) );
			return;
		}

		// Entry and exit are one comparison: a held block gets the band and longer-lived knowledge
		const float flFreshness = state.bBlocked ? CARRY_BLOCK_STAY_FRESHNESS : CARRY_BLOCK_ENTRY_FRESHNESS;
		const float flMaxSlack = state.bBlocked ? CARRY_BLOCK_SLACK_BAND : 0.0f;

		CUtlVector< CarryBlockEnemy_t > enemies;
		CollectCountedEnemies( pCarrier, flFreshness, enemies );

		int iBest = -1;
		float flBestSlack = FLT_MAX;
		for ( int i = 0; i < enemies.Count(); ++i )
		{
			const float flSlack = ComputeSlack( pCarrier, enemies[i].pArea, route );
			if ( flSlack < flBestSlack )
			{
				flBestSlack = flSlack;
				iBest = i;
			}
		}

		const int nMicroseconds = GetElapsedMicroseconds( timer );

		if ( ( iBest < 0 ) || ( flBestSlack > flMaxSlack ) )
		{
			if ( state.bBlocked )
			{
				ClearState( state, iTeam, GetReleaseReason( state, enemies ), nMicroseconds );
			}

			return;
		}

		const int iBlocker = enemies[iBest].pEnemy->entindex();
		if ( state.bBlocked && ( state.iBlocker == iBlocker ) )
		{
			return;
		}

		if ( !state.bBlocked )
		{
			state.flBlockedSince = gpGlobals->curtime;
		}

		state.bBlocked = true;
		state.iBlocker = iBlocker;

		// NEO-CARRY-TEMP: a new block, or another enemy now holds it
		Msg( "NEO_CARRY block team=%d carrier=%d enemy=%d slack=%.0f route=%.0f known=%.1f detour=0 us=%d\n",
			iTeam, state.iCarrier, iBlocker, flBestSlack, route.travel.Tail(), enemies[iBest].flSinceKnown, nMicroseconds );
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
			state.iCarrier = pCarrier->entindex();
			state.flNextEval = 0.0f;
		}

		if ( gpGlobals->curtime < state.flNextEval )
		{
			return;
		}

		state.flNextEval = gpGlobals->curtime + CARRY_BLOCK_EVAL_INTERVAL;

		CFastTimer timer;
		timer.Start();
		EvaluateCarryBlock( pCarrier, bCapInReach, state, timer );
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
				state.iCarrier = 0;
				continue;
			}

			CBasePlayer *pCarrier = UTIL_PlayerByIndex( state.iCarrier );
			if ( ( state.iCarrier != iGhoster ) || !pCarrier || !pCarrier->IsAlive() )
			{
				ClearState( state, iTeam, "carrier", 0 );
				state.iCarrier = 0;
			}
		}
	}

	//---------------------------------------------------------------------------------------------
	void Reset()
	{
		for ( int iTeam = 0; iTeam < TEAM__TOTAL; ++iTeam )
		{
			s_state[iTeam] = {};
		}
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
