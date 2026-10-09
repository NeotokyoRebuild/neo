#include "cbase.h"
#include "neo_bot.h"
#include "neo_bot_path_cost.h"
#include "neo_gamerules.h"
#include "neo_bot_locomotion.h"
#include "nav_mesh.h"
#include "neo_bot_path_reservation.h"

ConVar neo_bot_path_around_friendly_cooldown("neo_bot_path_around_friendly_cooldown", "2.0", FCVAR_CHEAT,
	"How often to check for friendly path dispersion", true, 0, true, 60);

ConVar neo_bot_path_penalty_jump_multiplier("neo_bot_path_penalty_jump_multiplier", "100000.0", FCVAR_CHEAT,
	"Maximum penalty multiplier for jump height changes in pathfinding", true, 0.01f, false, 0.0f);

ConVar neo_bot_path_penalty_ladder_multiplier("neo_bot_path_penalty_ladder_multiplier", "3.0", FCVAR_CHEAT,
	"Penalty multiplier for ladder traversal in pathfinding", true, 0.1f, false, 0.0f);

ConVar neo_bot_path_penalty_exposure_base("neo_bot_path_penalty_exposure_base", "5.0", FCVAR_CHEAT,
	"General additional penalty per visible area for bots to avoid exposed areas", true, 0.0f, false, 0.0f);

ConVar neo_bot_path_penalty_exposure_pistol("neo_bot_path_penalty_exposure_pistol", "10.0", FCVAR_CHEAT,
	"Additional penalty per visible area for bots wielding pistol caliber weapons", true, 0.0f, false, 0.0f);

ConVar neo_bot_path_penalty_exposure_shotgun("neo_bot_path_penalty_exposure_shotgun", "20.0", FCVAR_CHEAT,
	"Additional penalty per visible area for shotgun-wielding bots", true, 0.0f, false, 0.0f);

ConVar neo_bot_path_penalty_exposure_inverse_base_battle_rifle("neo_bot_path_penalty_exposure_inverse_base_battle_rifle", "500.0", FCVAR_CHEAT,
	"Base penalty for calculating inverse traversal penalty for semi-auto battle rifles", true, 1.0f, false, 0.0f);

ConVar neo_bot_path_penalty_exposure_inverse_base_scoped("neo_bot_path_penalty_exposure_inverse_base_scoped", "1000.0", FCVAR_CHEAT,
	"Base penalty for calculating inverse traversal penalty for scoped weapons", true, 1.0f, false, 0.0f);

ConVar neo_bot_path_visibility_exposure_enable("neo_bot_path_visibility_exposure_enable", "1", FCVAR_NONE,
	"Enable visibility-exposure pathing penalties", true, 0, true, 1);

//-------------------------------------------------------------------------------------------------
CNEOBotPathCost::CNEOBotPathCost(CNEOBot* me, RouteType routeType)
{
	m_me = me;
	m_routeType = routeType;
	m_stepHeight = me->GetLocomotionInterface()->GetStepHeight();
	m_maxJumpHeight = me->GetLocomotionInterface()->GetMaxJumpHeight();
	m_maxDropHeight = me->GetLocomotionInterface()->GetDeathDropHeight();
	m_bIgnoreReservations = !neo_bot_path_reservation_enable.GetBool();
	m_bIgnoreHazards = (me->m_hCommandingPlayer.Get() != nullptr);
	m_bIgnoreVisibilityExposure = !neo_bot_path_visibility_exposure_enable.GetBool();
}

//-------------------------------------------------------------------------------------------------
// A crouch area promises HumanCrouchHeight (55 u) of room, but Support ducks to 59 u and the
// Juggernaut to 75 u, so some crouch gaps in the mesh are ones they cannot get through.

static constexpr float NEO_DUCK_LANE_SPACING = 4.0f;	// one lane per this much portal width, edge to edge
static constexpr float NEO_DUCK_LANE_REACH = 2.0f;	// how far a lane runs into each area
static constexpr float NEO_DUCK_LANE_INSET = 0.5f;	// lane ends keep this far inside their area

// A crouch portal and a ducked hull height: the key of one lane verdict
struct NeoDuckLaneKey
{
	unsigned int m_fromId;
	unsigned int m_toId;
	int m_height;

	bool operator<( const NeoDuckLaneKey &other ) const
	{
		if ( m_fromId != other.m_fromId )
		{
			return m_fromId < other.m_fromId;
		}

		if ( m_toId != other.m_toId )
		{
			return m_toId < other.m_toId;
		}

		return m_height < other.m_height;
	}
};

// The lane traces see only the world and static props, so a verdict holds until the level changes
static CUtlMap<NeoDuckLaneKey, bool> s_duckLaneVerdicts( DefLessFunc( NeoDuckLaneKey ) );

