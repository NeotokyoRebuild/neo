
#include "cbase.h"
#include "neo_player.h"
#include "bot/neo_bot.h"
#include "bot/behavior/neo_bot_retreat_from_grenade.h"
#include "bot/neo_bot_path_compute.h"

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

extern ConVar neo_bot_path_lookahead_range;
ConVar neo_bot_retreat_from_grenade_range( "neo_bot_retreat_from_grenade_range", "1000", FCVAR_CHEAT );
ConVar neo_bot_debug_retreat_from_grenade( "neo_bot_debug_retreat_from_grenade", "0", FCVAR_CHEAT );
extern ConVar neo_bot_wait_in_cover_min_time;
extern ConVar neo_bot_wait_in_cover_max_time;


//---------------------------------------------------------------------------------------------
CNEOBotRetreatFromGrenade::CNEOBotRetreatFromGrenade( CBaseEntity *grenade, float hideDuration )
{
	m_grenade = grenade;
	m_hideDuration = hideDuration;
	m_actionToChangeToOnceCoverReached = NULL;
}


//---------------------------------------------------------------------------------------------
CNEOBotRetreatFromGrenade::CNEOBotRetreatFromGrenade( CBaseEntity *grenade, Action< CNEOBot > *actionToChangeToOnceCoverReached )
{
	m_grenade = grenade;
	m_hideDuration = -1.0f;
	m_actionToChangeToOnceCoverReached = actionToChangeToOnceCoverReached;
}


//collect nearby areas that provide cover from our grenade
class CSearchForCoverFromGrenade : public ISearchSurroundingAreasFunctor
{
public:
	CSearchForCoverFromGrenade( CNEOBot *me, CBaseEntity *grenade )
	{
		m_me = me;
		m_grenade = grenade;

		if ( neo_bot_debug_retreat_from_grenade.GetBool() )
			TheNavMesh->ClearSelectedSet();
	}

	virtual bool operator() ( CNavArea *baseArea, CNavArea *priorArea, float travelDistanceSoFar )
	{
		VPROF_BUDGET( "CSearchForCoverFromGrenade::operator()", "NextBot" );

		CNavArea *area = (CNavArea *)baseArea;

		if ( !m_grenade )
		{
			return false;
		}

		const CNavArea *grenadeArea = TheNavMesh->GetNavArea( m_grenade->GetAbsOrigin() );
		if ( grenadeArea )
		{
			// if the grenade sees this area, it's not cover
			if ( area->IsPotentiallyVisible( grenadeArea ) )
			{
				return true;
			}
		}

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
	CUtlVector< CNavArea * > m_coverAreaVector;
};


//---------------------------------------------------------------------------------------------
CNavArea *CNEOBotRetreatFromGrenade::FindCoverArea( CNEOBot *me )
{
	VPROF_BUDGET( "CNEOBotRetreatFromGrenade::FindCoverArea", "NextBot" );

	CSearchForCoverFromGrenade search( me, m_grenade );
	SearchSurroundingAreas( me->GetLastKnownArea(), search );

	if ( search.m_coverAreaVector.Count() == 0 )
	{
		return NULL;
	}

	// first in vector should be closest via travel distance
	// pick from the closest 10 areas to avoid the whole team bunching up in one spot
	int last = MIN( 10, search.m_coverAreaVector.Count() );
	int which = RandomInt( 0, last-1 );
	return search.m_coverAreaVector[ which ];
}


//---------------------------------------------------------------------------------------------
ActionResult< CNEOBot >	CNEOBotRetreatFromGrenade::OnStart( CNEOBot *me, Action< CNEOBot > *priorAction )
{
	m_path.SetMinLookAheadDistance( me->GetDesiredPathLookAheadRange() );

	m_coverArea = FindCoverArea( me );

	if ( m_coverArea == NULL )
		return Done( "No cover available!" );

	if ( m_hideDuration < 0.0f )
	{
		m_hideDuration = RandomFloat( neo_bot_wait_in_cover_min_time.GetFloat(), neo_bot_wait_in_cover_max_time.GetFloat() );
	}

	m_waitInCoverTimer.Start( m_hideDuration );

	return Continue();
}


//---------------------------------------------------------------------------------------------
ActionResult< CNEOBot >	CNEOBotRetreatFromGrenade::Update( CNEOBot *me, float interval )
{
	// If grenade object is gone, we are done
	if ( !m_grenade )
	{
		return Done( "Grenade threat is over" );
	}
	
	// move to cover, or stop if we've found opportunistic cover (no visible threats right now)
	const CNavArea *grenadeArea = TheNavMesh->GetNavArea( m_grenade->GetAbsOrigin() );
	bool bIsExposed = false;
	if ( grenadeArea && me->GetLastKnownArea() )
	{
		if ( me->GetLastKnownArea()->IsPotentiallyVisible( grenadeArea ) )
		{
			bIsExposed = true;
		}
	}

	if ( me->GetLastKnownArea() == m_coverArea || !bIsExposed )
	{
		// we are now in cover (or think we are safe)

		if ( bIsExposed )
		{
			// threats are still visible - find new cover
			m_coverArea = FindCoverArea( me );

			if ( m_coverArea == NULL )
			{
				return Done( "My cover is exposed, and there is no other cover available!" );
			}
		}

		if ( m_actionToChangeToOnceCoverReached )
		{
			return ChangeTo( m_actionToChangeToOnceCoverReached, "Doing given action now that I'm in cover" );
		}

		if ( m_waitInCoverTimer.IsElapsed() )
		{
			return Done( "Been in cover long enough" );
		}
	}
	else
	{
		// not in cover yet
		m_waitInCoverTimer.Reset();

		if ( m_repathTimer.IsElapsed() )
		{
			m_repathTimer.Start( RandomFloat( 0.3f, 0.5f ) );

			CNEOBotPathCompute( me, m_path, m_coverArea->GetCenter(), RETREAT_ROUTE );
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
