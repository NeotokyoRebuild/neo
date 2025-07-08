#include "cbase.h"
#include "nav_mesh.h"
#include "neo_player.h"
#include "bot/neo_bot.h"
#include "bot/behavior/nav_entities/neo_bot_nav_ent_destroy_entity.h"

extern ConVar neo_bot_path_lookahead_range;

//---------------------------------------------------------------------------------------------
CNEOBotNavEntDestroyEntity::CNEOBotNavEntDestroyEntity( const CFuncNavPrerequisite *prereq )
{
	m_prereq = prereq;
}


//---------------------------------------------------------------------------------------------
ActionResult< CNEOBot >	CNEOBotNavEntDestroyEntity::OnStart( CNEOBot *me, Action< CNEOBot > *priorAction )
{
	if ( m_prereq == NULL )
	{
		return Done( "Prerequisite has been removed before we started" );
	}

	m_path.SetMinLookAheadDistance( me->GetDesiredPathLookAheadRange() );

	m_wasIgnoringEnemies = me->HasAttribute( CNEOBot::IGNORE_ENEMIES );

	//m_isReadyToLaunchSticky = true;

	return Continue();
}

// NEO TODO (Adam) Do this for remote det instead someday
#if 0
//---------------------------------------------------------------------------------------------
void CNEOBotNavEntDestroyEntity::DetonateStickiesWhenSet( CNEOBot *me, CWeapon_SLAM *slam ) const
{
	// TODO(misyl): Hook this up for SLAM someday.
#if 0
	if ( !stickyLauncher )
		return;

	if ( stickyLauncher->GetPipeBombCount() >= 8 || me->GetAmmoCount( TF_AMMO_SECONDARY ) <= 0 )
	{
		// stickies laid - detonate them once they are on the ground
		const CUtlVector< CHandle< CTFGrenadePipebombProjectile > > &pipeVector = stickyLauncher->GetPipeBombVector();

		int i;
		for( i=0; i<pipeVector.Count(); ++i )
		{
			if ( pipeVector[i].Get() && !pipeVector[i]->m_bTouched )
			{
				break;
			}
		}

		if ( i == pipeVector.Count() )
		{
			// stickies are on the ground
			me->PressFireButton();
		}
	}
#endif
}
#endif


//---------------------------------------------------------------------------------------------
ActionResult< CNEOBot >	CNEOBotNavEntDestroyEntity::Update( CNEOBot *me, float interval )
{
	if ( m_prereq == NULL )
	{
		return Done( "Prerequisite has been removed" );
	}

	CBaseEntity *target = m_prereq->GetTaskEntity();
	if ( target == NULL )
	{
		return Done( "Target entity is NULL" );
	}

	float attackRange = me->GetMaxAttackRange();
		
	if ( m_prereq->GetTaskValue() > 0.0f ) 
	{
		attackRange = MIN( attackRange, m_prereq->GetTaskValue() );
	}

	if ( me->IsDistanceBetweenLessThan( target, attackRange ) && me->GetVisionInterface()->IsLineOfSightClearToEntity( target ) )
	{
		me->SetAttribute( CNEOBot::IGNORE_ENEMIES );

		me->GetBodyInterface()->AimHeadTowards( target->WorldSpaceCenter(), IBody::CRITICAL, 0.2f, NULL, "Aiming at target we need to destroy to progress" );

		if ( me->GetBodyInterface()->IsHeadAimingOnTarget() )
		{
			// NEO TODO (Adam) remotedet
#if 0
			// attack
			CWeapon_SLAM *pSLAM = ( CWeapon_SLAM * ) me->Weapon_OwnsThisType( "weapon_slam" );
			if ( pSLAM )
			{
				me->Weapon_Switch( pSLAM );

				if ( m_isReadyToLaunchSticky )
				{
					me->PressAltFireButton();
				}

				m_isReadyToLaunchSticky = !m_isReadyToLaunchSticky;

				DetonateStickiesWhenSet( me, pSLAM );

				return Continue();
			}
#endif 

			me->EquipBestWeaponForThreat( NULL );
			me->PressFireButton();
		}

		return Continue();
	}


	if ( !m_wasIgnoringEnemies )
	{
		me->ClearAttribute( CNEOBot::IGNORE_ENEMIES );
	}

	// move into view of our target
	if ( m_repathTimer.IsElapsed() )
	{
		m_repathTimer.Start( RandomFloat( 1.0f, 2.0f ) );

		CNEOBotPathCost cost( me, FASTEST_ROUTE );
		m_path.Compute( me, target->GetAbsOrigin(), cost );
	}

	m_path.Update( me );

	return Continue();
}


//---------------------------------------------------------------------------------------------
void CNEOBotNavEntDestroyEntity::OnEnd( CNEOBot *me, Action< CNEOBot > *nextAction )
{
	if ( !m_wasIgnoringEnemies )
	{
		me->ClearAttribute( CNEOBot::IGNORE_ENEMIES );
	}
}
