#pragma once

#include "cbase.h"
#include "utlmap.h"
#include "utlvector.h"
#include "nav_area.h"
#include "neo_player_shared.h"

class CNEOBot;

struct AreaClaim_t
{
    EHANDLE hOwner;
    float flExpirationTime; // gpGlobals->curtime past which this claim no longer counts
};

struct AreaReservation_t
{
    CUtlVector<AreaClaim_t> claims;

    AreaReservation_t() {}
    AreaReservation_t(const AreaReservation_t& src) { claims = src.claims; }
    AreaReservation_t& operator=(const AreaReservation_t& src) { claims = src.claims; return *this; }
};

struct HazardInfo
{
	float smokeExpireTime;      // when the smoke hazard risk expires
	float hazardExpireTime;     // when a general deadly hazard risk expires
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
    static bool AreaIDLessFunc(const int &lhs, const int &rhs)
    {
        return lhs < rhs;
    }

    CNEOBotPathReservationSystem()
        : m_BotReservedAreas(DefLessFunc(int))
        , m_AreaAvoidPenalties(DefLessFunc(unsigned int))
    {
        for (int i = 0; i < TEAM__TOTAL; ++i)
        {
            m_Reservations[i].SetLessFunc(AreaIDLessFunc);
            m_HazardAreas[i].SetLessFunc(AreaIDLessFunc);
        }
    }

    void ReserveArea(CNavArea *area, CNEOBot *bot, float duration);
    void ReleaseAllAreas(CNEOBot *bot);
    void Clear();
    void ClearRound();

    int GetPredictedFriendlyPathCount( int areaID, int teamID, const CNEOBot *excluding = NULL ) const;

    void IncrementAreaAvoidPenalty(unsigned int navAreaID, float penaltyAmount);
    float GetAreaAvoidPenalty(unsigned int navAreaID) const;

    void AddDeadlyHazard(int navAreaID, float expireTime, int teamID, bool propagatePVS = false);
    void AddFragHazard(int navAreaID, float expireTime, int teamID);
    void AddSmokeHazard(int navAreaID, float expireTime, int teamID, bool propagatePVS = true);
    float GetAreaHazardousTime(int navAreaID, const CNEOBot *me) const;
    bool IsAreaHazardous(int navAreaID, const CNEOBot *me) const;

private:
    int CountLiveClaims(const AreaReservation_t &res, const CNEOBot *excluding) const;

    CUtlMap<int, AreaReservation_t> m_Reservations[TEAM__TOTAL];   // keyed by nav area ID
    CUtlMap<int, BotReservedAreas_t> m_BotReservedAreas;           // keyed by bot entindex
    CUtlMap<unsigned int, float> m_AreaAvoidPenalties;
    CUtlMap<int, HazardInfo> m_HazardAreas[TEAM__TOTAL];
};


CNEOBotPathReservationSystem* CNEOBotPathReservations();

extern ConVar neo_bot_path_reservation_enable;
extern ConVar neo_bot_path_reservation_penalty;
extern ConVar neo_bot_path_reservation_duration;
extern ConVar neo_bot_path_reservation_distance;
extern ConVar neo_bot_path_reservation_onstuck_penalty;
extern ConVar neo_bot_path_reservation_killed_penalty;
