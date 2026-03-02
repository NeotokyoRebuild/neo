#include "cbase.h"
#include "fmtstr.h"

#include "neo_gamerules.h"
#include "nav_ladder.h"
#include "NextBot/NavMeshEntities/func_nav_prerequisite.h"

#include "bot/neo_bot.h"
#include "bot/neo_bot_manager.h"

#include "bot/behavior/neo_bot_tactical_monitor.h"
#include "bot/behavior/neo_bot_scenario_monitor.h"

#include "bot/behavior/neo_bot_seek_and_destroy.h"
#include "bot/behavior/neo_bot_seek_weapon.h"
#include "bot/behavior/neo_bot_retreat_to_cover.h"
#include "bot/behavior/neo_bot_retreat_from_grenade.h"
#include "bot/behavior/neo_bot_ladder_approach.h"
#include "bot/behavior/neo_bot_ladder_climb.h"
#include "bot/behavior/neo_bot_pause.h"
#if 0 // NEO TODO (Adam) Fix picking up weapons, search for dropped weapons to pick up ammo
#include "bot/behavior/neo_bot_get_ammo.h"
#endif
#include "bot/behavior/nav_entities/neo_bot_nav_ent_destroy_entity.h"
#include "bot/behavior/nav_entities/neo_bot_nav_ent_move_to.h"
#include "bot/behavior/nav_entities/neo_bot_nav_ent_wait.h"
#include "neo/neo_player_shared.h"
#include "nav_mesh.h"

ConVar neo_bot_force_jump( "neo_bot_force_jump", "0", FCVAR_CHEAT, "Force bots to continuously jump" );

ConVar neo_bot_scavenge_upgrade_delay( "neo_bot_scavenge_upgrade_delay", "4", FCVAR_GAMEDLL,
	"Delay in seconds between checking for a better weapon if the bot already has a primary weapon.", true, 1, false, 0 );

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

CNEOBotTacticalMonitor::CNEOBotTacticalMonitor()
{
	m_pIgnoredWeapons = std::make_unique<CNEOIgnoredWeaponsCache>();
}

CNEOBotTacticalMonitor::~CNEOBotTacticalMonitor() = default;


Action< CNEOBot > *CNEOBotTacticalMonitor::InitialContainedAction( CNEOBot *me )
{
	return new CNEOBotScenarioMonitor;
}


//-----------------------------------------------------------------------------------------
ActionResult< CNEOBot >	CNEOBotTacticalMonitor::OnStart( CNEOBot *me, Action< CNEOBot > *priorAction )
{
	m_pIgnoredWeapons->Reset();
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
	if ( !m_maintainTimer.IsElapsed() )
	{
		return Continue();
	}

	// Avoid swapping weapon in the middle of a fight
	CNEO_Player* pBotPlayer = ToNEOPlayer( me->GetEntity() );
	if ( pBotPlayer && pBotPlayer->GetTimeSinceWeaponFired() < 3.0f )
	{
		return Continue();
	}

	CBaseCombatWeapon *pPrimary = me->Weapon_GetSlot( 0 );
	const bool bHasPrimaryAmmo = ( pPrimary != nullptr && pPrimary->HasAnyAmmo() );
	const float flDelay = bHasPrimaryAmmo ? neo_bot_scavenge_upgrade_delay.GetFloat() : 1.0f;
	m_maintainTimer.Start( flDelay );
	
	CBaseEntity *pNearestWeapon = FindNearestPrimaryWeapon( me, false, m_pIgnoredWeapons.get() );
	if ( pNearestWeapon )
	{
		return SuspendFor( new CNEOBotSeekWeapon( pNearestWeapon, m_pIgnoredWeapons.get() ), "Scavenging for a new primary weapon" );
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
