#include "cbase.h"
#include "neo/bot/neo_bot_path_reservation.h"
#include "neo/bot/neo_bot.h"
#include "nav_mesh.h"
#include "neo_gamerules.h"

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"


ConVar neo_bot_path_reservation_enable("neo_bot_path_reservation_enable", "1", FCVAR_NONE,
    "Enable the bot path reservation system.", true, 0, true, 1);

ConVar neo_bot_path_reservation_duration("neo_bot_path_reservation_duration", "30.0", FCVAR_NONE,
    "How long a path reservation lasts, in seconds.", true, 1, false, 0);

ConVar neo_bot_path_reservation_distance("neo_bot_path_reservation_distance", "100000", FCVAR_NONE,
    "How far along the path to reserve, in Hammer units.", true, 0, false, 0);

ConVar neo_bot_path_reservation_penalty("neo_bot_path_reservation_penalty", "3000", FCVAR_NONE,
    "Pathing cost penalty for a reserved area.", true, 0, false, 0);

ConVar neo_bot_path_reservation_friendly_penalty_enable("neo_bot_path_reservation_friendly_penalty_enable", "1", FCVAR_NONE,
    "Whether to update or retrieve the area friendly reservation penalty.", true, 0, true, 1);

ConVar neo_bot_path_reservation_avoid_penalty_enable("neo_bot_path_reservation_avoid_penalty_enable", "1", FCVAR_NONE,
    "Whether to update or retrieve the area avoid penalty.", true, 0, true, 1);

ConVar neo_bot_path_reservation_killed_penalty("neo_bot_path_reservation_killed_penalty", "10", FCVAR_NONE,
    "Path selection penalty added to a nav area each time a bot dies moving through that area.", true, 0, false, 0);

ConVar neo_bot_path_reservation_onstuck_penalty("neo_bot_path_reservation_onstuck_penalty", "1000", FCVAR_NONE,
    "Path selection penalty added to a nav area each time a bot gets stuck moving through that area.", true, 0, false, 0);



CNEOBotPathReservationSystem* CNEOBotPathReservations()
{
    static CNEOBotPathReservationSystem g_BotPathReservations;
    return &g_BotPathReservations;
}

//-------------------------------------------------------------------------------------------------
// Check if reservation claim is valid and not expired.
static bool ClaimIsLive(const AreaClaim_t &claim)
{
    return claim.hOwner.Get() != NULL && claim.flExpirationTime >= gpGlobals->curtime;
}

//-------------------------------------------------------------------------------------------------
// Count live reservations on a nav area.
int CNEOBotPathReservationSystem::CountLiveClaims(const AreaReservation_t &res, const CNEOBot *excluding) const
{
    int count = 0;
    for (int i = 0; i < res.claims.Count(); ++i)
    {
        const AreaClaim_t &claim = res.claims[i];
        if (!ClaimIsLive(claim))
        {
            continue;
        }
        if (excluding != NULL && claim.hOwner.Get() == excluding)
        {
            continue;
        }
        ++count;
    }
    return count;
}

//-------------------------------------------------------------------------------------------------
/**
 * Record (or refresh) this bot's claim on a nav area for the given duration.
 */
void CNEOBotPathReservationSystem::ReserveArea(CNavArea *area, CNEOBot *bot, float duration)
{
    if (!area || !bot)
    {
        return;
    }

    if (!NEORules()->GetTeamPlayEnabled())
    {
        return;
    }

    int team = bot->GetTeamNumber();
    if (team < 0 || team >= TEAM__TOTAL)
    {
        return;
    }

    const int areaID = area->GetID();
    const float flExpiration = gpGlobals->curtime + duration;

    int reservationIndex = m_Reservations[team].Find(areaID);
    if (reservationIndex == m_Reservations[team].InvalidIndex())
    {
        reservationIndex = m_Reservations[team].Insert(areaID);
    }

    // Refresh this bot's existing claim, or add a new one.
    // For tracking number of teammates routing through here.
    AreaReservation_t &res = m_Reservations[team][reservationIndex];
    bool bHadClaim = false;
    for (int i = 0; i < res.claims.Count(); ++i)
    {
        if (res.claims[i].hOwner.Get() == bot)
        {
            res.claims[i].flExpirationTime = flExpiration;
            bHadClaim = true;
            break;
        }
    }
    if (!bHadClaim)
    {
        AreaClaim_t claim;
        claim.hOwner = bot;
        claim.flExpirationTime = flExpiration;
        res.claims.AddToTail(claim);
    }

    // Reverse index for fast release.
    int botIndex = m_BotReservedAreas.Find(bot->entindex());
    if (botIndex == m_BotReservedAreas.InvalidIndex())
    {
        botIndex = m_BotReservedAreas.Insert(bot->entindex());
    }
    if (m_BotReservedAreas[botIndex].areas.Find(area) == -1)
    {
        m_BotReservedAreas[botIndex].areas.AddToTail(area);
    }
}

//-------------------------------------------------------------------------------------------------
/**
 * Release all navigation area reservations for a specific bot.
 */
