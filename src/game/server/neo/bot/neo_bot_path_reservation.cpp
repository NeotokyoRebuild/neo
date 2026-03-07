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
    "How long a path reservation lasts, in seconds.", true, 1, true, 1000000);

ConVar neo_bot_path_reservation_distance("neo_bot_path_reservation_distance", "10000", FCVAR_NONE,
    "How far along the path to reserve, in Hammer units.", true, 0, true, 1000000);

ConVar neo_bot_path_reservation_penalty("neo_bot_path_reservation_penalty", "100", FCVAR_NONE,
    "Pathing cost penalty for a reserved area.", true, 0, true, 1000000);

ConVar neo_bot_path_reservation_friendly_penalty_enable("neo_bot_path_reservation_friendly_penalty_enable", "1", FCVAR_NONE,
    "Whether to update or retrieve the area friendly reservation penalty.", true, 0, true, 1);

ConVar neo_bot_path_reservation_avoid_penalty_enable("neo_bot_path_reservation_avoid_penalty_enable", "1", FCVAR_NONE,
    "Whether to update or retrieve the area avoid penalty.", true, 0, true, 1);

ConVar neo_bot_path_reservation_killed_penalty("neo_bot_path_reservation_killed_penalty", "10", FCVAR_NONE,
    "Path selection penalty added to a nav area each time a bot dies moving through that area.", true, 0, false, 0);

ConVar neo_bot_path_reservation_onstuck_penalty("neo_bot_path_reservation_onstuck_penalty", "10000", FCVAR_NONE,
    "Path selection penalty added to a nav area each time a bot gets stuck moving through that area.", true, 0, false, 0);



CNEOBotPathReservationSystem* CNEOBotPathReservations()
{
    static CNEOBotPathReservationSystem g_BotPathReservations;
    return &g_BotPathReservations;
}

/**
 * Reserve a navigation area for a specific bot for a given duration.
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

    int areaID = area->GetID();
    int reservationIndex = m_Reservations[team].Find(areaID);

    if (reservationIndex == m_Reservations[team].InvalidIndex())
    {
        reservationIndex = m_Reservations[team].Insert(areaID);
    }
    else
    {
        // If the area is already reserved by this bot, and the reservation is not expired,
        // we should decrement the count before updating the reservation to avoid double counting.
        // This handles cases where a bot re-reserves an area it already owns.
        const ReservationInfo &existingInfo = m_Reservations[team].Element(reservationIndex);
        CNEOBot *existingOwner = ToNEOBot(existingInfo.hOwner.Get());
        if (existingOwner == bot && existingInfo.flExpirationTime >= gpGlobals->curtime)
        {
             DecrementPredictedFriendlyPathCount(area->GetID(), team);
        }
    }

    m_Reservations[team][reservationIndex].hOwner = bot;
    m_Reservations[team][reservationIndex].flExpirationTime = gpGlobals->curtime + duration;
    IncrementPredictedFriendlyPathCount(area->GetID(), team);

    // Add to bot's reserved areas list
    EHANDLE hBot = bot;
    int botReservedIndex = m_BotReservedAreas.Find(hBot);
    if (botReservedIndex == m_BotReservedAreas.InvalidIndex())
    {
        m_BotReservedAreas.Insert(hBot, BotReservedAreas_t());
        botReservedIndex = m_BotReservedAreas.Find(hBot);
    }
    
    if (m_BotReservedAreas.Element(botReservedIndex).areas.Find(area) == -1)
    {
        m_BotReservedAreas.Element(botReservedIndex).areas.AddToTail(area);
    }
}

/**
 * Release a navigation area reservation.
 */
