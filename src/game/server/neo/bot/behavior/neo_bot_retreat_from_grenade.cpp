
#include "cbase.h"
#include "neo_player.h"
#include "bot/neo_bot.h"
#include "bot/behavior/neo_bot_retreat_from_grenade.h"
#include "bot/behavior/neo_bot_retreat_to_cover.h"
#include "bot/neo_bot_path_compute.h"
#include "bot/neo_bot_path_reservation.h"
#include "nav_mesh.h"
#include "sdk/sdk_basegrenade_projectile.h"
#include "movevars_shared.h"

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

extern ConVar sv_neo_bot_grenade_frag_safety_range_multiplier;
extern ConVar sv_neo_grenade_fuse_timer;
extern ConVar sv_neo_grenade_blast_radius;
extern ConVar sv_neo_grenade_gravity;
extern ConVar sv_neo_grenade_cor;
ConVar neo_bot_retreat_from_grenade_range( "neo_bot_retreat_from_grenade_range", "2000", FCVAR_CHEAT );
ConVar neo_bot_debug_retreat_from_grenade( "neo_bot_debug_retreat_from_grenade", "0", FCVAR_CHEAT );
ConVar neo_bot_grenade_check_radius( "neo_bot_grenade_check_radius", "500", FCVAR_CHEAT );

// Extra travel charged for each known enemy that could see a cover area: hiding from the
// grenade comes first, hiding from enemies breaks ties between nearby choices
static constexpr float kCoverEnemyExposureCost = 300.0f;
static constexpr float kMaxGrenadeElasticity = 0.9f; // CBaseGrenadeProjectile::ResolveFlyCollisionCustom clamp
static constexpr float kSqrt2 = 1.41421356f; // M_SQRT2 is not defined by MSVC


//---------------------------------------------------------------------------------------------
// In flight a smoke looks like a frag, unless we know who threw it
static bool IsUnidentifiedSmoke( CNEOBot *me, CBaseEntity *grenade )
{
	if ( !FClassnameIs( grenade, "neo_grenade_smoke" ) )
	{
		return false;
	}

	CBaseCombatCharacter *thrower = static_cast< CBaseGrenade * >( grenade )->GetThrower();
	if ( !thrower )
	{
		return true;
	}

	if ( thrower == me )
	{
		return false;
	}

	if ( NEORules()->IsTeamplay() && thrower->GetTeamNumber() == me->GetTeamNumber() )
	{
		return false;
	}

	const CKnownEntity *known = me->GetVisionInterface()->GetKnown( thrower );
	if ( !known || !known->WasEverVisible() )
	{
		return true;
	}

	CNEO_Player *pThrower = ToNEOPlayer( thrower );
	return !pThrower || pThrower->GetClass() != NEO_CLASS_SUPPORT;
}


//---------------------------------------------------------------------------------------------
// Areas closer than this to where a grenade can go are never used as cover from it
float CNEOBotRetreatFromGrenade::GetGrenadeCoverDistance()
{
	return sv_neo_grenade_blast_radius.GetFloat() * sv_neo_bot_grenade_frag_safety_range_multiplier.GetFloat() * kSqrt2;
}


//---------------------------------------------------------------------------------------------
// Where a grenade comes to rest, from what anyone watching it can see: its position and velocity.
// Flight to the floor below, then each bounce keeps e of both speed components, a geometric
// series (ResolveFlyCollisionCustom), stopped at the first wall on the way.
Vector CNEOBotRetreatFromGrenade::PredictGrenadeRest( CBaseEntity *grenade )
{
	const Vector vecPos = grenade->GetAbsOrigin();
	const Vector vecVel = grenade->GetAbsVelocity();

	float flFloorZ;
	if ( !TheNavMesh->GetGroundHeight( vecPos, &flFloorZ ) )
	{
		return vecPos;
	}

	const float flGravity = GetCurrentGravity() * sv_neo_grenade_gravity.GetFloat();
	if ( flGravity <= 0.0f )
	{
		return vecPos;
	}

	const float flDrop = Max( 0.0f, vecPos.z - flFloorZ );
	const float flImpactVz = sqrt( vecVel.z * vecVel.z + 2.0f * flGravity * flDrop );
	const float flFlightTime = ( vecVel.z + flImpactVz ) / flGravity;
	const float flElasticity = Min( sv_neo_grenade_cor.GetFloat(), kMaxGrenadeElasticity );
	const float flHopShare = flElasticity * flElasticity;
	const float flBounceTime = 2.0f * flImpactVz / flGravity * flHopShare / ( 1.0f - flHopShare );

	Vector vecRest = vecPos + Vector( vecVel.x, vecVel.y, 0.0f ) * ( flFlightTime + flBounceTime );
	vecRest.z = flFloorZ + 1.0f;

	trace_t tr;
	UTIL_TraceLine( vecPos, vecRest, MASK_SOLID_BRUSHONLY, grenade, COLLISION_GROUP_NONE, &tr );
	return tr.endpos;
}


