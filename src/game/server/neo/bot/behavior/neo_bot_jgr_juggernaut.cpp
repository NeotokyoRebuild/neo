#include "cbase.h"
#include "neo_player.h"
#include "neo_gamerules.h"
#include "bot/neo_bot.h"
#include "bot/neo_bot_path_compute.h"
#include "bot/behavior/neo_bot_jgr_juggernaut.h"

//---------------------------------------------------------------------------------------------
ActionResult< CNEOBot > CNEOBotJgrJuggernaut::OnStart( CNEOBot *me, Action< CNEOBot > *priorAction )
{
	m_jgrSpawns.RemoveAll();

	CBaseEntity* pSearch = NULL;
	while ((pSearch = gEntList.FindEntityByClassname(pSearch, "neo_juggernautspawnpoint")) != NULL)
	{
		m_jgrSpawns.AddToTail(pSearch);
	}

	m_stuckTimer.Invalidate();
	m_bStuckCrouchPhase = false;

	return Continue();
}

//---------------------------------------------------------------------------------------------
ActionResult< CNEOBot > CNEOBotJgrJuggernaut::Update( CNEOBot *me, float interval )
{
	ActionResult< CNEOBot > result = UpdateCommon( me, interval );
	if ( result.IsRequestingChange() || result.IsDone() )
		return result;

	return Continue();
}

//---------------------------------------------------------------------------------------------
void CNEOBotJgrJuggernaut::RecomputeSeekPath( CNEOBot *me )
{
	m_hTargetEntity = NULL;
	m_bGoingToTargetEntity = false;
	m_vGoalPos = vec3_origin;

	// Listen for gunfights
	const Vector& vGunfireLocation = SearchGunfireLocation(me);
	if (vGunfireLocation != vec3_invalid)
	{
		m_vGoalPos = vGunfireLocation;
		m_bGoingToTargetEntity = false;

		if (CNEOBotPathCompute(me, m_path, m_vGoalPos, DEFAULT_ROUTE) && m_path.IsValid() && m_path.GetResult() == Path::COMPLETE_PATH)
		{
			return;
		}
		else
		{
			// NEO Jank: Sound is unreachable so wait for it clear from the sound list
			m_soundSearchTimer.Start( 3.0f );
		}
	}

	// If unaware of gunfights, patrol spawn points
	if (m_jgrSpawns.Count() > 0)
	{
		CBaseEntity* pTargetSpawn = m_jgrSpawns[ RandomInt(0, m_jgrSpawns.Count() - 1) ];
		if (pTargetSpawn)
		{
			m_vGoalPos = pTargetSpawn->GetAbsOrigin();
			m_bGoingToTargetEntity = false;

			if (CNEOBotPathCompute(me, m_path, m_vGoalPos, DEFAULT_ROUTE) && m_path.IsValid() && m_path.GetResult() == Path::COMPLETE_PATH)
			{
				return;
			}
		}
	}
	
	// Fallback to base behavior
	CNEOBotSeekAndDestroy::RecomputeSeekPath( me );
}


//---------------------------------------------------------------------------------------------
// Adds a periodic crouch attempt on top of CNEOBotMainAction::OnStuck for height clearance
EventDesiredResult< CNEOBot > CNEOBotJgrJuggernaut::OnStuck( CNEOBot *me )
{
	UTIL_LogPrintf( "\"%s<%i><%s>\" stuck (position \"%3.2f %3.2f %3.2f\") (duration \"%3.2f\") ",
			me->GetNeoPlayerName(),
			me->GetUserID(),
			me->GetNetworkIDString(),
			me->GetAbsOrigin().x, me->GetAbsOrigin().y, me->GetAbsOrigin().z,
			me->GetLocomotionInterface()->GetStuckDuration() );

	const PathFollower *path = me->GetCurrentPath();
	if ( path && path->GetCurrentGoal() )
	{
		UTIL_LogPrintf( "   path_goal ( \"%3.2f %3.2f %3.2f\" )\n",
				path->GetCurrentGoal()->pos.x, 
				path->GetCurrentGoal()->pos.y, 
				path->GetCurrentGoal()->pos.z );
	}
	else
	{
		UTIL_LogPrintf( "   path_goal ( \"NULL\" )\n" );
	}

	if ( !m_stuckTimer.HasStarted() || m_stuckTimer.IsElapsed() )
	{
		m_stuckTimer.Start( 1.0f );
		m_bStuckCrouchPhase = !m_bStuckCrouchPhase;
	}

	// Try to crouch under doorways/gates
	if ( m_bStuckCrouchPhase )
	{
		me->PressCrouchButton();
	}
	// ... in addition to usual OnStuck behavior
	else
	{
		me->GetLocomotionInterface()->Jump();
	}

	if ( RandomInt( 0, 1 ) )
	{
		me->PressLeftButton();
	}
	else
	{
		me->PressRightButton();
	}

	return TryContinue();
}

