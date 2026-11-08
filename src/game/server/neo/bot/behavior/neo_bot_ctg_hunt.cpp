#include "cbase.h"
#include "neo_player.h"
#include "neo_gamerules.h"
#include "bot/neo_bot.h"
#include "bot/behavior/neo_bot_ctg_hunt.h"
#include "bot/neo_bot_path_compute.h"

// Known positions move with callouts, so the target and its path are revisited this often
static const float HUNT_RETARGET_INTERVAL = 2.0f;

// Extra distance a target counts as for each teammate already hunting it, so hunters spread out
static const float HUNT_SHARED_TARGET_PENALTY = 1500.0f;

// A claim not refreshed within this time belongs to a hunter that died or moved on
static const float HUNT_CLAIM_LIFETIME = HUNT_RETARGET_INTERVAL * 2.0f;

// The enemy each hunter is after, by player index
static int s_iHuntTarget[MAX_PLAYERS + 1];
static float s_flHuntClaimTime[MAX_PLAYERS + 1];


//---------------------------------------------------------------------------------------------
static void ClaimHuntTarget( CNEOBot *me, CNEO_Player *pTarget )
{
	s_iHuntTarget[me->entindex()] = pTarget ? pTarget->entindex() : 0;
	s_flHuntClaimTime[me->entindex()] = gpGlobals->curtime;
}


//---------------------------------------------------------------------------------------------
// How many live teammates other than me hunt this enemy now
static int CountOtherHunters( CNEOBot *me, int iTarget )
{
	int count = 0;
	for ( int i = 1; i <= gpGlobals->maxClients; i++ )
	{
		if ( i == me->entindex() || s_iHuntTarget[i] != iTarget )
		{
			continue;
		}

		if ( gpGlobals->curtime - s_flHuntClaimTime[i] > HUNT_CLAIM_LIFETIME )
		{
			continue;
		}

		CNEO_Player *pMate = ToNEOPlayer( UTIL_PlayerByIndex( i ) );
		if ( pMate && pMate->IsAlive() && pMate->InSameTeam( me ) )
		{
			count++;
		}
	}

	return count;
}


//---------------------------------------------------------------------------------------------
static CNEO_Player *GetTeammateCarrier( CNEOBot *me )
{
	const int iGhoster = NEORules()->GetGhosterPlayer();
	if ( iGhoster <= 0 || iGhoster == me->entindex() )
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

	m_retargetTimer.Start( HUNT_RETARGET_INTERVAL );

	return CNEOBotSeekAndDestroy::OnStart( me, priorAction );
}


//---------------------------------------------------------------------------------------------
ActionResult< CNEOBot > CNEOBotCtgHunt::Update( CNEOBot *me, float interval )
{
	if ( !IsAtkDefendingTeam( me->GetTeamNumber() ) || NEORules()->IsRoundOver() )
	{
		return Done( "No longer defending in ATK" );
	}

	if ( !GetTeammateCarrier( me ) )
	{
		// NEO-ATKH-TEMP
		Msg( "NEO_ATKH retreat bot=%d ghoster=%d\n", me->entindex(), NEORules()->GetGhosterPlayer() );

		return Done( "No teammate carries the ghost, back to the ghost" );
	}

	if ( m_retargetTimer.IsElapsed() )
	{
		m_retargetTimer.Start( HUNT_RETARGET_INTERVAL );

		// A roaming hunter keeps its roam goal until an enemy becomes known
		if ( m_hHuntTarget.Get() || SelectHuntTarget( me ) )
		{
			RecomputeSeekPath( me );
		}
	}

	return UpdateCommon( me, interval );
}


//---------------------------------------------------------------------------------------------
void CNEOBotCtgHunt::OnEnd( CNEOBot *me, Action< CNEOBot > *nextAction )
{
	ClaimHuntTarget( me, nullptr );
}


//---------------------------------------------------------------------------------------------
EventDesiredResult< CNEOBot > CNEOBotCtgHunt::OnMoveToSuccess( CNEOBot *me, const Path *path )
{
	// Nobody at the last known position: drop the stale report so the hunt moves on
	CNEO_Player *pTarget = m_hHuntTarget.Get();
	if ( pTarget )
	{
		const CKnownEntity *known = me->GetVisionInterface()->GetKnown( pTarget );
		if ( known && !known->IsVisibleRecently() )
		{
			me->GetVisionInterface()->ForgetEntity( pTarget );
		}
	}

	return CNEOBotSeekAndDestroy::OnMoveToSuccess( me, path );
}


//---------------------------------------------------------------------------------------------
// The known enemy nearest me, counting each teammate already on it as extra distance
CNEO_Player *CNEOBotCtgHunt::SelectHuntTarget( CNEOBot *me ) const
{
	CUtlVector< CKnownEntity > knownVector;
	me->GetVisionInterface()->CollectKnownEntities( &knownVector );

	CNEO_Player *pBest = nullptr;
	float flBestScore = FLT_MAX;
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
		const float flScore = flDist + HUNT_SHARED_TARGET_PENALTY * CountOtherHunters( me, pEnemy->entindex() );
		if ( flScore < flBestScore )
		{
			flBestScore = flScore;
			pBest = pEnemy;
		}
	}

	return pBest;
}


//---------------------------------------------------------------------------------------------
void CNEOBotCtgHunt::RecomputeSeekPath( CNEOBot *me )
{
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
				Msg( "NEO_ATKH target bot=%d enemy=%d others=%d\n", me->entindex(), pTarget->entindex(), CountOtherHunters( me, pTarget->entindex() ) );
			}

			m_hHuntTarget = pTarget;
			ClaimHuntTarget( me, pTarget );
			return;
		}
	}

	m_hHuntTarget = nullptr;
	ClaimHuntTarget( me, nullptr );

	// No known enemy: roam and follow combat sounds as an unleashed defender does
	CNEOBotSeekAndDestroy::RecomputeSeekPath( me );
}
