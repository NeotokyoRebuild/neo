#include "cbase.h"
#include "neo_player.h"
#include "neo_gamerules.h"
#include "neo_smokelineofsightblocker.h"
#include "bot/neo_bot.h"
#include "bot/behavior/neo_bot_attack.h"
#include "bot/behavior/neo_bot_grenade_dispatch.h"
#include "bot/behavior/neo_bot_retreat_to_cover.h"
#include "bot/neo_bot_path_compute.h"

#include "nav_mesh.h"
#include "debugoverlay_shared.h"

ConVar sv_neo_bot_attack_debug_cover("sv_neo_bot_attack_debug_cover", "0", FCVAR_CHEAT,
	"Draw debug overlays for bot attack/cover behavior", true, 0, true, 1);


//---------------------------------------------------------------------------------------------
// Enter the Attack behavior to chase and destroy primary known threat.
CNEOBotAttack::CNEOBotAttack( void ) : m_chasePath( ChasePath::LEAD_SUBJECT )
{
	m_bSawEnemySinceLastPathCompute = true; // force first path calculation
	m_attackCoverArea = nullptr;
	m_goalArea = nullptr;
}


//---------------------------------------------------------------------------------------------
// Enter the Attack behavior with a goal position to move towards while engaging enemies.
// Bot will disengage from threat when advancement towards the goal is uncontested.
//
// When using GetPrimaryKnownThreat(true), the true parameter must be used
// such that the threat must be visible to the bot before entering attack behavior,
// because the m_goalArea terminates this behavior when the enemy is out of sight.
// When GetPrimaryKnownThreat(false) is used, this behavior may redundantly ping pong with prior state,
// such as in cases where the enemy is not visible, and is known behind a wall, while being unreachable.
CNEOBotAttack::CNEOBotAttack( const Vector &goalPosition ) : m_chasePath( ChasePath::LEAD_SUBJECT )
{
	m_bSawEnemySinceLastPathCompute = true; // force first path calculation
	m_attackCoverArea = nullptr;
	m_goalArea = TheNavMesh->GetNearestNavArea( goalPosition );
}


//---------------------------------------------------------------------------------------------
ActionResult< CNEOBot >	CNEOBotAttack::OnStart( CNEOBot *me, Action< CNEOBot > *priorAction )
{
	m_path.SetMinLookAheadDistance( me->GetDesiredPathLookAheadRange() );
	return Continue();
}


//---------------------------------------------------------------------------------------------
// for finding cover closer to our attack destination
class CSearchForAttackCover : public ISearchSurroundingAreasFunctor
{
public:
	CSearchForAttackCover( CNEOBot *me, const CKnownEntity *threat, const CNavArea *goalArea = nullptr ) : m_me( me ), m_threat( threat )
	{
		m_attackCoverArea = nullptr;
		m_myArea = m_me->GetLastKnownArea();
		m_threatArea = threat->GetLastKnownArea();
		m_goalArea = goalArea ? goalArea : m_threatArea; // prioritize movement towards input goal area or threat
		m_myDistToGoalSq = m_goalArea ? ( m_goalArea->GetCenter() - m_me->GetAbsOrigin() ).LengthSqr() : 0;
		m_onStuckPenalty = neo_bot_path_reservation_onstuck_penalty.GetFloat();
	}

