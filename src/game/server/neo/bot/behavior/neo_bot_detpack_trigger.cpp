#include "cbase.h"
#include "bot/neo_bot.h"
#include "bot/behavior/neo_bot_detpack_trigger.h"
#include "weapon_detpack.h"
#include "neo_detpack.h"
#include "neo_gamerules.h"

//---------------------------------------------------------------------------------------------
CNEOBotDetpackTrigger::CNEOBotDetpackTrigger( void )
{
	m_hDetpackWeapon = nullptr;
	m_bPushedWeapon = false;
}

//---------------------------------------------------------------------------------------------
// Check if teammates would be in the blast radius
bool CNEOBotDetpackTrigger::IsTeammateInBlast( const CNEOBot *me, const Vector &vecDetpackPos, float flRadius )
{
	if ( !NEORules()->IsTeamplay() )
	{
		return false;
	}

	CUtlVector< CNEO_Player * > friendVector;
	CollectPlayers( &friendVector, me->GetTeamNumber(), COLLECT_ONLY_LIVING_PLAYERS );

	for ( int i = 0; i < friendVector.Count(); ++i )
	{
		if ( friendVector[i] == me->GetEntity() )
		{
			continue;
		}

		if ( friendVector[i]->GetAbsOrigin().DistToSqr( vecDetpackPos ) <= Square( flRadius ) )
		{
			return true;
		}
	}

	return false;
}

//---------------------------------------------------------------------------------------------
ActionResult< CNEOBot >	CNEOBotDetpackTrigger::OnStart( CNEOBot *me, Action< CNEOBot > *priorAction )
{
	m_hDetpackWeapon = assert_cast<CWeaponDetpack*>( me->Weapon_OwnsThisType( "weapon_remotedet" ) );
	if ( m_hDetpackWeapon )
	{
		me->PushRequiredWeapon( m_hDetpackWeapon );
		m_bPushedWeapon = true;
	}
	else
	{
		return Done( "No detpack weapon found" );
	}

	// Ignore enemy to prevent weapon handling interference with detpack trigger
	me->StopLookingAroundForEnemies();
	me->SetAttribute( CNEOBot::IGNORE_ENEMIES );

	m_expiryTimer.Start( 5.0f );

	return Continue();
}

//---------------------------------------------------------------------------------------------
ActionResult< CNEOBot >	CNEOBotDetpackTrigger::Update( CNEOBot *me, float interval )
{
	if ( m_expiryTimer.IsElapsed() )
	{
		return Done( "Detpack trigger timer expired" );
	}

	if ( !m_hDetpackWeapon || !m_hDetpackWeapon->m_bThisDetpackHasBeenThrown || m_hDetpackWeapon->m_bRemoteHasBeenTriggered )
	{
		return Done( "Detpack triggered or invalid" );
	}

	// A teammate may have walked into the blast radius since the decision to trigger
	CBaseEntity *pDetpackEnt = m_hDetpackWeapon->GetDetpackEntity();
	if ( pDetpackEnt && IsTeammateInBlast( me, pDetpackEnt->GetAbsOrigin(), NEO_DETPACK_DAMAGE_RADIUS ) )
	{
		return Done( "Teammate in detpack blast radius" );
	}

	if ( me->GetActiveWeapon() == m_hDetpackWeapon && gpGlobals->curtime >= m_hDetpackWeapon->m_flNextPrimaryAttack )
	{
		me->PressFireButton();
	}

	return Continue();
}

//---------------------------------------------------------------------------------------------
void CNEOBotDetpackTrigger::OnEnd( CNEOBot *me, Action< CNEOBot > *nextAction )
{
	// Restore looking and weapon handling behaviors
	if ( m_bPushedWeapon )
	{
		me->PopRequiredWeapon();
		m_bPushedWeapon = false;
	}
	me->StartLookingAroundForEnemies();
	me->ClearAttribute( CNEOBot::IGNORE_ENEMIES );
}

//---------------------------------------------------------------------------------------------
ActionResult<CNEOBot> CNEOBotDetpackTrigger::OnSuspend( CNEOBot *me, Action<CNEOBot> *interruptingAction )
{
	return Done( "OnSuspend: Cancel out of detpack trigger behavior, situation will likely become stale." );
	// OnEnd will get called after Done
}

//---------------------------------------------------------------------------------------------
ActionResult<CNEOBot> CNEOBotDetpackTrigger::OnResume( CNEOBot *me, Action<CNEOBot> *interruptingAction )
{
	return Done( "OnResume: Cancel out of detpack trigger behavior, situation is likely stale." );
	// OnEnd will get called after Done
}