void CNEOBotPathReservationSystem::ReleaseAllAreas(CNEOBot *bot)
{
    if (!bot)
    {
        return;
    }

    int botIndex = m_BotReservedAreas.Find(bot->entindex());
    if (botIndex == m_BotReservedAreas.InvalidIndex())
    {
        return; // No reservations for this bot
    }

    const int team = bot->GetTeamNumber();
    if (team >= 0 && team < TEAM__TOTAL)
    {
        const CUtlVector<CNavArea*> &areas = m_BotReservedAreas[botIndex].areas;
        for (int a = 0; a < areas.Count(); ++a)
        {
            CNavArea *area = areas[a];
            if (!area)
            {
                continue;
            }

            int reservationIndex = m_Reservations[team].Find(area->GetID());
            if (reservationIndex == m_Reservations[team].InvalidIndex())
            {
                continue;
            }

            AreaReservation_t &res = m_Reservations[team][reservationIndex];
            for (int i = res.claims.Count() - 1; i >= 0; --i)
            {
                if (res.claims[i].hOwner.Get() == bot)
                {
                    res.claims.Remove(i);
                }
            }
            if (res.claims.Count() == 0)
            {
                m_Reservations[team].RemoveAt(reservationIndex);
            }
        }
    }

    m_BotReservedAreas.RemoveAt(botIndex);
}

//--------------------------------------------------------------------------------------------------------------
/**
 * Clear all current path reservations.
 */
void CNEOBotPathReservationSystem::Clear()
{
    ClearRound();
    m_AreaAvoidPenalties.RemoveAll();
}

//--------------------------------------------------------------------------------------------------------------
/**
 * Clear round specific path reservations.
 */
void CNEOBotPathReservationSystem::ClearRound()
{
    for (int team = 0; team < TEAM__TOTAL; ++team)
    {
        m_Reservations[team].RemoveAll();
        m_HazardAreas[team].RemoveAll();
    }
    m_BotReservedAreas.RemoveAll();
}

//-------------------------------------------------------------------------------------------------
int CNEOBotPathReservationSystem::GetPredictedFriendlyPathCount( int areaID, int teamID, const CNEOBot *excluding ) const
{
    if (!NEORules()->GetTeamPlayEnabled())
    {
        return 0;
    }

    if (!neo_bot_path_reservation_friendly_penalty_enable.GetBool()
        || teamID < 0 || teamID >= TEAM__TOTAL)
    {
        return 0;
    }

    int reservationIndex = m_Reservations[teamID].Find(areaID);
    if (reservationIndex == m_Reservations[teamID].InvalidIndex())
    {
        return 0;
    }

    return CountLiveClaims(m_Reservations[teamID][reservationIndex], excluding);
}

//-------------------------------------------------------------------------------------------------
void CNEOBotPathReservationSystem::IncrementAreaAvoidPenalty(unsigned int navAreaID, float penaltyAmount)
{
    if ( !neo_bot_path_reservation_avoid_penalty_enable.GetBool() )
    {
        return;
    }

    unsigned short index = m_AreaAvoidPenalties.Find(navAreaID);
    if (index == m_AreaAvoidPenalties.InvalidIndex())
    {
        index = m_AreaAvoidPenalties.Insert(navAreaID, 0.0f);
    }

    m_AreaAvoidPenalties[index] += penaltyAmount;
}

//-------------------------------------------------------------------------------------------------
float CNEOBotPathReservationSystem::GetAreaAvoidPenalty(unsigned int navAreaID) const
{
    if ( !neo_bot_path_reservation_avoid_penalty_enable.GetBool() )
    {
        return 0.0f;
    }

    unsigned short index = m_AreaAvoidPenalties.Find(navAreaID);
    if (index != m_AreaAvoidPenalties.InvalidIndex())
    {
        return m_AreaAvoidPenalties[index];
    }
    return 0.0f;
}

//-------------------------------------------------------------------------------------------------
// Functor to propagate deadly hazard to PVS-adjacent areas
struct CNEOFunctorPropagatePVSDeadlyHazard
{
	CNEOFunctorPropagatePVSDeadlyHazard(float expireTime, int teamID)
		: m_expireTime(expireTime), m_teamID(teamID)
	{
	}

	bool operator()(CNavArea *area)
	{
		CNEOBotPathReservations()->AddDeadlyHazard(area->GetID(), m_expireTime, m_teamID);
		return true;
	}

	float m_expireTime;
	int m_teamID;
};

