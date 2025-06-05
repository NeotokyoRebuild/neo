//========= Copyright Valve Corporation, All rights reserved. ============//

#include "cbase.h"
#include "nav_mesh.h"
#include "neo_player.h"
#include "bot/neo_bot.h"
#include "bot/behavior/nav_entities/neo_bot_nav_ent_wait.h"

extern ConVar neo_bot_path_lookahead_range;

//---------------------------------------------------------------------------------------------
CNEOBotNavEntWait::CNEOBotNavEntWait( const CFuncNavPrerequisite *prereq )
{
	m_prereq = prereq;
}


//---------------------------------------------------------------------------------------------
ActionResult< CNEOBot > CNEOBotNavEntWait::OnStart( CNEOBot *me, Action< CNEOBot > *priorAction )
{
	if ( m_prereq == NULL )
	{
		return Done( "Prerequisite has been removed before we started" );
	}

	m_timer.Start( m_prereq->GetTaskValue() );

	return Continue();
}


//---------------------------------------------------------------------------------------------
ActionResult< CNEOBot > CNEOBotNavEntWait::Update( CNEOBot *me, float interval )
{
	if ( m_prereq == NULL )
	{
		return Done( "Prerequisite has been removed" );
	}

	if ( !m_prereq->IsEnabled() )
	{
		return Done( "Prerequisite has been disabled" );
	}

	if ( m_timer.IsElapsed() )
	{
		return Done( "Wait time elapsed" );
	}

	return Continue();
}


