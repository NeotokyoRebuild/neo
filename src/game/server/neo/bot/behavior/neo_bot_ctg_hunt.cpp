#include "cbase.h"
#include "neo_player.h"
#include "neo_gamerules.h"
#include "bot/neo_bot.h"
#include "bot/behavior/neo_bot_ctg_hunt.h"
#include "bot/neo_bot_ctg_carry_block.h"
#include "bot/neo_bot_path_compute.h"
#include "nav_mesh.h"
#include "tier0/fasttimer.h"

// Known positions move with callouts, so the target and its path are revisited this often
static const float HUNT_RETARGET_INTERVAL = 2.0f;

// A completed route only investigates the report it actually reached
static const float HUNT_ARRIVAL_TOLERANCE = 32.0f;

// How recent the target's report must be to plan a watch on it, and to keep one
static const float HUNT_WATCH_ENTRY_FRESHNESS = 5.0f;
static const float HUNT_WATCH_STAY_FRESHNESS = 10.0f;

// A watch with no newer report than this is a guess gone cold
static const float HUNT_WATCH_MAX_HOLD = 12.0f;

// Same arrival test as the carrier's hold
static const float HUNT_WATCH_ARRIVE_RANGE = 48.0f;

// Spreads the team's first watch plans over a few half-second steps
static const int HUNT_WATCH_STAGGER_SLOTS = 4;
static const float HUNT_WATCH_STAGGER_STEP = 0.5f;

//---------------------------------------------------------------------------------------------
static CNEO_Player *GetTeammateCarrier( CNEOBot *me )
{
	const int iGhoster = NEORules()->GetGhosterPlayer();
	if ( !NEORules()->GhostExists() || iGhoster <= 0 || iGhoster == me->entindex() )
	{
		return nullptr;
	}

	CNEO_Player *pCarrier = ToNEOPlayer( UTIL_PlayerByIndex( iGhoster ) );
	if ( !pCarrier || !pCarrier->IsAlive() || !pCarrier->InSameTeam( me ) )
	{
		return nullptr;
	}

	return pCarrier;
}


//---------------------------------------------------------------------------------------------
ActionResult< CNEOBot > CNEOBotCtgHunt::OnStart( CNEOBot *me, Action< CNEOBot > *priorAction )
{
	// NEO-ATKH-TEMP
	Msg( "NEO_ATKH hunt start bot=%d\n", me->entindex() );

	m_retargetTimer.Invalidate();

	// Teammates entering the hunt together would otherwise all plan on one tick
	m_bHasWatch = false;
	m_bHolding = false;
	m_watchStaggerTimer.Start( ( me->entindex() % HUNT_WATCH_STAGGER_SLOTS ) * HUNT_WATCH_STAGGER_STEP );

	return CNEOBotSeekAndDestroy::OnStart( me, priorAction );
}


//---------------------------------------------------------------------------------------------
ActionResult< CNEOBot > CNEOBotCtgHunt::Update( CNEOBot *me, float interval )
{
	if ( !NEOBotCarryBlock::IsTeamHoldingGhost( me->GetTeamNumber() ) || NEORules()->IsRoundOver() )
	{
		return Done( "The team no longer holds the ghost" );
	}

	if ( !GetTeammateCarrier( me ) )
	{
		// NEO-ATKH-TEMP
		Msg( "NEO_ATKH retreat bot=%d ghoster=%d\n", me->entindex(), NEORules()->GetGhosterPlayer() );

		return Done( "No teammate carries the ghost, back to the ghost" );
	}

	if ( m_retargetTimer.IsElapsed() )
	{
		// A roaming hunter keeps its roam goal until an enemy becomes known
		if ( m_bRepathPending || !m_path.IsValid() || m_hHuntTarget.Get() || SelectHuntTarget( me ) )
		{
			RecomputeSeekPath( me );
		}
		else
		{
			m_retargetTimer.Start( HUNT_RETARGET_INTERVAL );
		}
	}

	if ( m_bHasWatch )
	{
		UpdateWatch( me );
	}

	return UpdateCommon( me, interval );
}


