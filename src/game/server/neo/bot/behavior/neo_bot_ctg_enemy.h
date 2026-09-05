#ifndef NEO_BOT_CTG_ENEMY_H
#define NEO_BOT_CTG_ENEMY_H

#include "bot/neo_bot.h"
#include "Path/NextBotChasePath.h"

class CNEO_Player;

//--------------------------------------------------------------------------------------------------------
// "An enemy has the ghost" behaviour. Owns the tactical choice between chasing the carrier directly
// (the ChasePath below) and peeling off into CNEOBotCtgEnemyInterceptCapPath to hold a cut-off on the
// carrier's predicted path -- CNEOBotCtgSeek just hands off to here, it does not decide.
class CNEOBotCtgEnemy : public Action< CNEOBot >
{
public:
	CNEOBotCtgEnemy( void );

	virtual ActionResult< CNEOBot >	OnStart( CNEOBot *me, Action< CNEOBot > *priorAction ) override;
	virtual ActionResult< CNEOBot >	Update( CNEOBot *me, float interval ) override;
	virtual ActionResult< CNEOBot >	OnResume( CNEOBot *me, Action< CNEOBot > *interruptingAction ) override;

	virtual EventDesiredResult< CNEOBot > OnStuck( CNEOBot *me ) override;
	virtual EventDesiredResult< CNEOBot > OnMoveToSuccess( CNEOBot *me, const Path *path ) override;
	virtual EventDesiredResult< CNEOBot > OnMoveToFailure( CNEOBot *me, const Path *path, MoveToFailureType reason ) override;

	virtual const char *GetName( void ) const override { return "ctgEnemy"; }

private:
	// true => this bot should peel off to hold a cut-off (interception) rather than chase directly.
	// Observable state only: this bot's distance rank among living friendly defenders to the
	// carrier's last-known area, versus sv_neo_bot_ctg_pressure_slots.
	bool ShouldInterceptInsteadOfChase( CNEOBot *me, CNEO_Player *pGhostCarrier ) const;

	ChasePath m_chasePath;
	CountdownTimer m_roleReviewTimer;	// throttle the chase-vs-intercept re-decision
	CountdownTimer m_forcePursueTimer;	// after returning from interception, chase directly for a bit before peeling again
};

#endif // NEO_BOT_CTG_ENEMY_H
