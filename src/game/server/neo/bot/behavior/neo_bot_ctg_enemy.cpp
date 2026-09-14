#include "cbase.h"
#include "neo_player.h"
#include "neo_gamerules.h"
#include "neo_ghost_cap_point.h"
#include "bot/neo_bot.h"
#include "bot/behavior/neo_bot_ctg_enemy.h"
#include "bot/behavior/neo_bot_ctg_enemy_intercept_cap_path.h"
#include "bot/behavior/neo_bot_attack.h"
#include "bot/neo_bot_path_compute.h"

// --- pressure vs. block --------------------------------------------------------------------------
// CNEOBotCtgSeek hands every enemy-carrier defender to CNEOBotCtgEnemy; the choice of whether to
// chase the carrier directly or peel off into CNEOBotCtgEnemyInterceptCapPath to hold a cut-off is
// made here, tick by tick, from observable state only -- no lookup of the carrier's real path.
// Routing every defender to interception pulls the last body off the carrier and, in testing,
// conceded more ghost captures rather than fewer, so the nearest few defenders always chase.
ConVar sv_neo_bot_ctg_pressure_slots( "sv_neo_bot_ctg_pressure_slots", "2", FCVAR_CHEAT,
	"CTG defence: the N friendly defenders nearest the enemy ghost carrier chase it directly; the "
	"rest peel off to hold a cut-off on its predicted path.", true, 0, true, 8 );

//---------------------------------------------------------------------------------------------
CNEOBotCtgEnemy::CNEOBotCtgEnemy( void )
{
}

//---------------------------------------------------------------------------------------------
// true => this bot should hold a cut-off (interception) rather than chase directly.
bool CNEOBotCtgEnemy::ShouldInterceptInsteadOfChase( CNEOBot *me, CNEO_Player *pGhostCarrier ) const
{
	CNavArea *pCarrierArea = pGhostCarrier->GetLastKnownArea();
	if ( !pCarrierArea )
	{
		// No observable fix on the carrier -- just chase; do not go set up a cut-off off stale data.
		return false;
	}

	// Nothing to intercept toward unless some cap zone is actually enemy-reachable this round
	// (owningTeamAlternate() flips on odd rounds). If none is, chasing is all there is.
	const int myTeam = me->GetTeamNumber();
	bool bAnyEnemyCap = false;
	for ( int i = 0; i < NEORules()->m_pGhostCaps.Count(); ++i )
	{
		CNEOGhostCapturePoint *pCapPoint = dynamic_cast< CNEOGhostCapturePoint* >( UTIL_EntityByIndex( NEORules()->m_pGhostCaps[i] ) );
		// The GetActive() guard has to match the interception behaviour's own cap scan. If the only
		// enemy zone is disabled, that behaviour finds nothing to cut off and hands straight back,
		// so peeling off to it here would churn between chase and interception every few seconds.
		if ( pCapPoint && pCapPoint->GetActive() && pCapPoint->owningTeamAlternate() != myTeam )
		{
			bAnyEnemyCap = true;
			break;
		}
	}
	if ( !bAnyEnemyCap )
	{
		return false;
	}

	// Rank this bot among living friendly defenders by distance to the carrier's last-known spot.
	// The nearest sv_neo_bot_ctg_pressure_slots chase; everyone farther peels off to a cut-off.
	// Tactically sound (near bodies pursue, rear bodies cut off) and stable frame to frame.
	const Vector vecCarrier = pCarrierArea->GetCenter();
	const float myDistSq = me->GetAbsOrigin().DistToSqr( vecCarrier );
	int iNearerTeammates = 0;
	for ( int i = 1; i <= gpGlobals->maxClients; ++i )
	{
		CNEO_Player *pTeammate = ToNEOPlayer( UTIL_PlayerByIndex( i ) );
		if ( !pTeammate || pTeammate == me || !pTeammate->IsAlive() || pTeammate->GetTeamNumber() != myTeam )
		{
			continue;
		}
		if ( pTeammate->GetAbsOrigin().DistToSqr( vecCarrier ) < myDistSq )
		{
			++iNearerTeammates;
		}
	}

	return iNearerTeammates >= sv_neo_bot_ctg_pressure_slots.GetInt();
}

