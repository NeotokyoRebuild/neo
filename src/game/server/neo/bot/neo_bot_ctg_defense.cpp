#include "cbase.h"
#include "bot/neo_bot_ctg_defense.h"
#include "bot/neo_bot.h"
#include "bot/neo_bot_path_reservation.h"
#include "bot/neo_bot_route_predict.h"
#include "nav_mesh.h"
#include "nav_pathfind.h"
#include "neo_player.h"
#include "utlhashtable.h"

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

namespace NEOBotDefense
{
	// Dead reckoning from the last report: a naive run speed, with no enemy class or velocity
	static constexpr float WATCH_ASSUMED_RUN_SPEED = 250.0f;

	// The stretch of the enemy's route worth watching: not on top of our carrier, and not so far out
	// that the enemy can take another way in
	static constexpr float WATCH_MIN_FROM_CARRIER = 256.0f;
	static constexpr float WATCH_MAX_FROM_CARRIER = 1500.0f;
	static constexpr int WATCH_MAX_ROUTE_AREAS = 6;

	// I must reach the watch area this much travel before the enemy reaches the watched area;
	// a held watch keeps a band of slack, so the plan does not flip each re-check
	static constexpr float WATCH_ENTRY_MARGIN = 300.0f;
	static constexpr float WATCH_KEEP_BAND = 384.0f;

	// Close enough to see a passing enemy well, far enough not to stand in its way
	static constexpr float WATCH_MIN_FROM_ROUTE = 200.0f;
	static constexpr float WATCH_MAX_FROM_ROUTE = 1200.0f;

	// A watch area must fit a standing player
	static constexpr float WATCH_MIN_AREA_SIZE = 2.0f * HalfHumanWidth;
	static constexpr int WATCH_BAD_ATTRIBUTES = NAV_MESH_CROUCH | NAV_MESH_JUMP | NAV_MESH_AVOID;

	// Bounds on one plan's nav work
	static constexpr float WATCH_MAX_FLOOD_RANGE = 2400.0f;
	static constexpr int WATCH_MAX_FLOOD_AREAS = 1500;
	static constexpr int WATCH_MAX_VISIBLE_CHECKS = 4096;

	// A cached enemy route serves every defender of the team until the enemy or the carrier changes area
	static constexpr float WATCH_ROUTE_MAX_AGE = 2.0f;

	struct WatchRoute_t
	{
		bool bValid;
		int iTeam;
		CNavArea *pFrom;
		CNavArea *pTo;
		float flTime;
		CNEOBotPredictedRoute route;
		CUtlHashtable< unsigned int, int > routeIndex;
	};

	// One slot per enemy player index
	static WatchRoute_t s_routes[MAX_PLAYERS + 1];

	//---------------------------------------------------------------------------------------------
	// The enemy's predicted route to our carrier, shared by the team
	static const WatchRoute_t *GetEnemyRoute( int iTeam, int iEnemy, CNavArea *pFrom, CNavArea *pTo, const Vector &vecCarrier )
	{
		if ( ( iEnemy <= 0 ) || ( iEnemy > MAX_PLAYERS ) )
		{
			return nullptr;
		}

		WatchRoute_t &cache = s_routes[iEnemy];
		if ( cache.bValid && ( cache.iTeam == iTeam ) && ( cache.pFrom == pFrom ) && ( cache.pTo == pTo )
			&& ( gpGlobals->curtime - cache.flTime < WATCH_ROUTE_MAX_AGE ) )
		{
			return &cache;
		}

		cache.bValid = false;
		cache.route.areas.RemoveAll();
		cache.route.travel.RemoveAll();
		cache.routeIndex.RemoveAll();

		if ( !PredictRoute( pFrom, vecCarrier, cache.route ) || ( cache.route.areas.Count() < 2 ) )
		{
			return nullptr;
		}

		for ( int i = 0; i < cache.route.areas.Count(); ++i )
		{
			cache.routeIndex.Insert( cache.route.areas[i]->GetID(), i );
		}

		cache.bValid = true;
		cache.iTeam = iTeam;
		cache.pFrom = pFrom;
		cache.pTo = pTo;
		cache.flTime = gpGlobals->curtime;
		return &cache;
	}

