#ifndef NEO_BOT_CTG_GUARD_CARRIER_H
#define NEO_BOT_CTG_GUARD_CARRIER_H

#include "bot/neo_bot.h"

//--------------------------------------------------------------------------------------------------------
// ATK defender with a teammate carrying the ghost: hold a spot around the carrier, quietly,
// and watch where enemies were last reported
class CNEOBotCtgGuardCarrier : public Action< CNEOBot >
{
public:
	CNEOBotCtgGuardCarrier( void );

	virtual ActionResult< CNEOBot >	OnStart( CNEOBot *me, Action< CNEOBot > *priorAction ) override;
	virtual ActionResult< CNEOBot >	Update( CNEOBot *me, float interval ) override;
	virtual void OnEnd( CNEOBot *me, Action< CNEOBot > *nextAction ) override;
	virtual ActionResult< CNEOBot >	OnResume( CNEOBot *me, Action< CNEOBot > *interruptingAction ) override;

	virtual EventDesiredResult< CNEOBot > OnStuck( CNEOBot *me ) override;
	virtual EventDesiredResult< CNEOBot > OnMoveToFailure( CNEOBot *me, const Path *path, MoveToFailureType reason ) override;

	virtual const char *GetName( void ) const override { return "ctgGuardCarrier"; }

private:
	void SelectSpot( CNEOBot *me, CNEO_Player *pCarrier );
	void LookForEnemies( CNEOBot *me, CNEO_Player *pCarrier );
	CNEO_Player *GetCarrier( CNEOBot *me ) const;

	PathFollower m_path;
	CNavArea *m_spot;
	Vector m_vecSpotCarrierPos;
	CountdownTimer m_reselectTimer;
	CountdownTimer m_claimTimer;
	bool m_bArrived;
};

#endif // NEO_BOT_CTG_GUARD_CARRIER_H
