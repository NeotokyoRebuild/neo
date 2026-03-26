#include "cbase.h"
#include "bot/behavior/neo_bot_jgr_capture.h"
#include "bot/behavior/neo_bot_retreat_to_cover.h"
#include "bot/behavior/neo_bot_attack.h"
#include "bot/neo_bot_path_compute.h"
#include "neo_gamerules.h"
#include "neo_juggernaut.h"

static const float JGR_CAPTURE_FACING_DOT = 0.9f;
static const float JGR_CAPTURE_LOCKED_RETREAT_TIME = 2.0f;
static const float JGR_CAPTURE_MAX_REPATH_DELAY = 5.0f;
static const float JGR_CAPTURE_MIN_REPATH_DELAY = 1.0f;
static const float JGR_CAPTURE_REPOSITION_RATIO = 0.7f;
static const float JGR_CAPTURE_TIMER_BUFFER = 1.0f;

//---------------------------------------------------------------------------------------------
void CNEOBotJgrCapture::StopMoving( CNEOBot *me )
{
	m_path.Invalidate();
	me->ReleaseForwardButton();
	me->ReleaseBackwardButton();
	me->ReleaseLeftButton();
	me->ReleaseRightButton();
}

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
	m_bStrafeRight = ( RandomInt( 0, 1 ) == 1 );
	
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
	me->SetAttribute( CNEOBot::IGNORE_ENEMIES );
	me->ReloadIfLowClip(); // might as well as we're preoccupied
	return Continue();
}

//---------------------------------------------------------------------------------------------
void CNEOBotJgrCapture::OnEnd( CNEOBot *me, Action<CNEOBot> *nextAction )
{
	me->ReleaseUseButton();
	me->StartLookingAroundForEnemies();
	me->ClearAttribute( CNEOBot::IGNORE_ENEMIES );
}

//---------------------------------------------------------------------------------------------
ActionResult<CNEOBot> CNEOBotJgrCapture::OnSuspend( CNEOBot *me, Action<CNEOBot> *interruptingAction )
{
	// Situation around juggernaut is possibly stale, reevaluate
	return Done( "OnSuspend: Reevaluating capture situation" );
}

//---------------------------------------------------------------------------------------------
ActionResult<CNEOBot> CNEOBotJgrCapture::OnResume( CNEOBot *me, Action<CNEOBot> *interruptingAction )
{
	// Situation around juggernaut is possibly stale, reevaluate
	return Done( "OnResume: Reevaluating capture situation" );
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

	CBasePlayer *pActivatingPlayer = m_hObjective->GetActivatingPlayer();
	if ( pActivatingPlayer )
	{
		if ( !me->InSameTeam( pActivatingPlayer ) )
		{
			return SuspendFor( new CNEOBotAttack, "Attacking enemy capturing the juggernaut" );
		}
		else if ( pActivatingPlayer != me )
		{
			if ( me->GetVisionInterface()->GetPrimaryKnownThreat() )
			{
				return SuspendFor( new CNEOBotAttack, "Defending teammate capturing the juggernaut" );
			}

			// Look away from the juggernaut to watch for threats
			CNEOBotJgrCapture::LookAwayFrom( me, m_hObjective );

			me->ReleaseUseButton();
			m_useAttemptTimer.Invalidate();
			return Continue();
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
			m_useAttemptTimer.Invalidate();

			// Look away from the juggernaut while it's locked to watch for threats
			CNEOBotJgrCapture::LookAwayFrom( me, m_hObjective );
		}

		const Vector vecObjectiveCenter = m_hObjective->WorldSpaceCenter();
		me->GetBodyInterface()->AimHeadTowards( vecObjectiveCenter, IBody::MANDATORY, 0.1f, nullptr, "Focusing on Juggernaut objective" );

		// Check if we are facing juggernaut and have a clear line of sight
		Vector vecToTargetDir = vecObjectiveCenter - me->EyePosition();
		vecToTargetDir.NormalizeInPlace();

		Vector vecEyeDirection;
		me->EyeVectors( &vecEyeDirection );
		const float flDot = vecEyeDirection.Dot( vecToTargetDir );
		const bool bIsFacing = flDot > JGR_CAPTURE_FACING_DOT;

		trace_t trace;
		UTIL_TraceLine( me->EyePosition(), vecObjectiveCenter, MASK_PLAYERSOLID, me, COLLISION_GROUP_NONE, &trace );

		if ( trace.m_pEnt == m_hObjective )
		{
			if ( bIsFacing && me->GetBodyInterface()->IsHeadAimingOnTarget() )
			{
				if ( !m_useAttemptTimer.HasStarted() )
				{
					m_useAttemptTimer.Start( CNEO_Juggernaut::GetUseDuration() + JGR_CAPTURE_TIMER_BUFFER );
				}

				me->PressUseButton();
				StopMoving( me );

				if ( m_useAttemptTimer.IsElapsed() )
				{
					return Done( "Activation attempt expired" );
				}
			}
		}
		else
		{
			// Trace blocked (usually by teammate), reposition to get a better angle
			me->ReleaseUseButton();
			m_useAttemptTimer.Invalidate();
			m_path.Invalidate();

			const float flRepositionDistSqr = JGR_CAPTURE_REPOSITION_RATIO * CNEO_Juggernaut::GetUseDistanceSquared();
			if ( me->GetAbsOrigin().DistToSqr( m_hObjective->GetAbsOrigin() ) > flRepositionDistSqr )
			{
				me->PressForwardButton( 0.2f );
			}

			if ( m_bStrafeRight )
			{
				me->PressRightButton( 1.0f );
				me->ReleaseLeftButton();
			}
			else
			{
				me->PressLeftButton( 1.0f );
				me->ReleaseRightButton();
			}
		}
	}
	else
	{
		// Objective is farther than we can reach for capture
		if ( !m_repathTimer.HasStarted() || m_repathTimer.IsElapsed() )
		{
			m_repathTimer.Start( RandomFloat( JGR_CAPTURE_MIN_REPATH_DELAY, JGR_CAPTURE_MAX_REPATH_DELAY ) );
			if ( !CNEOBotPathCompute( me, m_path, m_hObjective->GetAbsOrigin(), FASTEST_ROUTE ) )
			{
				return Done( "Unable to find a path to the Juggernaut objective" );
			}
		}
		m_path.Update( me );
	}

	return Continue();
}

//---------------------------------------------------------------------------------------------
void CNEOBotJgrCapture::LookAwayFrom( CNEOBot *me, CBaseEntity *pTarget )
{
	if ( !pTarget )
	{
		return;
	}

	Vector vecAwayFromTarget = me->GetAbsOrigin() - pTarget->GetAbsOrigin();
	Vector2D vecLookDir2D = vecAwayFromTarget.AsVector2D();
	vecLookDir2D.NormalizeInPlace();

	const Vector vecLookPos = me->EyePosition() + Vector( vecLookDir2D.x, vecLookDir2D.y, 0.0f ) * 500.0f;
	me->GetBodyInterface()->AimHeadTowards( vecLookPos, IBody::IMPORTANT, 0.1f, nullptr, "Facing away from target to watch for threats" );
}