	virtual bool operator() ( CNavArea *baseArea, CNavArea *priorArea, float travelDistanceSoFar )
	{
		// return true to keep searching, false ends search usually implying that suitable cover was found

		if ( !m_goalArea || !m_threatArea || !m_myArea )
		{
			return false; // don't have enough context to search
		}

		CNavArea *area = static_cast<CNavArea *>(baseArea);
		if ( area == m_myArea )
		{
			return true; // skip our starting area
		}

		// Skip areas that are hazardous or where bots get stuck
		if ( neo_bot_path_reservation_enable.GetBool() )
		{
			int navAreaId = area->GetID();
			if (CNEOBotPathReservations()->IsAreaHazardous(navAreaId, m_me))
			{
				return true;
			}
			if (CNEOBotPathReservations()->GetAreaAvoidPenalty(navAreaId) >= m_onStuckPenalty)
			{
				return true;
			}
		}

		if ( m_attackCoverArea )
		{
			// If we already have a previous candidate cover area,
			// only consider this new candidate area if it's an improvement
			// as we assume earlier breadth first search nodes are closer to bot
			// and thus faster to reach for safety.
			if ( neo_bot_path_reservation_enable.GetBool() )
			{
				int candidateReservations = CNEOBotPathReservations()->GetPredictedFriendlyPathCount(area->GetID(), m_me->GetTeamNumber());
				int previousReservations = CNEOBotPathReservations()->GetPredictedFriendlyPathCount(m_attackCoverArea->GetID(), m_me->GetTeamNumber());
				if (candidateReservations >= previousReservations)
				{
					return true; // skip areas that have been reserved relatively more or equal by friendly bots
				}
			}
			// Fallback in case the path reservation system is disabled
			else if (area->GetPotentiallyVisibleAreaCount() >= m_attackCoverArea->GetPotentiallyVisibleAreaCount())
			{
				// Use potentially visible area count as a rough proxy for how exposed the area is
				// The downsides of this approach are:
				// * It doesn't consider whether the nav area is actually reachable
				// * It doesn't do anything to discourage friendlies from bunching up in the same area
				return true; // skip areas that are relatively more exposed
			}
		}

		float goalAreaDistanceSq = ( m_goalArea->GetCenter() - area->GetCenter() ).LengthSqr();
		if (goalAreaDistanceSq >= m_myDistToGoalSq)
		{
			// skip as this candidate area doesn't get us closer to destination
			return true;
		}

		if ( m_threatArea->IsPotentiallyVisible( area ) )
		{
			if ( m_me->GetClass() == NEO_CLASS_SUPPORT )
			{
				// Even if area does not have hard cover, see if Support can use smoke concealment
				CNEO_Player *pThreatPlayer = ToNEOPlayer( m_threat->GetEntity() );
				if ( pThreatPlayer && ( pThreatPlayer->GetClass() != NEO_CLASS_SUPPORT ) )
				{
					ScopedSmokeLOS smokeScope( false );

					Vector vecThreatEye = m_threat->GetLastKnownPosition() + pThreatPlayer->GetViewOffset();
					Vector vecCandidateArea = area->GetCenter() + m_me->GetViewOffset();

					CTraceFilterSimple filter( m_threat->GetEntity(), COLLISION_GROUP_NONE );
					trace_t trSmoke;
					UTIL_TraceLine( vecThreatEye, vecCandidateArea, MASK_BLOCKLOS, &filter, &trSmoke );
					trace_t trNormal;
					UTIL_TraceLine( vecThreatEye, vecCandidateArea, MASK_SOLID, &filter, &trNormal );

					if ( trSmoke.fraction < trNormal.fraction )
					{
						m_attackCoverArea = area;
						return false; // found smoke as concealment
					}
				}
			}
			else if (!m_threatArea->IsCompletelyVisible(area))
			{
				m_attackCoverArea = area;
			}

			return true; // search for potentially better cover
		}

		// found hard cover
		m_attackCoverArea = area;
		return false; // found suitable cover
	}

