#ifndef NEO_BOT_JGR_ENEMY_H
#define NEO_BOT_JGR_ENEMY_H

#include "bot/neo_bot.h"
#include "Path/NextBotChasePath.h"

//--------------------------------------------------------------------------------------------------------
class CNEOBotJgrEnemy : public Action< CNEOBot >
{
public:
	CNEOBotJgrEnemy( void );

	virtual ActionResult< CNEOBot >	OnStart( CNEOBot *me, Action< CNEOBot > *priorAction );
	virtual ActionResult< CNEOBot >	Update( CNEOBot *me, float interval );
	virtual ActionResult< CNEOBot >	OnResume( CNEOBot *me, Action< CNEOBot > *interruptingAction );

	virtual EventDesiredResult< CNEOBot > OnStuck( CNEOBot *me );
	virtual EventDesiredResult< CNEOBot > OnMoveToSuccess( CNEOBot *me, const Path *path );
	virtual EventDesiredResult< CNEOBot > OnMoveToFailure( CNEOBot *me, const Path *path, MoveToFailureType reason );

	virtual const char *GetName( void ) const	{ return "JgrEnemy"; }

private:
	ChasePath m_chasePath;
};

#endif // NEO_BOT_JGR_ENEMY_H
