#include "cbase.h"
#include "neo_player.h"
#include "bot/neo_bot.h"
#include "bot/behavior/neo_bot_retreat_from_apc.h"
#include "bot/behavior/neo_bot_retreat_to_cover.h"
#include "bot/neo_bot_path_compute.h"
#include "neo_npc_targetsystem.h"

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

ConVar neo_bot_retreat_from_apc_range( "neo_bot_retreat_from_apc_range", "2000", FCVAR_CHEAT );
ConVar neo_bot_debug_retreat_from_apc( "neo_bot_debug_retreat_from_apc", "0", FCVAR_CHEAT );


//---------------------------------------------------------------------------------------------
CNEOBotRetreatFromAPC::CNEOBotRetreatFromAPC( CBaseEntity *pAPC )
	: m_hAPC(pAPC)
	, m_coverArea( NULL )
{
}


//---------------------------------------------------------------------------------------------
// Collect nearby areas that provide cover from the APC
class CSearchForCoverFromAPC : public ISearchSurroundingAreasFunctor
{
public:
	CSearchForCoverFromAPC( CNEOBot *me, CBaseEntity *apc )
	{
		m_me = me;
		m_hAPC = apc;

		if ( neo_bot_debug_retreat_from_apc.GetBool() )
			TheNavMesh->ClearSelectedSet();
	}

	virtual bool operator() ( CNavArea *baseArea, CNavArea *priorArea, float travelDistanceSoFar )
	{
		CNavArea *area = (CNavArea *)baseArea;

		if ( !m_hAPC.Get() )
		{
			return false;
		}

		// Don't choose cover that is right next to the APC
		if ( ( area->GetCenter() - m_hAPC->GetAbsOrigin() ).LengthSqr() < Square( 300.0f ) )
		{
			return true;
		}

		const CNavArea *apcArea = TheNavMesh->GetNavArea( m_hAPC->GetAbsOrigin() );
		if ( apcArea )
		{
			if ( area->IsPotentiallyVisible( apcArea ) )
			{
				// area is exposed to APC line of sight
				return true;
			}
		}
		
		// If we are here, it's not potentially visible via PVS.
		// We could do a trace if we wanted to be super sure, but PVS is usually good enough for "hiding" behavior
		// and way cheaper.

		// let's add this area to the candidate of escape destinations
		m_coverAreaVector.AddToTail( area );

		return true;
	}

	// return true if 'adjArea' should be included in the ongoing search
	virtual bool ShouldSearch( CNavArea *adjArea, CNavArea *currentArea, float travelDistanceSoFar ) 
	{
		if ( travelDistanceSoFar > neo_bot_retreat_from_apc_range.GetFloat() )
			return false;

		// allow falling off ledges, but don't jump up - too slow usually, but maybe ok? 
		// Copied from grenade retreat, seems reasonable to avoid high jumps when fleeing.
		return ( currentArea->ComputeAdjacentConnectionHeightChange( adjArea ) < m_me->GetLocomotionInterface()->GetStepHeight() );
	}

	virtual void PostSearch( void )
	{
		if ( neo_bot_debug_retreat_from_apc.GetBool() )
		{
			for( int i=0; i<m_coverAreaVector.Count(); ++i )
				TheNavMesh->AddToSelectedSet( m_coverAreaVector[i] );
		}
	}

	CNEOBot *m_me;
	EHANDLE m_hAPC;
	CUtlVector< CNavArea * > m_coverAreaVector;
};


//---------------------------------------------------------------------------------------------
CNavArea *CNEOBotRetreatFromAPC::FindCoverArea( CNEOBot *me )
{
	CSearchForCoverFromAPC search( me, m_hAPC.Get() );

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

	// Find the closest one
	float closestDistSqr = FLT_MAX;
	CNavArea *closestArea = NULL;

	for( int i=0; i<search.m_coverAreaVector.Count(); ++i )
	{
		float distSqr = ( search.m_coverAreaVector[i]->GetCenter() - me->GetAbsOrigin() ).LengthSqr();
		if ( distSqr < closestDistSqr )
		{
			closestDistSqr = distSqr;
			closestArea = search.m_coverAreaVector[i];
		}
	}
	
	return closestArea;
}


