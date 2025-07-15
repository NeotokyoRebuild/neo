// NextBotVisionInterface.cpp
// Implementation of common vision system
// Author: Michael Booth, May 2006
//========= Copyright Valve Corporation, All rights reserved. ============//

#include "cbase.h"

#include "nav.h"
#include "functorutils.h"

#include "NextBot.h"
#include "NextBotVisionInterface.h"
#include "NextBotBodyInterface.h"
#include "NextBotUtil.h"
#include "neo_player.h"

#ifdef TERROR
#include "querycache.h"
#endif

#include "tier0/vprof.h"

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"


ConVar nb_blind( "nb_blind", "0", FCVAR_CHEAT, "Disable vision" );
ConVar nb_debug_known_entities( "nb_debug_known_entities", "0", FCVAR_CHEAT, "Show the 'known entities' for the bot that is the current spectator target" );


//------------------------------------------------------------------------------------------
IVision::IVision( INextBot *bot ) : INextBotComponent( bot )
{ 
	Reset();
}


//------------------------------------------------------------------------------------------
/**
 * Reset to initial state
 */
void IVision::Reset( void )
{
	INextBotComponent::Reset();

	m_knownEntityVector.RemoveAll();
	m_lastVisionUpdateTimestamp = 0.0f;
	m_primaryThreat = NULL;

	m_FOV = GetDefaultFieldOfView();
	m_cosHalfFOV = cos( 0.5f * m_FOV * M_PI / 180.0f );
	
	for( int i=0; i<MAX_TEAMS; ++i )
	{
		m_notVisibleTimer[i].Invalidate();
	}
}


//------------------------------------------------------------------------------------------
/**
 * Determine if target can be seen despite obfuscation.
 */
