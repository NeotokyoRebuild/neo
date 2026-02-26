#pragma once

#include "cbase.h"
#include "utlmap.h"
#include "nav_area.h"
#include "neo_player_shared.h"

class CNEOBot;

struct ReservationInfo
{
    EHANDLE hOwner;             // The bot that reserved this area
    float flExpirationTime;     // When the reservation expires (gpGlobals->curtime)
};

struct BotReservedAreas_t
{
    CUtlVector<CNavArea*> areas;

    BotReservedAreas_t() {}
    BotReservedAreas_t(const BotReservedAreas_t& src) { areas = src.areas; }
    BotReservedAreas_t& operator=(const BotReservedAreas_t& src) { areas = src.areas; return *this; }
};

//----------------------------------------------------------------------------------------------------
/**
 * This singleton allows bots to temporarily "claim" navigation areas along their intended path,
 * discouraging other bots from using the same route simultaneously.
 */
class CNEOBotPathReservationSystem
{
public:
    // Need to define a less function for the CUtlMap
    static bool ReservationLessFunc(const int &lhs, const int &rhs)
    {
        return lhs < rhs;
    }

    // Less function for EHANDLE in m_BotReservedAreas
    inline static bool EHandleLessFunc(const EHANDLE &lhs, const EHANDLE &rhs)
    {
        return lhs.GetSerialNumber() < rhs.GetSerialNumber();
    }

    CNEOBotPathReservationSystem() : m_BotReservedAreas(EHandleLessFunc), m_AreaAvoidPenalties(DefLessFunc(unsigned int))
    {
        for (int i = 0; i < TEAM__TOTAL; ++i)
        {
            m_Reservations[i].SetLessFunc(ReservationLessFunc);
            m_AreaPathCounts[i].SetLessFunc(ReservationLessFunc);
        }
    }

    void ReserveArea(CNavArea *area, CNEOBot *bot, float duration);
    void ReleaseArea(CNavArea *area, CNEOBot *bot);
    bool IsAreaReservedByTeammate(CNavArea *area, CNEOBot *avoider) const;
    void Clear();
    void ReleaseAllAreas(CNEOBot *bot);

    void IncrementPredictedFriendlyPathCount( int areaID, int teamID );
    void DecrementPredictedFriendlyPathCount( int areaID, int teamID );
    int GetPredictedFriendlyPathCount( int areaID, int teamID ) const;

    void IncrementAreaAvoidPenalty(unsigned int navAreaID, float penaltyAmount);
    float GetAreaAvoidPenalty(unsigned int navAreaID) const;

    // Allow the global accessor to access private members if needed, though constructor handles init now.
    friend CNEOBotPathReservationSystem* CNEOBotPathReservations();

private:
    CUtlMap<int, ReservationInfo> m_Reservations[TEAM__TOTAL];
    CUtlMap<EHANDLE, BotReservedAreas_t> m_BotReservedAreas;
    CUtlMap<int, int> m_AreaPathCounts[TEAM__TOTAL];
    CUtlMap<unsigned int, float> m_AreaAvoidPenalties;
};


CNEOBotPathReservationSystem* CNEOBotPathReservations();

extern ConVar neo_bot_path_reservation_enable;
extern ConVar neo_bot_path_reservation_penalty;
extern ConVar neo_bot_path_reservation_duration;
extern ConVar neo_bot_path_reservation_distance;
extern ConVar neo_bot_path_reservation_onstuck_penalty;
extern ConVar neo_bot_path_reservation_killed_penalty;
