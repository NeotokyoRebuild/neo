#include "cbase.h"
#include "bot/neo_bot.h"
#include "bot/neo_bot_path_compute.h"
#include "bot/behavior/neo_bot_attack.h"
#include "bot/behavior/neo_bot_ctg_lone_wolf_seek.h"
#include "bot/behavior/neo_bot_detpack_deploy.h"
#include "neo_detpack.h"
#include "weapon_detpack.h"

//---------------------------------------------------------------------------------------------
CNEOBotDetpackDeploy::CNEOBotDetpackDeploy( const Vector &targetPos, Action< CNEOBot > *nextAction )
	: m_targetPos( targetPos ), m_nextAction( nextAction ), m_flDeployDistSq( 0.0f )
{
	m_bPushedWeapon = false;
	m_losTimer.Invalidate();
}


//---------------------------------------------------------------------------------------------
ActionResult< CNEOBot >	CNEOBotDetpackDeploy::OnStart( CNEOBot *me, Action< CNEOBot > *priorAction )
{
	m_hDetpackWeapon = assert_cast< CWeaponDetpack* >( me->Weapon_OwnsThisType( "weapon_remotedet" ) );
	if ( !m_hDetpackWeapon )
	{
		if (m_nextAction != nullptr)
		{
			return ChangeTo( m_nextAction, "No detpack weapon, transitioning to next action" );
		}
		return Done( "No detpack weapon found" );
	}

	me->PushRequiredWeapon( m_hDetpackWeapon );
	m_bPushedWeapon = true;

	// Ignore enemy to prevent weapon handling interference with detpack deployment
	me->StopLookingAroundForEnemies();
	me->SetAttribute( CNEOBot::IGNORE_ENEMIES );

	m_expiryTimer.Start( 10.0f );
	m_repathTimer.Invalidate();

	m_flDeployDistSq = Square( MAX( 100.0f, CWeaponDetpack::GetArmingTime() * me->GetNormSpeed() ) );

	return Continue();
}

//---------------------------------------------------------------------------------------------
ActionResult< CNEOBot >	CNEOBotDetpackDeploy::Update( CNEOBot *me, float interval )
{
	if ( !m_hDetpackWeapon )
	{
		if ( m_nextAction )
		{
			return ChangeTo( m_nextAction, "No detpack weapon, transitioning to next action" );
		}
		return Done( "No detpack weapon" );
	}

	if ( m_expiryTimer.IsElapsed() )
	{
		if ( m_nextAction )
		{
			return ChangeTo( m_nextAction, "Detpack deploy timer expired, transitioning to next action" );
		}
		return Done( "Detpack deploy timer expired" );
	}

	if ( m_hDetpackWeapon->m_bThisDetpackHasBeenThrown )
	{
		if ( m_nextAction )
		{
			return ChangeTo( m_nextAction, "Detpack deployed, transitioning to next action" );
		}
		return Done( "Detpack deployed" );
	}

	const CKnownEntity* threat = me->GetVisionInterface()->GetPrimaryKnownThreat( true );
	if ( threat && threat->GetEntity() )
	{
		if (m_nextAction != nullptr)
		{
			return ChangeTo( m_nextAction, "Interrupting detpack deploy to let next action handle enemy" );
		}
		return ChangeTo( new CNEOBotAttack(), "Engaging enemy encountered while deploying detpack" );
	}

	float flDistToTargetSq = me->GetAbsOrigin().DistToSqr( m_targetPos );
	if ( flDistToTargetSq < m_flDeployDistSq )
	{
		if ( me->GetActiveWeapon() == m_hDetpackWeapon )
		{
			if ( !m_losTimer.HasStarted() || m_losTimer.IsElapsed() )
			{
				if ( me->GetVisionInterface()->IsLineOfSightClear( m_targetPos ) )
				{
					CBaseEntity *pEnts[256];
					int numEnts = UTIL_EntitiesInSphere( pEnts, 256, me->GetAbsOrigin(), NEO_DETPACK_DAMAGE_RADIUS, 0 );
					bool bDetpackNear = false;
					for ( int i = 0; i < numEnts; ++i )
					{
						if ( pEnts[i] && FClassnameIs( pEnts[i], "neo_deployed_detpack" ) )
						{
							bDetpackNear = true;
							break;
						}
					}

					if ( bDetpackNear )
					{
						if ( m_nextAction )
						{
							return ChangeTo( m_nextAction, "Skipping detpack deploy: in blast radius of another detpack" );
						}
						return Done( "Aborting detpack deploy: in blast radius of another detpack" );
					}

					me->PressFireButton();
					
					if ( flDistToTargetSq < Square( 64.0f ) )
					{
						m_path.Invalidate();
						m_repathTimer.Start( 10.0f );
					}
				}

				m_losTimer.Start( RandomFloat( 0.2f, 0.4f ) );
			}
		}
	}

	if ( !m_path.IsValid() )
	{
		if ( !m_repathTimer.HasStarted() || m_repathTimer.IsElapsed() )
		{
			CNEOBotPathCompute( me, m_path, m_targetPos, FASTEST_ROUTE );
			m_repathTimer.Start( RandomFloat( 1.0f, 2.0f ) );
		}
	}
	else
	{
		m_path.Update( me );
	}

	return Continue();
}