//---------------------------------------------------------------------------------------------
ActionResult< CNEOBot >	CNEOBotCtgEnemy::OnStart( CNEOBot *me, Action< CNEOBot > *priorAction )
{
	m_chasePath.SetMinLookAheadDistance( me->GetDesiredPathLookAheadRange() );

	// Give positions a moment to settle before the first chase-vs-intercept decision, so the
	// "nearest N" ranking is meaningful rather than whatever the spawn order happened to be.
	m_roleReviewTimer.Start( 0.5f );
	m_forcePursueTimer.Invalidate();

	return Continue();
}

//---------------------------------------------------------------------------------------------
ActionResult< CNEOBot >	CNEOBotCtgEnemy::Update( CNEOBot *me, float interval )
{
	if ( !NEORules()->GhostExists() )
	{
		return Done( "Ghost does not exist" );
	}

	if ( NEORules()->GetGhosterPlayer() <= 0 )
	{
		return Done( "No ghost carrier" );
	}

	CNEO_Player* pGhostCarrier = ToNEOPlayer( UTIL_PlayerByIndex( NEORules()->GetGhosterPlayer() ) );
	if ( !pGhostCarrier || pGhostCarrier->GetTeamNumber() == me->GetTeamNumber() )
	{
		return Done( "Ghost carrier is friendly" );
	}

	// Chase-vs-intercept: once the initial settle delay has passed, and unless we are inside the
	// post-interception "keep chasing" window, a bot that is not one of the nearest defenders peels
	// off to hold a cut-off. Going the other way (intercept -> chase) is driven by the interception
	// behaviour handing control back on contact / timeout, handled in OnResume.
	//
	// This sits ahead of the threat check below because interception is a repositioning job, not a
	// disengagement: a bot that peels off still shoots at whatever it can see, since
	// CNEOBotMainAction::Update calls FireWeaponAtEnemy() every tick whatever behaviour is running.
	const bool bForcedPursue = m_forcePursueTimer.HasStarted() && !m_forcePursueTimer.IsElapsed();
	if ( m_roleReviewTimer.IsElapsed() && !bForcedPursue )
	{
		// Re-decide a few times a second rather than every tick. The ranking below walks every
		// player and every capture zone, and the answer cannot meaningfully change tick to tick.
		m_roleReviewTimer.Start( 0.5f );

		if ( ShouldInterceptInsteadOfChase( me, pGhostCarrier ) )
		{
			return SuspendFor( new CNEOBotCtgEnemyInterceptCapPath, "Repositioning to cut off the carrier" );
		}
	}

	const CKnownEntity *threat = me->GetVisionInterface()->GetPrimaryKnownThreat(true);
	if ( threat && !threat->IsObsolete() && me->GetIntentionInterface()->ShouldAttack( me, threat ) )
	{
		return SuspendFor( new CNEOBotAttack(pGhostCarrier->GetAbsOrigin()), "Attacking ghoster team" );
	}

	// Investigate the ghost carrier's position
	CNEOBotPathUpdateChase( me, m_chasePath, pGhostCarrier, DEFAULT_ROUTE );

	return Continue();
}



//---------------------------------------------------------------------------------------------
ActionResult< CNEOBot > CNEOBotCtgEnemy::OnResume( CNEOBot *me, Action< CNEOBot > *interruptingAction )
{
	// Coming back from the interception behaviour (it hands control back on real contact, on the
	// carrier slipping past, or on its give-up timer): chase directly for a bit before this bot is
	// eligible to peel off again, so it does not immediately abandon a contact it just made or
	// bounce straight back to a cut-off it just failed to hold.
	if ( interruptingAction && FStrEq( interruptingAction->GetName(), "ctgEnemyInterceptCapPath" ) )
	{
		m_forcePursueTimer.Start( 5.0f );
	}

	return Continue();
}

//---------------------------------------------------------------------------------------------
EventDesiredResult< CNEOBot > CNEOBotCtgEnemy::OnStuck( CNEOBot *me )
{
	return TryContinue();
}

//---------------------------------------------------------------------------------------------
EventDesiredResult< CNEOBot > CNEOBotCtgEnemy::OnMoveToSuccess( CNEOBot *me, const Path *path )
{
	return TryContinue();
}

//---------------------------------------------------------------------------------------------
EventDesiredResult< CNEOBot > CNEOBotCtgEnemy::OnMoveToFailure( CNEOBot *me, const Path *path, MoveToFailureType reason )
{
	return TryContinue();
}
