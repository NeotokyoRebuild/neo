#include "cbase.h"
#include "fmtstr.h"

#include "neo_gamerules.h"
#include "NextBot/NavMeshEntities/func_nav_prerequisite.h"

#include "bot/neo_bot.h"
#include "bot/neo_bot_manager.h"

#include "bot/behavior/neo_bot_tactical_monitor.h"
#include "bot/behavior/neo_bot_scenario_monitor.h"

#include "bot/behavior/neo_bot_seek_and_destroy.h"
#include "bot/behavior/neo_bot_retreat_to_cover.h"
#if 0 // NEO TODO (Adam) Fix picking up weapons, search for dropped weapons to pick up ammo
#include "bot/behavior/neo_bot_get_ammo.h"
#endif
#include "bot/behavior/nav_entities/neo_bot_nav_ent_destroy_entity.h"
#include "bot/behavior/nav_entities/neo_bot_nav_ent_move_to.h"
#include "bot/behavior/nav_entities/neo_bot_nav_ent_wait.h"
#include "neo/neo_player_shared.h"

ConVar neo_bot_force_jump( "neo_bot_force_jump", "0", FCVAR_CHEAT, "Force bots to continuously jump" );

////////////////////////////////////////////////////////////////////////////////////////////////////

////////////////////////////////////////////////////////////////////////////////////////////////////
// Attempts to kick/despawn the bot in the Update()

class CNEODespawn : public Action< CNEOBot >
{
public:
	virtual ActionResult< CNEOBot >	Update( CNEOBot* me, float interval );
	virtual const char* GetName( void ) const { return "Despawn"; };
};


ActionResult< CNEOBot > CNEODespawn::Update( CNEOBot* me, float interval )
{
	// players need to be kicked, not deleted
	if ( me->GetEntity()->IsPlayer() )
	{
		CBasePlayer* player = static_cast< CBasePlayer* >( me->GetEntity() );
		engine->ServerCommand( UTIL_VarArgs( "kickid %d\n", player->GetUserID() ) );
	}
	else
	{
		UTIL_Remove( me->GetEntity() );
	}
	return Continue();
}

////////////////////////////////////////////////////////////////////////////////////////////////////

Action< CNEOBot > *CNEOBotTacticalMonitor::InitialContainedAction( CNEOBot *me )
{
	return new CNEOBotScenarioMonitor;
}


//-----------------------------------------------------------------------------------------
ActionResult< CNEOBot >	CNEOBotTacticalMonitor::OnStart( CNEOBot *me, Action< CNEOBot > *priorAction )
{
	return Continue();
}


#ifndef NEO // NEO TODO (Adam) Monitor the remote detpack
//-----------------------------------------------------------------------------------------
void CNEOBotTacticalMonitor::MonitorArmedStickyBombs( CNEOBot *me )
{
	if ( m_stickyBombCheckTimer.IsElapsed() )
	{
		m_stickyBombCheckTimer.Start( RandomFloat( 0.3f, 1.0f ) );

		// are there any enemies on/near my sticky bombs?
		CWeapon_SLAM *slam = dynamic_cast< CWeapon_SLAM* >( me->Weapon_OwnsThisType( "weapon_slam" ) );
		if ( slam )
		{
			const CUtlVector< CBaseEntity* > &satchelVector = slam->GetSatchelVector();

			if ( satchelVector.Count() > 0 )
			{
				CUtlVector< CKnownEntity > knownVector;
				me->GetVisionInterface()->CollectKnownEntities( &knownVector );

				for( int p=0; p< satchelVector.Count(); ++p )
				{
					CBaseEntity *satchel = satchelVector[p];
					if ( !satchel )
					{
						continue;
					}

					for( int k=0; k<knownVector.Count(); ++k )
					{
						if ( knownVector[k].IsObsolete() )
						{
							continue;
						}

						if ( knownVector[k].GetEntity()->IsBaseObject() )
						{
							// we want to put several stickies on a sentry and det at once
							continue;
						}

						if ( satchel->GetTeamNumber() != GetEnemyTeam( knownVector[k].GetEntity()->GetTeamNumber() ) )
						{
							// "known" is either a spectator, or on our team
							continue;
						}

						const float closeRange = 150.0f;
						if ( ( knownVector[k].GetLastKnownPosition() - satchel->GetAbsOrigin() ).IsLengthLessThan( closeRange ) )
						{
							// they are close - blow it!
							me->PressFireButton();
							return;
						}
					}
				}
			}
		}
	}
}


