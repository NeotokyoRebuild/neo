#pragma once

#include "bot/neo_bot.h"

//--------------------------------------------------------------------------------------------------------
class CNEOBotCtgLoneWolf : public Action< CNEOBot >
{
public:
	CNEOBotCtgLoneWolf( void );

	virtual ActionResult< CNEOBot > OnStart( CNEOBot *me, Action< CNEOBot > *priorAction ) override;
	virtual ActionResult< CNEOBot > Update( CNEOBot *me, float interval ) override;
	virtual ActionResult< CNEOBot > OnSuspend( CNEOBot *me, Action< CNEOBot > *interruptingAction ) override;
	virtual ActionResult< CNEOBot > OnResume( CNEOBot *me, Action< CNEOBot > *interruptingAction ) override;

	virtual EventDesiredResult< CNEOBot > OnStuck( CNEOBot *me ) override;
	virtual EventDesiredResult< CNEOBot > OnMoveToSuccess( CNEOBot *me, const Path *path ) override;
	virtual EventDesiredResult< CNEOBot > OnMoveToFailure( CNEOBot *me, const Path *path, MoveToFailureType reason ) override;

	virtual const char *GetName( void ) const override { return "ctgLoneWolf"; }

protected:
	static float GetDetpackDeployDistanceSq( CNEOBot *me );
	Vector GetNearestCapturePoint( CNEOBot *me, bool bEnemyCapPoint );
	bool UpdateGhostHandle( CNEOBot *me );
	ActionResult< CNEOBot > UpdateLookAround( CNEOBot *me );

	CHandle<CWeaponGhost> m_hGhost;
	CountdownTimer m_repathTimer;
	PathFollower m_path;

private:
	bool m_bPursuingDropThreat;

	CHandle<CBaseEntity> m_hPursueTarget;

	CountdownTimer m_capPointUpdateTimer;
	CountdownTimer m_lookAroundTimer;
	CountdownTimer m_stalemateTimer;
	CountdownTimer m_useAttemptTimer;

	CUtlVector< CNavArea * > m_visibleAreas;

	Vector m_closestCapturePoint;
	Vector m_vecDropThreatPos;
};
