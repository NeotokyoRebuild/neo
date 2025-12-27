#ifndef NEO_BOT_JGR_ESCORT_H
#define NEO_BOT_JGR_ESCORT_H

#include "bot/neo_bot.h"
#include "Path/NextBotChasePath.h"

//--------------------------------------------------------------------------------------------------------
class CNEOBotJgrEscort : public Action< CNEOBot >
{
public:
	CNEOBotJgrEscort( void );

	virtual ActionResult< CNEOBot >	OnStart( CNEOBot *me, Action< CNEOBot > *priorAction );
	virtual ActionResult< CNEOBot >	Update( CNEOBot *me, float interval );
	virtual ActionResult< CNEOBot >	OnResume( CNEOBot *me, Action< CNEOBot > *interruptingAction );

	virtual EventDesiredResult< CNEOBot > OnStuck( CNEOBot *me );
	virtual EventDesiredResult< CNEOBot > OnMoveToSuccess( CNEOBot *me, const Path *path );
	virtual EventDesiredResult< CNEOBot > OnMoveToFailure( CNEOBot *me, const Path *path, MoveToFailureType reason );

	virtual const char *GetName( void ) const	{ return "jgrEscort"; }

private:
	ChasePath m_chasePath;
};

#endif // NEO_BOT_JGR_ESCORT_H
