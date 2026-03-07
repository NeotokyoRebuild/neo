#include "cbase.h"
#include "bot/neo_bot.h"
#include "bot/behavior/neo_bot_attack.h"
#include "bot/behavior/neo_bot_ctg_lone_wolf_seek.h"
#include "bot/neo_bot_path_compute.h"
#include "neo_gamerules.h"
#include "neo_player.h"

//---------------------------------------------------------------------------------------------
class CSearchForUnexplored : public ISearchSurroundingAreasFunctor
{
public:
	static constexpr int DEFAULT_AREA_LIMIT = 1000;
	static constexpr int CANDIDATE_LIMIT = 10;

	CSearchForUnexplored( CNEOBot *me, CUtlMap<int, bool> &exploredAreaIds, int areaLimit = DEFAULT_AREA_LIMIT )
		: m_me( me ), m_exploredAreaIds( exploredAreaIds ), m_iAreaCount( 0 ), m_iAreaLimit( areaLimit )
	{
	}

	// return true to keep searching, return false to stop searching
	virtual bool operator() ( CNavArea *baseArea, CNavArea *priorArea, float travelDistanceSoFar ) override
	{
		if ( m_exploredAreaIds.Find( (int)baseArea->GetID() ) == m_exploredAreaIds.InvalidIndex() )
		{
			m_candidateAreas.AddToTail( baseArea );
		}

		if ( m_candidateAreas.Count() >= CANDIDATE_LIMIT )
		{
			return false;
		}

		return true;
	}

	// return true if 'adjArea' should be included in the ongoing search
	virtual bool ShouldSearch( CNavArea *adjArea, CNavArea *currentArea, float travelDistanceSoFar ) override
	{
		if ( m_candidateAreas.Count() >= CANDIDATE_LIMIT )
		{
			return false;
		}

		// hit the max search limit
		if ( ++m_iAreaCount > m_iAreaLimit )
		{
			return false;
		}

		// don't want to jump or drop
		const float heightChange = currentArea->ComputeAdjacentConnectionHeightChange( adjArea );
		if ( fabs( heightChange ) > m_me->GetLocomotionInterface()->GetStepHeight() )
		{
			return false;
		}

		return true;
	}

	CNavArea *GetRandomCandidate() const
	{
		if ( m_candidateAreas.IsEmpty() )
		{
			return nullptr;
		}
		int which = RandomInt( 0, m_candidateAreas.Count() - 1 );
		return m_candidateAreas[ which ];
	}

	CNEOBot *m_me;
	CUtlMap<int, bool> &m_exploredAreaIds;
	CUtlVector<CNavArea *> m_candidateAreas;
	int m_iAreaCount;
	int m_iAreaLimit;
};

//---------------------------------------------------------------------------------------------
ActionResult< CNEOBot >	CNEOBotCtgLoneWolfSeek::OnStart( CNEOBot *me, Action< CNEOBot > *priorAction )
{
	BaseClass::OnStart( me, priorAction );

	SetDefLessFunc( m_exploredAreaIds );
	m_exploredAreaIds.RemoveAll();
	m_iExplorationTargetId = -1;

	m_vecLastGhostPos = NEORules()->GetGhostPos();
	m_pCachedGhostArea = TheNavMesh->GetNearestNavArea( m_vecLastGhostPos );

	return Continue();
}

