#include "cbase.h"
#include "vprof.h"

#include "neo_bot.h"
#include "neo_bot_vision.h"
#include "neo_player.h"
#include "neo_gamerules.h"

ConVar neo_bot_choose_target_interval( "neo_bot_choose_target_interval", "0.3f", FCVAR_CHEAT, "How often, in seconds, a NEOBot can reselect his target" );
ConVar neo_bot_sniper_choose_target_interval( "neo_bot_sniper_choose_target_interval", "3.0f", FCVAR_CHEAT, "How often, in seconds, a zoomed-in Sniper can reselect his target" );

extern ConVar neo_bot_ignore_real_players;

//------------------------------------------------------------------------------------------
void CNEOBotVision::CollectPotentiallyVisibleEntities( CUtlVector< CBaseEntity* >* potentiallyVisible )
{
	VPROF_BUDGET( "CNEOBotVision::CollectPotentiallyVisibleEntities", "NextBot" );

	potentiallyVisible->RemoveAll();

	// include all players
	for ( int i = 1; i <= gpGlobals->maxClients; ++i )
	{
		CBasePlayer* player = UTIL_PlayerByIndex( i );

		if ( player == NULL )
			continue;

		if ( FNullEnt( player->edict() ) )
			continue;

		if ( !player->IsPlayer() )
			continue;

		if ( !player->IsConnected() )
			continue;

		if ( !player->IsAlive() )
			continue;

		if ( neo_bot_ignore_real_players.GetBool() )
		{
			if ( !player->IsBot() )
				continue;
		}

		potentiallyVisible->AddToTail( player );
	}

	// include sentry guns
	UpdatePotentiallyVisibleNPCVector();

	FOR_EACH_VEC( m_potentiallyVisibleNPCVector, it )
	{
		potentiallyVisible->AddToTail( m_potentiallyVisibleNPCVector[it] );
	}
}


//------------------------------------------------------------------------------------------
void CNEOBotVision::UpdatePotentiallyVisibleNPCVector( void )
{
	if ( m_potentiallyVisibleUpdateTimer.IsElapsed() )
	{
		m_potentiallyVisibleUpdateTimer.Start( RandomFloat( 3.0f, 4.0f ) );

		// collect list of active buildings
		m_potentiallyVisibleNPCVector.RemoveAll();

		CUtlVector< INextBot* > botVector;
		TheNextBots().CollectAllBots( &botVector );
		for ( int i = 0; i < botVector.Count(); ++i )
		{
			CBaseCombatCharacter* botEntity = botVector[i]->GetEntity();
			if ( botEntity && !botEntity->IsPlayer() )
			{
				// NPC
				m_potentiallyVisibleNPCVector.AddToTail( botEntity );
			}
		}
	}
}


//------------------------------------------------------------------------------------------
/**
 * Return true to completely ignore this entity.
 * This is mostly for enemy spies.  If we don't ignore them, we will look at them.
 */
bool CNEOBotVision::IsIgnored( CBaseEntity* subject ) const
{
	CNEOBot* me = ( CNEOBot* )GetBot()->GetEntity();

	if ( me->IsAttentionFocused() )
	{
		// our attention is restricted to certain subjects
		if ( !me->IsAttentionFocusedOn( subject ) )
		{
			return false;
		}
	}

	if ( !me->IsEnemy( subject ) )
	{
		// don't ignore friends
		return false;
	}

	if ( subject->IsEffectActive( EF_NODRAW ) )
	{
		return true;
	}

	return false;
}


//------------------------------------------------------------------------------------------
// Return VISUAL reaction time
float CNEOBotVision::GetMinRecognizeTime( void ) const
{
	CNEOBot* me = ( CNEOBot* )GetBot();

	switch ( me->GetDifficulty() )
	{
	case CNEOBot::EASY:	return 1.0f;
	case CNEOBot::NORMAL:	return 0.5f;
	case CNEOBot::HARD:	return 0.3f;
	case CNEOBot::EXPERT:	return 0.2f;
	}

	return 1.0f;
}



//------------------------------------------------------------------------------------------
float CNEOBotVision::GetMaxVisionRange( void ) const
{
	CNEOBot *me = (CNEOBot *)GetBot();

	if ( me->GetMaxVisionRangeOverride() > 0.0f )
	{
		// designer specified vision range
		return me->GetMaxVisionRangeOverride();
	}

	// long range, particularly for snipers
	return 6000.0f;
}
