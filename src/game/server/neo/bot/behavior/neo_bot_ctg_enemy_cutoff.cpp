#include "cbase.h"
#include "neo_player.h"
#include "bot/neo_bot.h"
#include "bot/behavior/neo_bot_ctg_enemy_cutoff.h"
#include "bot/behavior/neo_bot_ctg_enemy.h"
#include "bot/behavior/neo_bot_ctg_enemy_chase.h"
#include "bot/neo_bot_path_compute.h"
#include "neo_gamerules.h"

ConVar sv_neo_bot_ctg_enemy_cutoff_replan_seconds( "sv_neo_bot_ctg_enemy_cutoff_replan_seconds", "5", FCVAR_CHEAT,
	"CTG: seconds between a bot heading for a cut-off re-picking it on the enemy ghost carrier's route, to catch the carrier taking a line it did not predict.",
	true, 0.5f, false, 0.0f );

//---------------------------------------------------------------------------------------------
// A partial path would walk the bot to wherever the pathfinder gave up, short of the cut-off
bool CNEOBotCtgEnemyCutOff::RepathToCutOff( CNEOBot *me )
{
	return CNEOBotPathCompute( me, m_path, m_pCutOff->GetCenter(), FASTEST_ROUTE, PATH_NO_LENGTH_LIMIT, false );
}

//---------------------------------------------------------------------------------------------
// Re-pick the cut-off. False when there is none left to head for, so the caller drops to the chase.
bool CNEOBotCtgEnemyCutOff::Replan( CNEOBot *me, CNEO_Player *pGhostCarrier )
{
	CNavArea *pCutOff = CNEOBotCtgEnemy::FindCutOff( me, pGhostCarrier );
	if ( !pCutOff )
	{
		return false;
	}

	if ( pCutOff == m_pCutOff )
	{
		return true;
	}

	m_pCutOff = pCutOff;
	return RepathToCutOff( me );
}

//---------------------------------------------------------------------------------------------
ActionResult< CNEOBot > CNEOBotCtgEnemyCutOff::OnStart( CNEOBot *me, Action< CNEOBot > *priorAction )
{
	m_path.SetMinLookAheadDistance( me->GetDesiredPathLookAheadRange() );

	if ( !RepathToCutOff( me ) )
	{
		// Every hand-over but arrival chases by FASTEST_ROUTE: an unreachable cut-off is no sign we are ahead
		return ChangeTo( new CNEOBotCtgEnemyChase( FASTEST_ROUTE ), "No path to the cut-off - chasing" );
	}

	m_replanTimer.Start( sv_neo_bot_ctg_enemy_cutoff_replan_seconds.GetFloat() );

	return Continue();
}

//---------------------------------------------------------------------------------------------
ActionResult< CNEOBot > CNEOBotCtgEnemyCutOff::Update( CNEOBot *me, float interval )
{
	CNEO_Player *pGhostCarrier = CNEOBotCtgEnemy::EnemyGhostCarrier( me );
	if ( !pGhostCarrier )
	{
		return Done( "No enemy ghost carrier" );
	}

	if ( m_replanTimer.IsElapsed() )
	{
		m_replanTimer.Start( sv_neo_bot_ctg_enemy_cutoff_replan_seconds.GetFloat() );

		if ( !Replan( me, pGhostCarrier ) )
		{
			return ChangeTo( new CNEOBotCtgEnemyChase( FASTEST_ROUTE ), "Lost the cut-off - chasing" );
		}
	}

	// The cut-off's value is the ground it wins, not an ambush: once on it, chase from here
	if ( me->GetLastKnownArea() == m_pCutOff )
	{
		return ChangeTo( new CNEOBotCtgEnemyChase( DEFAULT_ROUTE ), "Arrived at the cut-off - chasing" );
	}

	m_path.Update( me );
	if ( !m_path.IsValid() )
	{
		return ChangeTo( new CNEOBotCtgEnemyChase( FASTEST_ROUTE ), "Lost the path to the cut-off - chasing" );
	}

	return Continue();
}

//---------------------------------------------------------------------------------------------
EventDesiredResult< CNEOBot > CNEOBotCtgEnemyCutOff::OnStuck( CNEOBot *me )
{
	if ( !RepathToCutOff( me ) )
	{
		return TryChangeTo( new CNEOBotCtgEnemyChase( FASTEST_ROUTE ), RESULT_TRY, "Stuck with no way to the cut-off - chasing" );
	}

	return TryContinue();
}

//---------------------------------------------------------------------------------------------
EventDesiredResult< CNEOBot > CNEOBotCtgEnemyCutOff::OnMoveToFailure( CNEOBot *me, const Path *path, MoveToFailureType reason )
{
	if ( !RepathToCutOff( me ) )
	{
		return TryChangeTo( new CNEOBotCtgEnemyChase( FASTEST_ROUTE ), RESULT_TRY, "Cannot reach the cut-off - chasing" );
	}

	return TryContinue();
}

//---------------------------------------------------------------------------------------------
QueryResultType CNEOBotCtgEnemyCutOff::ShouldHurry( const INextBot *me ) const
{
	return CNEOBotCtgEnemy::CarrierUrgency( static_cast< const CNEOBot * >( me ) );
}