//---------------------------------------------------------------------------------------------
void CNEOBotDetpackDeploy::OnEnd( CNEOBot *me, Action< CNEOBot > *nextAction )
{
	if ( m_bPushedWeapon )
	{
		me->PopRequiredWeapon();
		m_bPushedWeapon = false;
	}
	me->StartLookingAroundForEnemies();
	me->ClearAttribute( CNEOBot::IGNORE_ENEMIES );

	if ( m_hDetpackWeapon && m_hDetpackWeapon->m_bThisDetpackHasBeenThrown )
	{
		me->EquipBestWeaponForThreat( me->GetVisionInterface()->GetPrimaryKnownThreat( true ) );
	}
}


//---------------------------------------------------------------------------------------------
ActionResult<CNEOBot> CNEOBotDetpackDeploy::OnSuspend( CNEOBot *me, Action<CNEOBot> *interruptingAction )
{
	if (m_nextAction != nullptr)
	{
		return ChangeTo( m_nextAction, "Detpack deploy suspend cancelled, transitioning to next action" );
	}

	return Done( "Detpack deploy suspended, situation will likely become stale." );
	// OnEnd will get called after Done
}

//---------------------------------------------------------------------------------------------
ActionResult<CNEOBot> CNEOBotDetpackDeploy::OnResume( CNEOBot *me, Action<CNEOBot> *interruptingAction )
{
	if (m_nextAction != nullptr)
	{
		return ChangeTo( m_nextAction, "Detpack deploy resume cancelled, transitioning to next action" );
	}

	return Done( "Detpack deploy resumed, situation is likely stale." );
	// OnEnd will get called after Done
}

//---------------------------------------------------------------------------------------------
EventDesiredResult< CNEOBot > CNEOBotDetpackDeploy::OnStuck( CNEOBot *me )
{
	if (m_nextAction != nullptr)
	{
		return TryChangeTo( m_nextAction, RESULT_CRITICAL, "Detpack deploy stuck, transitioning to next action" );
	}

	return TryDone( RESULT_CRITICAL, "Detpack deploy stuck, situation will likely become stale." );
	// OnEnd will get called after Done
}

//---------------------------------------------------------------------------------------------
EventDesiredResult< CNEOBot > CNEOBotDetpackDeploy::OnMoveToSuccess( CNEOBot *me, const Path *path )
{
	return TryContinue();
}

//---------------------------------------------------------------------------------------------
EventDesiredResult< CNEOBot > CNEOBotDetpackDeploy::OnMoveToFailure( CNEOBot *me, const Path *path, MoveToFailureType reason )
{
	if (m_nextAction != nullptr)
	{
		return TryChangeTo( m_nextAction, RESULT_CRITICAL, "Detpack deploy move to failure, transitioning to next action" );
	}

	return TryDone( RESULT_CRITICAL, "Detpack deploy move to failure, situation will likely become stale." );
	// OnEnd will get called after Done
}