#endif //NEO
//-----------------------------------------------------------------------------------------
void CNEOBotTacticalMonitor::AvoidBumpingEnemies( CNEOBot *me )
{
	if ( me->GetDifficulty() < CNEOBot::HARD )
		return;

	const float avoidRange = 200.0f;

	CUtlVector< CNEO_Player * > enemyVector;
	CollectPlayers( &enemyVector, GetEnemyTeam( me->GetTeamNumber() ), COLLECT_ONLY_LIVING_PLAYERS );

	CNEO_Player *closestEnemy = NULL;
	float closestRangeSq = avoidRange * avoidRange;

	for( int i=0; i<enemyVector.Count(); ++i )
	{
		CNEO_Player *enemy = enemyVector[i];

		float rangeSq = ( enemy->GetAbsOrigin() - me->GetAbsOrigin() ).LengthSqr();
		if ( rangeSq < closestRangeSq )
		{
			closestEnemy = enemy;
			closestRangeSq = rangeSq;
		}
	}

	if ( !closestEnemy )
		return;

	// avoid unless hindrance returns a definitive "no"
	if ( me->GetIntentionInterface()->IsHindrance( me, closestEnemy ) == ANSWER_UNDEFINED )
	{
		me->ReleaseForwardButton();
		me->ReleaseLeftButton();
		me->ReleaseRightButton();
		me->ReleaseBackwardButton();

		Vector away = me->GetAbsOrigin() - closestEnemy->GetAbsOrigin();

		me->GetLocomotionInterface()->Approach( me->GetLocomotionInterface()->GetFeet() + away );
	}
}


//-----------------------------------------------------------------------------------------
void CNEOBotTacticalMonitor::ReconConsiderSuperJump( CNEOBot *me )
{
	CNEO_Player *pNeoMe = ToNEOPlayer(me);
	if ( !pNeoMe || pNeoMe->GetClass() != NEO_CLASS_RECON )
	{
		return;
	}

	const CKnownEntity *pKnownThreat = me->GetVisionInterface()->GetPrimaryKnownThreat();
	if ( !pKnownThreat || !pKnownThreat->GetEntity() )
	{
		return;
	}

	CBaseEntity *pThreatEntity = pKnownThreat->GetEntity();

	if ( !pNeoMe->IsThreatFiringAtMe( pThreatEntity ) )
	{
		return;
	}

	// Don't want to waste aux jumping behind cover
	if ( !me->IsLineOfFireClear( pThreatEntity, CNEOBot::LINE_OF_FIRE_FLAGS_DEFAULT ) )
	{
		return;
	}

	// Check that bot isn't only moving sideways which wastes aux power
	if ( ( pNeoMe->m_nButtons & IN_SPEED ) == 0 || ( pNeoMe->m_nButtons & ( IN_FORWARD | IN_BACK ) ) == 0 )
	{
		return;
	}

	// Ceiling check
	const float ceilingCheckHeight = 128.0f;
	trace_t tr;
	Vector start = pNeoMe->GetAbsOrigin();
	Vector end = start + Vector( 0, 0, ceilingCheckHeight );
	UTIL_TraceLine( start, end, MASK_PLAYERSOLID, pNeoMe, COLLISION_GROUP_PLAYER_MOVEMENT, &tr );

	if ( tr.fraction < 1.0f )
	{
		return;
	}

	// Trajectory checks
	const float trajectoryCheckDistance = 256.0f;
	Vector forward;
	AngleVectors( pNeoMe->EyeAngles(), &forward );

	if (pNeoMe->m_nButtons & IN_FORWARD)
	{
		// Forward check
		Vector forwardDir = forward;
		forwardDir.z = 1.0f;
		forwardDir.NormalizeInPlace();
		end = start + forwardDir * trajectoryCheckDistance;
		UTIL_TraceLine(start, end, MASK_PLAYERSOLID, pNeoMe, COLLISION_GROUP_PLAYER_MOVEMENT, &tr);

		if (tr.fraction < 1.0f)
		{
			return;
		}

		// Feet check
		forward.z = 0;
		end = start + forward * trajectoryCheckDistance;
		UTIL_TraceLine(start, end, MASK_PLAYERSOLID, pNeoMe, COLLISION_GROUP_PLAYER_MOVEMENT, &tr);
		if (tr.fraction < 1.0f)
		{
			return;
		}
	}
	else if (pNeoMe->m_nButtons & IN_BACK)
	{
		// Backward check
		Vector backwardDir = -forward;
		backwardDir.z = 1.0f;
		backwardDir.NormalizeInPlace();
		end = start + backwardDir * trajectoryCheckDistance;
		UTIL_TraceLine(start, end, MASK_PLAYERSOLID, pNeoMe, COLLISION_GROUP_PLAYER_MOVEMENT, &tr);

		if (tr.fraction < 1.0f)
		{
			return;
		}

		// Feet check
		forward.z = 0;
		end = start - forward * trajectoryCheckDistance;
		UTIL_TraceLine(start, end, MASK_PLAYERSOLID, pNeoMe, COLLISION_GROUP_PLAYER_MOVEMENT, &tr);
		if (tr.fraction < 1.0f)
		{
			return;
		}
	}

	if (pNeoMe->IsAllowedToSuperJump())
	{
		me->GetLocomotionInterface()->Jump();
		// NEO Jank: It's easier to give bots a slight boost to compensate for their erratic movement inputs
		me->SuperJump();
	}
}


