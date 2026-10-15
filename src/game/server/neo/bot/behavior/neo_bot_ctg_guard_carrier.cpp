#include "cbase.h"
#include "neo_player.h"
#include "neo_gamerules.h"
#include "bot/neo_bot.h"
#include "bot/neo_bot_ghost_guard.h"
#include "bot/neo_bot_path_compute.h"
#include "bot/neo_bot_path_reservation.h"
#include "bot/behavior/neo_bot_ctg_guard_carrier.h"
#include "bot/behavior/neo_bot_attack.h"

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

// A guard keeps its spot until the carrier has moved this far from where the spot was chosen
static constexpr float GUARD_RESELECT_DIST = 300.0f;
// and retries this often when it found no spot
static constexpr float GUARD_RETRY_INTERVAL = 2.0f;
// A guard re-claims its spot this often, and the claim lasts a few refreshes
static constexpr float GUARD_CLAIM_REFRESH = 5.0f;
static constexpr float GUARD_CLAIM_DURATION = 15.0f;
// A guard counts as arrived this close to its spot's center
static constexpr float GUARD_ARRIVE_RANGE = 48.0f;
// and walks back to it when pushed this far away
static constexpr float GUARD_LEFT_SPOT_RANGE = 120.0f;

//---------------------------------------------------------------------------------------------
CNEOBotCtgGuardCarrier::CNEOBotCtgGuardCarrier( void )
{
	m_spot = nullptr;
	m_vecSpotCarrierPos = vec3_origin;
	m_bArrived = false;
}

//---------------------------------------------------------------------------------------------
ActionResult< CNEOBot >	CNEOBotCtgGuardCarrier::OnStart( CNEOBot *me, Action< CNEOBot > *priorAction )
{
	m_path.Invalidate();
	m_spot = nullptr;
	m_bArrived = false;
	m_reselectTimer.Invalidate();
	m_claimTimer.Invalidate();
	return Continue();
}

//---------------------------------------------------------------------------------------------
CNEO_Player *CNEOBotCtgGuardCarrier::GetCarrier( CNEOBot *me ) const
{
	CNEO_Player *pCarrier = ToNEOPlayer( UTIL_PlayerByIndex( NEORules()->GetGhosterPlayer() ) );
	if ( !pCarrier || !pCarrier->IsAlive() || pCarrier == me || pCarrier->GetTeamNumber() != me->GetTeamNumber() )
	{
		return nullptr;
	}

	return pCarrier;
}

//---------------------------------------------------------------------------------------------
void CNEOBotCtgGuardCarrier::SelectSpot( CNEOBot *me, CNEO_Player *pCarrier )
{
	m_path.Invalidate();
	m_bArrived = false;
	m_vecSpotCarrierPos = pCarrier->GetAbsOrigin();
	m_spot = FindGuardArea( me, pCarrier->GetLastKnownArea() );
	SetGuardSpot( me, m_spot );

	if ( !m_spot )
	{
		m_reselectTimer.Start( GUARD_RETRY_INTERVAL );
		return;
	}

	// The path claims its areas for the team, the spot included
	CNEOBotPathCompute( me, m_path, m_spot->GetCenter(), SAFEST_ROUTE );
}

//---------------------------------------------------------------------------------------------
ActionResult< CNEOBot >	CNEOBotCtgGuardCarrier::Update( CNEOBot *me, float interval )
{
	if ( NEORules()->IsRoundOver() )
	{
		return Done( "Round over" );
	}

	CNEO_Player *pCarrier = GetCarrier( me );
	if ( !pCarrier )
	{
		return Done( "No teammate carries the ghost" );
	}

	const CKnownEntity *threat = me->GetVisionInterface()->GetPrimaryKnownThreat( true );
	if ( threat && threat->IsVisibleRecently() )
	{
		// Shoot from the spot, but leave it to cut off a threat that is closer to the carrier than I am
		const Vector &vecCarrierPos = pCarrier->GetAbsOrigin();
		if ( threat->GetLastKnownPosition().DistToSqr( vecCarrierPos ) < me->GetAbsOrigin().DistToSqr( vecCarrierPos ) )
		{
			return SuspendFor( new CNEOBotAttack, "Intercepting a threat to the ghost carrier" );
		}
	}

	const bool bCarrierMoved = pCarrier->GetAbsOrigin().DistToSqr( m_vecSpotCarrierPos ) > Square( GUARD_RESELECT_DIST );
	if ( ( !m_spot && m_reselectTimer.IsElapsed() ) || ( m_spot && bCarrierMoved ) )
	{
		SelectSpot( me, pCarrier );
	}

	if ( m_spot )
	{
		const float flDistSq = me->GetAbsOrigin().AsVector2D().DistToSqr( m_spot->GetCenter().AsVector2D() );
		if ( !m_bArrived && ( me->GetLastKnownArea() == m_spot || flDistSq < Square( GUARD_ARRIVE_RANGE ) ) )
		{
			// Hold only the spot, so teammates may walk through the areas I came by
			m_bArrived = true;
			m_path.Invalidate();
			CNEOBotPathReservations()->ReleaseAllAreas( me );
			m_claimTimer.Invalidate();
		}
		else if ( m_bArrived && flDistSq > Square( GUARD_LEFT_SPOT_RANGE ) )
		{
			m_bArrived = false;
		}

		if ( m_bArrived )
		{
			if ( m_claimTimer.IsElapsed() )
			{
				CNEOBotPathReservations()->ReserveArea( m_spot, me, GUARD_CLAIM_DURATION );
				m_claimTimer.Start( GUARD_CLAIM_REFRESH );
			}
		}
		else
		{
			if ( !m_path.IsValid() )
			{
				CNEOBotPathCompute( me, m_path, m_spot->GetCenter(), SAFEST_ROUTE );
			}

			// Walk quietly, so attackers cannot hear the defense
			me->ReleaseRunButton();
			me->PressSneakButton( 0.3f );
			m_path.Update( me );
		}
	}

	LookForEnemies( me, pCarrier );
	return Continue();
}

