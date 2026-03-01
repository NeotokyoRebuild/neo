
#include "cbase.h"
#include "neo_player.h"
#include "bot/neo_bot.h"
#include "bot/behavior/neo_bot_retreat_from_grenade.h"
#include "bot/behavior/neo_bot_retreat_to_cover.h"
#include "bot/neo_bot_path_compute.h"
#include "sdk/sdk_basegrenade_projectile.h"

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

extern ConVar sv_neo_bot_grenade_frag_safety_range_multiplier;
extern ConVar sv_neo_grenade_fuse_timer;
ConVar neo_bot_retreat_from_grenade_range( "neo_bot_retreat_from_grenade_range", "2000", FCVAR_CHEAT );
ConVar neo_bot_debug_retreat_from_grenade( "neo_bot_debug_retreat_from_grenade", "0", FCVAR_CHEAT );
ConVar neo_bot_grenade_check_radius( "neo_bot_grenade_check_radius", "500", FCVAR_CHEAT );


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
				if ( pOwner && ( FClassnameIs( pOwner, pszGrenadeClass ) || FClassnameIs( pOwner, "neo_grenade_smoke" ) ) )
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
		m_pGrenadeStats = dynamic_cast<CBaseGrenadeProjectile *>( grenade );
		m_safeRadiusSqr = m_pGrenadeStats ? Square(m_pGrenadeStats->m_DmgRadius * sv_neo_bot_grenade_frag_safety_range_multiplier.GetFloat()) : 0.0f;

		if ( neo_bot_debug_retreat_from_grenade.GetBool() )
			TheNavMesh->ClearSelectedSet();
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

		if ( m_pGrenadeStats )
		{
			if ( ( area->GetCenter() - m_pGrenadeStats->GetAbsOrigin() ).LengthSqr() < m_safeRadiusSqr * 2 )
			{
				// using cover that is too near a grenade is prone to errors and it looks oblivious
				return true;
			}
		}

		const CNavArea *grenadeArea = TheNavMesh->GetNavArea( m_grenade->GetAbsOrigin() );
		if ( grenadeArea )
		{
			if ( area->IsPotentiallyVisible( grenadeArea ) )
			{
				// area is exposed to grenade line of sight
				return true;
			}
		}

		// let's add this area to the candidate of escape destinations
		m_coverAreaVector.AddToTail( area );

		return true;
	}

	// return true if 'adjArea' should be included in the ongoing search
	virtual bool ShouldSearch( CNavArea *adjArea, CNavArea *currentArea, float travelDistanceSoFar ) 
	{
		if ( travelDistanceSoFar > neo_bot_retreat_from_grenade_range.GetFloat() )
			return false;

		// allow falling off ledges, but don't jump up - too slow
		return ( currentArea->ComputeAdjacentConnectionHeightChange( adjArea ) < m_me->GetLocomotionInterface()->GetStepHeight() );
	}

	virtual void PostSearch( void )
	{
		if ( neo_bot_debug_retreat_from_grenade.GetBool() )
		{
			for( int i=0; i<m_coverAreaVector.Count(); ++i )
				TheNavMesh->AddToSelectedSet( m_coverAreaVector[i] );
		}
	}

	CNEOBot *m_me;
	CBaseEntity *m_grenade;
	CBaseGrenadeProjectile *m_pGrenadeStats;
	float m_safeRadiusSqr;
	CUtlVector< CNavArea * > m_coverAreaVector;
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

	// first in vector should be closest via travel distance
	// pick from the closest 10 areas to avoid the whole team bunching up in one spot
	int last = Min( 10, search.m_coverAreaVector.Count() );
	int which = RandomInt( 0, last-1 );
	return search.m_coverAreaVector[ which ];
}


//---------------------------------------------------------------------------------------------
ActionResult< CNEOBot >	CNEOBotRetreatFromGrenade::OnStart( CNEOBot *me, Action< CNEOBot > *priorAction )
{
	m_path.SetMinLookAheadDistance( me->GetDesiredPathLookAheadRange() );

	if ( !m_grenade )
	{
		m_grenade = FindDangerousGrenade( me );
	}

	// Sometimes grenades can be in a bad limbo state, so force exit eventually
	m_expiryTimer.Start( sv_neo_grenade_fuse_timer.GetFloat() );

	m_coverArea = FindCoverArea( me );

	if ( m_coverArea == NULL )
		return Done( "No grenade cover available!" );

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
	
	const CNavArea *grenadeArea = TheNavMesh->GetNavArea( m_grenade->GetAbsOrigin() );
	bool bIsExposed = false;
	if ( grenadeArea && me->GetLastKnownArea() )
	{
		if ( me->GetLastKnownArea()->IsPotentiallyVisible( grenadeArea ) )
		{
			bIsExposed = true;
		}
	}

	// track projectile and relation to escape destination every update
	if ( !m_coverArea || ( grenadeArea && m_coverArea->IsPotentiallyVisible( grenadeArea ) ) )
	{
		m_coverArea = FindCoverArea( me );
	}

	if (!m_coverArea)
	{
		return SuspendFor(new CNEOBotRetreatToCover, "Reacting to contact instead");
	}

	if ( me->GetLastKnownArea() != m_coverArea || !bIsExposed )
	{
		// not in cover yet
		if (m_repathTimer.IsElapsed())
		{
			CNEOBotPathCompute(me, m_path, m_coverArea->GetCenter(), FASTEST_ROUTE);
			m_repathTimer.Start(0.2f); // Recompute path every 0.2 seconds
		}
		m_path.Update( me );
	}

	return Continue();
}


//---------------------------------------------------------------------------------------------
EventDesiredResult< CNEOBot > CNEOBotRetreatFromGrenade::OnStuck( CNEOBot *me )
{
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
	return ANSWER_NO;
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
	return ANSWER_NO;
}
