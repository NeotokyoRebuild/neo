#ifndef NEO_BOT_CTG_ENEMY_H
#define NEO_BOT_CTG_ENEMY_H

#include "bot/neo_bot.h"

//--------------------------------------------------------------------------------------------------------
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
	PathFollower m_path;
	CountdownTimer m_repathTimer;
};

#endif // NEO_BOT_CTG_ENEMY_H