	virtual bool ShouldSearch( CNavArea *adjArea, CNavArea *currentArea, float travelDistanceSoFar )
	{
		if ( travelDistanceSoFar > 1000.0f )
		{
			return false;
		}

		if (!m_goalArea || !adjArea)
		{
			return false;
		}

		// The adjacent area to search should not be farther from the attack goal
		float adjGoalDistance = ( m_goalArea->GetCenter() - adjArea->GetCenter() ).LengthSqr();
		if ( adjGoalDistance > m_myDistToGoalSq )
		{
			return false; // Candidate adjacent area veers farther from goal
		}

		// The adjacent area to search should not be beyond the attack goal
		float adjMeDistance = ( m_me->GetAbsOrigin() - adjArea->GetCenter() ).LengthSqr();
		if ( adjMeDistance > m_myDistToGoalSq )
		{
			return false; // Candidate adjacent area is beyond goal distance
		}

		if (!currentArea)
		{
			return false;
		}

		// Don't consider areas that require jumping when engaging enemy
		return ( currentArea->ComputeAdjacentConnectionHeightChange( adjArea ) < m_me->GetLocomotionInterface()->GetStepHeight() );
	}

	CNEOBot *m_me;
	const CKnownEntity *m_threat;
	const CNavArea *m_attackCoverArea;
	const CNavArea *m_goalArea;   // reference point of the optional goal direction
	const CNavArea *m_myArea;     // reference point of myself
	const CNavArea *m_threatArea; // reference point of the threat
	float m_myDistToGoalSq;       // the bot's current distance to the threat
	float m_onStuckPenalty;       // cache onstuck penalty
};


