#include "cbase.h"
#include "neo_player.h"
#include "neo_gamerules.h"
#include "neo_smokelineofsightblocker.h"
#include "team_control_point_master.h"
#include "bot/neo_bot.h"
#include "bot/behavior/neo_bot_attack.h"
#include "bot/behavior/neo_bot_grenade_dispatch.h"
#include "bot/neo_bot_path_compute.h"

#include "nav_mesh.h"

extern ConVar neo_bot_path_lookahead_range;
extern ConVar neo_bot_offense_must_push_time;

ConVar neo_bot_aggressive( "neo_bot_aggressive", "0", FCVAR_NONE );

//---------------------------------------------------------------------------------------------
CNEOBotAttack::CNEOBotAttack( void ) : m_chasePath( ChasePath::LEAD_SUBJECT )
{
	m_attackCoverArea = nullptr;
}


//---------------------------------------------------------------------------------------------
ActionResult< CNEOBot >	CNEOBotAttack::OnStart( CNEOBot *me, Action< CNEOBot > *priorAction )
{
	m_path.SetMinLookAheadDistance( me->GetDesiredPathLookAheadRange() );
	m_attackCoverTimer.Invalidate();

	return Continue();
}


//---------------------------------------------------------------------------------------------
// for finding cover closer to our threat
class CSearchForAttackCover : public ISearchSurroundingAreasFunctor
{
public:
	CSearchForAttackCover( CNEOBot *me, const CKnownEntity *threat ) : m_me( me ), m_threat( threat )
	{
		m_attackCoverArea = nullptr;
		m_threatArea = threat->GetLastKnownArea();
		m_distToThreatSq = ( threat->GetLastKnownPosition() - me->GetAbsOrigin() ).LengthSqr();
	}

