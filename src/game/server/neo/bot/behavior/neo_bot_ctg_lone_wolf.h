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

private:
	PathFollower m_path;
	CHandle<CWeaponGhost> m_hGhost;
	CountdownTimer m_repathTimer;
	CountdownTimer m_useAttemptTimer;
	bool m_bHasRetreatedFromGhost;

	Vector m_vecDropThreatPos;
	CHandle<CBaseEntity> m_hPursueTarget;
	bool m_bPursuingDropThreat;

	ActionResult< CNEOBot > UpdateLookAround( CNEOBot *me, const Vector &anchorPos );
	CountdownTimer m_lookAroundTimer;
	CountdownTimer m_stalemateTimer;

	CountdownTimer m_capPointUpdateTimer;
	Vector m_closestCapturePoint;

	CUtlVector< CNavArea * > m_visibleAreas;
};
