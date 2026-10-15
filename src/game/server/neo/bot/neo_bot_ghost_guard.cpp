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
static constexpr float GUARD_MAX_TRAVEL = 700.0f;

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
// Searches outward from the carrier for the nearest area a guard can stand in:
// out of grenade range of the carrier, potentially visible from it, and not taken by a teammate
class CSearchForGuardArea : public ISearchSurroundingAreasFunctor
{
public:
	CSearchForGuardArea( CNEOBot *me, CNavArea *pCenterArea ) : m_me( me ), m_pCenterArea( pCenterArea )
	{
		m_pBest = nullptr;
		m_nBestCrowding = INT_MAX;
		m_flBestTravel = FLT_MAX;
		m_bCenterHasVis = pCenterArea->GetPotentiallyVisibleAreaCount() > 0;
	}

	virtual bool operator() ( CNavArea *area, CNavArea *priorArea, float travelDistanceSoFar ) override
	{
		if ( !IsCandidate( area ) )
		{
			return true;
		}

		const int nCrowding = CNEOBotPathReservations()->GetPredictedFriendlyPathCount( area->GetID(), m_me->GetTeamNumber(), m_me )
			+ CountGuardsNear( m_me, area );

		// The search goes outward, but not strictly by travel distance, so compare it too
		const float flTravel = area->GetCostSoFar();
		if ( nCrowding < m_nBestCrowding || ( nCrowding == m_nBestCrowding && flTravel < m_flBestTravel ) )
		{
			m_pBest = area;
			m_nBestCrowding = nCrowding;
			m_flBestTravel = flTravel;
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
		if ( area->GetAttributes() & ( NAV_MESH_CROUCH | NAV_MESH_JUMP | NAV_MESH_AVOID ) )
		{
			return false;
		}

		const AreaPenalty_t penalty = CNEOBotPathReservations()->GetAreaPenalty( area->GetID() );
		if ( penalty.IsStuckProne() || CNEOBotPathReservations()->IsAreaHazardous( area->GetID(), m_me ) )
		{
			return false;
		}

		// the carrier and its guard can see each other's areas, so the two support each other
		return !m_bCenterHasVis || m_pCenterArea->IsPotentiallyVisible( area );
	}

	CNEOBot *m_me;
	CNavArea *m_pCenterArea;
	int m_nBestCrowding;
	float m_flBestTravel;
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