//---------------------------------------------------------------------------------------------
CBaseEntity *CNEOBotRetreatFromGrenade::FindDangerousGrenade( CNEOBot *me )
{
	const float flGrenadeCheckRadius = neo_bot_grenade_check_radius.GetFloat();
	CBaseEntity *closestThreat = NULL;
	const char *pszGrenadeClass = "neo_grenade_frag";
	
	int iSound = CSoundEnt::ActiveList();
	while ( iSound != SOUNDLIST_EMPTY )
	{
		CSound *pSound = CSoundEnt::SoundPointerForIndex( iSound );
		if ( !pSound )
			break;

		if ( (pSound->SoundType() & SOUND_DANGER) && pSound->ValidateOwner() )
		{
			float distSqr = ( pSound->GetSoundOrigin() - me->GetAbsOrigin() ).LengthSqr();
			if ( distSqr <= (flGrenadeCheckRadius * flGrenadeCheckRadius) )
			{
				CBaseEntity *pOwner = pSound->m_hOwner.Get();
				// Use FClassnameIs to check if it's a generic base grenade or our specific ones
				// FClassnameIs is better than dynamic_cast inside a loop when possible
				if ( pOwner && ( FClassnameIs( pOwner, pszGrenadeClass ) || IsUnidentifiedSmoke( me, pOwner ) ) )
				{
					// Found a dangerous grenade
					closestThreat = pOwner;
					break;
				}
			}
		}

		iSound = pSound->NextSound();
	}

	return closestThreat;
}


//---------------------------------------------------------------------------------------------
CNEOBotRetreatFromGrenade::CNEOBotRetreatFromGrenade( CBaseEntity *grenade )
	: m_grenade(grenade)
	, m_coverArea( NULL )
{
}


//---------------------------------------------------------------------------------------------
// Collect nearby areas that provide cover from our grenade
class CSearchForCoverFromGrenade : public ISearchSurroundingAreasFunctor
{
public:
	CSearchForCoverFromGrenade( CNEOBot *me, CBaseEntity *grenade )
	{
		m_me = me;
		m_grenade = grenade;
		m_onStuckPenalty = neo_bot_path_reservation_onstuck_penalty.GetFloat();
		m_coverDist = CNEOBotRetreatFromGrenade::GetGrenadeCoverDistance();
		m_blastRadius = sv_neo_grenade_blast_radius.GetFloat();

		// Judge cover against everywhere the grenade can still go, from here to where it will rest:
		// an airborne grenade has no nav area, and fleeing its current position runs along its flight
		m_vecDangerStart = vec3_origin;
		m_vecDangerEnd = vec3_origin;
		m_restArea = nullptr;
		if ( grenade )
		{
			m_vecDangerStart = grenade->GetAbsOrigin();
			m_vecDangerEnd = CNEOBotRetreatFromGrenade::PredictGrenadeRest( grenade );
			m_restArea = TheNavMesh->GetNavArea( m_vecDangerEnd );
			if ( !m_restArea )
			{
				m_restArea = TheNavMesh->GetNearestNavArea( m_vecDangerEnd );
			}
		}

		if ( neo_bot_debug_retreat_from_grenade.GetBool() )
			TheNavMesh->ClearSelectedSet();
	}

	float DistToDanger( const CNavArea *area ) const
	{
		return CalcDistanceToLineSegment( area->GetCenter(), m_vecDangerStart, m_vecDangerEnd );
	}

