#include "cbase.h"
#include "bot/neo_bot_ctg_defense.h"
#include "bot/neo_bot.h"
#include "bot/neo_bot_ctg_carry_block.h"
#include "tier0/fasttimer.h"
#include "bot/neo_bot_path_reservation.h"
#include "bot/neo_bot_route_predict.h"
#include "nav_mesh.h"
#include "nav_pathfind.h"
#include "neo_player.h"
#include "neo_gamerules.h"
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
	static constexpr float WATCH_CROSSFIRE_MAX_DOT = 0.70710678f;
	static constexpr float WATCH_REPLAN_INTERVAL = 2.0f;
	static constexpr float WATCH_TEAM_STAGGER = 0.125f;
	static constexpr int WATCH_TEAM_PHASES = 4;
	static constexpr int WATCH_TEAM_MAX_AREAS = 2000;
	static constexpr int WATCH_TEAM_MAX_CHECKS = 4096;

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

	// One slot per team and enemy player index, including failed attempts
	static WatchRoute_t s_routes[TEAM__TOTAL][MAX_PLAYERS + 1];
	static int s_nRouteSearches = 0;

	struct WatchBudget_t
	{
		int nAreas = WATCH_TEAM_MAX_AREAS;
		int nChecks = WATCH_TEAM_MAX_CHECKS;
	};

	//---------------------------------------------------------------------------------------------
	// The enemy's predicted route to our carrier, shared by the team
	static const WatchRoute_t *GetEnemyRoute( int iTeam, int iEnemy, CNavArea *pFrom, CNavArea *pTo, const Vector &vecCarrier )
	{
		if ( ( iTeam < 0 ) || ( iTeam >= TEAM__TOTAL ) || ( iEnemy <= 0 ) || ( iEnemy > MAX_PLAYERS ) )
		{
			return nullptr;
		}

		WatchRoute_t &cache = s_routes[iTeam][iEnemy];
		if ( ( cache.iTeam == iTeam ) && ( cache.pFrom == pFrom ) && ( cache.pTo == pTo )
			&& ( gpGlobals->curtime - cache.flTime < WATCH_ROUTE_MAX_AGE ) )
		{
			return cache.bValid ? &cache : nullptr;
		}

		cache.iTeam = iTeam;
		cache.pFrom = pFrom;
		cache.pTo = pTo;
		cache.flTime = gpGlobals->curtime;
		cache.bValid = false;
		cache.route.areas.RemoveAll();
		cache.route.travel.RemoveAll();
		cache.routeIndex.RemoveAll();

		++s_nRouteSearches;
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
		CWatchFlood( CNEOBot *me, WatchBudget_t *pBudget ) : CNEOBotTraversableFlood( me ), m_pBudget( pBudget )
		{
		}

		virtual bool operator()( CNavArea *area, CNavArea *priorArea, float travelDistanceSoFar ) override
		{
			if ( m_pBudget && ( --m_pBudget->nAreas <= 0 ) )
			{
				return false;
			}
			return ++m_nAreas < WATCH_MAX_FLOOD_AREAS;
		}

		int m_nAreas = 0;
		WatchBudget_t *m_pBudget;
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
	static bool FindWatchOnRoute( CNEOBot *me, const WatchRoute_t *pCache, float flAge,
		const WatchArea_t *pHeld, const WatchArea_t *pFirst, WatchArea_t &out, const char **ppszFailReason, WatchBudget_t *pBudget = nullptr )
	{
		CNavArea *pMyArea = me->GetLastKnownArea();
		if ( !pMyArea || !pCache )
		{
			return Fail( ppszFailReason, "route" );
		}

		const CNEOBotPredictedRoute &route = pCache->route;
		const float flAdvanced = Min( flAge * WATCH_ASSUMED_RUN_SPEED, route.travel.Tail() );
		const float flKeepMargin = WATCH_ENTRY_MARGIN - WATCH_KEEP_BAND;

		// Revalidate a cached watch without a flood, including while walking to it
		if ( pHeld && pHeld->pWatch && pHeld->pWatched )
		{
			const int *pIndex = pCache->routeIndex.GetPtr( pHeld->pWatched->GetID() );
			if ( !pIndex || pCache->routeIndex.GetPtr( pHeld->pWatch->GetID() ) )
			{
				return Fail( ppszFailReason, "route" );
			}

			const float flMyTravel = pMyArea == pHeld->pWatch ? 0.0f : me->GetAbsOrigin().DistTo( pHeld->pWatch->GetCenter() );
			if ( !IsInWatchWindow( route, *pIndex, flAdvanced )
				|| !IsWatchableArea( me, pHeld->pWatch )
				|| ( flMyTravel + flKeepMargin > route.travel[*pIndex] - flAdvanced ) )
			{
				return Fail( ppszFailReason, "passed" );
			}

			if ( pFirst )
			{
				Vector a = pFirst->pWatch->GetCenter() - pFirst->pWatched->GetCenter();
				Vector b = pHeld->pWatch->GetCenter() - pFirst->pWatched->GetCenter();
				a.z = b.z = 0.0f;
				if ( ( pHeld->pWatched != pFirst->pWatched ) || ( a.NormalizeInPlace() <= 0.0f )
					|| ( b.NormalizeInPlace() <= 0.0f ) || ( DotProduct( a, b ) > WATCH_CROSSFIRE_MAX_DOT ) )
				{
					return Fail( ppszFailReason, "angle" );
				}
			}

			out = *pHeld;
			out.flReportAge = flAge;
			out.flMyTravel = flMyTravel;
			out.flEnemyEta = route.travel[*pIndex] - flAdvanced;
			return true;
		}

		// The contact window, from the enemy's end toward the carrier: meet it as far out as we can win
		CUtlVector< int > window;
		float flMaxEta = 0.0f;
		for ( int i = 0; ( i < route.areas.Count() ) && ( window.Count() < WATCH_MAX_ROUTE_AREAS ); ++i )
		{
			if ( IsInWatchWindow( route, i, flAdvanced ) && ( !pFirst || ( route.areas[i] == pFirst->pWatched ) ) )
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
		if ( pBudget && ( ( pBudget->nAreas <= 0 ) || ( pBudget->nChecks <= 0 ) ) )
		{
			return Fail( ppszFailReason, "budget" );
		}
		CWatchFlood flood( me, pBudget );
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
				if ( ( ++nChecks > WATCH_MAX_VISIBLE_CHECKS ) || ( pBudget && ( --pBudget->nChecks <= 0 ) ) )
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

				// Both defenders watch the same contact point from separated horizontal bearings.
				if ( pFirst )
				{
					Vector a = pFirst->pWatch->GetCenter() - pWatched->GetCenter();
					Vector b = pArea->GetCenter() - pWatched->GetCenter();
					a.z = 0.0f;
					b.z = 0.0f;
					if ( ( a.NormalizeInPlace() <= 0.0f ) || ( b.NormalizeInPlace() <= 0.0f )
						|| ( DotProduct( a, b ) > WATCH_CROSSFIRE_MAX_DOT ) )
					{
						return true;
					}
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
				out.flReportAge = flAge;
				return true;
			}

			if ( ( nChecks > WATCH_MAX_VISIBLE_CHECKS ) || ( pBudget && ( pBudget->nChecks <= 0 ) ) )
			{
				break;
			}
		}

		return Fail( ppszFailReason, "race" );
	}

	bool FindWatchArea( CNEOBot *me, const CKnownEntity &known, CNEO_Player *pCarrier,
		const WatchArea_t *pHeld, WatchArea_t &out, const char **ppszFailReason )
	{
		CNEO_Player *pEnemy = ToNEOPlayer( known.GetEntity() );
		if ( !pEnemy || !pCarrier || NEOBotCarryBlock::IsNamedDead( pEnemy ) )
		{
			return Fail( ppszFailReason, "lost" );
		}
		CNavArea *pFrom = const_cast< CNavArea * >( known.GetLastKnownArea() );
		if ( !pFrom )
		{
			pFrom = TheNavMesh->GetNearestNavArea( known.GetLastKnownPosition() );
		}
		const WatchRoute_t *pCache = pFrom && pCarrier->GetLastKnownArea()
			? GetEnemyRoute( me->GetTeamNumber(), pEnemy->entindex(), pFrom, pCarrier->GetLastKnownArea(), pCarrier->GetAbsOrigin() ) : nullptr;
		return FindWatchOnRoute( me, pCache, known.GetTimeSinceLastKnown(), pHeld, nullptr, out, ppszFailReason );
	}

	static constexpr int WATCH_TEAM_THREATS = 3;
	static constexpr int WATCH_TEAM_SLOTS = 4;
	static constexpr float WATCH_TEAM_INTERVAL = 1.0f;
	static constexpr float WATCH_TEAM_ENTRY_AGE = 5.0f;
	static constexpr float WATCH_TEAM_STAY_AGE = 10.0f;
	static constexpr float WATCH_TEAM_SWITCH_RATIO = 0.7f;

	struct TeamThreat_t
	{
		CHandle< CNEO_Player > enemy;
		CNavArea *pArea = nullptr;
		Vector vecKnown;
		float flReportTime = -100.0f;
		float flEta = 0.0f;
		const WatchRoute_t *pRoute = nullptr;
	};

	struct TeamSlot_t
	{
		CHandle< CNEOBot > bot;
		CHandle< CNEO_Player > enemy;
		int iSlot = 0;
		float flNextWatch = 0.0f;
		WatchArea_t watch;
	};

	struct TeamPlan_t
	{
		CHandle< CNEO_Player > carrier;
		bool bStarted = false;
		int iRound = -1;
		float flNextPlan = 0.0f;
		TeamThreat_t threats[MAX_PLAYERS + 1];
		TeamSlot_t slots[WATCH_TEAM_SLOTS];
	};

	static TeamPlan_t s_teams[TEAM__TOTAL];

	static bool HasThreatAssignment( const TeamPlan_t &state, CNEO_Player *pEnemy )
	{
		for ( int i = 0; i < WATCH_TEAM_SLOTS; ++i )
		{
			if ( state.slots[i].bot && ( state.slots[i].enemy == pEnemy ) )
			{
				return true;
			}
		}
		return false;
	}

	static float ContactDistance( CNEOBot *pBot, const TeamThreat_t &threat )
	{
		float flBest = FLT_MAX;
		const CNEOBotPredictedRoute &route = threat.pRoute->route;
		const float flAdvanced = ( gpGlobals->curtime - threat.flReportTime ) * WATCH_ASSUMED_RUN_SPEED;
		int nAreas = 0;
		for ( int i = 0; ( i < route.areas.Count() ) && ( nAreas < WATCH_MAX_ROUTE_AREAS ); ++i )
		{
			if ( IsInWatchWindow( route, i, flAdvanced ) )
			{
				flBest = Min( flBest, pBot->GetAbsOrigin().DistTo( route.areas[i]->GetCenter() ) );
				++nAreas;
			}
		}
		return flBest;
	}

	static bool IsRoleValid( const TeamPlan_t &state, const TeamSlot_t &slot )
	{
		CNEOBot *pBot = slot.bot.Get();
		CNEO_Player *pEnemy = slot.enemy.Get();
		return pBot && pBot->IsAlive() && state.carrier && pBot->InSameTeam( state.carrier.Get() )
			&& pEnemy && !pBot->InSameTeam( pEnemy ) && !NEOBotCarryBlock::IsNamedDead( pEnemy )
			&& ( state.threats[pEnemy->entindex()].enemy == pEnemy )
			&& ( gpGlobals->curtime - state.threats[pEnemy->entindex()].flReportTime <= WATCH_TEAM_STAY_AGE );
	}

	// One whole-team comparison supplies assignment hysteresis; watch validity has its own cheap check.
	static float AssignmentCost( const TeamPlan_t &state, const TeamSlot_t *slots, int &nSlots )
	{
		float flTotal = 0.0f;
		nSlots = 0;
		for ( int i = 0; i < WATCH_TEAM_SLOTS; ++i )
		{
			CNEOBot *pBot = slots[i].bot.Get();
			CNEO_Player *pEnemy = slots[i].enemy.Get();
			if ( !pBot )
			{
				continue;
			}
			if ( !IsRoleValid( state, slots[i] ) )
			{
				return FLT_MAX;
			}
			const TeamThreat_t &threat = state.threats[pEnemy->entindex()];
			float flDistance = threat.pRoute ? ContactDistance( pBot, threat ) : FLT_MAX;
			if ( flDistance == FLT_MAX )
			{
				// A temporarily missing contact window invalidates the watch, not its sticky enemy role.
				flDistance = pBot->GetAbsOrigin().DistTo( threat.vecKnown );
			}
			flTotal += flDistance;
			++nSlots;
		}
		return flTotal;
	}

	static void PlanTeam( TeamPlan_t &state, int iTeam, CNEO_Player *pCarrier )
	{
		CFastTimer timer;
		timer.Start();
		const int nSearchesBefore = s_nRouteSearches;
		state.flNextPlan = gpGlobals->curtime + WATCH_TEAM_INTERVAL;
		CUtlVector< CNEOBot * > defenders;
		// These are teammate origins and teammate life state. Enemy state comes exclusively from reports/death feed.
		for ( int i = 1; i <= gpGlobals->maxClients; ++i )
		{
			CNEOBot *pBot = ToNEOBot( UTIL_PlayerByIndex( i ) );
			if ( !pBot || ( pBot == pCarrier ) || ( pBot->GetTeamNumber() != iTeam ) || !pBot->IsAlive() )
			{
				continue;
			}
			defenders.AddToTail( pBot );
			CUtlVector< CKnownEntity > known;
			pBot->GetVisionInterface()->CollectKnownEntities( &known );
			for ( int k = 0; k < known.Count(); ++k )
			{
				CNEO_Player *pEnemy = ToNEOPlayer( known[k].GetEntity() );
				if ( !pEnemy || pBot->InSameTeam( pEnemy ) || NEOBotCarryBlock::IsNamedDead( pEnemy ) )
				{
					continue;
				}
				const float flAge = known[k].GetTimeSinceLastKnown();
				const bool bStay = HasThreatAssignment( state, pEnemy );
				if ( flAge > ( bStay ? WATCH_TEAM_STAY_AGE : WATCH_TEAM_ENTRY_AGE ) )
				{
					continue;
				}
				TeamThreat_t &threat = state.threats[pEnemy->entindex()];
				const float flReportTime = gpGlobals->curtime - flAge;
				if ( ( threat.enemy == pEnemy ) && ( threat.flReportTime >= flReportTime ) )
				{
					continue;
				}
				threat.enemy = pEnemy;
				threat.vecKnown = known[k].GetLastKnownPosition();
				threat.pArea = const_cast< CNavArea * >( known[k].GetLastKnownArea() );
				threat.flReportTime = flReportTime;
			}
		}

		CUtlVector< int > ranked;
		for ( int i = 1; i <= MAX_PLAYERS; ++i )
		{
			TeamThreat_t &threat = state.threats[i];
			threat.pRoute = nullptr;
			if ( !threat.enemy || NEOBotCarryBlock::IsNamedDead( threat.enemy.Get() ) )
			{
				continue;
			}
			const float flAge = gpGlobals->curtime - threat.flReportTime;
			if ( flAge > ( HasThreatAssignment( state, threat.enemy.Get() ) ? WATCH_TEAM_STAY_AGE : WATCH_TEAM_ENTRY_AGE ) )
			{
				continue;
			}
			// A bounded shortlist uses straight-line dead-reckoned travel; only these three and sticky incumbents pay A*.
			threat.flEta = Max( 0.0f, threat.vecKnown.DistTo( pCarrier->GetAbsOrigin() ) - flAge * WATCH_ASSUMED_RUN_SPEED );
			ranked.AddToTail( i );
		}
		ranked.SortPredicate( [&]( int a, int b )
		{
			return ( state.threats[a].flEta < state.threats[b].flEta )
				|| ( ( state.threats[a].flEta == state.threats[b].flEta ) && ( a < b ) );
		} );
		if ( ranked.Count() > WATCH_TEAM_THREATS )
		{
			ranked.RemoveMultipleFromTail( ranked.Count() - WATCH_TEAM_THREATS );
		}
		for ( int i = 0; i < ranked.Count(); ++i )
		{
			TeamThreat_t &threat = state.threats[ranked[i]];
			if ( !threat.pArea )
			{
				threat.pArea = TheNavMesh->GetNearestNavArea( threat.vecKnown );
			}
			threat.pRoute = threat.pArea ? GetEnemyRoute( iTeam, ranked[i], threat.pArea, pCarrier->GetLastKnownArea(), pCarrier->GetAbsOrigin() ) : nullptr;
			if ( threat.pRoute )
			{
				threat.flEta = Max( 0.0f, threat.pRoute->route.travel.Tail()
					- ( gpGlobals->curtime - threat.flReportTime ) * WATCH_ASSUMED_RUN_SPEED );
			}
		}
		// Sticky threats outside the proposed shortlist remain valid alternatives. At most three
		// additional routes are refreshed, once per distinct enemy, before any defender flood.
		int nRoutes = ranked.Count();
		bool attempted[MAX_PLAYERS + 1] = {};
		for ( int i = 0; i < ranked.Count(); ++i )
		{
			attempted[ranked[i]] = true;
		}
		for ( int i = 0; i < WATCH_TEAM_SLOTS; ++i )
		{
			CNEO_Player *pEnemy = state.slots[i].enemy.Get();
			if ( !pEnemy || !state.slots[i].bot || NEOBotCarryBlock::IsNamedDead( pEnemy ) )
			{
				continue;
			}
			const int iEnemy = pEnemy->entindex();
			TeamThreat_t &threat = state.threats[iEnemy];
			if ( attempted[iEnemy] || threat.pRoute
				|| ( gpGlobals->curtime - threat.flReportTime > WATCH_TEAM_STAY_AGE ) )
			{
				continue;
			}
			if ( !threat.pArea )
			{
				threat.pArea = TheNavMesh->GetNearestNavArea( threat.vecKnown );
			}
			attempted[iEnemy] = true;
			if ( threat.pArea )
			{
				threat.pRoute = GetEnemyRoute( iTeam, iEnemy, threat.pArea,
					pCarrier->GetLastKnownArea(), pCarrier->GetAbsOrigin() );
				++nRoutes;
			}
			if ( threat.pRoute )
			{
				threat.flEta = Max( 0.0f, threat.pRoute->route.travel.Tail()
					- ( gpGlobals->curtime - threat.flReportTime ) * WATCH_ASSUMED_RUN_SPEED );
			}
		}

		ranked.SortPredicate( [&]( int a, int b )
		{
			return ( state.threats[a].flEta < state.threats[b].flEta )
				|| ( ( state.threats[a].flEta == state.threats[b].flEta ) && ( a < b ) );
		} );

		TeamSlot_t proposed[WATCH_TEAM_SLOTS];
		bool used[MAX_PLAYERS + 1] = {};
		int nProposed = 0;
		bool bMainAssigned = false;
		for ( int t = 0; t < ranked.Count(); ++t )
		{
			const TeamThreat_t &threat = state.threats[ranked[t]];
			if ( !threat.pRoute )
			{
				continue;
			}
			const int nWanted = bMainAssigned ? 1 : 2;
			bMainAssigned = true;
			for ( int slot = 1; slot <= nWanted; ++slot )
			{
				CNEOBot *pBest = nullptr;
				float flBest = FLT_MAX;
				for ( int b = 0; b < defenders.Count(); ++b )
				{
					CNEOBot *pBot = defenders[b];
					if ( used[pBot->entindex()] )
					{
						continue;
					}
					const float flDistance = ContactDistance( pBot, threat );
					if ( flDistance < flBest )
					{
						pBest = pBot;
						flBest = flDistance;
					}
				}
				if ( pBest && ( nProposed < WATCH_TEAM_SLOTS ) )
				{
					TeamSlot_t &next = proposed[nProposed++];
					next.bot = pBest;
					next.enemy = threat.enemy;
					next.iSlot = slot;
					used[pBest->entindex()] = true;
				}
			}
		}
		bool bAnyValid = false;
		for ( int i = 0; i < WATCH_TEAM_SLOTS; ++i )
		{
			if ( IsRoleValid( state, state.slots[i] ) )
			{
				bAnyValid = true;
			}
			else
			{
				state.slots[i] = TeamSlot_t();
			}
		}
		// A surviving second becomes the first when its partner was invalidated.
		for ( int i = 0; i < WATCH_TEAM_SLOTS; ++i )
		{
			if ( !state.slots[i].bot || ( state.slots[i].iSlot != 2 ) )
			{
				continue;
			}
			bool bHasFirst = false;
			for ( int j = 0; j < WATCH_TEAM_SLOTS; ++j )
			{
				bHasFirst = bHasFirst || ( state.slots[j].bot && ( state.slots[j].enemy == state.slots[i].enemy )
					&& ( state.slots[j].iSlot == 1 ) );
			}
			if ( !bHasFirst )
			{
				state.slots[i].iSlot = 1;
			}
		}

		// Repair a vacancy without reassigning living teammates whose reports are still fresh.
		if ( bAnyValid )
		{
			for ( int p = 0; p < nProposed; ++p )
			{
				int iEmpty = -1;
				bool bRoleTaken = false;
				bool bEnemyPresent = false;
				bool assigned[MAX_PLAYERS + 1] = {};
				bool enemies[MAX_PLAYERS + 1] = {};
				int nEnemies = 0;
				for ( int i = 0; i < WATCH_TEAM_SLOTS; ++i )
				{
					if ( !state.slots[i].bot )
					{
						if ( iEmpty < 0 )
						{
							iEmpty = i;
						}
						continue;
					}
					assigned[state.slots[i].bot->entindex()] = true;
					const int iEnemy = state.slots[i].enemy->entindex();
					if ( !enemies[iEnemy] )
					{
						enemies[iEnemy] = true;
						++nEnemies;
					}
					if ( state.slots[i].enemy == proposed[p].enemy )
					{
						bEnemyPresent = true;
						bRoleTaken = bRoleTaken || ( state.slots[i].iSlot == proposed[p].iSlot );
					}
				}
				if ( bRoleTaken || ( iEmpty < 0 ) || ( !bEnemyPresent && ( nEnemies >= WATCH_TEAM_THREATS ) ) )
				{
					continue;
				}
				CNEOBot *pBest = nullptr;
				float flBest = FLT_MAX;
				const TeamThreat_t &threat = state.threats[proposed[p].enemy->entindex()];
				for ( int b = 0; b < defenders.Count(); ++b )
				{
					CNEOBot *pBot = defenders[b];
					if ( assigned[pBot->entindex()] )
					{
						continue;
					}
					const float flDistance = ContactDistance( pBot, threat );
					if ( flDistance < flBest )
					{
						flBest = flDistance;
						pBest = pBot;
					}
				}
				if ( pBest )
				{
					state.slots[iEmpty] = proposed[p];
					state.slots[iEmpty].bot = pBest;
				}
			}
		}
		int nOld = 0;
		int nNew = 0;
		const float flOld = AssignmentCost( state, state.slots, nOld );
		const float flNew = AssignmentCost( state, proposed, nNew );
		const bool bReplace = ( flOld == FLT_MAX ) || ( nOld == 0 )
			|| ( ( nNew >= nOld ) && ( flNew < flOld * WATCH_TEAM_SWITCH_RATIO ) );
		if ( bReplace )
		{
			for ( int i = 0; i < WATCH_TEAM_SLOTS; ++i )
			{
				// Preserve a still-valid watch when its role survived the whole-plan comparison.
				for ( int j = 0; j < WATCH_TEAM_SLOTS; ++j )
				{
					if ( proposed[i].bot && ( proposed[i].bot == state.slots[j].bot )
						&& ( proposed[i].enemy == state.slots[j].enemy ) && ( proposed[i].iSlot == state.slots[j].iSlot ) )
					{
						proposed[i].watch = state.slots[j].watch;
						proposed[i].flNextWatch = state.slots[j].flNextWatch;
					}
				}
			}
			for ( int i = 0; i < WATCH_TEAM_SLOTS; ++i )
			{
				state.slots[i] = proposed[i];
			}
		}

		// Spend the shared search budget on imminent threat groups first, with each pair adjacent.
		// Repair can reorder slots: the first watch must precede its partner within each group.
		for ( int i = 0; i < WATCH_TEAM_SLOTS; ++i )
		{
			for ( int j = i + 1; j < WATCH_TEAM_SLOTS; ++j )
			{
				const TeamSlot_t &a = state.slots[j];
				const TeamSlot_t &b = state.slots[i];
				bool bBefore = a.bot && !b.bot;
				if ( a.bot && b.bot )
				{
					const int iEnemyA = a.enemy->entindex();
					const int iEnemyB = b.enemy->entindex();
					const float flEtaA = state.threats[iEnemyA].flEta;
					const float flEtaB = state.threats[iEnemyB].flEta;
					bBefore = ( flEtaA < flEtaB )
						|| ( ( flEtaA == flEtaB ) && ( iEnemyA < iEnemyB ) )
						|| ( ( iEnemyA == iEnemyB ) && ( a.iSlot < b.iSlot ) );
				}
				if ( bBefore )
				{
					const TeamSlot_t temporary = state.slots[i];
					state.slots[i] = state.slots[j];
					state.slots[j] = temporary;
				}
			}
		}

		int nFloods = 0;
		WatchBudget_t budget;
		for ( int i = 0; i < WATCH_TEAM_SLOTS; ++i )
		{
			TeamSlot_t &slot = state.slots[i];
			if ( !slot.bot || !slot.enemy )
			{
				continue;
			}
			const TeamThreat_t &threat = state.threats[slot.enemy->entindex()];
			const WatchArea_t *pFirst = nullptr;
			if ( slot.iSlot == 2 )
			{
				for ( int j = 0; j < i; ++j )
				{
					if ( ( state.slots[j].enemy == slot.enemy ) && state.slots[j].watch.pWatch )
					{
						pFirst = &state.slots[j].watch;
					}
				}
				if ( !pFirst )
				{
					slot.watch = WatchArea_t();
					continue;
				}
			}
			WatchArea_t watch;
			const float flAge = gpGlobals->curtime - threat.flReportTime;
			bool bValid = slot.watch.pWatch && FindWatchOnRoute( slot.bot.Get(), threat.pRoute, flAge,
				&slot.watch, pFirst, watch, nullptr );
			if ( !bValid && ( gpGlobals->curtime >= slot.flNextWatch ) )
			{
				slot.flNextWatch = gpGlobals->curtime + WATCH_REPLAN_INTERVAL;
				++nFloods;
				bValid = FindWatchOnRoute( slot.bot.Get(), threat.pRoute, flAge, nullptr, pFirst, watch, nullptr, &budget );
			}
			slot.watch = bValid ? watch : WatchArea_t();
			if ( bValid )
			{
				float flAngle = 0.0f;
				if ( pFirst )
				{
					Vector a = pFirst->pWatch->GetCenter() - watch.pWatched->GetCenter();
					Vector b = watch.pWatch->GetCenter() - watch.pWatched->GetCenter();
					a.z = b.z = 0.0f;
					a.NormalizeInPlace();
					b.NormalizeInPlace();
					flAngle = RAD2DEG( acosf( clamp( DotProduct( a, b ), -1.0f, 1.0f ) ) );
				}
				Msg( "NEO_DEF assign team=%d enemy=%d bot=%d slot=%d angle=%.1f\n",
					iTeam, slot.enemy->entindex(), slot.bot->entindex(), slot.iSlot, flAngle ); // NEO-DEF-TEMP
			}
		}
		timer.End();
		Msg( "NEO_DEF team plan team=%d threats=%d route_requests=%d searches=%d floods=%d areas=%d checks=%d us=%d\n", iTeam, ranked.Count(), nRoutes, s_nRouteSearches - nSearchesBefore, nFloods,
			WATCH_TEAM_MAX_AREAS - Max( 0, budget.nAreas ), WATCH_TEAM_MAX_CHECKS - Max( 0, budget.nChecks ),
			(int)timer.GetDuration().GetMicroseconds() ); // NEO-DEF-TEMP
	}

	bool GetAssignment( CNEOBot *me, CNEO_Player *pCarrier, CNEO_Player *&pEnemy, WatchArea_t &out )
	{
		pEnemy = nullptr;
		const int iTeam = me->GetTeamNumber();
		if ( ( iTeam < 0 ) || ( iTeam >= TEAM__TOTAL ) || !pCarrier || !pCarrier->GetLastKnownArea() )
		{
			return false;
		}
		TeamPlan_t &state = s_teams[iTeam];
		if ( ( state.carrier != pCarrier ) || ( state.iRound != NEORules()->roundNumber() ) )
		{
			const float flNextPlan = state.flNextPlan;
			const bool bStarted = state.bStarted;
			state = TeamPlan_t();
			state.carrier = pCarrier;
			state.iRound = NEORules()->roundNumber();
			state.bStarted = bStarted;
			state.flNextPlan = flNextPlan;
		}
		if ( !state.bStarted )
		{
			state.bStarted = true;
			state.flNextPlan = gpGlobals->curtime + ( iTeam % WATCH_TEAM_PHASES ) * WATCH_TEAM_STAGGER;
		}
		if ( gpGlobals->curtime >= state.flNextPlan )
		{
			PlanTeam( state, iTeam, pCarrier );
		}
		for ( int i = 0; i < WATCH_TEAM_SLOTS; ++i )
		{
			const TeamSlot_t &slot = state.slots[i];
			if ( ( slot.bot == me ) && slot.enemy && slot.watch.pWatch
				&& !NEOBotCarryBlock::IsNamedDead( slot.enemy.Get() )
				&& ( gpGlobals->curtime - state.threats[slot.enemy->entindex()].flReportTime <= WATCH_TEAM_STAY_AGE ) )
			{
				pEnemy = slot.enemy.Get();
				out = slot.watch;
				out.flReportAge = gpGlobals->curtime - state.threats[slot.enemy->entindex()].flReportTime;
				return true;
			}
		}
		return false;
	}

	//---------------------------------------------------------------------------------------------
	void Reset()
	{
		for ( int t = 0; t < TEAM__TOTAL; ++t )
		{
			s_teams[t] = TeamPlan_t();
			for ( int i = 0; i <= MAX_PLAYERS; ++i )
			{
				WatchRoute_t &cache = s_routes[t][i];
				cache.bValid = false;
				cache.pFrom = nullptr;
				cache.pTo = nullptr;
				cache.flTime = -WATCH_ROUTE_MAX_AGE;
				cache.route.areas.RemoveAll();
				cache.route.travel.RemoveAll();
				cache.routeIndex.RemoveAll();
			}
		}
	}
}