class CNeoDuckLaneReset : public CAutoGameSystem
{
public:
	CNeoDuckLaneReset() : CAutoGameSystem( "CNeoDuckLaneReset" )
	{
	}

	virtual void LevelShutdownPostEntity() override
	{
		s_duckLaneVerdicts.RemoveAll();
	}
};

static CNeoDuckLaneReset s_duckLaneReset;

// The point of the area nearest (x, y), on the floor under the hull's footprint, which can lie
// a few units off the nav plane
static Vector NeoLaneEnd( const CNavArea *area, float x, float y, const Vector &vecMins, const Vector &vecMaxs )
{
	const Vector nw = area->GetCorner( NORTH_WEST );
	const Vector se = area->GetCorner( SOUTH_EAST );
	const float flInsetX = MIN( NEO_DUCK_LANE_INSET, ( se.x - nw.x ) / 2.0f );
	const float flInsetY = MIN( NEO_DUCK_LANE_INSET, ( se.y - nw.y ) / 2.0f );
	Vector pos( clamp( x, nw.x + flInsetX, se.x - flInsetX ), clamp( y, nw.y + flInsetY, se.y - flInsetY ), 0.0f );
	pos.z = area->GetZ( pos.x, pos.y );

	// a footprint-sized slab, 1 u thick, dropped from a step above the nav plane to a step below
	CTraceFilterWorldAndPropsOnly filter;
	trace_t tr;
	UTIL_TraceHull( pos + Vector( 0.0f, 0.0f, StepHeight ), pos - Vector( 0.0f, 0.0f, StepHeight ),
		Vector( vecMins.x, vecMins.y, 0.0f ), Vector( vecMaxs.x, vecMaxs.y, 1.0f ), MASK_PLAYERSOLID, &filter, &tr );
	if ( !tr.startsolid && tr.fraction < 1.0f )
	{
		pos.z = tr.endpos.z;
	}

	return pos;
}

// Can the hull get from a to b? It is swept with its underside a step above the floor, the way
// CGameMovement steps, so a lip or slope lower than a step does not block it
static bool NeoLaneClear( const Vector &a, const Vector &b, const Vector &vecMins, const Vector &vecMaxs )
{
	const Vector mins( vecMins.x, vecMins.y, StepHeight );
	const Vector maxs( vecMaxs.x, vecMaxs.y, vecMaxs.z - vecMins.z );
	const float flTop = MAX( a.z, b.z );
	const Vector legs[] = { a, Vector( a.x, a.y, flTop ), Vector( b.x, b.y, flTop ), b };
	const int nLegs = ARRAYSIZE( legs );

	CTraceFilterWorldAndPropsOnly filter;
	for ( int i = 0; i + 1 < nLegs; ++i )
	{
		trace_t tr;
		UTIL_TraceHull( legs[i], legs[i + 1], mins, maxs, MASK_PLAYERSOLID, &filter, &tr );
		if ( tr.startsolid || tr.fraction < 1.0f )
		{
			return false;
		}
	}

	return true;
}

// Does the hull fit through the portal from 'from' into 'to' on at least one lane? Lanes run
// across the portal, each from just inside one area to just inside the other
static bool NeoDuckLaneFits( const CNavArea *from, const CNavArea *to, const Vector &vecMins, const Vector &vecMaxs )
{
	NavDirType dir = NUM_DIRECTIONS;
	for ( int d = 0; d < NUM_DIRECTIONS; ++d )
	{
		if ( from->IsConnected( to, (NavDirType)d ) )
		{
			dir = (NavDirType)d;
			break;
		}
	}

	if ( dir == NUM_DIRECTIONS )
	{
		return true;
	}

	Vector center;
	float flHalfWidth;
	from->ComputePortal( to, dir, &center, &flHalfWidth );

	Vector2D across;
	DirectionToVector2D( dir, &across );
	const Vector2D along( across.y != 0.0f ? 1.0f : 0.0f, across.x != 0.0f ? 1.0f : 0.0f );
	const float flSpan = MAX( 0.0f, flHalfWidth - NEO_DUCK_LANE_INSET );
	const int nLanes = 1 + (int)( 2.0f * flSpan / NEO_DUCK_LANE_SPACING );
	for ( int i = 0; i < nLanes; ++i )
	{
		const float flOffset = ( nLanes == 1 ) ? 0.0f : -flSpan + 2.0f * flSpan * i / ( nLanes - 1 );
		const float x = center.x + along.x * flOffset;
		const float y = center.y + along.y * flOffset;
		const float dx = across.x * NEO_DUCK_LANE_REACH;
		const float dy = across.y * NEO_DUCK_LANE_REACH;
		const Vector a = NeoLaneEnd( from, x - dx, y - dy, vecMins, vecMaxs );
		const Vector b = NeoLaneEnd( to, x + dx, y + dy, vecMins, vecMaxs );
		if ( NeoLaneClear( a, b, vecMins, vecMaxs ) )
		{
			return true;
		}
	}

	return false;
}

