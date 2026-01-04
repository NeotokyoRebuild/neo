#include "cbase.h"
#include "neo_player.h"
#include "bot/neo_bot.h"
#include "bot/behavior/neo_bot_jgr_enemy.h"
#include "bot/neo_bot_path_compute.h"
#include "neo_gamerules.h"
#include "bot/behavior/neo_bot_attack.h"

//---------------------------------------------------------------------------------------------
CNEOBotJgrEnemy::CNEOBotJgrEnemy( void )
{
}

//---------------------------------------------------------------------------------------------
ActionResult< CNEOBot >	CNEOBotJgrEnemy::OnStart( CNEOBot *me, Action< CNEOBot > *priorAction )
{
	return Continue();
}

//---------------------------------------------------------------------------------------------
ActionResult< CNEOBot >	CNEOBotJgrEnemy::Update( CNEOBot *me, float interval )
{
	const int iJuggernautPlayer = NEORules()->GetJuggernautPlayer();
	if ( iJuggernautPlayer <= 0 )
	{
		return Done( "No juggernaut player" );
	}

	CNEO_Player* pJuggernaut = ToNEOPlayer( UTIL_PlayerByIndex( iJuggernautPlayer ) );
	if ( !pJuggernaut )
	{
		return Done( "Juggernaut is invalid" );
	}

	if ( !pJuggernaut->IsAlive() )
	{
		return Done( "Juggernaut is dead" );
	}

	if ( pJuggernaut->GetTeamNumber() == me->GetTeamNumber() )
	{
		return Done( "Juggernaut is friendly" );
	}

	const CKnownEntity *threat = me->GetVisionInterface()->GetPrimaryKnownThreat();
	if ( threat && !threat->IsObsolete() && me->GetIntentionInterface()->ShouldAttack( me, threat ) )
	{
		return SuspendFor( new CNEOBotAttack, "Attacking threat" );
	}

	// Chase the Juggernaut
	CNEOBotPathUpdateChase( me, m_chasePath, pJuggernaut, DEFAULT_ROUTE );

	return Continue();
}

//---------------------------------------------------------------------------------------------
ActionResult< CNEOBot > CNEOBotJgrEnemy::OnResume( CNEOBot *me, Action< CNEOBot > *interruptingAction )
{
	return Continue();
}

//---------------------------------------------------------------------------------------------
EventDesiredResult< CNEOBot > CNEOBotJgrEnemy::OnStuck( CNEOBot *me )
{
	return TryContinue();
}

//---------------------------------------------------------------------------------------------
EventDesiredResult< CNEOBot > CNEOBotJgrEnemy::OnMoveToSuccess( CNEOBot *me, const Path *path )
{
	return TryContinue();
}

//---------------------------------------------------------------------------------------------
EventDesiredResult< CNEOBot > CNEOBotJgrEnemy::OnMoveToFailure( CNEOBot *me, const Path *path, MoveToFailureType reason )
{
	return TryContinue();
}
