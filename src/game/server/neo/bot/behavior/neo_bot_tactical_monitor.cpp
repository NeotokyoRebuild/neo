#include "cbase.h"
#include "fmtstr.h"

#include "neo_gamerules.h"
#include "nav_ladder.h"
#include "NextBot/NavMeshEntities/func_nav_prerequisite.h"

#include "bot/neo_bot.h"
#include "bot/neo_bot_manager.h"

#include "bot/behavior/neo_bot_detpack_trigger.h"
#include "bot/behavior/neo_bot_tactical_monitor.h"
#include "bot/behavior/neo_bot_scenario_monitor.h"

#include "bot/behavior/neo_bot_seek_and_destroy.h"
#include "bot/behavior/neo_bot_seek_weapon.h"
#include "bot/behavior/neo_bot_retreat_to_cover.h"
#include "bot/behavior/neo_bot_retreat_from_grenade.h"
#include "bot/behavior/neo_bot_ladder_approach.h"
#include "bot/behavior/neo_bot_ladder_climb.h"
#include "bot/behavior/neo_bot_path_clear_breakable.h"
#include "bot/behavior/neo_bot_pause.h"
#if 0 // NEO TODO (Adam) Fix picking up weapons, search for dropped weapons to pick up ammo
#include "bot/behavior/neo_bot_get_ammo.h"
#endif
#include "bot/behavior/nav_entities/neo_bot_nav_ent_destroy_entity.h"
#include "bot/behavior/nav_entities/neo_bot_nav_ent_move_to.h"
#include "bot/behavior/nav_entities/neo_bot_nav_ent_wait.h"
#include "neo/neo_player_shared.h"
#include "neo_detpack.h"
#include "weapon_detpack.h"
#include "weapon_ghost.h"
#include "nav_mesh.h"

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