// Is a level step into or out of a crouch area open to this bot's ducked hull? Climbs and drops,
// and classes that duck within HumanCrouchHeight, are not tested
static bool NeoDuckLaneOpen( CNEOBot *me, const CNavArea *from, const CNavArea *to )
{
	if ( !from->HasAttributes( NAV_MESH_CROUCH ) && !to->HasAttributes( NAV_MESH_CROUCH ) )
	{
		return true;
	}

	const Vector vecMins = VEC_DUCK_HULL_MIN_SCALED( me );
	const Vector vecMaxs = VEC_DUCK_HULL_MAX_SCALED( me );
	const float flHeight = vecMaxs.z - vecMins.z;
	if ( flHeight <= HumanCrouchHeight )
	{
		return true;
	}

	if ( fabs( from->ComputeAdjacentConnectionHeightChange( to ) ) > StepHeight )
	{
		return true;
	}

	const NeoDuckLaneKey key = { from->GetID(), to->GetID(), (int)flHeight };
	const auto idx = s_duckLaneVerdicts.Find( key );
	if ( idx != s_duckLaneVerdicts.InvalidIndex() )
	{
		return s_duckLaneVerdicts[idx];
	}

	const bool bOpen = NeoDuckLaneFits( from, to, vecMins, vecMaxs );
	s_duckLaneVerdicts.Insert( key, bOpen );
	return bOpen;
}

