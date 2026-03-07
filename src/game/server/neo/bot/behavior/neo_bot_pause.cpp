#include "cbase.h"
#include "neo_player.h"
#include "bot/neo_bot.h"
#include "bot/behavior/neo_bot_pause.h"
#include "bot/behavior/neo_bot_command_follow.h"

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

extern ConVar sv_neo_bot_cmdr_debug_pause_uncommanded;

//---------------------------------------------------------------------------------------------
ActionResult< CNEOBot > CNEOBotPause::OnStart( CNEOBot *me, Action< CNEOBot > *priorAction )
{
	if ( sv_neo_bot_cmdr_debug_pause_uncommanded.GetBool() )
	{
		me->SetAttribute( CNEOBot::IGNORE_ENEMIES );
		me->StopLookingAroundForEnemies();
	}
	return Continue();
}

//---------------------------------------------------------------------------------------------
ActionResult< CNEOBot > CNEOBotPause::Update( CNEOBot* me, float interval )
{
	// If the bot gets commanded, transition to CommandFollow immediately
	if (me->m_hLeadingPlayer.Get() || me->m_hCommandingPlayer.Get())
	{
		return ChangeTo(new CNEOBotCommandFollow, "Commanded while paused");
	}

	if (!sv_neo_bot_cmdr_debug_pause_uncommanded.GetBool())
	{
		return Done("Unpaused");
	}
	else
	{
		if ( !me->HasAttribute( CNEOBot::IGNORE_ENEMIES ) )
		{
			// Ensure these are set if cvar changed while in state
			me->SetAttribute( CNEOBot::IGNORE_ENEMIES );
			me->StopLookingAroundForEnemies();
		}
	}

	// Stop moving
	if ( me->GetCurrentPath() )
	{
		me->SetCurrentPath( NULL );
	}

	me->GetLocomotionInterface()->Reset();
	return Continue();
}

//---------------------------------------------------------------------------------------------
void CNEOBotPause::OnEnd( CNEOBot *me, Action< CNEOBot > *nextAction )
{
	me->ClearAttribute( CNEOBot::IGNORE_ENEMIES );
	me->StartLookingAroundForEnemies();
}
