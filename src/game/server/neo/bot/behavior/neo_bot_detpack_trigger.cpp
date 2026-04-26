#include "cbase.h"
#include "bot/neo_bot.h"
#include "bot/behavior/neo_bot_detpack_trigger.h"
#include "weapon_detpack.h"

//---------------------------------------------------------------------------------------------
CNEOBotDetpackTrigger::CNEOBotDetpackTrigger( void )
{
	m_hDetpackWeapon = nullptr;
	m_bPushedWeapon = false;
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