//-------------------------------------------------------------------------------------------------
float CNEOBotPathCost::operator()(CNavArea* baseArea, CNavArea* fromArea, const CNavLadder* ladder, const CFuncElevator* elevator, float length) const
{
	VPROF_BUDGET("CNEOBotPathCost::operator()", "NextBot");

	CNavArea* area = (CNavArea*)baseArea;

	if (fromArea == NULL)
	{
		// first area in path, no cost
		return 0.0f;
	}

	if (!m_me->GetLocomotionInterface()->IsAreaTraversable(area))
	{
		return -1.0f;
	}

	// Support and Juggernaut duck too tall for some crouch gaps
	if ( !ladder && !elevator && !NeoDuckLaneOpen( m_me, fromArea, area ) )
	{
		return -1.0f;
	}

	if ( !m_bIgnoreHazards && CNEOBotPathReservations()->IsAreaHazardous(area->GetID(), m_me) )
	{
		if ( m_routeType == DEFAULT_ROUTE )
		{
			// attempt to route around hazards
			// if blocked, assumes includeGoalIfPathFails=true
			// to partially path up to boundary of hazard area
			return -1.0f;
		}
	}

	// compute distance traveled along path so far
	float dist;

	if (ladder)
	{
		dist = ladder->m_length;

		// ladders leave bots exposed, but can be a shortcut
		const float ladderPenalty = neo_bot_path_penalty_ladder_multiplier.GetFloat();
		dist *= ladderPenalty;
	}
	else if (length > 0.0)
	{
		dist = length;
	}
	else
	{
		dist = (area->GetCenter() - fromArea->GetCenter()).Length();
	}

	// Only apply height restrictions for non-ladder jump paths
	if (!ladder)
	{
		// check height change
		float deltaZ = fromArea->ComputeAdjacentConnectionHeightChange(area);

		if (deltaZ >= m_stepHeight)
		{
			if (deltaZ >= m_maxJumpHeight)
			{
				// too high to reach
				return -1.0f;
			}

			// jumping is slower than flat ground
			const float jumpPenalty = neo_bot_path_penalty_jump_multiplier.GetFloat() * Square( deltaZ / m_maxJumpHeight );
			dist *= jumpPenalty;
		}
		else if (deltaZ < -m_maxDropHeight)
		{
			// too far to drop
			return -1.0f;
		}
	}

	// add a random penalty unique to this character so they choose different routes to the same place
	float preference = 1.0f;

	if (m_routeType == DEFAULT_ROUTE)
	{
		// this term causes the same bot to choose different routes over time,
		// but keep the same route for a period in case of repaths
		int timeMod = (int)(gpGlobals->curtime / 10.0f) + 1;
		preference = 1.0f + 50.0f * (1.0f + FastCos((float)(m_me->GetEntity()->entindex() * area->GetID() * timeMod)));
	}

	if (m_routeType == SAFEST_ROUTE)
	{
		// misyl: combat areas.
#if 0
		// avoid combat areas
		if (area->IsInCombat())
		{
			const float combatDangerCost = 4.0f;
			dist *= combatDangerCost * area->GetCombatIntensity();
		}
#endif
	}


	float cost = (dist * preference);

	// ------------------------------------------------------------------------------------------------
	// New path reservation related cost adjustments
	if ( !m_bIgnoreReservations && (m_routeType != FASTEST_ROUTE) )
	{
		cost += CNEOBotPathReservations()->GetAreaAvoidPenalty(area->GetID());

		if ( NEORules()->IsTeamplay() )
		{
			const int nFriendly = CNEOBotPathReservations()->GetPredictedFriendlyPathCount(area->GetID(), m_me->GetTeamNumber(), m_me);
			if (nFriendly > 0)
			{
				// Discourage team clustering: (n^2 * penalty)
				cost += nFriendly * nFriendly * neo_bot_path_reservation_penalty.GetFloat();
			}

			if (m_routeType == SAFEST_ROUTE)
			{
				// NEO Jank Cheat: Incorporate enemy bot paths so that we don't run directly into their line of fire
				// Intended for use by ghost carrier team, to emulate a team that knows where enemies are likely to ambush
				// Compensates for bots' lack of meta knowledge by making them prefer routes not reserved by enemies
				// Adheres to cheat against bots but not against humans philosophy by not considering human players' positions
				const int nEnemy = CNEOBotPathReservations()->GetPredictedFriendlyPathCount(area->GetID(), GetEnemyTeam(m_me->GetTeamNumber()));
				cost += nEnemy * neo_bot_path_reservation_penalty.GetFloat() * 2.0f;
			}
		}
	}

	if ( !m_bIgnoreVisibilityExposure && (m_routeType != FASTEST_ROUTE) )
	{
		// Weapon range penalties
		auto* myWeapon = assert_cast<CNEOBaseCombatWeapon*>(m_me->GetActiveWeapon());
		if (myWeapon)
		{
			const int nWeaponBits = myWeapon->GetNeoWepBits();
			if (nWeaponBits & NEO_WEP_FIREARM)
			{
				const int visibleAreaCount = area->GetPotentiallyVisibleAreaCount();
				if (visibleAreaCount > 0)
				{
					constexpr int nShotgunBits = NEO_WEP_AA13 | NEO_WEP_SUPA7;
					constexpr int nBattleRifleBits = NEO_WEP_M41 | NEO_WEP_M41_S;
					constexpr int nPistolCaliberBits = NEO_WEP_MILSO | NEO_WEP_TACHI | NEO_WEP_KYLA
						| NEO_WEP_MPN | NEO_WEP_MPN_S | NEO_WEP_JITTE | NEO_WEP_JITTE_S | NEO_WEP_SRM | NEO_WEP_SRM_S;

					if (nWeaponBits & nPistolCaliberBits)
					{
						// Weapons that don't have max first shot accuracy
						const float exposurePenalty = neo_bot_path_penalty_exposure_pistol.GetFloat();
						cost += visibleAreaCount * exposurePenalty;
					}
					else if (nWeaponBits & nShotgunBits)
					{
						// Weapons that have spread that can't hit long range targets
						const float exposurePenalty = neo_bot_path_penalty_exposure_shotgun.GetFloat();
						cost += visibleAreaCount * exposurePenalty;
					}
					else if (nWeaponBits & nBattleRifleBits)
					{
						// Weapons that benefit from medium sightlines that can see many NavAreas
						const float baseline_penalty = neo_bot_path_penalty_exposure_inverse_base_battle_rifle.GetFloat();
						cost += baseline_penalty / visibleAreaCount;
					}
					else if (nWeaponBits & NEO_WEP_SCOPEDWEAPON)
					{
						// Weapons that benefit from long sightlines that can see many NavAreas
						const float baseline_penalty = neo_bot_path_penalty_exposure_inverse_base_scoped.GetFloat();
						cost += baseline_penalty / visibleAreaCount;
					}
					else
					{
						// Generally avoiding exposed areas when traversing a wide open area
						const float exposurePenalty = neo_bot_path_penalty_exposure_base.GetFloat();
						cost += visibleAreaCount * exposurePenalty;
					}
				}
			}
		}
	}
	// ------------------------------------------------------------------------------------------------

	if (area->HasAttributes(NAV_MESH_FUNC_COST))
	{
		cost *= area->ComputeFuncNavCost(m_me);
		DebuggerBreakOnNaN_StagingOnly(cost);
	}

	return cost + fromArea->GetCostSoFar();
}