bool IVision::IsPerceptiveOf(const CKnownEntity& known) const
{
	auto targetPlayer = static_cast<CNEO_Player*>(known.GetEntity());
	auto myPlayer = static_cast<CNEO_Player*>(GetBot()->GetEntity());
	if ((targetPlayer == nullptr) || (myPlayer == nullptr))
	{
		// If it's not a player, this cloaking logic doesn't apply, so it is always visible
		return true;
	}

	bool detected = true;
	auto targetIsCloaked = targetPlayer->GetCloakState();

	if (targetIsCloaked)
	{
		// --- Base Detection Chance (Per Tick) ---
		// This is the baseline chance of detection in ideal conditions (stationary, healthy, class-agnostic bot).
		// Aiming for ~5% detection per second at 66 ticks/sec, this translates to ~0.078% per tick.
		float detectionChance = 0.0008f; // Starting very low (0.08% per tick)

		// --- Multipliers for Detection Chance ---
		// Multipliers > 1.0 increase detection likelihood. Multipliers < 1.0 decrease it.

		// Target Movement Multipliers
		const float MULT_TARGET_WALKING = 20.0f;   // Walking increases detection chance by 20x
		const float MULT_TARGET_RUNNING = 50.0f;   // Running increases detection chance by 50x (very high risk)

		// Bot's State Multipliers (How the bot's state affects its perception)
		const float MULT_MY_PLAYER_MOVING = 0.5f;  // Bot moving: 50% less chance to detect (detectionChance *= 0.5)
		const float MULT_SUPPORT_BOT_VISION = 0.6; // Support bot: 40% less chance to detect (detectionChance *= 0.6)
		const float MIN_ASSAULT_DETECTION_CHANCE_PER_TICK = 0.20f; // 20% detection per tick (very high)

		// Injured Target Multiplier (How target's health affects their stealth)
		// Each health point lost increases detection by 1% of current chance (additive to multiplier)
		const float MULT_INJURED_PER_HEALTH_POINT_FACTOR = 0.01f;

		// Distance Multipliers (How distance affects detection)
		// These define ranges where detection scales.
		const float DISTANCE_MAX_DETECTION_SQ = 10000.0f;  // Max detection effect at 100 units (100^2)
		const float DISTANCE_MIN_DETECTION_SQ = 30000000.0f; // Min detection effect at 3000 units (3000^2)

		const float DISTANCE_MULT_CLOSE = 5.0f; // Multiplier when very close (e.g., within 100 units)
		const float DISTANCE_MULT_FAR = 0.01f;    // Multiplier when very far (e.g., beyond 3000 units)

		// --- Helper Lambdas for Movement ---
		auto isMoving = [](CNEO_Player* player, float tolerance = 10.0f) {
			return !player->GetAbsVelocity().IsZero(tolerance);
			};
		// Defined a clear threshold for 'running' velocity.
		auto isRunning = [](CNEO_Player* player, float runSpeedThreshold = 200.0f) {
			return player->GetAbsVelocity().LengthSqr() > (runSpeedThreshold * runSpeedThreshold);
			};

		bool myPlayerIsMoving = isMoving(myPlayer);
		bool targetIsMoving = isMoving(targetPlayer);
		bool targetIsRunning = isRunning(targetPlayer);

		// --- Apply Multipliers to Base Detection Chance ---

		// Player Movement Impact
		if (targetIsRunning) // Running is the most severe penalty
		{
			detectionChance *= MULT_TARGET_RUNNING;
		}
		else if (targetIsMoving) // Walking/strafing
		{
			detectionChance *= MULT_TARGET_WALKING;
		}

		// Bot Movement Impact
		if (myPlayerIsMoving)
		{
			detectionChance *= MULT_MY_PLAYER_MOVING;
		}

		// Distance Impact
		const Vector& myPos = GetBot()->GetPosition();
		float currentRangeSq = (known.GetLastKnownPosition() - myPos).LengthSqr();

		float distanceMultiplier;
		if (currentRangeSq <= DISTANCE_MAX_DETECTION_SQ) // Very close range
		{
			distanceMultiplier = DISTANCE_MULT_CLOSE;
		}
		else if (currentRangeSq >= DISTANCE_MIN_DETECTION_SQ) // Very far range
		{
			distanceMultiplier = DISTANCE_MULT_FAR;
		}
		else // Interpolate between max and min detection effects
		{
			// Alpha: 1.0 when at DISTANCE_MAX_DETECTION_SQ, 0.0 when at DISTANCE_MIN_DETECTION_SQ
			float alpha = 1.0f - ((currentRangeSq - DISTANCE_MAX_DETECTION_SQ) / (DISTANCE_MIN_DETECTION_SQ - DISTANCE_MAX_DETECTION_SQ));
			distanceMultiplier = DISTANCE_MULT_FAR * (1.0f - alpha) + DISTANCE_MULT_CLOSE * alpha;
		}
		detectionChance *= distanceMultiplier;

		// Class-specific Bot Perception
		if (myPlayer->GetClass() == NEO_CLASS_SUPPORT)
		{
			detectionChance *= MULT_SUPPORT_BOT_VISION;
		}

		// Injured Target Impact
		if (targetPlayer->GetHealth() < 100)
		{
			float healthDeficit = 100.0f - targetPlayer->GetHealth();
			detectionChance *= (1.0f + (healthDeficit * MULT_INJURED_PER_HEALTH_POINT_FACTOR));
		}

		// Assault class motion vision
		if (myPlayer->GetClass() == NEO_CLASS_ASSAULT && targetIsMoving)
		{
			detectionChance = fmaxf(detectionChance, MIN_ASSAULT_DETECTION_CHANCE_PER_TICK);
		}

		// Ensure the final detection chance is within valid bounds [0, 1] (as a ratio)
		detectionChance = fmaxf(0.0f, fminf(1.0f, detectionChance));

		// Convert to percentage for the RandomInt roll
		float detectionChancePercent = detectionChance * 100.0f;

		// Roll the chance to determine if the target is detected
		detected = (RandomInt(1, 100) <= detectionChancePercent);

		// --- Debugging Output ---
		if (GetBot()->IsDebugging(NEXTBOT_VISION))
		{
			bool meIsMovingDebug = isMoving(myPlayer);
			bool threatIsMovingDebug = isMoving(targetPlayer);
			bool threatIsRunningDebug = isRunning(targetPlayer);
			const Vector& myPosDebug = GetBot()->GetPosition();
			Vector toDebug = known.GetLastKnownPosition() - myPosDebug;
			float currentRangeSqDebug = toDebug.LengthSqr();
			float healthDeficitDebug = (targetPlayer->GetHealth() < 100) ? (100.0f - targetPlayer->GetHealth()) : 0.0f;

			// Recalculate multipliers for logging purposes (or store them)
			float log_player_move_mult = myPlayerIsMoving ? MULT_MY_PLAYER_MOVING : 1.0f;
			float log_target_move_mult = 1.0f;
			if (threatIsRunningDebug) log_target_move_mult = MULT_TARGET_RUNNING;
			else if (threatIsMovingDebug) log_target_move_mult = MULT_TARGET_WALKING;

			float log_dist_mult;
			if (currentRangeSqDebug <= DISTANCE_MAX_DETECTION_SQ) log_dist_mult = DISTANCE_MULT_CLOSE;
			else if (currentRangeSqDebug >= DISTANCE_MIN_DETECTION_SQ) log_dist_mult = DISTANCE_MULT_FAR;
			else {
				float alpha = 1.0f - ((currentRangeSqDebug - DISTANCE_MAX_DETECTION_SQ) / (DISTANCE_MIN_DETECTION_SQ - DISTANCE_MAX_DETECTION_SQ));
				log_dist_mult = DISTANCE_MULT_FAR * (1.0f - alpha) + DISTANCE_MULT_CLOSE * alpha;
			}

			float log_support_mult = (myPlayer->GetClass() == NEO_CLASS_SUPPORT) ? MULT_SUPPORT_BOT_VISION : 1.0f;
			float log_injured_mult = (targetPlayer->GetHealth() < 100) ? (1.0f + (healthDeficitDebug * MULT_INJURED_PER_HEALTH_POINT_FACTOR)) : 1.0f;

			float log_assault_override_val = (myPlayer->GetClass() == NEO_CLASS_ASSAULT && !myPlayerIsMoving && targetIsMoving) ? MIN_ASSAULT_DETECTION_CHANCE_PER_TICK * 100.0f : -1.0f;


			ConColorMsg(Color(0, 255, 0, 255), "%3.2f: %s Cloak Detection for %s(#%d):\n"
				"  - Cloaked: %s\n"
				"  - Me Moving: %s, Target Moving: %s (Running: %s)\n"
				"  - RangeSq: %.2f (Dist Mult: %.2fx)\n"
				"  - Health Deficit: %.2f (Injured Mult: %.2fx)\n"
				"  - Bot Class: %s (Class Mult: %.2fx)\n"
				"  - Base Chance (Per Tick): %.4f%%\n"
				"  - Applied Multipliers: BotMove: %.2fx, TargetMove: %.2fx, Distance: %.2fx, Class: %.2fx, Injured: %.2fx\n"
				"  - Assault Override Triggered: %s (Min Chance: %.2f%%)\n"
				"  - Final Detection Chance (Clamped): %.4f%%\n"
				"  - Detection Result: %s\n",
				gpGlobals->curtime,
				GetBot()->GetDebugIdentifier(),
				targetPlayer->GetClassname(),
				targetPlayer->entindex(),
				targetIsCloaked ? "TRUE" : "FALSE",
				meIsMovingDebug ? "TRUE" : "FALSE",
				threatIsMovingDebug ? "TRUE" : "FALSE",
				threatIsRunningDebug ? "TRUE" : "FALSE",
				currentRangeSqDebug,
				distanceMultiplier,
				healthDeficitDebug,
				log_injured_mult,
				(myPlayer->GetClass() == NEO_CLASS_ASSAULT) ? "ASSAULT" : ((myPlayer->GetClass() == NEO_CLASS_SUPPORT) ? "SUPPORT" : "OTHER"),
				log_support_mult, // This will be 1.0 for non-support, but 0.2 for support
				0.0008f * 100.0f, // Base chance for logging
				log_player_move_mult,
				log_target_move_mult,
				distanceMultiplier,
				log_support_mult,
				log_injured_mult,
				(log_assault_override_val != -1.0f) ? "TRUE" : "FALSE",
				MIN_ASSAULT_DETECTION_CHANCE_PER_TICK * 100.0f,
				detectionChancePercent,
				detected ? "DETECTED" : "NOT DETECTED"
			);

			NDebugOverlay::Line(GetBot()->GetBodyInterface()->GetEyePosition(), known.GetLastKnownPosition(), 255, 255, 0, false, 0.2f);
		}
		// --- End of Debugging Output ---
	}

	return detected;
}


