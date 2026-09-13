#include "cbase.h"
#include "neo_player.h"
#include "bot/neo_bot.h"
#include "bot/behavior/neo_bot_ctg_enemy.h"
#include "bot/behavior/neo_bot_attack.h"
#include "bot/neo_bot_path_compute.h"
#include "neo_gamerules.h"


//---------------------------------------------------------------------------------------------
CNEOBotCtgEnemy::CNEOBotCtgEnemy( void )
{
}

//---------------------------------------------------------------------------------------------
ActionResult< CNEOBot >	CNEOBotCtgEnemy::OnStart( CNEOBot *me, Action< CNEOBot > *priorAction )
{
	m_chasePath.SetMinLookAheadDistance( me->GetDesiredPathLookAheadRange() );

	return Continue();
}

//---------------------------------------------------------------------------------------------
ActionResult< CNEOBot >	CNEOBotCtgEnemy::Update( CNEOBot *me, float interval )
{
	if ( !NEORules()->GhostExists() )
	{
		return Done( "Ghost does not exist" );
	}

	if ( NEORules()->GetGhosterPlayer() <= 0 )
	{
		return Done( "No ghost carrier" );
	}

	CNEO_Player* pGhostCarrier = ToNEOPlayer( UTIL_PlayerByIndex( NEORules()->GetGhosterPlayer() ) );
	if ( !pGhostCarrier || pGhostCarrier->GetTeamNumber() == me->GetTeamNumber() )
	{
		return Done( "Ghost carrier is friendly" );
	}

	const CKnownEntity *threat = me->GetVisionInterface()->GetPrimaryKnownThreat(true);
	if ( threat && !threat->IsObsolete() && me->GetIntentionInterface()->ShouldAttack( me, threat ) )
	{
		return SuspendFor( new CNEOBotAttack(pGhostCarrier->GetAbsOrigin()), "Attacking ghoster team" );
	}

	// Investigate the ghost carrier's position
	CNEOBotPathUpdateChase( me, m_chasePath, pGhostCarrier, DEFAULT_ROUTE );
	
	return Continue();
}



//---------------------------------------------------------------------------------------------
ActionResult< CNEOBot > CNEOBotCtgEnemy::OnResume( CNEOBot *me, Action< CNEOBot > *interruptingAction )
{
	return Continue();
}

//---------------------------------------------------------------------------------------------
EventDesiredResult< CNEOBot > CNEOBotCtgEnemy::OnStuck( CNEOBot *me )
{
	return TryContinue();
}

//---------------------------------------------------------------------------------------------
EventDesiredResult< CNEOBot > CNEOBotCtgEnemy::OnMoveToSuccess( CNEOBot *me, const Path *path )
{
	return TryContinue();
}

//---------------------------------------------------------------------------------------------
EventDesiredResult< CNEOBot > CNEOBotCtgEnemy::OnMoveToFailure( CNEOBot *me, const Path *path, MoveToFailureType reason )
{
	return TryContinue();
}

//---------------------------------------------------------------------------------------------
QueryResultType CNEOBotCtgEnemy::ShouldHurry( const INextBot *me ) const
{
	const CNEOBot *meBot = static_cast<const CNEOBot *>( me );

	CNEO_Player *pCarrier = ToNEOPlayer( UTIL_PlayerByIndex( NEORules()->GetGhosterPlayer() ) );
	if ( !pCarrier || !pCarrier->IsAlive() || pCarrier->GetTeamNumber() == meBot->GetTeamNumber() )
	{
		return ANSWER_UNDEFINED;
	}

	const Vector vecCarrierGoal = NEORules()->GetNearestGhostCapPoint( pCarrier->GetTeamNumber(), pCarrier->GetAbsOrigin() );
	if ( vecCarrierGoal == CNEO_Player::VECTOR_INVALID_WAYPOINT )
	{
		return ANSWER_UNDEFINED;
	}

	const float flCarrierToGoalSq = pCarrier->GetAbsOrigin().DistToSqr( vecCarrierGoal );
	const float flMeToGoalSq = meBot->GetAbsOrigin().DistToSqr( vecCarrierGoal );

	return ( flMeToGoalSq > flCarrierToGoalSq ) ? ANSWER_YES : ANSWER_UNDEFINED;
}