//---------------------------------------------------------------------------------------------
// Face the newest enemy report that could come into view from here, else away from the carrier
void CNEOBotCtgGuardCarrier::LookForEnemies( CNEOBot *me, CNEO_Player *pCarrier )
{
	const CKnownEntity *threat = me->GetVisionInterface()->GetPrimaryKnownThreat( true );
	if ( threat && threat->IsVisibleRecently() )
	{
		return;
	}

	const CNavArea *pMyArea = m_spot ? m_spot : me->GetLastKnownArea();
	const bool bHasVis = pMyArea && pMyArea->GetPotentiallyVisibleAreaCount() > 0;

	CUtlVector< CKnownEntity > knownVector;
	me->GetVisionInterface()->CollectKnownEntities( &knownVector );

	const CKnownEntity *pNewest = nullptr;
	for ( int i = 0; i < knownVector.Count(); ++i )
	{
		const CKnownEntity &known = knownVector[i];
		CBaseEntity *pKnown = known.GetEntity();
		if ( !pKnown || !pKnown->IsPlayer() || !pKnown->IsAlive() || pKnown->GetTeamNumber() == me->GetTeamNumber() )
		{
			continue;
		}

		const CNavArea *pKnownArea = known.GetLastKnownArea();
		if ( !pKnownArea || ( bHasVis && !pMyArea->IsPotentiallyVisible( pKnownArea ) ) )
		{
			continue;
		}

		if ( !pNewest || known.GetTimeSinceLastKnown() < pNewest->GetTimeSinceLastKnown() )
		{
			pNewest = &known;
		}
	}

	if ( pNewest )
	{
		const Vector vecLook = pNewest->GetLastKnownPosition() + Vector( 0.0f, 0.0f, HumanEyeHeight );
		me->GetBodyInterface()->AimHeadTowards( vecLook, IBody::IMPORTANT, 0.5f, nullptr, "Guard: watching a reported enemy" );
		return;
	}

	if ( !m_bArrived )
	{
		return;
	}

	// Cover the carrier's blind side
	Vector vecFromCarrier = me->GetAbsOrigin() - pCarrier->GetAbsOrigin();
	vecFromCarrier.z = 0.0f;
	if ( VectorNormalize( vecFromCarrier ) > 32.0f )
	{
		const Vector vecLook = me->EyePosition() + vecFromCarrier * 500.0f;
		me->GetBodyInterface()->AimHeadTowards( vecLook, IBody::IMPORTANT, 0.2f, nullptr, "Guard: watching away from the carrier" );
	}
}

//---------------------------------------------------------------------------------------------
void CNEOBotCtgGuardCarrier::OnEnd( CNEOBot *me, Action< CNEOBot > *nextAction )
{
	ClearGuardSpot( me );
}

//---------------------------------------------------------------------------------------------
ActionResult< CNEOBot > CNEOBotCtgGuardCarrier::OnResume( CNEOBot *me, Action< CNEOBot > *interruptingAction )
{
	// A fight may have moved me off the spot
	m_path.Invalidate();
	m_bArrived = false;
	return Continue();
}

//---------------------------------------------------------------------------------------------
EventDesiredResult< CNEOBot > CNEOBotCtgGuardCarrier::OnStuck( CNEOBot *me )
{
	m_path.Invalidate();
	return TryContinue();
}

//---------------------------------------------------------------------------------------------
EventDesiredResult< CNEOBot > CNEOBotCtgGuardCarrier::OnMoveToFailure( CNEOBot *me, const Path *path, MoveToFailureType reason )
{
	// Try another spot after a short wait
	m_spot = nullptr;
	ClearGuardSpot( me );
	m_path.Invalidate();
	m_reselectTimer.Start( GUARD_RETRY_INTERVAL );
	return TryContinue();
}
