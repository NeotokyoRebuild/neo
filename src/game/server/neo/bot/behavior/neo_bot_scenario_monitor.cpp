#include "cbase.h"
#include "fmtstr.h"

#include "neo_gamerules.h"
#include "NextBot/NavMeshEntities/func_nav_prerequisite.h"

#include "bot/neo_bot.h"
#include "bot/neo_bot_manager.h"
#include "bot/behavior/nav_entities/neo_bot_nav_ent_destroy_entity.h"
#include "bot/behavior/nav_entities/neo_bot_nav_ent_move_to.h"
#include "bot/behavior/nav_entities/neo_bot_nav_ent_wait.h"
#include "bot/behavior/neo_bot_tactical_monitor.h"
#include "bot/behavior/neo_bot_retreat_to_cover.h"
#include "bot/behavior/neo_bot_get_health.h"
#include "bot/behavior/neo_bot_get_ammo.h"

#include "bot/behavior/neo_bot_attack.h"
#include "bot/behavior/neo_bot_seek_and_destroy.h"

#include "bot/behavior/neo_bot_scenario_monitor.h"


//-----------------------------------------------------------------------------------------
// Returns the initial Action we will run concurrently as a child to us
Action< CNEOBot > *CNEOBotScenarioMonitor::InitialContainedAction( CNEOBot *me )
{
	if ( me->IsInASquad() )
	{
		if ( me->GetSquad()->IsLeader( me ) )
		{
			// I'm the leader of this Squad, so I can do what I want and the other Squaddies will support me
			return DesiredScenarioAndClassAction( me );
		}

		// I'm in a Squad but not the leader, do "escort and support" Squad behavior
		// until the Squad disbands, and then do my normal thing
		//
		// TODO: Implement this if we ever want squads in HL2MP.
		// It's like, an MVM thing, not really useful for us.
		//return new CNEOBotEscortSquadLeader( DesiredScenarioAndClassAction( me ) );
	}

	return DesiredScenarioAndClassAction( me );
}


//-----------------------------------------------------------------------------------------
// Returns Action specific to the scenario and my class
Action< CNEOBot > *CNEOBotScenarioMonitor::DesiredScenarioAndClassAction( CNEOBot *me )
{
	return new CNEOBotSeekAndDestroy;
}


//-----------------------------------------------------------------------------------------
ActionResult< CNEOBot >	CNEOBotScenarioMonitor::OnStart( CNEOBot *me, Action< CNEOBot > *priorAction )
{
	m_ignoreLostFlagTimer.Start( 20.0f );
	m_lostFlagTimer.Invalidate();
	return Continue();
}


ConVar neo_bot_fetch_lost_flag_time( "neo_bot_fetch_lost_flag_time", "10", FCVAR_CHEAT, "How long busy NEOBots will ignore the dropped flag before they give up what they are doing and go after it" );
ConVar neo_bot_flag_kill_on_touch( "neo_bot_flag_kill_on_touch", "0", FCVAR_CHEAT, "If nonzero, any bot that picks up the flag dies. For testing." );


//-----------------------------------------------------------------------------------------
ActionResult< CNEOBot >	CNEOBotScenarioMonitor::Update( CNEOBot *me, float interval )
{
	return Continue();
}

