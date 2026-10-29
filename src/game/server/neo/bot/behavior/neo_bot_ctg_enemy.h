#ifndef NEO_BOT_CTG_ENEMY_H
#define NEO_BOT_CTG_ENEMY_H

#include "bot/neo_bot.h"

class CNEO_Player;
class CNavArea;

//--------------------------------------------------------------------------------------------------------
// Decides whether to cut the enemy ghost carrier off or chase it, hands over, and never stays on the
// stack. Plans only from what any opponent sees: the ghost marker and the fixed cap zones.
class CNEOBotCtgEnemy : public Action< CNEOBot >
{
public:
	virtual ActionResult< CNEOBot >	Update( CNEOBot *me, float interval ) override;

	virtual const char *GetName( void ) const override { return "ctgEnemy"; }

	// The living enemy player carrying the ghost, or nullptr if there is none
	static CNEO_Player *EnemyGhostCarrier( const CNEOBot *me );

	// The earliest area on the carrier's predicted route that this bot reaches first; nullptr when
	// there is none, which means the carrier is ahead of this bot
	static CNavArea *FindCutOff( CNEOBot *me, CNEO_Player *pGhostCarrier );

	// ShouldHurry for the behaviors this one hands over to
	static QueryResultType CarrierUrgency( const CNEOBot *me );
};

#endif // NEO_BOT_CTG_ENEMY_H