//---------------------------------------------------------------------------------------------
// head aiming and weapon firing is handled elsewhere - we just need to get into position to fight
ActionResult< CNEOBot >	CNEOBotAttack::Update( CNEOBot *me, float interval )
{
	const CKnownEntity *threat = me->GetVisionInterface()->GetPrimaryKnownThreat();

	if ( threat == nullptr || threat->IsObsolete() || !me->GetIntentionInterface()->ShouldAttack( me, threat ) )
	{
		return Done( "No threat" );
	}

	const Vector& threatLastKnownPos = threat->GetLastKnownPosition();

	if ( sv_neo_bot_attack_debug_cover.GetBool() )
	{
		// red - last known position of the threat
		NDebugOverlay::HorzArrow( me->GetAbsOrigin(), threatLastKnownPos, 2.0f, 255, 0, 0, 255, true, 0.1f );
		NDebugOverlay::Box( threatLastKnownPos, Vector( -16, -16, 0 ), Vector( 16, 16, 2 ), 255, 0, 0, 30, 0.1f );

		if ( m_attackCoverArea ) // yellow - cover area that bot has selected to attempt moving towards
		{
			const Vector& coverPos = m_attackCoverArea->GetCenter();
			NDebugOverlay::HorzArrow( me->GetAbsOrigin(), coverPos, 2.0f, 255, 255, 0, 255, true, 0.1f );
			m_attackCoverArea->DrawFilled( 255, 255, 0, 30, 0.1f, true );
		}

		if (m_goalArea) // green - the goal direction the bot is trying to advance towards
		{
			const Vector& goalPos = m_goalArea->GetCenter();
			NDebugOverlay::HorzArrow( me->GetAbsOrigin(), goalPos, 2.0f, 0, 255, 0, 255, true, 0.1f );
			m_goalArea->DrawFilled( 0, 255, 0, 30, 0.1f, true );
		}
	}

	CNEO_Player *pThreatPlayer = ToNEOPlayer( threat->GetEntity() );

	CNEOBaseCombatWeapon* myWeapon = static_cast<CNEOBaseCombatWeapon* >( me->GetActiveWeapon() );
	bool isUsingCloseRangeWeapon = me->IsCloseRange( myWeapon );

	if (!m_attackCoverArea // don't slow movement to cover with strafing
		&& isUsingCloseRangeWeapon && threat->IsVisibleRecently() && me->IsRangeLessThan( threatLastKnownPos, 1.1f * me->GetDesiredAttackRange() ) )
	{
		// circle around our victim
		if ( me->TransientlyConsistentRandomValue( 3.0f ) < 0.5f )
		{
			me->PressLeftButton();
		}
		else
		{
			me->PressRightButton();
		}
	}

	if ( !me->IsRanged(myWeapon) || ( pThreatPlayer && pThreatPlayer->IsCarryingGhost() ) )
	{
		m_path.Invalidate();
		CNEOBotPathUpdateChase( me, m_chasePath, threat->GetEntity(), FASTEST_ROUTE );
		return Continue();
	}

	if ( me->GetVisionInterface()->IsAbleToSee( threat->GetEntity(), CNEOBotVision::DISREGARD_FOV ) )
	{
		m_bSawEnemySinceLastPathCompute = true;

		me->EnableCloak(3.0f);

		if ( me->GetLastKnownArea() == m_goalArea )
		{
			return ChangeTo( new CNEOBotRetreatToCover(), "Taking cover as enemy is still present at my goal" );
		}
		else if ( myWeapon && (myWeapon->GetNeoWepBits() & NEO_WEP_SCOPEDWEAPON) && 
			me->IsRangeLessThan( threatLastKnownPos, me->GetDesiredAttackRange() ) )
		{
			return SuspendFor( new CNEOBotRetreatToCover(), "Retreating to cover to prepare next sniper shot" );
		}
	}
	else
	{
		// SUPA7 reload can be interrupted so proactively reload
		if (!m_bSawEnemySinceLastPathCompute // don't interrupt shooting with reloads if seen enemy on this path
			&& myWeapon && (myWeapon->GetNeoWepBits() & NEO_WEP_SUPA7) && (myWeapon->Clip1() < myWeapon->GetMaxClip1()))
		{
			me->ReloadIfLowClip(true);
		}

		if ( me->GetLastKnownArea() == threat->GetLastKnownArea()
			|| me->IsRangeLessThan( threatLastKnownPos, 64.0f ) )
		{
			me->GetVisionInterface()->ForgetEntity( threat->GetEntity() );
			return Done( "I lost my target!" );
		}
		else if ( m_goalArea && !m_bSawEnemySinceLastPathCompute &&
			( me->GetLastKnownArea() == m_attackCoverArea
			|| me->GetLastKnownArea() == m_goalArea ) )
		{
			return Done( "Disengaging from threat to pursue goal." );
		}
		
		if ( threat->IsVisibleRecently() )
		{
			me->EnableCloak(3.0f);
			
			// Consider throwing a grenade
			if ( !m_grenadeThrowCooldownTimer.HasStarted() || m_grenadeThrowCooldownTimer.IsElapsed() )
			{
				Action<CNEOBot> *pGrenadeBehavior = CNEOBotGrenadeDispatch::ChooseGrenadeThrowBehavior( me, threat );
				if ( pGrenadeBehavior )
				{
					m_grenadeThrowCooldownTimer.Start( sv_neo_bot_grenade_throw_cooldown.GetFloat() );
					return SuspendFor( pGrenadeBehavior, "Throwing grenade before chasing threat!" );
				}
			}

			// look where we last saw threat as we approach
			if ( me->IsRangeLessThan( threatLastKnownPos, me->GetMaxAttackRange() ) )
			{
				me->GetBodyInterface()->AimHeadTowards( threatLastKnownPos + Vector( 0, 0, HumanEyeHeight ), IBody::IMPORTANT, 0.1f, nullptr, "Looking towards where we lost sight of our victim" );
			}
		}
		else
		{
			// pre-cloak needs more thermoptic budget when chasing threats
			me->EnableCloak(6.0f);
		}
	}

	// Consider if there is cover between me and goal to leapfrog to
	if ( m_bSawEnemySinceLastPathCompute &&
		(!m_attackCoverArea || (me->GetLastKnownArea() == m_attackCoverArea)) )
	{
		m_bSawEnemySinceLastPathCompute = false;
		CSearchForAttackCover search( me, threat, m_goalArea );
		SearchSurroundingAreas( me->GetLastKnownArea(), search );

		if ( search.m_attackCoverArea )
		{
			m_attackCoverArea = search.m_attackCoverArea;
			m_chasePath.Invalidate();
		}
		else if (m_goalArea)
		{
			// Even if we bounce back to Attack, goal position may get refreshed in prior behavior
			return Done( "Reconsidering goal: Failed to find cover towards goal." );
		}
		else
		{
			m_attackCoverArea = nullptr;
		}
		m_path.Invalidate();
	}

	if ( m_attackCoverArea && !m_path.IsValid() )
	{
		if ( CNEOBotPathCompute( me, m_path, m_attackCoverArea->GetCenter(), DEFAULT_ROUTE ) )
		{
			// Disable chase follower to move to cover area
			m_chasePath.Invalidate();
		}
		else
		{
			// If no valid path, fallback to chasing threat
			m_attackCoverArea = nullptr;
			m_path.Invalidate();
		}
	}

	if (m_path.IsValid())
	{
		m_path.Update(me);

		if (m_path.GetEndPosition().DistTo(me->GetAbsOrigin()) <= 64.0f)
		{
			m_path.Invalidate();
			m_attackCoverArea = nullptr;
		}
	}
	else if (m_goalArea)
	{
		return Done( "Reconsidering goal: No path to cover found." );
	}
	else
	{
		// Directly chase threat if no cover was found or haven't seen enemy since last path compute
		m_attackCoverArea = nullptr;
		m_bSawEnemySinceLastPathCompute = false; // delay next cover search
		if ( isUsingCloseRangeWeapon )
		{
			CNEOBotPathUpdateChase( me, m_chasePath, threat->GetEntity(), FASTEST_ROUTE );
		}
		else
		{
			CNEOBotPathUpdateChase( me, m_chasePath, threat->GetEntity(), DEFAULT_ROUTE );
		}
	}

	return Continue();
}