//------------------------------------------------------------------------------------------
/**
 * Ask the current behavior to select the most dangerous threat from
 * our set of currently known entities
 * TODO: Find a semantically better place for this to live.
 */
const CKnownEntity *IVision::GetPrimaryKnownThreat( bool onlyVisibleThreats ) const
{
	if ( m_knownEntityVector.Count() == 0 )
		return NULL;

	const CKnownEntity *threat = NULL;
	int i;

	// find the first valid entity
	for( i=0; i<m_knownEntityVector.Count(); ++i )
	{
		const CKnownEntity &firstThreat = m_knownEntityVector[i];

		// check in case status changes between updates
		if ( IsAwareOf( firstThreat ) && !firstThreat.IsObsolete() && !IsIgnored( firstThreat.GetEntity() ) && GetBot()->IsEnemy( firstThreat.GetEntity() ) )
		{
			if ( !onlyVisibleThreats || firstThreat.IsVisibleRecently() )
			{
				if (IsPerceptiveOf(firstThreat))
				{
					threat = &firstThreat;
					break;
				}
			}
		}
	}

	if ( threat == NULL )
	{
		m_primaryThreat = NULL;
		return NULL;
	}

	for( ++i; i<m_knownEntityVector.Count(); ++i )
	{
		const CKnownEntity &newThreat = m_knownEntityVector[i];

		// check in case status changes between updates
		if ( IsAwareOf( newThreat ) && !newThreat.IsObsolete() && !IsIgnored( newThreat.GetEntity() ) && GetBot()->IsEnemy( newThreat.GetEntity() ) )
		{
			if ( !onlyVisibleThreats || newThreat.IsVisibleRecently() )
			{
				if (IsPerceptiveOf(newThreat))
				{
					threat = GetBot()->GetIntentionInterface()->SelectMoreDangerousThreat(GetBot(), GetBot()->GetEntity(), threat, &newThreat);
				}
			}
		}
	}

	// cache off threat
	m_primaryThreat = threat ? threat->GetEntity() : NULL;

	return threat;
}


