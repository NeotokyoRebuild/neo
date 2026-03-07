#pragma once

#include "bot/neo_bot.h"
#include "bot/behavior/neo_bot_ctg_lone_wolf.h"

class CWeaponDetpack;

//--------------------------------------------------------------------------------------------------------
class CNEOBotCtgLoneWolfAmbush : public CNEOBotCtgLoneWolf
{
public:
	CNEOBotCtgLoneWolfAmbush( void ) = default;

	virtual ActionResult< CNEOBot > OnStart( CNEOBot *me, Action< CNEOBot > *priorAction ) override;
	virtual ActionResult< CNEOBot > Update( CNEOBot *me, float interval ) override;
	virtual ActionResult<CNEOBot> OnSuspend( CNEOBot *me, Action<CNEOBot> *interruptingAction ) override;
	virtual ActionResult<CNEOBot> OnResume( CNEOBot *me, Action<CNEOBot> *interruptingAction ) override;
	virtual QueryResultType ShouldRetreat( const INextBot *me ) const override;

	virtual const char *GetName( void ) const override { return "ctgLoneWolfAmbush"; }

private:
	static bool CanBotHearPosition( CNEOBot *me, const Vector &pos );
	void EquipDetpackIfOwned( CNEOBot *me );

	bool m_bInvestigatingGunfire{false};

	CHandle<CWeaponDetpack> m_hDetpackWeapon{nullptr};

	CountdownTimer m_ambushExpirationTimer;

	Vector m_vecAmbushHidingSpot{vec3_invalid};
};