//---------------------------------------------------------------------------------------------
ActionResult< CNEOBot >	CNEOBotCtgLoneWolfSeek::Update( CNEOBot *me, float interval )
{
	me->PressCrouchButton( 0.2f ); // Keep a lower profile

	if ( !NEORules()->GhostExists() || !NEORules()->m_pGhost )
	{
		return Done( "Ghost not found" );
	}

	if ( me->DropGhost() )
	{
		return Continue(); // ghost drop in progress
	}

	ActionResult< CNEOBot > interceptionResult = ConsiderGhostInterception( me );
	if ( interceptionResult.IsRequestingChange() )
	{
		return interceptionResult;
	}

	CWeaponGhost *pGhostWeapon = NEORules()->m_pGhost;
	const CBaseCombatCharacter *pGhostOwner = pGhostWeapon ? pGhostWeapon->GetOwner() : nullptr;

	if ( pGhostOwner && !me->InSameTeam( pGhostOwner ) )
	{
		// Don't interrupt enemy carrier pursuit with search pathing
		return Continue();
	}

	const CKnownEntity *threat = me->GetVisionInterface()->GetPrimaryKnownThreat();
	if ( threat && threat->GetEntity() )
	{
		me->ReleaseCrouchButton(); // move faster
		return ChangeTo( new CNEOBotAttack(), "Engaging enemy from seek" );
	}

	if ( !threat && me->GetActiveWeapon() )
	{
		// Aggressively reload due to lack of backup
		me->ReloadIfLowClip(true); // force reload true
	}

	Vector vecSoundPos = me->GetAudibleEnemySoundPos();
	if ( vecSoundPos != CNEO_Player::VECTOR_INVALID_WAYPOINT )
	{
		// Don't veer path for sound if waypoint is not that far off
		if ( m_vecSearchWaypoint.DistToSqr( vecSoundPos ) > Square( 200.0f ) )
		{
			m_vecSearchWaypoint = vecSoundPos;
			m_path.Invalidate();
			m_repathTimer.Invalidate(); // path to sound next tick

			CNavArea *soundArea = TheNavMesh->GetNearestNavArea( vecSoundPos );
			if ( soundArea )
			{
				m_iExplorationTargetId = (int)soundArea->GetID();
				// Mark sound area as not explored
				m_exploredAreaIds.Remove( m_iExplorationTargetId );
			}
		}
	}

	const Vector currentGhostPos = NEORules()->GetGhostPos();
	if ( !m_pCachedGhostArea || currentGhostPos.DistToSqr( m_vecLastGhostPos ) > Square( 64.0f ) )
	{
		CNavArea *pLastGhostArea = m_pCachedGhostArea;
		m_pCachedGhostArea = TheNavMesh->GetNearestNavArea( currentGhostPos );
		m_vecLastGhostPos = currentGhostPos;

		if ( m_pCachedGhostArea != pLastGhostArea )
		{
			m_iExplorationTargetId = -1;
			m_vecSearchWaypoint = CNEO_Player::VECTOR_INVALID_WAYPOINT;

			// Restart search, as most likely someone is moving the ghost
			m_path.Invalidate();
			m_exploredAreaIds.RemoveAll();

			if ( me->IsDebugging( NEXTBOT_BEHAVIOR ) )
			{
				DevMsg( "Lone Wolf Seek: Ghost moved, re-calculating exploration targets locally\n" );
			}
		}
	}

	CNavArea *ghostArea = m_pCachedGhostArea;

	const bool bReachedSearchTarget = ( m_vecSearchWaypoint != CNEO_Player::VECTOR_INVALID_WAYPOINT && me->GetAbsOrigin().DistToSqr( m_vecSearchWaypoint ) < Square( SEARCH_WAYPOINT_REACHED_DIST ) );

	if ( bReachedSearchTarget )
	{
		if ( me->IsDebugging( NEXTBOT_BEHAVIOR ) )
		{
			DevMsg( "Lone Wolf Seek: Search area reached, looking for new search area\n" );
		}

		m_iExplorationTargetId = -1;
		m_vecSearchWaypoint = CNEO_Player::VECTOR_INVALID_WAYPOINT;
		m_path.Invalidate();
		m_repathTimer.Invalidate();
	}

	if ( m_vecSearchWaypoint == CNEO_Player::VECTOR_INVALID_WAYPOINT )
	{
		if ( !me->IsCarryingGhost() )
		{
			if ( ghostArea )
			{
				// Before searching, mark what we can see from our current position as explored,
				CNavArea *currentArea = me->GetLastKnownArea();
				if ( currentArea )
				{
					MarkVisibleAreasAsExplored( me, currentArea, ghostArea );
				}

				CSearchForUnexplored search( me, m_exploredAreaIds );
				SearchSurroundingAreas( ghostArea, search );

				if ( search.m_candidateAreas.IsEmpty() )
				{
					// Track already explored areas around the ghost
					auto searchFromVisible = [&]( CNavArea *visibleArea ) -> bool
					{
						if ( search.m_candidateAreas.Count() >= CSearchForUnexplored::CANDIDATE_LIMIT || search.m_iAreaCount >= search.m_iAreaLimit )
						{
							return false;
						}
						SearchSurroundingAreas( visibleArea, search );
						return search.m_candidateAreas.Count() < CSearchForUnexplored::CANDIDATE_LIMIT;
					};
					ghostArea->ForAllPotentiallyVisibleAreas( searchFromVisible );
				}

				if ( m_iExplorationTargetId == -1 && !search.m_candidateAreas.IsEmpty() )
				{
					CNavArea *target = search.GetRandomCandidate();
					m_iExplorationTargetId = (int)target->GetID();
					m_exploredAreaIds.InsertOrReplace( m_iExplorationTargetId, true );
					m_vecSearchWaypoint = target->GetCenter();
					m_path.Invalidate();
					m_repathTimer.Invalidate();

					if ( me->IsDebugging( NEXTBOT_PATH ) )
					{
						target->DrawFilled( 255, 255, 0, 128, DEBUG_OVERLAY_DURATION );
					}
				}
				else if ( m_iExplorationTargetId == -1 )
				{
					// All nearby areas explored, or search failed to find new search area.
					// Reset search to restart patrol
					m_exploredAreaIds.RemoveAll();

					// Fallback: move towards the ghost itself while we search
					m_path.Invalidate();
					m_repathTimer.Invalidate();
					
					if ( me->IsDebugging( NEXTBOT_BEHAVIOR ) )
					{
						DevMsg( "Lone Wolf Seek: Searched all areas around ghost, resetting seen tracking\n" );
					}
				}
			}
		}
	}

	if ( m_vecSearchWaypoint == CNEO_Player::VECTOR_INVALID_WAYPOINT )
	{
		m_vecSearchWaypoint = NEORules()->GetGhostPos();
	}

	if ( me->GetAbsOrigin().DistToSqr( m_vecSearchWaypoint ) > Square( PATH_RECOMPUTE_DIST ) )
	{
		if ( !m_repathTimer.HasStarted() || m_repathTimer.IsElapsed() )
		{
			CNEOBotPathCompute( me, m_path, m_vecSearchWaypoint, FASTEST_ROUTE );
			m_repathTimer.Start( RandomFloat( 0.3f, 1.0f ) );
		}
	}

	m_path.Update( me );

	return Continue();
}