//------------------------------------------------------------------------------------------
/**
 * Return the closest recognized entity
 */
const CKnownEntity *IVision::GetClosestKnown( int team ) const
{
	const Vector &myPos = GetBot()->GetPosition();
	
	const CKnownEntity *close = NULL;
	float closeRange = 999999999.9f;
	
	for( int i=0; i < m_knownEntityVector.Count(); ++i )
	{
		const CKnownEntity &known = m_knownEntityVector[i];

		if ( !known.IsObsolete() && IsAwareOf( known ) )
		{
			if ( team == TEAM_ANY || known.GetEntity()->GetTeamNumber() == team )
			{
				Vector to = known.GetLastKnownPosition() - myPos;
				float rangeSq = to.LengthSqr();
				
				if ( rangeSq < closeRange )
				{
					close = &known;
					closeRange = rangeSq;
				}
			}
		}
	}
	
	return close;
}


//------------------------------------------------------------------------------------------
/**
 * Return the closest recognized entity that passes the given filter
 */
const CKnownEntity *IVision::GetClosestKnown( const INextBotEntityFilter &filter ) const
{
	const Vector &myPos = GetBot()->GetPosition();

	const CKnownEntity *close = NULL;
	float closeRange = 999999999.9f;

	for( int i=0; i < m_knownEntityVector.Count(); ++i )
	{
		const CKnownEntity &known = m_knownEntityVector[i];

		if ( !known.IsObsolete() && IsAwareOf( known ) )
		{
			if ( filter.IsAllowed( known.GetEntity() ) )
			{
				Vector to = known.GetLastKnownPosition() - myPos;
				float rangeSq = to.LengthSqr();

				if ( rangeSq < closeRange )
				{
					close = &known;
					closeRange = rangeSq;
				}
			}
		}
	}

	return close;	
}


//------------------------------------------------------------------------------------------
/**
 * Given an entity, return our known version of it (or NULL if we don't know of it)
 */
const CKnownEntity *IVision::GetKnown( const CBaseEntity *entity ) const
{
	if ( entity == NULL )
		return NULL;

	for( int i=0; i < m_knownEntityVector.Count(); ++i )
	{
		const CKnownEntity &known = m_knownEntityVector[i];

		if ( known.GetEntity() && known.GetEntity()->entindex() == entity->entindex() && !known.IsObsolete() )
		{
			return &known;
		}
	}

	return NULL;
}


//------------------------------------------------------------------------------------------
/**
 * Introduce a known entity into the system. Its position is assumed to be known
 * and will be updated, and it is assumed to not yet have been seen by us, allowing for learning
 * of known entities by being told about them, hearing them, etc.
 */
void IVision::AddKnownEntity( CBaseEntity *entity )
{
	if ( entity == NULL || entity->IsWorld() )
	{
		// the world is not an entity we can deal with
		return;
	}

	CKnownEntity known( entity );

	// only add it if we don't already know of it
	if ( m_knownEntityVector.Find( known ) == m_knownEntityVector.InvalidIndex() )
	{
		m_knownEntityVector.AddToTail( known );
	}
}


//------------------------------------------------------------------------------------------
// Remove the given entity from our awareness (whether we know if it or not)
// Useful if we've moved to where we last saw the entity, but it's not there any longer.
void IVision::ForgetEntity( CBaseEntity *forgetMe )
{
	if ( !forgetMe )
		return;

	FOR_EACH_VEC( m_knownEntityVector, it )
	{
		const CKnownEntity &known = m_knownEntityVector[ it ];

		if ( known.GetEntity() && known.GetEntity()->entindex() == forgetMe->entindex() )
		{
			m_knownEntityVector.FastRemove( it );
			return;
		}
	}
}


//------------------------------------------------------------------------------------------
void IVision::ForgetAllKnownEntities( void )
{
	m_knownEntityVector.RemoveAll();
}


//------------------------------------------------------------------------------------------
/**
 * Return the number of entity on the given team known to us closer than rangeLimit
 */