void CNEOBotPathReservationSystem::ReleaseArea(CNavArea *area, CNEOBot *bot)
{
    if (!area || !bot)
    {
        return;
    }

    int team = bot->GetTeamNumber();
    if (team < 0 || team >= TEAM__TOTAL)
    {
        return;
    }

    int areaID = area->GetID();
    int reservationIndex = m_Reservations[team].Find(areaID);

    if (reservationIndex == m_Reservations[team].InvalidIndex())
    {
        return; // No reservation for this area
    }

    const ReservationInfo &info = m_Reservations[team].Element(reservationIndex);
    CNEOBot *owner = ToNEOBot(info.hOwner.Get());

    // Only remove if this bot is the current owner and the reservation hasn't expired naturally.
    // If it has expired, or another bot has taken ownership, we shouldn't decrement the count or remove the entry.
    if (owner == bot && info.flExpirationTime >= gpGlobals->curtime)
    {
        DecrementPredictedFriendlyPathCount(area->GetID(), team);
        m_Reservations[team].RemoveAt(reservationIndex);
    }

    // Remove from bot's reserved areas list
    EHANDLE hBot = bot;
    int botReservedIndex = m_BotReservedAreas.Find(hBot);
    if (botReservedIndex != m_BotReservedAreas.InvalidIndex())
    {
        m_BotReservedAreas.Element(botReservedIndex).areas.FindAndRemove(area);
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

    int team = bot->GetTeamNumber();
    if (team < 0 || team >= TEAM__TOTAL)
    {
        return;
    }

    EHANDLE hBot = bot;
    int botReservedIndex = m_BotReservedAreas.Find(hBot);

    if (botReservedIndex == m_BotReservedAreas.InvalidIndex())
    {
        return; // No reservations for this bot
    }

    BotReservedAreas_t &botReservedAreas = m_BotReservedAreas.Element(botReservedIndex);
    for (int i = 0; i < botReservedAreas.areas.Count(); ++i)
    {
        CNavArea *area = botReservedAreas.areas[i];
        if (area)
        {
            int areaID = area->GetID();
            int reservationIndex = m_Reservations[team].Find(areaID);
            if (reservationIndex != m_Reservations[team].InvalidIndex())
            {
                const ReservationInfo &info = m_Reservations[team].Element(reservationIndex);
                if (ToNEOBot(info.hOwner.Get()) == bot)
                {
                     DecrementPredictedFriendlyPathCount(area->GetID(), team);
                     m_Reservations[team].RemoveAt(reservationIndex);
                }
            }
        }
    }
    m_BotReservedAreas.RemoveAt(botReservedIndex);
}

//-------------------------------------------------------------------------------------------------
/**
 * Check if a navigation area is currently reserved by a teammate of the bot avoiding friendlies.
 */
bool CNEOBotPathReservationSystem::IsAreaReservedByTeammate(CNavArea *area, CNEOBot *avoider) const
{
    if (!area || !avoider)
    {
        return false;
    }

    if (!NEORules()->GetTeamPlayEnabled())
    {
        return false;
    }

    int team = avoider->GetTeamNumber();
    if (team < 0 || team >= TEAM__TOTAL)
    {
        return false;
    }

    int areaID = area->GetID();
    int reservationIndex = m_Reservations[team].Find(areaID);

    if (reservationIndex == m_Reservations[team].InvalidIndex())
    {
        return false;
    }

    const ReservationInfo &info = m_Reservations[team].Element(reservationIndex);

    if (info.flExpirationTime < gpGlobals->curtime)
    {
        return false;
    }

    CNEOBot *owner = ToNEOBot(info.hOwner.Get());

    if (owner && owner != avoider)
    {
        return true;
    }

    return false;
}

//--------------------------------------------------------------------------------------------------------------
/**
 * Clear all current path reservations.
 */
void CNEOBotPathReservationSystem::Clear()
{
    for (int team = 0; team < TEAM__TOTAL; ++team)
    {
        m_Reservations[team].RemoveAll();
        m_AreaPathCounts[team].RemoveAll();
    }
    m_BotReservedAreas.RemoveAll();
    m_AreaAvoidPenalties.RemoveAll();
}

//-------------------------------------------------------------------------------------------------
void CNEOBotPathReservationSystem::IncrementPredictedFriendlyPathCount( int areaID, int teamID )
{
    if (teamID < 0 || teamID >= TEAM__TOTAL)
    {
        return;
    }

    int i = m_AreaPathCounts[teamID].Find( areaID );
    if ( m_AreaPathCounts[teamID].IsValidIndex( i ) )
    {
        m_AreaPathCounts[teamID][i]++;
    }
    else
    {
        m_AreaPathCounts[teamID].Insert( areaID, 1 );
    }
}

//-------------------------------------------------------------------------------------------------
void CNEOBotPathReservationSystem::DecrementPredictedFriendlyPathCount( int areaID, int teamID )
{
    if (teamID < 0 || teamID >= TEAM__TOTAL)
    {
        return;
    }

    int i = m_AreaPathCounts[teamID].Find( areaID );
    if ( m_AreaPathCounts[teamID].IsValidIndex( i ) )
    {
        m_AreaPathCounts[teamID][i]--;
        if ( m_AreaPathCounts[teamID][i] <= 0 )
        {
            m_AreaPathCounts[teamID].RemoveAt( i );
        }
    }
}

//-------------------------------------------------------------------------------------------------
int CNEOBotPathReservationSystem::GetPredictedFriendlyPathCount( int areaID, int teamID ) const
{
    if (!neo_bot_path_reservation_friendly_penalty_enable.GetBool()
        || teamID < 0 || teamID >= TEAM__TOTAL)
    {
        return 0;
    }

    int i = m_AreaPathCounts[teamID].Find( areaID );
    if ( m_AreaPathCounts[teamID].IsValidIndex( i ) )
    {
        return m_AreaPathCounts[teamID][i];
    }

    return 0;
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