	virtual bool operator() ( CNavArea *baseArea, CNavArea *priorArea, float travelDistanceSoFar )
	{
		// return true to keep searching, false when suitable cover is found
		CNavArea *area = static_cast<CNavArea *>(baseArea);

		if ( !m_threatArea )
		{
			return false; // threat area is unknown
		}

		constexpr float distanceThresholdRatio = 1.2f;
		float candidateDistFromMeSq = ( m_me->GetAbsOrigin() - area->GetCenter() ).LengthSqr();
		if ( candidateDistFromMeSq > m_distToThreatSq * distanceThresholdRatio )
		{
			return true; // not close enough to justify rerouting
		}

		if ( area->IsPotentiallyVisible( m_threatArea ) )
		{
			// Even if area does not have hard cover, see if Support can use smoke concealment
			if ( m_me->GetClass() == NEO_CLASS_SUPPORT )
			{
				CNEO_Player *pThreatPlayer = ToNEOPlayer( m_threat->GetEntity() );
				if ( pThreatPlayer && ( pThreatPlayer->GetClass() != NEO_CLASS_SUPPORT ) )
				{
					ScopedSmokeLOS smokeScope( false );

					Vector vecThreatEye = m_threat->GetLastKnownPosition() + pThreatPlayer->GetViewOffset();
					Vector vecCandidateArea = area->GetCenter() + m_me->GetViewOffset();

					trace_t tr;
					CTraceFilterSimple filter( m_threat->GetEntity(), COLLISION_GROUP_NONE );
					UTIL_TraceLine( vecThreatEye, vecCandidateArea, MASK_BLOCKLOS, &filter, &tr );

					if ( tr.fraction < 1.0f )
					{
						m_attackCoverArea = area;
						return false; // found smoke as concealment
					}
				}
			}

			return true; // not cover
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

		// For considering areas off to the side of current area
		constexpr float distanceThresholdRatio = 0.9f;

		// The adjacent area to search should not be farther from the threat
		float adjThreatDistance = ( m_threatArea->GetCenter() - adjArea->GetCenter() ).LengthSqr();
		float curThreatDistance = ( m_threatArea->GetCenter() - currentArea->GetCenter() ).LengthSqr();
		if ( adjThreatDistance * distanceThresholdRatio > curThreatDistance )
		{
			return false; // Candidate adjacent area veers farther from threat
		}

		// The adjacent area to search should not be beyond the threat
		if ( adjThreatDistance > m_distToThreatSq )
		{
			return false; // Candidate adjacent area is beyond threat distance
		}

		// Don't consider areas that require jumping when engaging enemy
		return ( currentArea->ComputeAdjacentConnectionHeightChange( adjArea ) < m_me->GetLocomotionInterface()->GetStepHeight() );
	}

	CNEOBot *m_me;
	const CKnownEntity *m_threat;
	CNavArea *m_attackCoverArea;
	const CNavArea *m_threatArea; // reference point of the threat
	float m_distToThreatSq; // the bot's current distance to the threat
};


//---------------------------------------------------------------------------------------------
// head aiming and weapon firing is handled elsewhere - we just need to get into position to fight
ActionResult< CNEOBot >	CNEOBotAttack::Update( CNEOBot *me, float interval )
{
	const CKnownEntity *threat = me->GetVisionInterface()->GetPrimaryKnownThreat();

	if ( threat == NULL || threat->IsObsolete() || !me->GetIntentionInterface()->ShouldAttack( me, threat ) )
	{
		return Done( "No threat" );
	}

	CNEOBaseCombatWeapon* myWeapon = static_cast<CNEOBaseCombatWeapon* >( me->GetActiveWeapon() );
	bool isUsingCloseRangeWeapon = me->IsCloseRange( myWeapon );
	if ( isUsingCloseRangeWeapon && threat->IsVisibleRecently() && me->IsRangeLessThan( threat->GetLastKnownPosition(), 1.1f * me->GetDesiredAttackRange() ) )
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

	bool bHasRangedWeapon = me->IsRanged( myWeapon );

	// Go after them!
	bool bAggressive = neo_bot_aggressive.GetBool() &&
					   !bHasRangedWeapon &&
					   me->GetDifficulty() > CNEOBot::EASY;

	// pursue the threat. if not visible, go to the last known position
	if ( bAggressive ||
	     !threat->IsVisibleRecently() || 
		 me->IsRangeGreaterThan( threat->GetEntity()->GetAbsOrigin(), me->GetDesiredAttackRange() ) || 
		 !me->IsLineOfFireClear( threat->GetEntity()->EyePosition(), CNEOBot::LINE_OF_FIRE_FLAGS_DEFAULT ) )
	{
		// SUPA7 reload can be interrupted so proactively reload
		if (myWeapon && (myWeapon->GetNeoWepBits() & NEO_WEP_SUPA7) && (myWeapon->Clip1() < myWeapon->GetMaxClip1()))
		{
			me->ReloadIfLowClip(true);
		}
		
		if ( threat->IsVisibleRecently() )
		{
			// pre-cloak needs more thermoptic budget when chasing threats
			me->EnableCloak(6.0f);

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

			// Look for opportunities to leap frog from cover to cover
			if ( m_attackCoverTimer.IsElapsed() )
			{
				m_attackCoverTimer.Start( 5.0f );

				CSearchForAttackCover search( me, threat );
				SearchSurroundingAreas( me->GetLastKnownArea(), search );

				if ( search.m_attackCoverArea )
				{
					m_attackCoverArea = search.m_attackCoverArea;
					m_path.Invalidate(); // recompute path
					m_chasePath.Invalidate();
				}
			}

			if ( m_attackCoverArea )
			{
				if ( me->GetLastKnownArea() == m_attackCoverArea )
				{
					// Immediately look for new cover
					m_attackCoverArea = nullptr;
					m_attackCoverTimer.Invalidate();
				}
				else
				{
					if ( !m_path.IsValid() )
					{
						if ( !CNEOBotPathCompute( me, m_path, m_attackCoverArea->GetCenter(), DEFAULT_ROUTE ) )
						{
							// If no valid path, fallback to chasing threat
							m_attackCoverArea = nullptr;
							m_path.Invalidate();
						}
					}

					if ( m_attackCoverArea )
					{
						m_path.Update( me );
						return Continue();
					}
				}
			}
			else
			{
				m_path.Invalidate();
			}

			// Fallback when there is no advancing cover
			if ( isUsingCloseRangeWeapon )
			{
				CNEOBotPathUpdateChase( me, m_chasePath, threat->GetEntity(), FASTEST_ROUTE );
			}
			else
			{
				CNEOBotPathUpdateChase( me, m_chasePath, threat->GetEntity(), DEFAULT_ROUTE );
			}
		}
		else
		{
			// if we're at the threat's last known position and he's still not visible, we lost him
			m_chasePath.Invalidate();

			if ( me->IsRangeLessThan( threat->GetLastKnownPosition(), 20.0f ) )
			{
				me->GetVisionInterface()->ForgetEntity( threat->GetEntity() );
				return Done( "I lost my target!" );
			}

			// look where we last saw him as we approach
			if ( me->IsRangeLessThan( threat->GetLastKnownPosition(), me->GetMaxAttackRange() ) )
			{
				me->GetBodyInterface()->AimHeadTowards( threat->GetLastKnownPosition() + Vector( 0, 0, HumanEyeHeight ), IBody::IMPORTANT, 0.2f, NULL, "Looking towards where we lost sight of our victim" );
			}

			m_path.Update( me );

			if ( m_repathTimer.IsElapsed() )
			{
				//m_repathTimer.Start( RandomFloat( 0.3f, 0.5f ) );
				m_repathTimer.Start( RandomFloat( 3.0f, 5.0f ) );

				if ( isUsingCloseRangeWeapon )
				{
					CNEOBotPathCompute( me, m_path, threat->GetLastKnownPosition(), FASTEST_ROUTE );
				}
				else
				{
					CNEOBotPathCompute( me, m_path, threat->GetLastKnownPosition(), DEFAULT_ROUTE );
				}
			}
		}
	}

	return Continue();
}


//---------------------------------------------------------------------------------------------
EventDesiredResult< CNEOBot > CNEOBotAttack::OnStuck( CNEOBot *me )
{
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
	return TryContinue();
}


//---------------------------------------------------------------------------------------------
QueryResultType	CNEOBotAttack::ShouldRetreat( const INextBot *me ) const
{
	return ANSWER_UNDEFINED;
}


//---------------------------------------------------------------------------------------------
QueryResultType CNEOBotAttack::ShouldHurry( const INextBot *me ) const
{
	return ANSWER_UNDEFINED;
}