//---------------------------------------------------------------------------------------------
ActionResult< CNEOBot >	CNEOBotRetreatFromAPC::OnStart( CNEOBot *me, Action< CNEOBot > *priorAction )
{
	m_path.SetMinLookAheadDistance( me->GetDesiredPathLookAheadRange() );

	m_coverArea = FindCoverArea( me );

	if ( m_coverArea == NULL )
		return Done( "No APC cover available!" );

	return Continue();
}


//---------------------------------------------------------------------------------------------
ActionResult< CNEOBot >	CNEOBotRetreatFromAPC::Update( CNEOBot *me, float interval )
{
	// If APC object is gone, we are done
	if ( !m_hAPC.Get() )
	{
		return Done( "APC threat is gone" );
	}
	
	CNEO_NPCTargetSystem *pTurret = dynamic_cast<CNEO_NPCTargetSystem*>(m_hAPC.Get());
	if ( pTurret && !pTurret->IsHostileTo(me->GetEntity()) )
	{
		return Done( "APC is no longer hostile" );
	}

	const CNavArea *apcArea = TheNavMesh->GetNavArea( m_hAPC->GetAbsOrigin() );
	bool bIsExposed = false;
	if ( apcArea && me->GetLastKnownArea() )
	{
		if ( me->GetLastKnownArea()->IsPotentiallyVisible( apcArea ) )
		{
			bIsExposed = true;
		}
	}

	// If we lost our cover or it became exposed
	if ( !m_coverArea || ( apcArea && m_coverArea->IsPotentiallyVisible( apcArea ) ) )
	{
		m_coverArea = FindCoverArea( me );
	}

	if (!m_coverArea)
	{
		// Just run away generically if we can't find cover? Or switch to retreat to cover?
		// For now, fail out.
		return Done( "No cover found from APC" );
	}

	if ( me->GetLastKnownArea() != m_coverArea || bIsExposed )
	{
		// not in cover yet
		if (m_repathTimer.IsElapsed())
		{
			CNEOBotPathCompute(me, m_path, m_coverArea->GetCenter(), FASTEST_ROUTE);
			m_repathTimer.Start(0.5f);
		}
		m_path.Update( me );
	}
	else
	{
		// We are in cover and not exposed. We can probably wait or done?
		// If we are done, the bot might stick its head out again.
		// Maybe just wait here until threat is gone? 
		// Or return Done and let tactical monitor decide if it's safe?
		// If we return Done, Tactical Monitor might put us back in SeekAndDestroy, which might walk us out.
		// But UpdatePathCost should prevent us from walking into danger.
		// So staying here is safe.
		// But how long?
		// Let's just hold position for a bit.
		return Continue();
	}

	return Continue();
}


//---------------------------------------------------------------------------------------------
EventDesiredResult< CNEOBot > CNEOBotRetreatFromAPC::OnStuck( CNEOBot *me )
{
	return TryContinue();
}


//---------------------------------------------------------------------------------------------
EventDesiredResult< CNEOBot > CNEOBotRetreatFromAPC::OnMoveToSuccess( CNEOBot *me, const Path *path )
{
	return TryContinue();
}


//---------------------------------------------------------------------------------------------
EventDesiredResult< CNEOBot > CNEOBotRetreatFromAPC::OnMoveToFailure( CNEOBot *me, const Path *path, MoveToFailureType reason )
{
	return TryContinue();
}


//---------------------------------------------------------------------------------------------
QueryResultType CNEOBotRetreatFromAPC::ShouldHurry( const INextBot *me ) const
{
	return ANSWER_YES;
}


//---------------------------------------------------------------------------------------------
QueryResultType CNEOBotRetreatFromAPC::ShouldWalk( const CNEOBot *me, const QueryResultType qShouldAimQuery ) const
{
	return ANSWER_NO;
}


//---------------------------------------------------------------------------------------------
QueryResultType CNEOBotRetreatFromAPC::ShouldRetreat( const CNEOBot *me ) const
{
	return ANSWER_YES;
}


//---------------------------------------------------------------------------------------------
QueryResultType CNEOBotRetreatFromAPC::ShouldAim( const CNEOBot *me, const bool bWepHasClip ) const
{
	return ANSWER_NO;
}
