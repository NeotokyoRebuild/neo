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
	m_path.SetMinLookAheadDistance( me->GetDesiredPathLookAheadRange() );
	m_repathTimer.Invalidate();

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

	const CKnownEntity *threat = me->GetVisionInterface()->GetPrimaryKnownThreat();
	if ( threat && !threat->IsObsolete() && me->GetIntentionInterface()->ShouldAttack( me, threat ) )
	{
		return SuspendFor( new CNEOBotAttack, "Attacking ghoster team" );
	}

	// Investigate the ghost carrier's position
	if ( m_repathTimer.IsElapsed() )
	{
		CNEOBotPathCompute( me, m_path, pGhostCarrier->GetAbsOrigin(), DEFAULT_ROUTE );
		m_repathTimer.Start( RandomFloat( 0.2f, 1.0f ) );
	}
	m_path.Update( me );
	
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