//---------------------------------------------------------------------------------------------
EventDesiredResult< CNEOBot > CNEOBotCtgHunt::OnMoveToSuccess( CNEOBot *me, const Path *path )
{
	// Suspended actions also receive movement events from the action fighting above them
	if ( path != &m_path )
	{
		return TryContinue();
	}

	// A newer callout or a partial route must not erase an uninvestigated report,
	// and a path to a watch area ends beside the route, not at the report
	CNEO_Player *pTarget = m_hHuntTarget.Get();
	if ( pTarget && !m_bListenForCombatSounds && !m_bHasWatch )
	{
		const CKnownEntity *known = me->GetVisionInterface()->GetKnown( pTarget );
		if ( known && !known->IsVisibleRecently()
			&& m_vGoalPos.DistToSqr( known->GetLastKnownPosition() ) <= Square( HUNT_ARRIVAL_TOLERANCE )
			&& path->GetEndPosition().DistToSqr( known->GetLastKnownPosition() ) <= Square( HUNT_ARRIVAL_TOLERANCE ) )
		{
			me->GetVisionInterface()->ForgetEntity( pTarget );
		}
	}

	return CNEOBotSeekAndDestroy::OnMoveToSuccess( me, path );
}


//---------------------------------------------------------------------------------------------
// The valid known enemy nearest me by its last known position
CNEO_Player *CNEOBotCtgHunt::SelectHuntTarget( CNEOBot *me ) const
{
	CUtlVector< CKnownEntity > knownVector;
	me->GetVisionInterface()->CollectKnownEntities( &knownVector );

	CNEO_Player *pBest = nullptr;
	float flBestDist = FLT_MAX;
	for ( int i = 0; i < knownVector.Count(); i++ )
	{
		const CKnownEntity &known = knownVector[i];
		if ( known.IsObsolete() )
		{
			continue;
		}

		CNEO_Player *pEnemy = ToNEOPlayer( known.GetEntity() );
		if ( !pEnemy || !pEnemy->IsAlive() || pEnemy->InSameTeam( me ) )
		{
			continue;
		}

		const float flDist = me->GetAbsOrigin().DistTo( known.GetLastKnownPosition() );
		if ( flDist < flBestDist )
		{
			flBestDist = flDist;
			pBest = pEnemy;
		}
	}

	return pBest;
}


//---------------------------------------------------------------------------------------------
void CNEOBotCtgHunt::RecomputeSeekPath( CNEOBot *me )
{
	// Resume and movement callbacks share the retry budget with regular updates
	if ( m_retargetTimer.HasStarted() && !m_retargetTimer.IsElapsed() )
	{
		m_bRepathPending = true;
		return;
	}
	m_retargetTimer.Start( HUNT_RETARGET_INTERVAL );
	m_bRepathPending = false;

	CNEO_Player *pTarget = SelectHuntTarget( me );
	const CKnownEntity *known = pTarget ? me->GetVisionInterface()->GetKnown( pTarget ) : nullptr;
	if ( known && TryPlanWatch( me, pTarget, *known ) )
	{
		return;
	}

	if ( m_bHasWatch )
	{
		DropWatch( me, "lost" );
	}

	if ( known )
	{
		m_hTargetEntity = nullptr;
		m_bGoingToTargetEntity = false;
		m_vGoalPos = known->GetLastKnownPosition();

		if ( CNEOBotPathCompute( me, m_path, m_vGoalPos, DEFAULT_ROUTE ) && m_path.IsValid() )
		{
			// NEO-ATKH-TEMP
			if ( m_hHuntTarget.Get() != pTarget )
			{
				Msg( "NEO_ATKH target bot=%d enemy=%d dist=%.1f\n", me->entindex(), pTarget->entindex(), me->GetAbsOrigin().DistTo( known->GetLastKnownPosition() ) );
			}

			m_hHuntTarget = pTarget;
			m_bListenForCombatSounds = false;
			return;
		}
	}

	m_hHuntTarget = nullptr;
	m_bListenForCombatSounds = true;

	// No known enemy: roam and follow combat sounds as an unleashed teammate does
	CNEOBotSeekAndDestroy::RecomputeSeekPath( me );
}


