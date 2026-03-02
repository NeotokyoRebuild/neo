#include "cbase.h"
#include "bot/behavior/neo_bot_jgr_capture.h"
#include "bot/behavior/neo_bot_retreat_to_cover.h"
#include "bot/neo_bot_path_compute.h"
#include "neo_gamerules.h"
#include "neo_juggernaut.h"

//---------------------------------------------------------------------------------------------
CNEOBotJgrCapture::CNEOBotJgrCapture( CNEO_Juggernaut *pObjective )
{
	m_hObjective = pObjective;
}

//---------------------------------------------------------------------------------------------
ActionResult<CNEOBot> CNEOBotJgrCapture::OnStart( CNEOBot *me, Action<CNEOBot> *priorAction )
{
	m_useAttemptTimer.Invalidate();
	m_path.Invalidate();
	m_repathTimer.Invalidate();
	
	if ( !m_hObjective )
	{
		return Done( "No capture target specified." );
	}
	
	if ( NEORules()->GetGameType() != NEO_GAME_TYPE_JGR )
	{
		return Done( "Not in JGR game mode" );
	}

	if ( !FStrEq( m_hObjective->GetClassname(), "neo_juggernaut" ) )
	{
		return Done( "Capture target is not the juggernaut" );
	}

	// Ignore enemies while capturing juggernaut
	me->StopLookingAroundForEnemies();
	me->ReloadIfLowClip(); // might as well as we're preoccupied
	return Continue();
}

//---------------------------------------------------------------------------------------------
void CNEOBotJgrCapture::OnEnd( CNEOBot *me, Action<CNEOBot> *nextAction )
{
	me->ReleaseUseButton();
	me->StartLookingAroundForEnemies();
}

//---------------------------------------------------------------------------------------------
ActionResult<CNEOBot> CNEOBotJgrCapture::OnSuspend( CNEOBot *me, Action<CNEOBot> *interruptingAction )
{
	// CNEOBotTacticalMonitor -> CNEOBotRetreatToCover will handle reacting to enemies if under fire
	// Debatably, maybe bots should just ignore enemies, but that would require a change to CNEOBotTacticalMonitor
	// Also it might be more fun for humans if they can interrupt bots from taking the Juggernaut.
	me->ReleaseUseButton();
	me->StartLookingAroundForEnemies();
	return Continue();
}

//---------------------------------------------------------------------------------------------
ActionResult<CNEOBot> CNEOBotJgrCapture::OnResume( CNEOBot *me, Action<CNEOBot> *interruptingAction )
{
	me->StopLookingAroundForEnemies();
	return Continue();
}

//---------------------------------------------------------------------------------------------
ActionResult<CNEOBot> CNEOBotJgrCapture::Update( CNEOBot *me, float interval )
{
	if ( me->IsDead() )
	{
		return Done( "I am dead" );
	}

	if ( !m_hObjective )
	{
		return Done( "Capture target gone" );
	}

	if ( me->GetClass() == NEO_CLASS_JUGGERNAUT )
	{
		return Done( "Became the Juggernaut" );
	}

	const int iJuggernautPlayer = NEORules()->GetJuggernautPlayer();
	if ( iJuggernautPlayer > 0 )
	{
		CNEO_Player *pJuggernautPlayer = ToNEOPlayer( UTIL_PlayerByIndex( iJuggernautPlayer ) );
		if ( pJuggernautPlayer && pJuggernautPlayer != me )
		{
			return Done( "Juggernaut captured by someone else" );
		}
	}

	if ( me->GetAbsOrigin().DistToSqr( m_hObjective->GetAbsOrigin() ) < CNEO_Juggernaut::GetUseDistanceSquared() )
	{
		if ( NEORules()->IsJuggernautLocked() )
		{
#ifdef DEBUG
			auto pJuggernaut = m_hObjective.Get();
			Assert( NEORules()->IsJuggernautLocked() == (pJuggernaut && pJuggernaut->m_bLocked) );
#endif
			me->ReleaseUseButton();
			return SuspendFor( new CNEOBotRetreatToCover( 2.0f ), "Juggernaut is locked, taking cover to wait for it to unlock" );
		}
		
		// Stop moving while using
		m_path.Invalidate();
		me->ReleaseForwardButton();
		me->ReleaseBackwardButton();
		me->ReleaseLeftButton();
		me->ReleaseRightButton();

		const Vector vecObjectiveCenter = m_hObjective->WorldSpaceCenter();
		Vector vecToTargetDir = vecObjectiveCenter - me->EyePosition();
		vecToTargetDir.NormalizeInPlace();

		// Ensure we are facing the target before attempting to use
		Vector vecEyeDirection;
		me->EyeVectors( &vecEyeDirection );
		const float flDot = vecEyeDirection.Dot( vecToTargetDir );
		const bool bIsFacing = flDot > 0.9f;

		me->GetBodyInterface()->AimHeadTowards( vecObjectiveCenter, IBody::CRITICAL, 0.1f, NULL, "Looking at Juggernaut objective to use" );

		if ( m_useAttemptTimer.HasStarted() )
		{
			if ( m_useAttemptTimer.IsElapsed() )
			{
				return Done( "Use timer elapsed, failed to capture" );
			}
		}
		else if ( bIsFacing && me->GetBodyInterface()->IsHeadAimingOnTarget() )
		{
			m_useAttemptTimer.Start( CNEO_Juggernaut::GetUseDuration() + 1.0f );
			me->PressUseButton( CNEO_Juggernaut::GetUseDuration() + 1.0f );
		}
	}
	else
	{
		// Objective is farther than we can reach for cpature
		if ( m_repathTimer.IsElapsed() )
		{
			m_repathTimer.Start( RandomFloat( 1.0f, 5.0f ) );
			if ( !CNEOBotPathCompute( me, m_path, m_hObjective->GetAbsOrigin(), FASTEST_ROUTE ) )
			{
				return Done( "Unable to find a path to the Juggernaut objective" );
			}
		}
		m_path.Update( me );
	}

	return Continue();
}
