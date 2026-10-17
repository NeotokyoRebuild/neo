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

// What bots learned about a nav area this match, kept as path cost penalties
struct AreaPenalty_t
{
    // where bots got stuck this round: a path cost, and to cover and retreat searches a cost, then a filter
    float flStuck = 0.0f;
    // where bots died: only a path cost, since an area where bots died can still be good cover
    float flDeath = 0.0f;

    float GetPathCost() const { return flStuck + flDeath; }
    bool IsStuckProne() const;
};

struct HazardInfo
{
	float smokeExpireTime;      // when the smoke hazard risk expires
	float hazardExpireTime;     // when a general deadly hazard risk expires
	float smokePropagateTime;   // expiry this area has already propagated to its visible set
	float detpackExpireTime;    // when an enemy detpack's blast radius stops blocking every route
};

// A detpack lying in the world, with the nav area it rests on
struct DeployedDetpack_t
{
	EHANDLE hDetpack;
	CNavArea *pArea;
};

// An enemy detpack whose blast radius a team has marked, so it is not re-marked every update
struct DetpackMark_t
{
	EHANDLE hDetpack;
	float flFirstMarkTime;
	float flMarkTime;
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
        , m_AreaPenalties(DefLessFunc(unsigned int))
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
    void IncrementAreaDeathPenalty(unsigned int navAreaID, float penaltyAmount);
    AreaPenalty_t GetAreaPenalty(unsigned int navAreaID) const;

    void AddDeadlyHazard(int navAreaID, float expireTime, int teamID, bool propagatePVS = false);
    void AddFragHazard(int navAreaID, float expireTime, int teamID);
    void AddSmokeHazard(int navAreaID, float expireTime, int teamID, bool propagatePVS = true);
    float GetAreaHazardousTime(int navAreaID, const CNEOBot *me) const;
    bool IsAreaHazardous(int navAreaID, const CNEOBot *me) const;

    const CUtlVector<DeployedDetpack_t> &GetDeployedDetpacks();
    bool IsDetpackMarkFresh(const CBaseEntity *pDetpack, int teamID) const;
    bool MarkDetpackBlast(CBaseEntity *pDetpack, int teamID);
    void AddDetpackHazard(int navAreaID, float expireTime, int teamID);
    bool IsAreaDetpackHazard(int navAreaID, int teamID) const;
    bool HasDetpackMark(int teamID) const;
    float GetLastDetpackMarkTime(int teamID) const;

private:
    void ClearDetpackMarks();

    CUtlVector<DetpackMark_t> m_DetpackMarks[TEAM__TOTAL];
    float m_flNextDetpackScan = 0.0f;
    CUtlVector<DeployedDetpack_t> m_DeployedDetpacks;

    int CountLiveClaims(const AreaReservation_t &res, const CNEOBot *excluding) const;

    CUtlMap<int, AreaReservation_t> m_Reservations[TEAM__TOTAL];   // keyed by nav area ID
    CUtlMap<int, BotReservedAreas_t> m_BotReservedAreas;           // keyed by bot entindex
    AreaPenalty_t *FindOrAddAreaPenalty(unsigned int navAreaID);

    CUtlMap<unsigned int, AreaPenalty_t> m_AreaPenalties;          // keyed by nav area ID
    CUtlMap<int, HazardInfo> m_HazardAreas[TEAM__TOTAL];
};


CNEOBotPathReservationSystem* CNEOBotPathReservations();

extern ConVar neo_bot_path_reservation_enable;
extern ConVar neo_bot_path_reservation_penalty;
extern ConVar neo_bot_path_reservation_duration;
extern ConVar neo_bot_path_reservation_distance;
extern ConVar neo_bot_path_reservation_onstuck_penalty;
extern ConVar neo_bot_path_reservation_killed_penalty;