//-----------------------------------------------------------------------------------------
ActionResult< CNEOBot >	CNEOBotTacticalMonitor::Update( CNEOBot *me, float interval )
{
	if ( neo_bot_force_jump.GetBool() )
	{
		if ( !me->GetLocomotionInterface()->IsClimbingOrJumping() )
		{
			me->GetLocomotionInterface()->Jump();
		}
	}

	ReconConsiderSuperJump( me );

	const CKnownEntity *threat = me->GetVisionInterface()->GetPrimaryKnownThreat();

	// check if we need to get to cover
	QueryResultType shouldRetreat = me->GetIntentionInterface()->ShouldRetreat( me );

	if ( shouldRetreat == ANSWER_YES )
	{
		return SuspendFor( new CNEOBotRetreatToCover, "Backing off" );
	}
	else if ( shouldRetreat != ANSWER_NO )
	{
		// retreat if we need to do a full reload
		if ( me->IsDifficulty( CNEOBot::HARD ) || me->IsDifficulty( CNEOBot::EXPERT ) )
		{
			CNEOBaseCombatWeapon *weapon = (CNEOBaseCombatWeapon*) me->GetActiveWeapon();
			if ( weapon && weapon->GetPrimaryAmmoCount() > 0 && me->IsBarrageAndReloadWeapon(weapon))
			{
				if ( weapon->Clip1() <= 1 )
				{
					return SuspendFor( new CNEOBotRetreatToCover, "Moving to cover to reload" );
				}
			}
		}
	}

#if 0 // NEO TODO (Adam) search for dropped weapons to resupply ammunition
	bool isAvailable = ( me->GetIntentionInterface()->ShouldHurry( me ) != ANSWER_YES );

	// collect ammo and health kits, unless we're in a big hurry
	if ( isAvailable )
	{
		if ( m_maintainTimer.IsElapsed() )
		{
			m_maintainTimer.Start( RandomFloat( 0.3f, 0.5f ) );

			if ( me->IsAmmoLow() && CNEOBotGetAmmo::IsPossible( me ) )
			{
				return SuspendFor( new CNEOBotGetAmmo, "Grabbing nearby ammo" );
			}
		}
	}
#endif

#if 0 // NEO TODO (Adam) detonate remote detpacks
	// detonate sticky bomb traps when victims are near
	MonitorArmedStickyBombs( me );
#endif

	me->UpdateDelayedThreatNotices();

	return Continue();
}


//-----------------------------------------------------------------------------------------
EventDesiredResult< CNEOBot > CNEOBotTacticalMonitor::OnOtherKilled( CNEOBot *me, CBaseCombatCharacter *victim, const CTakeDamageInfo &info )
{
	return TryContinue();
}


//-----------------------------------------------------------------------------------------
EventDesiredResult< CNEOBot > CNEOBotTacticalMonitor::OnNavAreaChanged( CNEOBot *me, CNavArea *newArea, CNavArea *oldArea )
{
	return TryContinue();
}

//-----------------------------------------------------------------------------------------
EventDesiredResult< CNEOBot > CNEOBotTacticalMonitor::OnCommandString( CNEOBot *me, const char *command )
{
	if ( FStrEq( command, "despawn" ) )
	{
		return TrySuspendFor( new CNEODespawn(), RESULT_CRITICAL, "Received command to go to de-spawn" );
	}

	return TryContinue();
}
