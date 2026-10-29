#include "cbase.h"
#include "neo_player.h"
#include "bot/neo_bot.h"
#include "bot/behavior/neo_bot_ctg_enemy_chase.h"
#include "bot/behavior/neo_bot_ctg_enemy.h"
#include "bot/behavior/neo_bot_attack.h"
#include "bot/neo_bot_path_compute.h"
#include "neo_gamerules.h"

// Seconds between a chasing bot re-checking whether it is ahead of the carrier,
// which picks the fastest or the default route
static constexpr float CTG_ENEMY_CHASE_REPLAN_SECONDS = 20.0f;

//---------------------------------------------------------------------------------------------
ActionResult< CNEOBot > CNEOBotCtgEnemyChase::OnStart( CNEOBot *me, Action< CNEOBot > *priorAction )
{
	m_chasePath.SetMinLookAheadDistance( me->GetDesiredPathLookAheadRange() );
	m_routeTypeTimer.Start( CTG_ENEMY_CHASE_REPLAN_SECONDS );

	return Continue();
}

//---------------------------------------------------------------------------------------------
ActionResult< CNEOBot > CNEOBotCtgEnemyChase::Update( CNEOBot *me, float interval )
{
	CNEO_Player *pGhostCarrier = CNEOBotCtgEnemy::EnemyGhostCarrier( me );
	if ( !pGhostCarrier )
	{
		return Done( "No enemy ghost carrier" );
	}

	// We are ahead of the carrier exactly when a cut-off still exists
	if ( m_routeTypeTimer.IsElapsed() )
	{
		m_routeTypeTimer.Start( CTG_ENEMY_CHASE_REPLAN_SECONDS );
		m_routeType = CNEOBotCtgEnemy::FindCutOff( me, pGhostCarrier ) ? DEFAULT_ROUTE : FASTEST_ROUTE;
	}

	const CKnownEntity *threat = me->GetVisionInterface()->GetPrimaryKnownThreat( true );
	if ( threat && !threat->IsObsolete() && me->GetIntentionInterface()->ShouldAttack( me, threat ) )
	{
		return SuspendFor( new CNEOBotAttack( pGhostCarrier->GetAbsOrigin() ), "Attacking ghoster team" );
	}

	CNEOBotPathUpdateChase( me, m_chasePath, pGhostCarrier, m_routeType );

	return Continue();
}

//---------------------------------------------------------------------------------------------
ActionResult< CNEOBot > CNEOBotCtgEnemyChase::OnResume( CNEOBot *me, Action< CNEOBot > *interruptingAction )
{
	m_chasePath.Invalidate();

	return Continue();
}

//---------------------------------------------------------------------------------------------
QueryResultType CNEOBotCtgEnemyChase::ShouldHurry( const INextBot *me ) const
{
	return CNEOBotCtgEnemy::CarrierUrgency( static_cast< const CNEOBot * >( me ) );
}