//---------------------------------------------------------------------------------------------
// Plans, keeps or drops the watch on the target; one call decides entry and exit
bool CNEOBotCtgHunt::TryPlanWatch( CNEOBot *me, CNEO_Player *pTarget, const CKnownEntity &known )
{
	if ( m_bHasWatch && ( m_hHuntTarget.Get() != pTarget ) )
	{
		DropWatch( me, "target" );
	}

	if ( !m_watchStaggerTimer.IsElapsed() )
	{
		return false;
	}

	CNEO_Player *pCarrier = GetTeammateCarrier( me );
	if ( !pCarrier )
	{
		if ( m_bHasWatch )
		{
			DropWatch( me, "lost" );
		}

		return false;
	}

	const float flAge = known.GetTimeSinceLastKnown();
	const float flFreshness = m_bHasWatch ? HUNT_WATCH_STAY_FRESHNESS : HUNT_WATCH_ENTRY_FRESHNESS;
	if ( flAge > flFreshness )
	{
		if ( m_bHasWatch )
		{
			DropWatch( me, "stale" );
		}

		return false;
	}

	CFastTimer timer;
	timer.Start();

	WatchArea_t watch;
	const char *pszFailReason = "race";
	const bool bFound = NEOBotDefense::FindWatchArea( me, known, pCarrier, m_bHasWatch ? &m_watch : nullptr, watch, &pszFailReason );

	timer.End();
	const int nMicroseconds = (int)timer.GetDuration().GetMicroseconds();

	if ( !bFound )
	{
		if ( m_bHasWatch )
		{
			DropWatch( me, pszFailReason );
		}

		return false;
	}

	const bool bNewWatch = !m_bHasWatch || ( watch.pWatch != m_watch.pWatch );
	if ( bNewWatch || ( !m_bHolding && !m_path.IsValid() ) )
	{
		if ( !CNEOBotPathCompute( me, m_path, watch.pWatch->GetCenter(), SAFEST_ROUTE ) || !m_path.IsValid() )
		{
			if ( m_bHasWatch )
			{
				DropWatch( me, "route" );
			}

			return false;
		}
	}

	if ( bNewWatch )
	{
		// NEO-DEF-TEMP
		Msg( "NEO_DEF watch bot=%d enemy=%d watched=%u watch=%u mine=%.0f eta=%.0f age=%.1f us=%d\n",
			me->entindex(), pTarget->entindex(), watch.pWatched->GetID(), watch.pWatch->GetID(),
			watch.flMyTravel, watch.flEnemyEta, flAge, nMicroseconds );

		m_bHolding = false;
		m_flLastReport = gpGlobals->curtime - flAge;
	}
	else
	{
		m_flLastReport = Max( m_flLastReport, gpGlobals->curtime - flAge );
	}

	m_watch = watch;
	m_bHasWatch = true;
	m_hHuntTarget = pTarget;
	m_hTargetEntity = nullptr;
	m_bGoingToTargetEntity = false;
	m_vGoalPos = watch.pWatch->GetCenter();
	m_bListenForCombatSounds = false;
	return true;
}


//---------------------------------------------------------------------------------------------
// Holds a reached watch area still, low and aimed at the route; combat stays with UpdateCommon
void CNEOBotCtgHunt::UpdateWatch( CNEOBot *me )
{
	const CKnownEntity *threat = me->GetVisionInterface()->GetPrimaryKnownThreat( true );
	if ( threat && threat->IsVisibleRecently() && threat->GetEntity() )
	{
		const CNavArea *pThreatArea = threat->GetLastKnownArea();
		const bool bOnRoute = pThreatArea
			&& ( ( pThreatArea == m_watch.pWatched ) || m_watch.pWatched->IsPotentiallyVisible( pThreatArea ) );

		// NEO-DEF-TEMP
		Msg( "NEO_DEF contact bot=%d enemy=%d holding=%d onroute=%d first=na\n",
			me->entindex(), threat->GetEntity()->entindex(), m_bHolding ? 1 : 0, bOnRoute ? 1 : 0 );

		m_bHasWatch = false;
		m_bHolding = false;
		return;
	}

	const bool bArrived = ( me->GetLastKnownArea() == m_watch.pWatch )
		|| ( me->GetAbsOrigin().AsVector2D().DistToSqr( m_watch.pWatch->GetCenter().AsVector2D() ) < Square( HUNT_WATCH_ARRIVE_RANGE ) );
	if ( !bArrived )
	{
		return;
	}

	if ( !m_bHolding )
	{
		m_bHolding = true;
		m_flHoldStart = gpGlobals->curtime;

		// NEO-DEF-TEMP
		Msg( "NEO_DEF hold bot=%d watch=%u\n", me->entindex(), m_watch.pWatch->GetID() );
	}

	if ( gpGlobals->curtime - Max( m_flHoldStart, m_flLastReport ) > HUNT_WATCH_MAX_HOLD )
	{
		DropWatch( me, "stale" );
		return;
	}

	m_path.Invalidate();
	me->PressCrouchButton( 0.3f );
	me->GetBodyInterface()->AimHeadTowards( m_watch.pWatched->GetCenter() + Vector( 0.0f, 0.0f, HumanEyeHeight ),
		IBody::INTERESTING, 0.3f, nullptr, "Watching the enemy's route to our carrier" );
}


//---------------------------------------------------------------------------------------------
void CNEOBotCtgHunt::DropWatch( CNEOBot *me, const char *pszReason )
{
	// NEO-DEF-TEMP
	Msg( "NEO_DEF drop bot=%d reason=%s\n", me->entindex(), pszReason );

	m_bHasWatch = false;
	m_bHolding = false;
	m_path.Invalidate();
}
