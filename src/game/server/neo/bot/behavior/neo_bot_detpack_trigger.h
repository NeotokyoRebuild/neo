#pragma once

#include "bot/neo_bot.h"

class CWeaponDetpack;

//-----------------------------------------------------------------------------
class CNEOBotDetpackTrigger : public Action< CNEOBot >
{
public:
	CNEOBotDetpackTrigger( void );

	virtual ActionResult< CNEOBot > OnStart( CNEOBot *me, Action< CNEOBot > *priorAction ) override;
	virtual ActionResult< CNEOBot > Update( CNEOBot *me, float interval ) override;
	virtual void OnEnd( CNEOBot *me, Action< CNEOBot > *nextAction ) override;

	virtual ActionResult<CNEOBot> OnSuspend( CNEOBot *me, Action<CNEOBot> *interruptingAction ) override;
	virtual ActionResult<CNEOBot> OnResume( CNEOBot *me, Action<CNEOBot> *interruptingAction ) override;

	virtual const char *GetName( void ) const override { return "DetpackTrigger"; }

private:
	bool m_bPushedWeapon;
	CHandle< CWeaponDetpack > m_hDetpackWeapon;
	CountdownTimer m_expiryTimer;
};
