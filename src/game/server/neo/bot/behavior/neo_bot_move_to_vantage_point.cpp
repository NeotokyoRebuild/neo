#include "cbase.h"
#include "neo_player.h"
#include "bot/neo_bot.h"
#include "bot/behavior/neo_bot_move_to_vantage_point.h"

#include "nav_mesh.h"

extern ConVar neo_bot_path_lookahead_range;


//---------------------------------------------------------------------------------------------
CNEOBotMoveToVantagePoint::CNEOBotMoveToVantagePoint( float maxTravelDistance )
{
	m_maxTravelDistance = maxTravelDistance;
}


//---------------------------------------------------------------------------------------------
ActionResult< CNEOBot >	CNEOBotMoveToVantagePoint::OnStart( CNEOBot *me, Action< CNEOBot > *priorAction )
{
	m_path.SetMinLookAheadDistance( me->GetDesiredPathLookAheadRange() );

	m_vantageArea = me->FindVantagePoint( m_maxTravelDistance );
	if ( !m_vantageArea )
	{
		return Done( "No vantage point found" );
	}

	m_path.Invalidate();
	m_repathTimer.Invalidate();

	return Continue();
}


//---------------------------------------------------------------------------------------------
ActionResult< CNEOBot >	CNEOBotMoveToVantagePoint::Update( CNEOBot *me, float interval )
{
	const CKnownEntity *threat = me->GetVisionInterface()->GetPrimaryKnownThreat();
	if ( threat && threat->IsVisibleInFOVNow() )
	{
		return Done( "Enemy is visible" );
	}

	if ( !m_path.IsValid() && m_repathTimer.IsElapsed() )
	{
		m_repathTimer.Start( 1.0f );

		CNEOBotPathCost cost( me, FASTEST_ROUTE );
		if ( !m_path.Compute( me, m_vantageArea->GetCenter(), cost ) )
		{
			return Done( "No path to vantage point exists" );
		}
	}

	// move along path to vantage point
	m_path.Update( me );

	return Continue();
}


//---------------------------------------------------------------------------------------------
EventDesiredResult< CNEOBot > CNEOBotMoveToVantagePoint::OnStuck( CNEOBot *me )
{
	m_path.Invalidate();
	return TryContinue();
}


//---------------------------------------------------------------------------------------------
EventDesiredResult< CNEOBot > CNEOBotMoveToVantagePoint::OnMoveToSuccess( CNEOBot *me, const Path *path )
{
	return TryDone( RESULT_CRITICAL, "Vantage point reached" );
}


//---------------------------------------------------------------------------------------------
EventDesiredResult< CNEOBot > CNEOBotMoveToVantagePoint::OnMoveToFailure( CNEOBot *me, const Path *path, MoveToFailureType reason )
{
	m_path.Invalidate();
	return TryContinue();
}