	//---------------------------------------------------------------------------------------------
	// My own flood, capped so a big map stays cheap
	class CWatchFlood : public CNEOBotTraversableFlood
	{
	public:
		CWatchFlood( CNEOBot *me ) : CNEOBotTraversableFlood( me ) {}

		virtual bool operator()( CNavArea *area, CNavArea *priorArea, float travelDistanceSoFar ) override
		{
			return ++m_nAreas < WATCH_MAX_FLOOD_AREAS;
		}

		int m_nAreas = 0;
	};

	static bool Fail( const char **ppszFailReason, const char *pszReason )
	{
		if ( ppszFailReason )
		{
			*ppszFailReason = pszReason;
		}

		return false;
	}

	static bool IsWatchableArea( CNEOBot *me, const CNavArea *pArea )
	{
		if ( pArea->HasAttributes( WATCH_BAD_ATTRIBUTES ) )
		{
			return false;
		}

		if ( ( pArea->GetSizeX() < WATCH_MIN_AREA_SIZE ) || ( pArea->GetSizeY() < WATCH_MIN_AREA_SIZE ) )
		{
			return false;
		}

		if ( pArea->IsBlocked( me->GetTeamNumber() ) )
		{
			return false;
		}

		return !CNEOBotPathReservations()->IsAreaHazardous( pArea->GetID(), me );
	}

	static bool IsInWatchWindow( const CNEOBotPredictedRoute &route, int iRouteIndex, float flAdvanced )
	{
		const float flEta = route.travel[iRouteIndex] - flAdvanced;
		const float flFromCarrier = route.travel.Tail() - route.travel[iRouteIndex];
		return ( flEta > 0.0f ) && ( flFromCarrier >= WATCH_MIN_FROM_CARRIER ) && ( flFromCarrier <= WATCH_MAX_FROM_CARRIER );
	}

