#include "cbase.h"
#include "neo_bot_retreat_from_hazard_area.h"
#include "neo_bot_retreat_from_grenade.h"
#include "neo_bot_retreat_to_cover.h"
#include "neo/bot/neo_bot_path_compute.h"
#include "neo/bot/neo_bot_path_reservation.h"

const int MAX_NON_HAZARD_AREA_CANDIDATES = 5;

class CSearchForSafeArea : public ISearchSurroundingAreasFunctor
{
public:
    CSearchForSafeArea(CNEOBot *me)
        : m_me(me)
        , m_onStuckPenalty(neo_bot_path_reservation_onstuck_penalty.GetFloat())
    {
    }

    virtual bool operator()(CNavArea *baseArea, CNavArea *priorArea, float travelDistanceSoFar)
    {
        int id = baseArea->GetID();
        float candidateHazardTime = CNEOBotPathReservations()->GetAreaHazardousTime(id, m_me);
        if (candidateHazardTime > 0)
        {
            if ( m_me->IsDebugging( NEXTBOT_PATH ) )
            {
                baseArea->DrawFilled(255, 0, 0, MIN(255, candidateHazardTime), candidateHazardTime);
            }
        }
        else if (CNEOBotPathReservations()->GetAreaAvoidPenalty(id) < m_onStuckPenalty)
        {
            m_safeAreas.AddToTail(baseArea);
        }

        if (m_safeAreas.Count() >= MAX_NON_HAZARD_AREA_CANDIDATES)
        {
            return false; // found enough candidates
        }

        return true;
    }

    virtual bool ShouldSearch(CNavArea *adjArea, CNavArea *currentArea, float travelDistanceSoFar)
    {
        return (currentArea->ComputeAdjacentConnectionHeightChange(adjArea) <
                m_me->GetLocomotionInterface()->GetMaxJumpHeight());
    }

    CUtlVector<CNavArea *> m_safeAreas;

private:
    CNEOBot *m_me;
    float m_onStuckPenalty;
};

CNEOBotRetreatFromHazardArea::CNEOBotRetreatFromHazardArea()
{
}

CNavArea *CNEOBotRetreatFromHazardArea::FindSafeArea(CNEOBot *me)
{
    CNavArea *start = me->GetLastKnownArea();
    if (!start)
    {
        return nullptr;
    }

    CSearchForSafeArea search(me);
    SearchSurroundingAreas(start, search);

    if (search.m_safeAreas.Count() == 0)
    {
        return nullptr;
    }

    int last = MIN(MAX_NON_HAZARD_AREA_CANDIDATES, search.m_safeAreas.Count());
    int which = RandomInt(0, last - 1);
    return search.m_safeAreas[which];
}

ActionResult<CNEOBot> CNEOBotRetreatFromHazardArea::OnStart(CNEOBot *me, Action<CNEOBot> *priorAction)
{
    m_safeArea = FindSafeArea(me);

    if (!m_safeArea)
    {
        return Done("No safe area available!");
    }

    m_path.SetMinLookAheadDistance(me->GetDesiredPathLookAheadRange());
    m_repathTimer.Invalidate();

    return Continue();
}

ActionResult<CNEOBot> CNEOBotRetreatFromHazardArea::Update(CNEOBot *me, float interval)
{
    CBaseEntity *dangerousGrenade = CNEOBotRetreatFromGrenade::FindDangerousGrenade(me);
    if (dangerousGrenade)
    {
        return ChangeTo(new CNEOBotRetreatFromGrenade(dangerousGrenade), "Encountered grenade while avoiding hazard!");
    }

    CNavArea *myArea = me->GetLastKnownArea();
    if (!myArea)
    {
        return Done("Can't identify my nav area!");
    }

    if ( !CNEOBotPathReservations()->IsAreaHazardous(myArea->GetID(), me))
    {
        return Done("Left hazardous area");
    }

    if ( (myArea->GetID() == m_safeArea->GetID()) || (CNEOBotPathReservations()->IsAreaHazardous(m_safeArea->GetID(), me)) )
    {
        // Safe area is no longer safe at this point, look for new candidate
        m_safeArea = FindSafeArea(me);
        m_repathTimer.Invalidate();
    }

    if (!m_safeArea)
    {
        return Done("Could not find replacement safe area!");
    }

    if (m_repathTimer.IsElapsed())
    {
        m_repathTimer.Start(10.0f);
        // RETREAT_ROUTE: Allow pathing through hazardous areas in CNEOBotPathCost
        CNEOBotPathCompute(me, m_path, m_safeArea->GetCenter(), RETREAT_ROUTE);
    }

    m_path.Update(me);
    return Continue();
}

EventDesiredResult< CNEOBot > CNEOBotRetreatFromHazardArea::OnStuck( CNEOBot *me )
{
    m_safeArea = FindSafeArea(me);
	CNEOBotPathCompute(me, m_path, m_safeArea->GetCenter(), RETREAT_ROUTE);
	return TryContinue();
}

EventDesiredResult< CNEOBot > CNEOBotRetreatFromHazardArea::OnMoveToFailure( CNEOBot *me, const Path *path, MoveToFailureType reason )
{
    m_safeArea = FindSafeArea(me);
	CNEOBotPathCompute(me, m_path, m_safeArea->GetCenter(), RETREAT_ROUTE);
	return TryContinue();
}