int IVision::GetKnownCount( int team, bool onlyVisible, float rangeLimit ) const
{
	int count = 0;

	FOR_EACH_VEC( m_knownEntityVector, it )
	{
		const CKnownEntity &known = m_knownEntityVector[ it ];

		if ( !known.IsObsolete() && IsAwareOf( known ) )
		{
			if ( team == TEAM_ANY || known.GetEntity()->GetTeamNumber() == team )
			{
				if ( !onlyVisible || known.IsVisibleRecently() )
				{
					if ( rangeLimit < 0.0f || GetBot()->IsRangeLessThan( known.GetLastKnownPosition(), rangeLimit ) )
					{
						++count;
					}
				}
			}
		}
	}

	return count;
}


//------------------------------------------------------------------------------------------
class PopulateVisibleVector
{
public:
	PopulateVisibleVector( CUtlVector< CBaseEntity * > *potentiallyVisible )
	{
		m_potentiallyVisible = potentiallyVisible;
	}

	bool operator() ( CBaseEntity *actor )
	{
		m_potentiallyVisible->AddToTail( actor );
		return true;
	}

	CUtlVector< CBaseEntity * > *m_potentiallyVisible;
};


//------------------------------------------------------------------------------------------
/**
 * Populate "potentiallyVisible" with the set of all entities we could potentially see. 
 * Entities in this set will be tested for visibility/recognition in IVision::Update()
 */
void IVision::CollectPotentiallyVisibleEntities( CUtlVector< CBaseEntity * > *potentiallyVisible )
{
	potentiallyVisible->RemoveAll();

	// by default, only consider players and other bots as potentially visible
	PopulateVisibleVector populate( potentiallyVisible );
	ForEachActor( populate );
}


//------------------------------------------------------------------------------------------
class CollectVisible
{
public:
	CollectVisible( IVision *vision )
	{
		m_vision = vision;
	}
	
	bool operator() ( CBaseEntity *entity )
	{
		if ( entity &&
			 !m_vision->IsIgnored( entity ) &&
			 entity->IsAlive() &&
			 entity != m_vision->GetBot()->GetEntity() &&
			 m_vision->IsAbleToSee( entity, IVision::USE_FOV ) )
		{
			m_recognized.AddToTail( entity );	
		}
			
		return true;
	}
	
	bool Contains( CBaseEntity *entity ) const
	{
		for( int i=0; i < m_recognized.Count(); ++i )
		{
			if ( entity->entindex() == m_recognized[ i ]->entindex() )
			{
				return true;
			}
		}
		return false;
	}
	
	IVision *m_vision;
	CUtlVector< CBaseEntity * > m_recognized;
};