	//---------------------------------------------------------------------------------------------
	bool FindWatchArea( CNEOBot *me, const CKnownEntity &known, CNEO_Player *pCarrier,
		const WatchArea_t *pHeld, WatchArea_t &out, const char **ppszFailReason )
	{
		CNavArea *pMyArea = me->GetLastKnownArea();
		CNavArea *pCarrierArea = pCarrier ? pCarrier->GetLastKnownArea() : nullptr;
		if ( !pMyArea || !pCarrierArea || !known.GetEntity() )
		{
			return Fail( ppszFailReason, "lost" );
		}

		// The searches only write the areas' search state, so the const area is safe to search from
		CNavArea *pEnemyArea = const_cast< CNavArea * >( known.GetLastKnownArea() );
		if ( !pEnemyArea )
		{
			pEnemyArea = TheNavMesh->GetNearestNavArea( known.GetLastKnownPosition() );
		}

		const WatchRoute_t *pCache = pEnemyArea ? GetEnemyRoute( me->GetTeamNumber(), known.GetEntity()->entindex(),
			pEnemyArea, pCarrierArea, pCarrier->GetAbsOrigin() ) : nullptr;
		if ( !pCache )
		{
			return Fail( ppszFailReason, "route" );
		}

		const CNEOBotPredictedRoute &route = pCache->route;
		const float flAdvanced = Min( known.GetTimeSinceLastKnown() * WATCH_ASSUMED_RUN_SPEED, route.travel.Tail() );
		const float flKeepMargin = WATCH_ENTRY_MARGIN - WATCH_KEEP_BAND;

		// Standing on a held watch: re-check it without a flood
		if ( pHeld && pHeld->pWatch && pHeld->pWatched && ( pMyArea == pHeld->pWatch ) )
		{
			const int *pIndex = pCache->routeIndex.GetPtr( pHeld->pWatched->GetID() );
			if ( !pIndex || pCache->routeIndex.GetPtr( pHeld->pWatch->GetID() ) )
			{
				return Fail( ppszFailReason, "route" );
			}

			if ( !IsInWatchWindow( route, *pIndex, flAdvanced ) )
			{
				return Fail( ppszFailReason, "passed" );
			}

			out = *pHeld;
			out.flMyTravel = 0.0f;
			out.flEnemyEta = route.travel[*pIndex] - flAdvanced;
			return true;
		}

		// The contact window, from the enemy's end toward the carrier: meet it as far out as we can win
		CUtlVector< int > window;
		float flMaxEta = 0.0f;
		for ( int i = 0; ( i < route.areas.Count() ) && ( window.Count() < WATCH_MAX_ROUTE_AREAS ); ++i )
		{
			if ( IsInWatchWindow( route, i, flAdvanced ) )
			{
				window.AddToTail( i );
				flMaxEta = Max( flMaxEta, route.travel[i] - flAdvanced );
			}
		}

		if ( window.IsEmpty() )
		{
			return Fail( ppszFailReason, "passed" );
		}

		// The flood must come after the route is copied out, and its marks are read before any other search
		CWatchFlood flood( me );
		SearchSurroundingAreas( pMyArea, flood, Min( flMaxEta - flKeepMargin, WATCH_MAX_FLOOD_RANGE ) );

		const CNavArea *pHeldWatch = pHeld ? pHeld->pWatch : nullptr;
		int nChecks = 0;
		for ( int w = 0; w < window.Count(); ++w )
		{
			const int iWatched = window[w];
			CNavArea *pWatched = route.areas[iWatched];
			const float flEta = route.travel[iWatched] - flAdvanced;

			CNavArea *pBest = nullptr;
			bool bHeldFits = false;
			auto visit = [&]( CNavArea *pArea ) -> bool
			{
				if ( ++nChecks > WATCH_MAX_VISIBLE_CHECKS )
				{
					return false;
				}

				if ( !pArea->IsMarked() )
				{
					return true;
				}

				const bool bHeldArea = ( pArea == pHeldWatch ) && pHeld && ( pHeld->pWatched == pWatched );
				const float flMargin = bHeldArea ? flKeepMargin : WATCH_ENTRY_MARGIN;
				if ( pArea->GetCostSoFar() + flMargin > flEta )
				{
					return true;
				}

				if ( pCache->routeIndex.GetPtr( pArea->GetID() ) )
				{
					return true;
				}

				const float flDistSq = pArea->GetCenter().DistToSqr( pWatched->GetCenter() );
				if ( ( flDistSq < Square( WATCH_MIN_FROM_ROUTE ) ) || ( flDistSq > Square( WATCH_MAX_FROM_ROUTE ) ) )
				{
					return true;
				}

				if ( !IsWatchableArea( me, pArea ) )
				{
					return true;
				}

				if ( bHeldArea )
				{
					bHeldFits = true;
				}

				// The least exposed spot sees the route and little else
				if ( !pBest
					|| ( pArea->GetPotentiallyVisibleAreaCount() < pBest->GetPotentiallyVisibleAreaCount() )
					|| ( ( pArea->GetPotentiallyVisibleAreaCount() == pBest->GetPotentiallyVisibleAreaCount() )
						&& ( pArea->GetID() < pBest->GetID() ) ) )
				{
					pBest = pArea;
				}

				return true;
			};

			pWatched->ForAllPotentiallyVisibleAreas( visit );

			if ( bHeldFits )
			{
				pBest = const_cast< CNavArea * >( pHeldWatch );
			}

			if ( pBest )
			{
				out.pWatch = pBest;
				out.pWatched = pWatched;
				out.flMyTravel = pBest->GetCostSoFar();
				out.flEnemyEta = flEta;
				return true;
			}

			if ( nChecks > WATCH_MAX_VISIBLE_CHECKS )
			{
				break;
			}
		}

		return Fail( ppszFailReason, "race" );
	}

	//---------------------------------------------------------------------------------------------
	void Reset()
	{
		for ( int i = 0; i <= MAX_PLAYERS; ++i )
		{
			WatchRoute_t &cache = s_routes[i];
			cache.bValid = false;
			cache.pFrom = nullptr;
			cache.pTo = nullptr;
			cache.route.areas.RemoveAll();
			cache.route.travel.RemoveAll();
			cache.routeIndex.RemoveAll();
		}
	}
}
