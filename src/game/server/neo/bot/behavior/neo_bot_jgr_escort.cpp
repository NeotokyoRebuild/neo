#include "cbase.h"
#include "neo_player.h"
#include "bot/neo_bot.h"
#include "bot/behavior/neo_bot_attack.h"
#include "bot/behavior/neo_bot_jgr_escort.h"
#include "bot/behavior/neo_bot_retreat_to_cover.h"
#include "bot/neo_bot_path_compute.h"
#include "neo_gamerules.h"

//---------------------------------------------------------------------------------------------
CNEOBotJgrEscort::CNEOBotJgrEscort( void )
{
}

//---------------------------------------------------------------------------------------------
ActionResult< CNEOBot >	CNEOBotJgrEscort::OnStart( CNEOBot *me, Action< CNEOBot > *priorAction )
{
	m_chasePath.Invalidate();
	return Continue();
}



//---------------------------------------------------------------------------------------------
ActionResult< CNEOBot >	CNEOBotJgrEscort::Update( CNEOBot *me, float interval )
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

	if ( pJuggernaut->GetTeamNumber() != me->GetTeamNumber() )
	{
		return Done( "Juggernaut is not friendly" );
	}

	// Check if we can assist the Juggernaut (if they are a bot)
	CNEOBot *pBotJuggernaut = ToNEOBot( pJuggernaut );
	if ( pBotJuggernaut )
	{
		const CKnownEntity *juggernautThreat = pBotJuggernaut->GetVisionInterface()->GetPrimaryKnownThreat();
		if ( juggernautThreat )
		{
			// Check if the threat has a clear shot at our friend
			if ( me->IsLineOfFireClear( juggernautThreat->GetLastKnownPosition(), pJuggernaut, CNEOBot::LINE_OF_FIRE_FLAGS_DEFAULT ) )
			{
				return SuspendFor( new CNEOBotAttack, "Assisting Juggernaut with their threat" );
			}
		}
	}

	// Check for own threats
	const CKnownEntity *myThreat = me->GetVisionInterface()->GetPrimaryKnownThreat();
	if ( myThreat )
	{
		return SuspendFor( new CNEOBotRetreatToCover, "Retreating to let Juggernaut get the kill" );
	}

	// Just follow the Juggernaut
	// If too close, stop moving to avoid crowding
	constexpr float flMinFollowDistSq = 200.0f * 200.0f;
	float flDistSq = me->GetAbsOrigin().DistToSqr( pJuggernaut->GetAbsOrigin() );

	if ( flDistSq < flMinFollowDistSq )
	{
		// Too close, stop moving
		if ( m_chasePath.IsValid() )
		{
			m_chasePath.Invalidate();
		}
		return Continue();
	}

	CNEOBotPathUpdateChase( me, m_chasePath, pJuggernaut, DEFAULT_ROUTE );

	return Continue();
}

//---------------------------------------------------------------------------------------------
ActionResult< CNEOBot > CNEOBotJgrEscort::OnResume( CNEOBot *me, Action< CNEOBot > *interruptingAction )
{
	return Continue();
}

//---------------------------------------------------------------------------------------------
EventDesiredResult< CNEOBot > CNEOBotJgrEscort::OnStuck( CNEOBot *me )
{
	return TryContinue();
}

//---------------------------------------------------------------------------------------------
EventDesiredResult< CNEOBot > CNEOBotJgrEscort::OnMoveToSuccess( CNEOBot *me, const Path *path )
{
	return TryContinue();
}

//---------------------------------------------------------------------------------------------
EventDesiredResult< CNEOBot > CNEOBotJgrEscort::OnMoveToFailure( CNEOBot *me, const Path *path, MoveToFailureType reason )
{
	return TryContinue();
}
