#include "cbase.h"
#include "neo_player.h"
#include "neo_gamerules.h"
#include "bot/neo_bot.h"
#include "bot/behavior/neo_bot_ctg_hunt.h"
#include "bot/neo_bot_path_compute.h"

// Known positions move with callouts, so the target and its path are revisited this often
static const float HUNT_RETARGET_INTERVAL = 2.0f;

// A completed route only investigates the report it actually reached
static const float HUNT_ARRIVAL_TOLERANCE = 32.0f;

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

	return CNEOBotSeekAndDestroy::OnStart( me, priorAction );
}


//---------------------------------------------------------------------------------------------
ActionResult< CNEOBot > CNEOBotCtgHunt::Update( CNEOBot *me, float interval )
{
	if ( !IsGhostGameType() || HasGhostCapZone( me->GetTeamNumber() ) || NEORules()->IsRoundOver() )
	{
		return Done( "No longer hunting for a zoneless team" );
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

	// A newer callout or a partial route must not erase an uninvestigated report
	CNEO_Player *pTarget = m_hHuntTarget.Get();
	if ( pTarget && !m_bListenForCombatSounds )
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
