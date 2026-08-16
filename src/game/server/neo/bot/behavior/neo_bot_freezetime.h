#pragma once

#include "bot/neo_bot.h"

class CNEOBotFreezeTime : public Action< CNEOBot >
{
public:
	virtual ActionResult< CNEOBot > OnStart( CNEOBot *me, Action< CNEOBot > *priorAction ) override;
	virtual ActionResult< CNEOBot > Update( CNEOBot *me, float interval ) override;

	virtual const char* GetName( void ) const override { return "FreezeTime"; }

private:
	bool m_bBuddyPairingDecisionMade = false;

	bool TryFindBuddy( CNEOBot *me );
	bool WantsToPairWithBuddyCandidate( const CNEO_Player *me, const CNEO_Player *buddy );
};