//------------------------------------------------------------------------------------------
void IVision::UpdateKnownEntities( void )
{
	VPROF_BUDGET( "IVision::UpdateKnownEntities", "NextBot" );

	// construct set of potentially visible objects
	CUtlVector< CBaseEntity * > potentiallyVisible;
	CollectPotentiallyVisibleEntities( &potentiallyVisible );

	// collect set of visible and recognized entities at this moment
	CollectVisible visibleNow( this );
	FOR_EACH_VEC( potentiallyVisible, pit )
	{
		VPROF_BUDGET( "IVision::UpdateKnownEntities( collect visible )", "NextBot" );

		if ( visibleNow( potentiallyVisible[ pit ] ) == false )
			break;
	}
	
	// update known set with new data
	{	VPROF_BUDGET( "IVision::UpdateKnownEntities( update status )", "NextBot" );

		int i;
		for( i=0; i < m_knownEntityVector.Count(); ++i )
		{
			CKnownEntity &known = m_knownEntityVector[i];

			// clear out obsolete knowledge
			if ( known.GetEntity() == NULL || known.IsObsolete() )
			{
				m_knownEntityVector.Remove( i );
				--i;
				continue;
			}
			
			if ( visibleNow.Contains( known.GetEntity() ) )
			{
				// this visible entity was already known (but perhaps not visible until now)
				known.UpdatePosition();
				known.UpdateVisibilityStatus( true );

				// has our reaction time just elapsed?
				if ( gpGlobals->curtime - known.GetTimeWhenBecameVisible() >= GetMinRecognizeTime() &&
					 m_lastVisionUpdateTimestamp - known.GetTimeWhenBecameVisible() < GetMinRecognizeTime() )
				{
					if ( GetBot()->IsDebugging( NEXTBOT_VISION ) )
					{
						ConColorMsg( Color( 0, 255, 0, 255 ), "%3.2f: %s caught sight of %s(#%d)\n", 
										gpGlobals->curtime,
										GetBot()->GetDebugIdentifier(),
										known.GetEntity()->GetClassname(),
										known.GetEntity()->entindex() );

						NDebugOverlay::Line( GetBot()->GetBodyInterface()->GetEyePosition(), known.GetLastKnownPosition(), 255, 255, 0, false, 0.2f );
					}

					GetBot()->OnSight( known.GetEntity() );
				}
		
				// restart 'not seen' timer
				m_notVisibleTimer[ known.GetEntity()->GetTeamNumber() ].Start();
			}
			else // known entity is not currently visible
			{
				if ( known.IsVisibleInFOVNow() )
				{
					// previously known and visible entity is now no longer visible
					known.UpdateVisibilityStatus( false );

					// lost sight of this entity
					if ( GetBot()->IsDebugging( NEXTBOT_VISION ) )
					{
						ConColorMsg( Color( 255, 0, 0, 255 ), "%3.2f: %s Lost sight of %s(#%d)\n", 
										gpGlobals->curtime,
										GetBot()->GetDebugIdentifier(),
										known.GetEntity()->GetClassname(),
										known.GetEntity()->entindex() );
					}

					GetBot()->OnLostSight( known.GetEntity() );
				}

				if ( !known.HasLastKnownPositionBeenSeen() )
				{
					// can we see the entity's last know position?
					if ( IsAbleToSee( known.GetLastKnownPosition(), IVision::USE_FOV ) )
					{
						known.MarkLastKnownPositionAsSeen();
					}
				}
			}
		}
	}
		
	// check for new recognizes that were not in the known set
	{	VPROF_BUDGET( "IVision::UpdateKnownEntities( new recognizes )", "NextBot" );

		int i, j;
		for( i=0; i < visibleNow.m_recognized.Count(); ++i )
		{	
			for( j=0; j < m_knownEntityVector.Count(); ++j )
			{
				if ( visibleNow.m_recognized[i] == m_knownEntityVector[j].GetEntity() )
				{
					break;
				}
			}
			
			if ( j == m_knownEntityVector.Count() )
			{
				// recognized a previously unknown entity (emit OnSight() event after reaction time has passed)
				CKnownEntity known( visibleNow.m_recognized[i] );
				known.UpdatePosition();
				known.UpdateVisibilityStatus( true );
				m_knownEntityVector.AddToTail( known );
			}
		}
	}

	// debugging
	if ( nb_debug_known_entities.GetBool() )
	{
		CBasePlayer *watcher = UTIL_GetListenServerHost();
		if ( watcher )
		{
			CBaseEntity *subject = watcher->GetObserverTarget();

			if ( subject && GetBot()->IsSelf( subject ) )
			{
				CUtlVector< CKnownEntity > knownVector;
				CollectKnownEntities( &knownVector );

				for( int i=0; i < knownVector.Count(); ++i )
				{
					CKnownEntity &known = knownVector[i];

					if ( GetBot()->IsFriend( known.GetEntity() ) )
					{
						if ( IsAwareOf( known ) )
						{
							if ( known.IsVisibleInFOVNow() )
								NDebugOverlay::HorzArrow( GetBot()->GetEntity()->GetAbsOrigin(), known.GetLastKnownPosition(), 5.0f, 0, 255, 0, 255, true, NDEBUG_PERSIST_TILL_NEXT_SERVER );
							else
								NDebugOverlay::HorzArrow( GetBot()->GetEntity()->GetAbsOrigin(), known.GetLastKnownPosition(), 2.0f, 0, 100, 0, 255, true, NDEBUG_PERSIST_TILL_NEXT_SERVER );
						}
						else
						{
							NDebugOverlay::HorzArrow( GetBot()->GetEntity()->GetAbsOrigin(), known.GetLastKnownPosition(), 1.0f, 0, 100, 0, 128, true, NDEBUG_PERSIST_TILL_NEXT_SERVER );
						}
					}
					else
					{
						if ( IsAwareOf( known ) )
						{
							if ( known.IsVisibleInFOVNow() )
								NDebugOverlay::HorzArrow( GetBot()->GetEntity()->GetAbsOrigin(), known.GetLastKnownPosition(), 5.0f, 255, 0, 0, 255, true, NDEBUG_PERSIST_TILL_NEXT_SERVER );
							else
								NDebugOverlay::HorzArrow( GetBot()->GetEntity()->GetAbsOrigin(), known.GetLastKnownPosition(), 2.0f, 100, 0, 0, 255, true, NDEBUG_PERSIST_TILL_NEXT_SERVER );
						}
						else
						{
							NDebugOverlay::HorzArrow( GetBot()->GetEntity()->GetAbsOrigin(), known.GetLastKnownPosition(), 1.0f, 100, 0, 0, 128, true, NDEBUG_PERSIST_TILL_NEXT_SERVER );
						}
					}
				}
			}
		}
	}
}


//------------------------------------------------------------------------------------------
/**
 * Update internal state
 */
