#include "cbase.h"
#include "neo/bot/neo_bot_ghost_guard.h"
#include "neo/bot/neo_bot.h"
#include "neo/bot/neo_bot_path_reservation.h"
#include "nav_mesh.h"
#include "nav_pathfind.h"
#include "neo_gamerules.h"

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

extern ConVar sv_neo_grenade_blast_radius;

// A guard searches for its spot at most this far from the carrier by travel distance, so it can still cover the carrier
static constexpr float GUARD_MAX_TRAVEL = 1000.0f;

// The carrier looks for a less exposed area among the areas its own area can see within this distance,
static constexpr float CARRIER_CLIMB_STEP_DIST = 600.0f;
// at most this many steps
static constexpr int CARRIER_CLIMB_MAX_STEPS = 8;
// and only where a path is at most this much longer than the straight line, so a step does not lead around the map
static constexpr float CARRIER_CLIMB_MAX_DETOUR = 2.0f;
// and tests the walk to at most this many of the best candidates a step
static constexpr int CARRIER_CLIMB_MAX_PATH_TESTS = 8;

// A guard or carrier holds only an area at least a player wide each way:
// nav_generate leaves narrow strips along walls, where a bot often cannot reach the center
static constexpr float HOLD_AREA_MIN_WIDTH = 2.0f * HalfHumanWidth;

static bool IsWideEnoughToHold( const CNavArea *area )
{
	return area->GetSizeX() >= HOLD_AREA_MIN_WIDTH && area->GetSizeY() >= HOLD_AREA_MIN_WIDTH;
}

// The nav area each carrier holds, by player index
static int s_iHoldAreaID[MAX_PLAYERS + 1];

// The nav area each guard holds, by player index, so guards spread out instead of bunching up
static int s_iGuardSpotAreaID[MAX_PLAYERS + 1];


//-----------------------------------------------------------------------------------------------------
bool IsGhostGuardRound( int team )
{
	if ( !IsAtkDefendingTeam( team ) )
	{
		return false;
	}

	return NEORules()->GetNearestGhostCapPoint( team, vec3_origin ) == CNEO_Player::VECTOR_INVALID_WAYPOINT;
}


//-----------------------------------------------------------------------------------------------------
void SetGuardSpot( CNEOBot *me, CNavArea *pArea )
{
	s_iGuardSpotAreaID[me->entindex()] = pArea ? pArea->GetID() : 0;
}


//-----------------------------------------------------------------------------------------------------
void ClearGuardSpot( CNEOBot *me )
{
	s_iGuardSpotAreaID[me->entindex()] = 0;
}


//-----------------------------------------------------------------------------------------------------
// How many live teammate guards hold a spot within a grenade's blast radius of this area
static int CountGuardsNear( const CNEOBot *me, const CNavArea *pArea )
{
	const float flSpacingSq = Square( sv_neo_grenade_blast_radius.GetFloat() );
	int nNear = 0;

	for ( int i = 1; i <= gpGlobals->maxClients; i++ )
	{
		if ( i == me->entindex() || s_iGuardSpotAreaID[i] == 0 )
		{
			continue;
		}

		CBasePlayer *pPlayer = UTIL_PlayerByIndex( i );
		if ( !pPlayer || !pPlayer->IsAlive() || pPlayer->GetTeamNumber() != me->GetTeamNumber() )
		{
			continue;
		}

		const CNavArea *pSpot = TheNavMesh->GetNavAreaByID( s_iGuardSpotAreaID[i] );
		if ( pSpot && pSpot->GetCenter().DistToSqr( pArea->GetCenter() ) < flSpacingSq )
		{
			nNear++;
		}
	}

	return nNear;
}


//-----------------------------------------------------------------------------------------------------
// Searches outward from the carrier for an area a guard can stand in:
// out of grenade range of the carrier, not taken by a teammate, and potentially visible from the carrier if it can be
class CSearchForGuardArea : public ISearchSurroundingAreasFunctor
{
public:
	CSearchForGuardArea( CNEOBot *me, CNavArea *pCenterArea ) : m_me( me ), m_pCenterArea( pCenterArea )
	{
		m_pBest = nullptr;
		m_nBestCrowding = INT_MAX;
		m_flBestDistSq = FLT_MAX;
		m_bCenterHasVis = pCenterArea->GetPotentiallyVisibleAreaCount() > 0;
	}

