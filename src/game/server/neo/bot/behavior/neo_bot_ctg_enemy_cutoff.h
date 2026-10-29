#pragma once

#include "bot/neo_bot.h"

class CNEO_Player;
class CNavArea;

//--------------------------------------------------------------------------------------------------------
// Runs to the earliest area on the enemy ghost carrier's predicted route that this bot reaches first,
// then chases from there. Re-picks the area on a timer, because the carrier's route is only a guess.
class CNEOBotCtgEnemyCutOff : public Action< CNEOBot >
{
public:
	CNEOBotCtgEnemyCutOff( CNavArea *pCutOff ) : m_pCutOff( pCutOff ) {}

	virtual ActionResult< CNEOBot >	OnStart( CNEOBot *me, Action< CNEOBot > *priorAction ) override;
	virtual ActionResult< CNEOBot >	Update( CNEOBot *me, float interval ) override;

	virtual EventDesiredResult< CNEOBot > OnStuck( CNEOBot *me ) override;
	virtual EventDesiredResult< CNEOBot > OnMoveToFailure( CNEOBot *me, const Path *path, MoveToFailureType reason ) override;

	virtual QueryResultType ShouldHurry( const INextBot *me ) const override;

	virtual const char *GetName( void ) const override { return "ctgEnemyCutOff"; }

private:
	bool Replan( CNEOBot *me, CNEO_Player *pGhostCarrier );
	bool RepathToCutOff( CNEOBot *me );

	CNavArea *m_pCutOff;
	PathFollower m_path;
	CountdownTimer m_replanTimer;			// throttles re-picking the cut-off
};
