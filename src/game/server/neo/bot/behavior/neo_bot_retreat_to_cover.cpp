//========= Copyright Valve Corporation, All rights reserved. ============//

#include "cbase.h"
#include "neo_player.h"
#include "bot/neo_bot.h"
#include "bot/behavior/neo_bot_retreat_to_cover.h"

extern ConVar neo_bot_path_lookahead_range;
ConVar neo_bot_retreat_to_cover_range( "neo_bot_retreat_to_cover_range", "1000", FCVAR_CHEAT );
ConVar neo_bot_debug_retreat_to_cover( "neo_bot_debug_retreat_to_cover", "0", FCVAR_CHEAT );
ConVar neo_bot_wait_in_cover_min_time( "neo_bot_wait_in_cover_min_time", "1", FCVAR_CHEAT );
ConVar neo_bot_wait_in_cover_max_time( "neo_bot_wait_in_cover_max_time", "2", FCVAR_CHEAT );


//---------------------------------------------------------------------------------------------
CNEOBotRetreatToCover::CNEOBotRetreatToCover( float hideDuration )
{
	m_hideDuration = hideDuration;
	m_actionToChangeToOnceCoverReached = NULL;
}


//---------------------------------------------------------------------------------------------
CNEOBotRetreatToCover::CNEOBotRetreatToCover( Action< CNEOBot > *actionToChangeToOnceCoverReached )
{
	m_hideDuration = -1.0f;
	m_actionToChangeToOnceCoverReached = actionToChangeToOnceCoverReached;
}


//---------------------------------------------------------------------------------------------
// for testing a given area's exposure to known threats
class CTestAreaAgainstThreats :  public IVision::IForEachKnownEntity
{
public:
	CTestAreaAgainstThreats( CNEOBot *me, CNavArea *area )
	{
		m_me = me;
		m_area = area;
		m_exposedThreatCount = 0;
	}

	virtual bool Inspect( const CKnownEntity &known )
	{
		VPROF_BUDGET( "CTestAreaAgainstThreats::Inspect", "NextBot" );

		if ( m_me->IsEnemy( known.GetEntity() ) )
		{
			const CNavArea *threatArea = known.GetLastKnownArea();

			if ( threatArea )
			{
				// is area visible by known threat
				if ( m_area->IsPotentiallyVisible( threatArea ) )
					++m_exposedThreatCount;
			}
		}

		return true;
	}

	CNEOBot *m_me;
	CNavArea *m_area;
	int m_exposedThreatCount;
};


// collect nearby areas that provide cover from our known threats
class CSearchForCover : public ISearchSurroundingAreasFunctor
{
public:
	CSearchForCover( CNEOBot *me )
	{
		m_me = me;
		m_minExposureCount = 9999;

		if ( neo_bot_debug_retreat_to_cover.GetBool() )
			TheNavMesh->ClearSelectedSet();
	}

	virtual bool operator() ( CNavArea *baseArea, CNavArea *priorArea, float travelDistanceSoFar )
	{
		VPROF_BUDGET( "CSearchForCover::operator()", "NextBot" );

		CNavArea *area = (CNavArea *)baseArea;

		CTestAreaAgainstThreats test( m_me, area );
		m_me->GetVisionInterface()->ForEachKnownEntity( test );

		if ( test.m_exposedThreatCount <= m_minExposureCount )
		{
			// this area is at least as good as already found cover
			if ( test.m_exposedThreatCount < m_minExposureCount )
			{
				// this area is better than already found cover - throw out list and start over
				m_coverAreaVector.RemoveAll();
				m_minExposureCount = test.m_exposedThreatCount;
			}

			m_coverAreaVector.AddToTail( area );
		}

		return true;				
	}

	// return true if 'adjArea' should be included in the ongoing search
	virtual bool ShouldSearch( CNavArea *adjArea, CNavArea *currentArea, float travelDistanceSoFar ) 
	{
		if ( travelDistanceSoFar > neo_bot_retreat_to_cover_range.GetFloat() )
			return false;

		// allow falling off ledges, but don't jump up - too slow
		return ( currentArea->ComputeAdjacentConnectionHeightChange( adjArea ) < m_me->GetLocomotionInterface()->GetStepHeight() );
	}

	virtual void PostSearch( void )
	{
		if ( neo_bot_debug_retreat_to_cover.GetBool() )
		{
			for( int i=0; i<m_coverAreaVector.Count(); ++i )
				TheNavMesh->AddToSelectedSet( m_coverAreaVector[i] );
		}
	}

	CNEOBot *m_me;
	CUtlVector< CNavArea * > m_coverAreaVector;
	int m_minExposureCount;
};


//---------------------------------------------------------------------------------------------
CNavArea *CNEOBotRetreatToCover::FindCoverArea( CNEOBot *me )
{
	VPROF_BUDGET( "CNEOBotRetreatToCover::FindCoverArea", "NextBot" );

	CSearchForCover search( me );
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
ActionResult< CNEOBot >	CNEOBotRetreatToCover::OnStart( CNEOBot *me, Action< CNEOBot > *priorAction )
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
ActionResult< CNEOBot >	CNEOBotRetreatToCover::Update( CNEOBot *me, float interval )
{
	const CKnownEntity *threat = me->GetVisionInterface()->GetPrimaryKnownThreat( true );

	if ( ShouldRetreat( me ) == ANSWER_NO )
		return Done( "No longer need to retreat" );

	// attack while retreating
	me->EquipBestWeaponForThreat( threat );

	// reload while moving to cover
	bool isDoingAFullReload = false;
	CNEOBaseCombatWeapon *myPrimary = (CNEOBaseCombatWeapon*)me->GetActiveWeapon();
	if ( myPrimary && myPrimary->GetPrimaryAmmoCount() > 0 && me->IsBarrageAndReloadWeapon(myPrimary))
	{
		if ( myPrimary->Clip1() < myPrimary->GetMaxClip1() )
		{
			me->PressReloadButton();
			isDoingAFullReload = true;
		}
	}


	// move to cover, or stop if we've found opportunistic cover (no visible threats right now)
	if ( me->GetLastKnownArea() == m_coverArea || !threat )
	{
		// we are now in cover

		if ( threat )
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

		// stay in cover while we fully reload
		if ( isDoingAFullReload )
		{
			return Continue();
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

			CNEOBotPathCost cost( me, RETREAT_ROUTE );
			m_path.Compute( me, m_coverArea->GetCenter(), cost );
		}

		m_path.Update( me );
	}

	return Continue();
}


//---------------------------------------------------------------------------------------------
EventDesiredResult< CNEOBot > CNEOBotRetreatToCover::OnStuck( CNEOBot *me )
{
	return TryContinue();
}


//---------------------------------------------------------------------------------------------
EventDesiredResult< CNEOBot > CNEOBotRetreatToCover::OnMoveToSuccess( CNEOBot *me, const Path *path )
{
	return TryContinue();
}


//---------------------------------------------------------------------------------------------
EventDesiredResult< CNEOBot > CNEOBotRetreatToCover::OnMoveToFailure( CNEOBot *me, const Path *path, MoveToFailureType reason )
{
	return TryContinue();
}


//---------------------------------------------------------------------------------------------
// Hustle yer butt to safety!
QueryResultType CNEOBotRetreatToCover::ShouldHurry( const INextBot *me ) const
{
	return ANSWER_YES;
}