	virtual bool operator() ( CNavArea *area, CNavArea *priorArea, float travelDistanceSoFar ) override
	{
		if ( !IsCandidate( area ) )
		{
			return true;
		}

		// A spot the carrier can see is best, so the two support each other, but in a small pocket
		// a free spot out of its sight beats one a teammate already covers
		const int nTaken = CNEOBotPathReservations()->GetPredictedFriendlyPathCount( area->GetID(), m_me->GetTeamNumber(), m_me )
			+ CountGuardsNear( m_me, area );
		const bool bSeesCarrier = !m_bCenterHasVis || m_pCenterArea->IsPotentiallyVisible( area );
		const int nCrowding = 2 * nTaken + ( bSeesCarrier ? 0 : 1 );

		// Of the free areas, take the one nearest to me, so guards do not cross each other's way
		const float flDistSq = area->GetCenter().DistToSqr( m_me->GetAbsOrigin() );
		if ( nCrowding < m_nBestCrowding || ( nCrowding == m_nBestCrowding && flDistSq < m_flBestDistSq ) )
		{
			m_pBest = area;
			m_nBestCrowding = nCrowding;
			m_flBestDistSq = flDistSq;
		}

		return true;
	}

	CNavArea *m_pBest;

private:
	bool IsCandidate( const CNavArea *area ) const
	{
		// a guard stands at least a grenade's blast radius from the carrier, so one frag does not take both
		const float flMinDist = sv_neo_grenade_blast_radius.GetFloat();
		if ( area->GetCenter().AsVector2D().DistToSqr( m_pCenterArea->GetCenter().AsVector2D() ) < Square( flMinDist ) )
		{
			return false;
		}

		// a guard stands upright, where it can see and move freely
		if ( !IsWideEnoughToHold( area ) || ( area->GetAttributes() & ( NAV_MESH_CROUCH | NAV_MESH_JUMP | NAV_MESH_AVOID ) ) )
		{
			return false;
		}

		const AreaPenalty_t penalty = CNEOBotPathReservations()->GetAreaPenalty( area->GetID() );
		return !penalty.IsStuckProne() && !CNEOBotPathReservations()->IsAreaHazardous( area->GetID(), m_me );
	}

	CNEOBot *m_me;
	CNavArea *m_pCenterArea;
	int m_nBestCrowding;
	float m_flBestDistSq;
	bool m_bCenterHasVis;
};


//-----------------------------------------------------------------------------------------------------
CNavArea *FindGuardArea( CNEOBot *me, CNavArea *pCenterArea )
{
	if ( !pCenterArea )
	{
		return nullptr;
	}

	CSearchForGuardArea search( me, pCenterArea );
	SearchSurroundingAreas( pCenterArea, search, GUARD_MAX_TRAVEL );
	return search.m_pBest;
}


//-----------------------------------------------------------------------------------------------------
void SetGhostHoldArea( CNEOBot *pCarrier, CNavArea *pArea )
{
	s_iHoldAreaID[pCarrier->entindex()] = pArea ? pArea->GetID() : 0;
}


//-----------------------------------------------------------------------------------------------------
CNavArea *GetGhostHoldArea( const CNEO_Player *pCarrier )
{
	const int iAreaID = s_iHoldAreaID[pCarrier->entindex()];
	return iAreaID ? TheNavMesh->GetNavAreaByID( iAreaID ) : nullptr;
}


//-----------------------------------------------------------------------------------------------------
// A step of the carrier's climb: an area the current area can see, its exposure and its distance to the attackers' zones
struct ClimbCandidate_t
{
	CNavArea *area;
	int nVisible;
	float flZoneDist;
};

static int CompareClimbCandidates( const ClimbCandidate_t *a, const ClimbCandidate_t *b )
{
	// least exposed first, then farther from the attackers' zones, then lower ID so the choice is the same every round
	if ( a->nVisible != b->nVisible )
	{
		return a->nVisible < b->nVisible ? -1 : 1;
	}

	if ( a->flZoneDist != b->flZoneDist )
	{
		return a->flZoneDist > b->flZoneDist ? -1 : 1;
	}

	return a->area->GetID() < b->area->GetID() ? -1 : 1;
}