//---------------------------------------------------------------------------------------------
EventDesiredResult< CNEOBot > CNEOBotCtgLoneWolfSeek::OnStuck( CNEOBot *me )
{
	if ( m_iExplorationTargetId != -1 )
	{
		m_exploredAreaIds.InsertOrReplace( m_iExplorationTargetId, true );
	}
	else if ( m_vecSearchWaypoint != CNEO_Player::VECTOR_INVALID_WAYPOINT )
	{
		CNavArea *area = TheNavMesh->GetNearestNavArea( m_vecSearchWaypoint );
		if ( area )
		{
			m_exploredAreaIds.InsertOrReplace( (int)area->GetID(), true );
		}
	}

	if ( me->IsDebugging( NEXTBOT_BEHAVIOR ) )
	{
		DevMsg( "Lone Wolf Seek: Bot stuck going to search area, marking as explored and finding new target\n" );
	}

	m_iExplorationTargetId = -1;
	m_vecSearchWaypoint = CNEO_Player::VECTOR_INVALID_WAYPOINT;
	m_path.Invalidate();

	return TryContinue();
}

//---------------------------------------------------------------------------------------------
EventDesiredResult< CNEOBot > CNEOBotCtgLoneWolfSeek::OnMoveToSuccess( CNEOBot *me, const Path *path )
{
	return TryContinue();
}

//---------------------------------------------------------------------------------------------
EventDesiredResult< CNEOBot > CNEOBotCtgLoneWolfSeek::OnMoveToFailure( CNEOBot *me, const Path *path, MoveToFailureType reason )
{
	m_path.Invalidate();
	return TryContinue();
}

//---------------------------------------------------------------------------------------------
void CNEOBotCtgLoneWolfSeek::MarkVisibleAreasAsExplored( CNEOBot *me, CNavArea *currentArea, CNavArea *ghostArea )
{
	if ( !currentArea )
	{
		return;
	}

	// Mark the currently occupied area explored
	if ( m_exploredAreaIds.InsertOrReplace( (int)currentArea->GetID(), true ) != m_exploredAreaIds.InvalidIndex() )
	{
		if ( me->IsDebugging( NEXTBOT_PATH ) )
		{
			currentArea->DrawFilled( 0, 255, 0, 128, DEBUG_OVERLAY_DURATION );
		}
	}

	// Mark all potentially visible areas
	auto markVisible = [this, me, ghostArea]( CNavArea *area ) -> bool
	{
		if ( area && area != ghostArea )
		{
			if ( m_exploredAreaIds.InsertOrReplace( (int)area->GetID(), true ) != m_exploredAreaIds.InvalidIndex() )
			{
				if ( me->IsDebugging( NEXTBOT_PATH ) )
				{
					area->DrawFilled( 0, 255, 0, 128, DEBUG_OVERLAY_DURATION );
				}
			}
		}
		return true;
	};
	currentArea->ForAllPotentiallyVisibleAreas( markVisible );
}

