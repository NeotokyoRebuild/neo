#pragma once

#include "NextBotBehavior.h"
#include "nav_mesh.h"
#include "neo/bot/neo_bot.h"
#include "neo/bot/neo_bot_path_compute.h"

class CNEOBotRetreatFromHazardArea : public Action<CNEOBot>
{
public:
    CNEOBotRetreatFromHazardArea();

    virtual ActionResult<CNEOBot> OnStart(CNEOBot *me, Action<CNEOBot> *priorAction) OVERRIDE;
    virtual ActionResult<CNEOBot> Update(CNEOBot *me, float interval) OVERRIDE;

    virtual EventDesiredResult<CNEOBot> OnStuck(CNEOBot *me) OVERRIDE;
    virtual EventDesiredResult<CNEOBot> OnMoveToFailure(CNEOBot *me, const Path *path, MoveToFailureType reason) OVERRIDE;

    virtual const char *GetName() const OVERRIDE { return "RetreatFromHazardArea"; }

private:
    CNavArea *m_safeArea;
    CountdownTimer m_repathTimer;
    PathFollower m_path;

    CNavArea *FindSafeArea(CNEOBot *me);
};