void IVision::Update( void )
{
	VPROF_BUDGET( "IVision::Update", "NextBotExpensive" );

/* This adds significantly to bot's reaction times
	// throttle update rate
	if ( !m_scanTimer.IsElapsed() )
	{
		return;
	}

	m_scanTimer.Start( 0.5f * GetMinRecognizeTime() );
*/

	if ( nb_blind.GetBool() )
	{
		m_knownEntityVector.RemoveAll();
		return;
	}

	UpdateKnownEntities();

	m_lastVisionUpdateTimestamp = gpGlobals->curtime;
}


//------------------------------------------------------------------------------------------
bool IVision::IsAbleToSee( CBaseEntity *subject, FieldOfViewCheckType checkFOV, Vector *visibleSpot ) const
{
	VPROF_BUDGET( "IVision::IsAbleToSee", "NextBotExpensive" );

	if ( GetBot()->IsRangeGreaterThan( subject, GetMaxVisionRange() ) )
	{
		return false;
	}

	
	if ( GetBot()->GetEntity()->IsHiddenByFog( subject ) )
	{
		// lost in the fog
		return false;
	}

	if ( checkFOV == USE_FOV && !IsInFieldOfView( subject ) )
	{
		return false;
	}

	CBaseCombatCharacter *combat = subject->MyCombatCharacterPointer();
	if ( combat )
	{
		CNavArea *subjectArea = combat->GetLastKnownArea();
		CNavArea *myArea = GetBot()->GetEntity()->GetLastKnownArea();
		if ( myArea && subjectArea )
		{
			if ( !myArea->IsPotentiallyVisible( subjectArea ) )
			{
				// subject is not potentially visible, skip the expensive raycast
				return false;
			}
		}
	}

	// do actual line-of-sight trace
	if ( !IsLineOfSightClearToEntity( subject ) )
	{
		return false;
	}

	return IsVisibleEntityNoticed( subject );
}


//------------------------------------------------------------------------------------------
bool IVision::IsAbleToSee( const Vector &pos, FieldOfViewCheckType checkFOV ) const
{
	VPROF_BUDGET( "IVision::IsAbleToSee", "NextBotExpensive" );

	
	if ( GetBot()->IsRangeGreaterThan( pos, GetMaxVisionRange() ) )
	{
		return false;
	}

	if ( GetBot()->GetEntity()->IsHiddenByFog( pos ) )
	{
		// lost in the fog
		return false;
	}

	if ( checkFOV == USE_FOV && !IsInFieldOfView( pos ) )
	{
		return false;
	}

	// do actual line-of-sight trace	
	return IsLineOfSightClear( pos );
}


//------------------------------------------------------------------------------------------
/**
 * Angle given in degrees
 */
void IVision::SetFieldOfView( float horizAngle )
{
	m_FOV = horizAngle;
	m_cosHalfFOV = cos( 0.5f * m_FOV * M_PI / 180.0f );
}


//------------------------------------------------------------------------------------------
bool IVision::IsInFieldOfView( const Vector &pos ) const
{
#ifdef CHECK_OLD_CODE_AGAINST_NEW
	bool bCheck = PointWithinViewAngle( GetBot()->GetBodyInterface()->GetEyePosition(), pos, GetBot()->GetBodyInterface()->GetViewVector(), m_cosHalfFOV );
	Vector to = pos - GetBot()->GetBodyInterface()->GetEyePosition();
	to.NormalizeInPlace();
	
	float cosDiff = DotProduct( GetBot()->GetBodyInterface()->GetViewVector(), to );
	
	if ( ( cosDiff > m_cosHalfFOV ) != bCheck )
	{
		Assert(0);
		bool bCheck2 =
			PointWithinViewAngle( GetBot()->GetBodyInterface()->GetEyePosition(), pos, GetBot()->GetBodyInterface()->GetViewVector(), m_cosHalfFOV );

	}

	return ( cosDiff > m_cosHalfFOV );
#else
	return PointWithinViewAngle( GetBot()->GetBodyInterface()->GetEyePosition(), pos, GetBot()->GetBodyInterface()->GetViewVector(), m_cosHalfFOV );
#endif
	
	return true;
}


//------------------------------------------------------------------------------------------
bool IVision::IsInFieldOfView( CBaseEntity *subject ) const
{
	/// @todo check more points
	if ( IsInFieldOfView( subject->WorldSpaceCenter() ) )
	{
		return true;
	}

	return IsInFieldOfView( subject->EyePosition() );
}


//------------------------------------------------------------------------------------------
/**
 * Return true if the ray to the given point is unobstructed
 */
bool IVision::IsLineOfSightClear( const Vector &pos ) const
{
	VPROF_BUDGET( "IVision::IsLineOfSightClear", "NextBot" );
	VPROF_INCREMENT_COUNTER( "IVision::IsLineOfSightClear", 1 );

	trace_t result;
	NextBotVisionTraceFilter filter( GetBot()->GetEntity(), COLLISION_GROUP_NONE );
	
	UTIL_TraceLine( GetBot()->GetBodyInterface()->GetEyePosition(), pos, MASK_BLOCKLOS_AND_NPCS|CONTENTS_IGNORE_NODRAW_OPAQUE, &filter, &result );
	
	return ( result.fraction >= 1.0f && !result.startsolid );
}