//-------------------------------------------------------------------------------------------------
// Marks an area temporarily for bots to avoid or escape from
void CNEOBotPathReservationSystem::AddDeadlyHazard(int navAreaID, float expireTime, int teamID, bool propagatePVS)
{
    if ( !neo_bot_path_reservation_avoid_penalty_enable.GetBool() )
    {
        return;
    }

	if ( (teamID < 0) || (teamID >= TEAM__TOTAL) )
	{
		return;
	}

	int index = m_HazardAreas[teamID].Find(navAreaID);
	if ( !m_HazardAreas[teamID].IsValidIndex(index) )
	{
        // Initialize blank slate lookup entry
		HazardInfo blank;
		blank.hazardExpireTime = expireTime;
		blank.smokeExpireTime = 0.0f;
		index = m_HazardAreas[teamID].Insert(navAreaID, blank);
	}
    else
    {
	    HazardInfo &existing = m_HazardAreas[teamID][index];
        // Optimizing simplification: assume new time is later to skip comparisons
        // May also work for resetting an area to be non-hazardous early
	    existing.hazardExpireTime = expireTime;
    }

    if ( propagatePVS )
    {
        CNavArea *area = TheNavMesh->GetNavAreaByID(navAreaID);
        if (area)
        {
            CNEOFunctorPropagatePVSDeadlyHazard propagate(expireTime, teamID);
            // CompletelyVisible: fewer areas to iterate through and definitely exposed
            // vs PotentiallyVisible: for narrow corridors, some areas would count even if only a sliver was exposed
            area->ForAllCompletelyVisibleAreas(propagate);
        }
    }
}

//-------------------------------------------------------------------------------------------------
void CNEOBotPathReservationSystem::AddFragHazard(int navAreaID, float expireTime, int teamID)
{
    AddDeadlyHazard(navAreaID, expireTime, teamID, true);
    // true (propagatePVS) - propagate hazard to all adjacent PVS areas
    // shorthand for labeling entire trajectory and potential stray angles as hazardous
    // also intended for grenade thrower to duck behind cover from perspective of target
}

//-------------------------------------------------------------------------------------------------
// Functor to propagate a smoke hazard to PVS-adjacent areas
struct CNEOFunctorPropagatePVSSmokeHazard
{
	CNEOFunctorPropagatePVSSmokeHazard(float expireTime, int teamID)
		: m_expireTime(expireTime), m_teamID(teamID)
	{
	}

	bool operator()(CNavArea *area)
	{
		CNEOBotPathReservations()->AddSmokeHazard(area->GetID(), m_expireTime, m_teamID, false);
		return true;
	}

	float m_expireTime;
	int m_teamID;
};

//-------------------------------------------------------------------------------------------------
// Marks an area temporarily for bots to avoid or escape from
// Support bots ignore this smoke hazard
void CNEOBotPathReservationSystem::AddSmokeHazard(int navAreaID, float expireTime, int teamID, bool propagatePVS)
{
    if ( !neo_bot_path_reservation_avoid_penalty_enable.GetBool() )
    {
        return;
    }

	if (teamID < 0 || teamID >= TEAM__TOTAL)
	{
		return;
	}

	int index = m_HazardAreas[teamID].Find(navAreaID);
	if (!m_HazardAreas[teamID].IsValidIndex(index))
	{
        // Initialize blank slate lookup entry
		HazardInfo blank;
		blank.hazardExpireTime = 0.0f;
		blank.smokeExpireTime = expireTime;
		index = m_HazardAreas[teamID].Insert(navAreaID, blank);
	}
    else
    {
        HazardInfo &existing = m_HazardAreas[teamID][index];
        // Optimizing simplification: assume new time is later to skip comparisons
        // May also work for resetting an area to be non-hazardous early
        existing.smokeExpireTime = expireTime;

    }

    if (propagatePVS)
    {
        CNavArea *area = TheNavMesh->GetNavAreaByID(navAreaID);
        if (area)
        {
            CNEOFunctorPropagatePVSSmokeHazard propagate(expireTime, teamID);
            // CompletelyVisible: fewer areas to iterate through and definitely exposed
            // vs PotentiallyVisible: for narrow corridors, some areas would count even if only a sliver was exposed
            area->ForAllCompletelyVisibleAreas(propagate);
        }
    }
}

//-------------------------------------------------------------------------------------------------
float CNEOBotPathReservationSystem::GetAreaHazardousTime(int navAreaID, const CNEOBot *me) const
{
    if (!neo_bot_path_reservation_avoid_penalty_enable.GetBool())
    {
        return 0.0f;
    }

	if (!me)
	{
		return 0.0f;
	}

	int teamID = me->GetTeamNumber();

	if (teamID < 0 || teamID >= TEAM__TOTAL)
	{
		return 0.0f;
	}

	int index = m_HazardAreas[teamID].Find(navAreaID);
	if (m_HazardAreas[teamID].IsValidIndex(index))
	{
		const HazardInfo &h = m_HazardAreas[teamID][index];

		auto hazardTimeLeft = h.hazardExpireTime - gpGlobals->curtime;
		if (hazardTimeLeft > 0)
		{
	        // All bots avoid explosive hazards
			return hazardTimeLeft;
		}

        // Support class can see through smoke
        if (me->GetClass() != NEO_CLASS_SUPPORT)
        {
            auto smokeTimeLeft = h.smokeExpireTime - gpGlobals->curtime;
            if (smokeTimeLeft > 0)
            {
                return smokeTimeLeft;
            }
        }
	}

	return 0.0f;
}

//-------------------------------------------------------------------------------------------------
bool CNEOBotPathReservationSystem::IsAreaHazardous(int navAreaID, const CNEOBot *me) const
{
   return GetAreaHazardousTime(navAreaID, me) > 0;
}