//---------------------------------------------------------------------------------------------
EventDesiredResult< CNEOBot > CNEOBotAttack::OnStuck( CNEOBot *me )
{
	m_path.Invalidate();
	m_attackCoverArea = nullptr;
	CNEOBotPathReservations()->IncrementAreaAvoidPenalty(me->GetLastKnownArea()->GetID(), neo_bot_path_reservation_onstuck_penalty.GetFloat());
	return TryContinue();
}


//---------------------------------------------------------------------------------------------
EventDesiredResult< CNEOBot > CNEOBotAttack::OnMoveToSuccess( CNEOBot *me, const Path *path )
{
	return TryContinue();
}


//---------------------------------------------------------------------------------------------
EventDesiredResult< CNEOBot > CNEOBotAttack::OnMoveToFailure( CNEOBot *me, const Path *path, MoveToFailureType reason )
{
	m_path.Invalidate();
	m_attackCoverArea = nullptr;
	CNEOBotPathReservations()->IncrementAreaAvoidPenalty(me->GetLastKnownArea()->GetID(), neo_bot_path_reservation_onstuck_penalty.GetFloat());
	return TryContinue();
}


//---------------------------------------------------------------------------------------------
QueryResultType	CNEOBotAttack::ShouldRetreat( const INextBot *me ) const
{

	const CNEOBot *meNeoBot = static_cast<const CNEOBot *>(me);
	CNEOBaseCombatWeapon* myWeapon = static_cast<CNEOBaseCombatWeapon* >( meNeoBot->GetActiveWeapon() );
	if ( !meNeoBot->IsRanged(myWeapon) )
	{
		return ANSWER_NO;
	}

	if (myWeapon)
	{
		if (myWeapon->m_bInReload || (myWeapon->Clip1() <= 1))
		{
			return ANSWER_YES;
		}
	}

	if (!m_bSawEnemySinceLastPathCompute)
	{
		return ANSWER_UNDEFINED;
	}

	return ANSWER_UNDEFINED;
}


//---------------------------------------------------------------------------------------------
QueryResultType CNEOBotAttack::ShouldHurry( const INextBot *me ) const
{
	return ANSWER_UNDEFINED;
}