//------------------------------------------------------------------------------------------
bool IVision::IsLineOfSightClearToEntity( const CBaseEntity *subject, Vector *visibleSpot ) const
{
#ifdef TERROR
	// TODO: Integration querycache & its dependencies

	VPROF_INCREMENT_COUNTER( "IVision::IsLineOfSightClearToEntity", 1 );
	VPROF_BUDGET( "IVision::IsLineOfSightClearToEntity", "NextBotSpiky" );

	bool bClear = IsLineOfSightBetweenTwoEntitiesClear( GetBot()->GetBodyInterface()->GetEntity(), EOFFSET_MODE_EYEPOSITION,
														subject, EOFFSET_MODE_WORLDSPACE_CENTER,
														subject, COLLISION_GROUP_NONE,
														MASK_BLOCKLOS_AND_NPCS|CONTENTS_IGNORE_NODRAW_OPAQUE, VisionTraceFilterFunction, 1.0 );

#ifdef USE_NON_CACHE_QUERY
	trace_t result;
	NextBotTraceFilterIgnoreActors filter( subject, COLLISION_GROUP_NONE );
	
	UTIL_TraceLine( GetBot()->GetBodyInterface()->GetEyePosition(), subject->WorldSpaceCenter(), MASK_BLOCKLOS_AND_NPCS|CONTENTS_IGNORE_NODRAW_OPAQUE, &filter, &result );
	Assert( result.DidHit() != bClear );
	if ( subject->IsPlayer() && ! bClear )
	{
		UTIL_TraceLine( GetBot()->GetBodyInterface()->GetEyePosition(), subject->EyePosition(), MASK_BLOCKLOS_AND_NPCS|CONTENTS_IGNORE_NODRAW_OPAQUE, &filter, &result );
		bClear = IsLineOfSightBetweenTwoEntitiesClear( GetBot()->GetEntity(),
													   EOFFSET_MODE_EYEPOSITION,
													   subject, EOFFSET_MODE_EYEPOSITION,
													   subject, COLLISION_GROUP_NONE,
													   MASK_BLOCKLOS_AND_NPCS|CONTENTS_IGNORE_NODRAW_OPAQUE, 
													   IgnoreActorsTraceFilterFunction, 1.0 );

		// this WILL assert - the query interface happens at a different time, and has hysteresis.
		Assert( result.DidHit() != bClear );
	}
#endif

	return bClear;

#else

	// TODO: Use plain-old traces until querycache/etc gets integrated
	VPROF_BUDGET( "IVision::IsLineOfSightClearToEntity", "NextBot" );

	trace_t result;
	NextBotTraceFilterIgnoreActors filter( subject, COLLISION_GROUP_NONE );

	UTIL_TraceLine( GetBot()->GetBodyInterface()->GetEyePosition(), subject->WorldSpaceCenter(), MASK_BLOCKLOS_AND_NPCS|CONTENTS_IGNORE_NODRAW_OPAQUE, &filter, &result );
	if ( result.DidHit() )
	{
		UTIL_TraceLine( GetBot()->GetBodyInterface()->GetEyePosition(), subject->EyePosition(), MASK_BLOCKLOS_AND_NPCS|CONTENTS_IGNORE_NODRAW_OPAQUE, &filter, &result );

		if ( result.DidHit() )
		{
			UTIL_TraceLine( GetBot()->GetBodyInterface()->GetEyePosition(), subject->GetAbsOrigin(), MASK_BLOCKLOS_AND_NPCS|CONTENTS_IGNORE_NODRAW_OPAQUE, &filter, &result );
		}
	}

	if ( visibleSpot )
	{
		*visibleSpot = result.endpos;
	}

	return ( result.fraction >= 1.0f && !result.startsolid );

#endif
}


//------------------------------------------------------------------------------------------
/**
 * Are we looking directly at the given position
 */
bool IVision::IsLookingAt( const Vector &pos, float cosTolerance ) const
{
	Vector to = pos - GetBot()->GetBodyInterface()->GetEyePosition();
	to.NormalizeInPlace();

	Vector forward;
	AngleVectors( GetBot()->GetEntity()->EyeAngles(), &forward );

	return DotProduct( to, forward ) > cosTolerance;
}


//------------------------------------------------------------------------------------------
/**
 * Are we looking directly at the given actor
 */
bool IVision::IsLookingAt( const CBaseCombatCharacter *actor, float cosTolerance ) const
{
	return IsLookingAt( actor->EyePosition(), cosTolerance );
}


