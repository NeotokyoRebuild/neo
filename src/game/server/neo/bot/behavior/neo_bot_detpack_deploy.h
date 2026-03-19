#pragma once

#include "bot/neo_bot.h"

class CWeaponDetpack;

//--------------------------------------------------------------------------------------------------------
class CNEOBotDetpackDeploy : public Action< CNEOBot >
{
public:
	CNEOBotDetpackDeploy( const Vector &targetPos, Action< CNEOBot > *nextAction = nullptr );

	virtual ActionResult< CNEOBot > OnStart( CNEOBot *me, Action< CNEOBot > *priorAction ) override;
	virtual ActionResult< CNEOBot > Update( CNEOBot *me, float interval ) override;
	virtual ActionResult< CNEOBot > OnSuspend( CNEOBot *me, Action< CNEOBot > *interruptingAction ) override;
	virtual ActionResult< CNEOBot > OnResume( CNEOBot *me, Action< CNEOBot > *interruptingAction ) override;
	virtual void OnEnd( CNEOBot *me, Action< CNEOBot > *nextAction ) override;

	virtual EventDesiredResult< CNEOBot > OnStuck( CNEOBot *me ) override;
	virtual EventDesiredResult< CNEOBot > OnMoveToSuccess( CNEOBot *me, const Path *path ) override;
	virtual EventDesiredResult< CNEOBot > OnMoveToFailure( CNEOBot *me, const Path *path, MoveToFailureType reason ) override;

	virtual const char *GetName( void ) const override { return "DetpackDeploy"; }

private:
	bool m_bPushedWeapon;
	float m_flDeployDistSq;
	Action< CNEOBot > *m_nextAction;
	CHandle< CWeaponDetpack > m_hDetpackWeapon;
	CountdownTimer m_expiryTimer;
	CountdownTimer m_losTimer;
	CountdownTimer m_repathTimer;
	PathFollower m_path;
	Vector m_targetPos;
};