//-----------------------------------------------------------------------------------------
ActionResult< CNEOBot > CNEOBotTacticalMonitor::MonitorArmedDetpack( CNEOBot *me )
{
	if ( !m_detpackCheckTimer.IsElapsed() )
	{
		return Continue();
	}
	m_detpackCheckTimer.Start( 0.2f );

	CWeaponDetpack *pDetWeapon = assert_cast<CWeaponDetpack*>( me->Weapon_OwnsThisType( "weapon_remotedet" ) );
	if ( !pDetWeapon || !pDetWeapon->m_bThisDetpackHasBeenThrown || pDetWeapon->m_bRemoteHasBeenTriggered )
	{
		return Continue();
	}

	CBaseEntity *pDetpackEnt = pDetWeapon->GetDetpackEntity();
	if ( !pDetpackEnt )
	{
		return Continue();
	}

	const Vector vecDetpackPos = pDetpackEnt->GetAbsOrigin();

	// Check if I am too close to the detpack
	if ( me->GetAbsOrigin().DistToSqr( vecDetpackPos ) <= Square( NEO_DETPACK_DAMAGE_RADIUS ) )
	{
		return Continue();
	}

	float flThresholdMultiplier;
	switch ( me->GetDifficulty() )
	{
	case CNEOBot::EASY:
		flThresholdMultiplier = 1.05f;
		break;
	case CNEOBot::NORMAL:
		flThresholdMultiplier = 0.95f;
		break;
	case CNEOBot::HARD:
		flThresholdMultiplier = 0.85f;
		break;
	case CNEOBot::EXPERT:
		flThresholdMultiplier = 0.75f;
		break;
	default:
		flThresholdMultiplier = 0.95f;
		break;
	}

	const float flMaxRadiusSq = Square( NEO_DETPACK_DAMAGE_RADIUS * flThresholdMultiplier );
	bool bShouldDetonate = false;

	// Check if any known threat or teammate is in range
	CUtlVector< CKnownEntity > knownVector;
	me->GetVisionInterface()->CollectKnownEntities( &knownVector );
	bool bIsTeamplay = NEORules()->IsTeamplay();

	for ( int i = 0; i < knownVector.Count(); ++i )
	{
		if ( knownVector[i].IsObsolete() )
		{
			continue;
		}

		CBaseEntity *pEntity = knownVector[i].GetEntity();
		if ( !pEntity )
		{
			continue;
		}

		if ( vecDetpackPos.DistToSqr( knownVector[i].GetLastKnownPosition() ) <= flMaxRadiusSq )
		{
			if ( bIsTeamplay && me->InSameTeam( pEntity ) )
			{
				// Teammate in blast radius
				return Continue();
			}

			// Enemy in range.
			bShouldDetonate = true;
		}
	}

	// Check if ghost carrier is in range
	if ( !bShouldDetonate )
	{
		if ( CWeaponGhost *pGhost = NEORules()->m_pGhost )
		{
			CBaseCombatCharacter *pGhostOwner = pGhost->GetOwner();
			if ( pGhostOwner && !me->InSameTeam( pGhostOwner ) )
			{
				if ( vecDetpackPos.DistToSqr( pGhostOwner->GetAbsOrigin() ) <= flMaxRadiusSq )
				{
					bShouldDetonate = true;
				}
			}
		}
	}

	if ( !bShouldDetonate )
	{
		if ( me->GetAudibleEnemySoundPos( vecDetpackPos, flMaxRadiusSq ) != CNEO_Player::VECTOR_INVALID_WAYPOINT )
		{
			bShouldDetonate = true;
		}
	}

	if ( bShouldDetonate )
	{
		return SuspendFor( new CNEOBotDetpackTrigger(), "Triggering detpack!" );
	}

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
void CNEOBotTacticalMonitor::AvoidBumpingFriends( CNEOBot *me )
{
	const float avoidRange = 32.0f;

	CUtlVector< CNEO_Player * > friendVector;
	CollectPlayers( &friendVector, me->GetTeamNumber(), COLLECT_ONLY_LIVING_PLAYERS );

	CNEO_Player *closestFriend = NULL;
	float closestRangeSq = avoidRange * avoidRange;

	for( int i=0; i<friendVector.Count(); ++i )
	{
		CNEO_Player *friendly = friendVector[i];

		if ( friendly == me->GetEntity() )
			continue;

		float rangeSq = ( friendly->GetAbsOrigin() - me->GetAbsOrigin() ).LengthSqr();
		if ( rangeSq < closestRangeSq )
		{
			closestFriend = friendly;
			closestRangeSq = rangeSq;
		}
	}

	if ( !closestFriend )
		return;

	// avoid unless hindrance returns a definitive "no"
	if ( me->GetIntentionInterface()->IsHindrance( me, closestFriend ) == ANSWER_UNDEFINED )
	{
		me->ReleaseForwardButton();
		me->ReleaseLeftButton();
		me->ReleaseRightButton();
		me->ReleaseBackwardButton();

		Vector away = me->GetAbsOrigin() - closestFriend->GetAbsOrigin();

		me->GetLocomotionInterface()->Approach( me->GetLocomotionInterface()->GetFeet() + away );
	}
}



//-----------------------------------------------------------------------------------------
ActionResult< CNEOBot > CNEOBotTacticalMonitor::WatchForLadders( CNEOBot *me )
{
	// Check if our current path has an approaching ladder segment
	const PathFollower *path = me->GetCurrentPath();
	if ( !path || !path->IsValid() )
	{
		return Continue();
	}

	const Path::Segment *goal = path->GetCurrentGoal();
	if ( !goal || !goal->ladder )
	{
		return Continue();
	}

	// We're approaching a ladder - check distance
	const float ladderApproachRange = CNEOBotLadderApproach::ALIGN_RANGE;
	bool goingUp = (goal->how == GO_LADDER_UP);
	Vector ladderPos = (goal->how == GO_LADDER_UP) 
		? goal->ladder->m_bottom 
		: goal->ladder->m_top;

	// Sometimes we accidentally run into a ladder without expecting to
	if ( me->IsOnLadder() )
	{
		return SuspendFor(
			new CNEOBotLadderClimb( goal->ladder, goingUp ),
			goingUp ? "Encountered ladder up" : "Encountered ladder down" 
		);
	}

	if ( me->GetAbsOrigin().DistToSqr( ladderPos ) < Square(ladderApproachRange) )
	{
		return SuspendFor( 
			new CNEOBotLadderApproach( goal->ladder, goingUp ), 
			goingUp ? "Approaching ladder up" : "Approaching ladder down" 
		);
	}

	return Continue();
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

	CBaseEntity *dangerousGrenade = CNEOBotRetreatFromGrenade::FindDangerousGrenade( me );
	if ( dangerousGrenade )
	{
		return SuspendFor( new CNEOBotRetreatFromGrenade( dangerousGrenade ), "Fleeing from grenade!" );
	}

	ActionResult< CNEOBot > result = WatchForLadders( me );
	if ( result.IsRequestingChange() )
	{
		return result;
	}

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

	if ( CBaseEntity *breakable = CNEOBotPathClearBreakable::GetBreakableInPath( me ) )
	{
		return SuspendFor( new CNEOBotPathClearBreakable( breakable ), "Clearing breakable in path" );
	}

	ActionResult< CNEOBot > scavengeResult = ScavengeForPrimaryWeapon( me );
	if ( scavengeResult.IsRequestingChange() )
	{
		return scavengeResult;
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

	ActionResult< CNEOBot > detpackResult = MonitorArmedDetpack( me );
	if ( detpackResult.IsRequestingChange() )
	{
		return detpackResult;
	}

#if 0 // NEO TODO (Adam) detonate remote detpacks
	// detonate sticky bomb traps when victims are near
	MonitorArmedStickyBombs( me );
#endif

	if ( !(me->m_nButtons & (IN_FORWARD | IN_BACK | IN_MOVELEFT | IN_MOVERIGHT | IN_USE)) )
	{
		AvoidBumpingFriends( me );
	}

	const CKnownEntity *threat = me->GetVisionInterface()->GetPrimaryKnownThreat();
	if ( !threat )
	{
		me->ReloadIfLowClip();
	}

	me->UpdateDelayedThreatNotices();

	return Continue();
}


//-----------------------------------------------------------------------------------------
ActionResult< CNEOBot > CNEOBotTacticalMonitor::ScavengeForPrimaryWeapon( CNEOBot *me )
{
	if ( me->Weapon_GetSlot( 0 ) )
	{
		return Continue();
	}

	if ( !m_maintainTimer.IsElapsed() )
	{
		return Continue();
	}
	m_maintainTimer.Start( 1.0f );
	
	// Look for any one valid primary weapon, then dispatch into behavior for more optimal search
	// true parameter: short-circuit the search if any valid primary weapon is found
	// We just want to sanity check if there's a valid weapon before suspending into the dedicated behavior
	if ( FindNearestPrimaryWeapon( me->GetAbsOrigin(), true ) )
	{
		return SuspendFor( new CNEOBotSeekWeapon, "Scavenging for a primary weapon" );
	}

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