//-----------------------------------------------------------------------------------------------------
// Collects the areas the current area can see within a step that are less exposed and no closer to the attackers' zones
class CCollectLessExposedAreas
{
public:
	CCollectLessExposedAreas( CNEOBot *me, CNavArea *pCurrentArea, CUtlVector< ClimbCandidate_t > *pCandidates )
		: m_me( me ), m_pCurrent( pCurrentArea ), m_pCandidates( pCandidates )
	{
		m_iAttackers = GetEnemyTeam( me->GetTeamNumber() );
		m_flCurrentZoneDist = GetZoneDist( pCurrentArea );
	}

	bool operator() ( CNavArea *area )
	{
		// strictly less exposed, so the climb cannot go back and forth between equal areas
		const int nVisible = area->GetPotentiallyVisibleAreaCount();
		if ( nVisible == 0 || nVisible >= m_pCurrent->GetPotentiallyVisibleAreaCount() )
		{
			return true;
		}

		if ( area->GetCenter().DistToSqr( m_pCurrent->GetCenter() ) > Square( CARRIER_CLIMB_STEP_DIST ) )
		{
			return true;
		}

		// the carrier stands upright, where it can move freely
		if ( !IsWideEnoughToHold( area ) || ( area->GetAttributes() & ( NAV_MESH_CROUCH | NAV_MESH_JUMP | NAV_MESH_AVOID ) ) )
		{
			return true;
		}

		const AreaPenalty_t penalty = CNEOBotPathReservations()->GetAreaPenalty( area->GetID() );
		if ( penalty.IsStuckProne() || CNEOBotPathReservations()->IsAreaHazardous( area->GetID(), m_me ) )
		{
			return true;
		}

		// carry the ghost away from the attackers' zones, never toward them
		const float flZoneDist = GetZoneDist( area );
		if ( flZoneDist < m_flCurrentZoneDist )
		{
			return true;
		}

		m_pCandidates->AddToTail( { area, nVisible, flZoneDist } );
		return true;
	}

private:
	float GetZoneDist( const CNavArea *area ) const
	{
		const Vector vecZone = NEORules()->GetNearestGhostCapPoint( m_iAttackers, area->GetCenter() );
		if ( vecZone == CNEO_Player::VECTOR_INVALID_WAYPOINT )
		{
			return FLT_MAX;
		}

		return vecZone.DistTo( area->GetCenter() );
	}

	CNEOBot *m_me;
	CNavArea *m_pCurrent;
	CUtlVector< ClimbCandidate_t > *m_pCandidates;
	int m_iAttackers;
	float m_flCurrentZoneDist;
};


//-----------------------------------------------------------------------------------------------------
// Can a bot walk from one area to the other without a long detour
static bool IsShortWalk( CNavArea *pFrom, CNavArea *pTo )
{
	const float flMaxLength = CARRIER_CLIMB_MAX_DETOUR * pFrom->GetCenter().DistTo( pTo->GetCenter() );
	ShortestPathCost costFunc;
	return NavAreaBuildPath( pFrom, pTo, nullptr, costFunc, nullptr, flMaxLength );
}


//-----------------------------------------------------------------------------------------------------
CNavArea *FindLeastExposedArea( CNEOBot *me, CNavArea *pStartArea )
{
	// Without nav_analyze visibility data there is nothing to judge by, so stay put
	if ( !pStartArea || pStartArea->GetPotentiallyVisibleAreaCount() == 0 )
	{
		return pStartArea;
	}

	CNavArea *pCurrent = pStartArea;
	for ( int iStep = 0; iStep < CARRIER_CLIMB_MAX_STEPS; iStep++ )
	{
		CUtlVector< ClimbCandidate_t > candidates;
		CCollectLessExposedAreas collect( me, pCurrent, &candidates );
		pCurrent->ForAllPotentiallyVisibleAreas( collect );
		candidates.Sort( CompareClimbCandidates );

		// take the least exposed candidate the carrier can walk to without a long detour
		CNavArea *pNext = nullptr;
		for ( int i = 0; i < candidates.Count() && i < CARRIER_CLIMB_MAX_PATH_TESTS; i++ )
		{
			if ( IsShortWalk( pCurrent, candidates[i].area ) )
			{
				pNext = candidates[i].area;
				break;
			}
		}

		// a local minimum: no visible area within a step is less exposed and reachable
		if ( !pNext )
		{
			break;
		}

		pCurrent = pNext;
	}

	return pCurrent;
}
