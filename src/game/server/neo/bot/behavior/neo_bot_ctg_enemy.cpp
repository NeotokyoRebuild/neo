#include "cbase.h"
#include "neo_player.h"
#include "bot/neo_bot.h"
#include "bot/behavior/neo_bot_ctg_enemy.h"
#include "bot/behavior/neo_bot_ctg_enemy_chase.h"
#include "bot/behavior/neo_bot_ctg_enemy_cutoff.h"
#include "bot/neo_bot_route_predict.h"
#include "neo_gamerules.h"
#include "nav_mesh.h"
#include "nav_pathfind.h"

//---------------------------------------------------------------------------------------------
CNEO_Player *CNEOBotCtgEnemy::EnemyGhostCarrier( const CNEOBot *me )
{
	if ( !NEORules()->GhostExists() )
	{
		return nullptr;
	}

	const int iGhoster = NEORules()->GetGhosterPlayer();
	if ( iGhoster <= 0 || iGhoster > gpGlobals->maxClients )
	{
		return nullptr;
	}

	CNEO_Player *pCarrier = ToNEOPlayer( UTIL_PlayerByIndex( iGhoster ) );
	if ( !pCarrier || !pCarrier->IsAlive() || pCarrier->GetTeamNumber() == me->GetTeamNumber() )
	{
		return nullptr;
	}

	return pCarrier;
}

//---------------------------------------------------------------------------------------------
CNavArea *CNEOBotCtgEnemy::FindCutOff( CNEOBot *me, CNEO_Player *pGhostCarrier )
{
	CNavArea *pMyArea = me->GetLastKnownArea();
	if ( !pGhostCarrier || !pMyArea )
	{
		return nullptr;
	}

	// The carrier's goal as a defender reads it off the ghost marker: the nearest zone it can score in
	const Vector vecGoal = NEORules()->GetNearestGhostCapPoint( pGhostCarrier->GetTeamNumber(), pGhostCarrier->GetAbsOrigin() );
	if ( vecGoal == CNEO_Player::VECTOR_INVALID_WAYPOINT )
	{
		return nullptr;
	}

	// A naive shortest route: the carrier's class, loadout and intent are not observable
	CNEOBotPredictedRoute carrierRoute;
	if ( !PredictRoute( pGhostCarrier->GetLastKnownArea(), vecGoal, carrierRoute ) )
	{
		return nullptr;
	}

	// Already in its cap's area: nothing to cut off, and a zero search range would mean no limit
	if ( carrierRoute.areas.Count() < 2 )
	{
		return nullptr;
	}

	// One flood prices every area of the route by our own travel. It must come after the route is
	// copied out: both searches use the nav areas' shared search state.
	CNEOBotTraversableFlood flood( me );
	SearchSurroundingAreas( pMyArea, flood, carrierRoute.travel.Tail() );

	// The earliest area we reach no later than the carrier is the furthest from its cap we can stand in its way
	for ( int i = 0; i < carrierRoute.areas.Count(); ++i )
	{
		CNavArea *pArea = carrierRoute.areas[i];
		if ( pArea->IsMarked() && pArea->GetCostSoFar() <= carrierRoute.travel[i] )
		{
			return pArea;
		}
	}

	return nullptr;
}

//---------------------------------------------------------------------------------------------
ActionResult< CNEOBot > CNEOBotCtgEnemy::Update( CNEOBot *me, float interval )
{
	CNEO_Player *pGhostCarrier = EnemyGhostCarrier( me );
	if ( !pGhostCarrier )
	{
		return Done( "No enemy ghost carrier" );
	}

	// No cut-off means the carrier is ahead of us, and a detour would only give up more ground
	CNavArea *pCutOff = FindCutOff( me, pGhostCarrier );
	if ( !pCutOff )
	{
		return ChangeTo( new CNEOBotCtgEnemyChase( FASTEST_ROUTE ), "Chasing the ghost carrier down" );
	}

	if ( pCutOff == me->GetLastKnownArea() )
	{
		return ChangeTo( new CNEOBotCtgEnemyChase( DEFAULT_ROUTE ), "Standing on the cut-off - chasing" );
	}

	return ChangeTo( new CNEOBotCtgEnemyCutOff( pCutOff ), "Cutting the carrier off short of its cap" );
}

//---------------------------------------------------------------------------------------------
QueryResultType CNEOBotCtgEnemy::CarrierUrgency( const CNEOBot *me )
{
	CNEO_Player *pCarrier = EnemyGhostCarrier( me );
	if ( !pCarrier )
	{
		return ANSWER_UNDEFINED;
	}

	const Vector vecCarrierGoal = NEORules()->GetNearestGhostCapPoint( pCarrier->GetTeamNumber(), pCarrier->GetAbsOrigin() );
	if ( vecCarrierGoal == CNEO_Player::VECTOR_INVALID_WAYPOINT )
	{
		return ANSWER_UNDEFINED;
	}

	const float flCarrierToGoalSq = pCarrier->GetAbsOrigin().DistToSqr( vecCarrierGoal );
	const float flMeToGoalSq = me->GetAbsOrigin().DistToSqr( vecCarrierGoal );

	return ( flMeToGoalSq > flCarrierToGoalSq ) ? ANSWER_YES : ANSWER_UNDEFINED;
}