	virtual bool operator() ( CNavArea *baseArea, CNavArea *priorArea, float travelDistanceSoFar )
	{
		VPROF_BUDGET( "CSearchForCoverFromGrenade::operator()", "NextBot" );

		CNavArea *area = (CNavArea *)baseArea;

		if ( !m_grenade )
		{
			// edge case: we just want to bail to let Update exit this behavior
			return false;  // can't search if there's no threat source
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

		if ( DistToDanger( area ) < m_coverDist )
		{
			// using cover that is too near a grenade is prone to errors and it looks oblivious
			return true;
		}

		if ( m_restArea && m_restArea->IsPotentiallyVisible( area ) )
		{
			// area is exposed to grenade line of sight
			return true;
		}

		// let's add this area to the candidate of escape destinations
		CoverCandidate candidate;
		candidate.area = area;
		candidate.cost = travelDistanceSoFar + kCoverEnemyExposureCost * CountThreatsExposingArea( m_me, area );
		m_coverAreaVector.AddToTail( candidate );

		return true;
	}

	// return true if 'adjArea' should be included in the ongoing search
	virtual bool ShouldSearch( CNavArea *adjArea, CNavArea *currentArea, float travelDistanceSoFar ) 
	{
		if ( travelDistanceSoFar > neo_bot_retreat_from_grenade_range.GetFloat() )
			return false;

		// inside the blast's reach, only ever step away from it
		const float flAdjDist = DistToDanger( adjArea );
		if ( flAdjDist < m_blastRadius && flAdjDist < DistToDanger( currentArea ) )
			return false;

		// allow falling off ledges, but don't jump up - too slow
		return ( currentArea->ComputeAdjacentConnectionHeightChange( adjArea ) < m_me->GetLocomotionInterface()->GetStepHeight() );
	}

	virtual void PostSearch( void )
	{
		if ( neo_bot_debug_retreat_from_grenade.GetBool() )
		{
			for( int i=0; i<m_coverAreaVector.Count(); ++i )
				TheNavMesh->AddToSelectedSet( m_coverAreaVector[i].area );
		}
	}

	struct CoverCandidate
	{
		CNavArea *area;
		float cost; // travel distance plus the known-enemy exposure charge
	};

	static int CompareCost( const CoverCandidate *a, const CoverCandidate *b )
	{
		return ( a->cost < b->cost ) ? -1 : ( a->cost > b->cost ) ? 1 : 0;
	}

	CNEOBot *m_me;
	CBaseEntity *m_grenade;
	float m_onStuckPenalty;
	float m_coverDist;
	float m_blastRadius;
	Vector m_vecDangerStart;
	Vector m_vecDangerEnd;
	const CNavArea *m_restArea;
	CUtlVector< CoverCandidate > m_coverAreaVector;
};


//---------------------------------------------------------------------------------------------
CNavArea *CNEOBotRetreatFromGrenade::FindCoverArea( CNEOBot *me )
{
	VPROF_BUDGET( "CNEOBotRetreatFromGrenade::FindCoverArea", "NextBot" );

	CSearchForCoverFromGrenade search( me, m_grenade );

	CNavArea *startArea = me->GetLastKnownArea();
	if ( !startArea )
	{
		return NULL;
	}

	SearchSurroundingAreas( startArea, search );

	if ( search.m_coverAreaVector.Count() == 0 )
	{
		return NULL;
	}

	// pick from the cheapest 10 areas to avoid the whole team bunching up in one spot
	search.m_coverAreaVector.Sort( CSearchForCoverFromGrenade::CompareCost );
	int last = Min( 10, search.m_coverAreaVector.Count() );
	int which = RandomInt( 0, last-1 );
	return search.m_coverAreaVector[ which ].area;
}


//---------------------------------------------------------------------------------------------
ActionResult< CNEOBot >	CNEOBotRetreatFromGrenade::OnStart( CNEOBot *me, Action< CNEOBot > *priorAction )
{
	m_path.SetMinLookAheadDistance( me->GetDesiredPathLookAheadRange() );

	if ( !m_grenade )
	{
		m_grenade = FindDangerousGrenade( me );
	}

	if ( !m_grenade ) // not a duplicate, check FindDangerousGrenade result
	{
		return Done("No grenade found");
	}

	// Register explosive hazard for the bot's team at the grenade's initial position
	CNavArea *grenadeArea = TheNavMesh->GetNearestNavArea( m_grenade->GetAbsOrigin() );
	if (grenadeArea)
	{
		int team = me->GetTeamNumber();
		CNEOBotPathReservations()->AddFragHazard(grenadeArea->GetID(), gpGlobals->curtime + sv_neo_grenade_fuse_timer.GetFloat(), team);
	}

	// Sometimes grenades can be in a bad limbo state, so force exit eventually
	m_expiryTimer.Start( sv_neo_grenade_fuse_timer.GetFloat() );

	m_coverArea = FindCoverArea( me );

	if ( m_coverArea == NULL )
		return Done( "No grenade cover available!" );

	// Scope out now unless trading fire: MainAction's aim-out delay is there to stop flicker, not for a live grenade
	if ( !IsTradingFire( me ) )
	{
		me->m_qPrevShouldAim = ANSWER_NO;
		me->m_flLastShouldAimTime = 0.0f;
	}

	return Continue();
}


//---------------------------------------------------------------------------------------------
ActionResult< CNEOBot >	CNEOBotRetreatFromGrenade::Update( CNEOBot *me, float interval )
{
	// Sometimes grenades can be in a bad limbo state, so force exit eventually
	if ( m_expiryTimer.IsElapsed() )
	{
		return Done( "Grenade fuse time elapsed" );
	}

	// If grenade object is gone, we are done
	if ( !m_grenade )
	{
		return Done( "Grenade threat is over" );
	}
	
	// track where the projectile will rest and its relation to the escape destination every update
	const Vector vecRest = PredictGrenadeRest( m_grenade );
	const CNavArea *restArea = TheNavMesh->GetNavArea( vecRest );
	if ( !restArea )
	{
		restArea = TheNavMesh->GetNearestNavArea( vecRest );
	}

	if ( !m_coverArea || ( restArea && restArea->IsPotentiallyVisible( m_coverArea ) ) )
	{
		CNavArea *pPrevCoverArea = m_coverArea;
		m_coverArea = FindCoverArea( me );

		// cover destination changed, stop following the path to the old spot
		if ( m_coverArea != pPrevCoverArea )
		{
			m_path.Invalidate();
			m_repathTimer.Invalidate();
		}
	}

	if (!m_coverArea)
	{
		return Done("Reacting to contact instead");
	}

	if ( m_repathTimer.IsElapsed() || !m_path.IsValid() )
	{
		CNEOBotPathCompute( me, m_path, m_coverArea->GetCenter(), FASTEST_ROUTE );
		m_repathTimer.Start( 1.0f );
	}

	// Walk() / Run() do not press sprint for player bots, so hold it here
	if ( me->CanSprint() && !IsTradingFire( me ) )
	{
		me->PressRunButton();
	}

	m_path.Update( me );

	return Continue();
}



//---------------------------------------------------------------------------------------------
EventDesiredResult< CNEOBot > CNEOBotRetreatFromGrenade::OnStuck( CNEOBot *me )
{
	m_path.Invalidate();
	m_repathTimer.Invalidate();
	return TryContinue();
}


//---------------------------------------------------------------------------------------------
EventDesiredResult< CNEOBot > CNEOBotRetreatFromGrenade::OnMoveToSuccess( CNEOBot *me, const Path *path )
{
	return TryContinue();
}


//---------------------------------------------------------------------------------------------
EventDesiredResult< CNEOBot > CNEOBotRetreatFromGrenade::OnMoveToFailure( CNEOBot *me, const Path *path, MoveToFailureType reason )
{
	m_path.Invalidate();
	m_repathTimer.Invalidate();
	return TryContinue();
}


//---------------------------------------------------------------------------------------------
QueryResultType CNEOBotRetreatFromGrenade::ShouldHurry( const INextBot *me ) const
{
	return ANSWER_YES;
}


//---------------------------------------------------------------------------------------------
QueryResultType CNEOBotRetreatFromGrenade::ShouldWalk( const CNEOBot *me, const QueryResultType qShouldAimQuery ) const
{
	return IsTradingFire( me ) ? ANSWER_UNDEFINED : ANSWER_NO;
}


//---------------------------------------------------------------------------------------------
QueryResultType CNEOBotRetreatFromGrenade::ShouldRetreat( const CNEOBot *me ) const
{
	// Disincentivize CNEOBotTacticalMonitor::Update from interrupting this behavior
	// as we don't want to switch to CNEOBotRetreatToCover while evading a grenade
	return ANSWER_NO;
}


//---------------------------------------------------------------------------------------------
QueryResultType CNEOBotRetreatFromGrenade::ShouldAim( const CNEOBot *me, const bool bWepHasClip ) const
{
	return IsTradingFire( me ) ? ANSWER_UNDEFINED : ANSWER_NO;
}


//---------------------------------------------------------------------------------------------
// While an enemy is in view and the magazine has rounds, keep shooting at it the usual way.
// Otherwise stop aiming and sprint.
bool CNEOBotRetreatFromGrenade::IsTradingFire( const CNEOBot *me ) const
{
	const CKnownEntity *threat = me->GetVisionInterface()->GetPrimaryKnownThreat();
	if ( !threat || !threat->IsVisibleInFOVNow() )
	{
		return false;
	}

	CBaseCombatWeapon *weapon = me->GetActiveWeapon();
	return weapon && weapon->Clip1() > 0;
}
