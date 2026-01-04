#ifndef NEO_BOT_JGR_ESCORT_H
#define NEO_BOT_JGR_ESCORT_H

#include "bot/neo_bot.h"
#include "Path/NextBotChasePath.h"

//--------------------------------------------------------------------------------------------------------
class CNEOBotJgrEscort : public Action< CNEOBot >
{
public:
	CNEOBotJgrEscort( void );

	virtual ActionResult< CNEOBot >	OnStart( CNEOBot *me, Action< CNEOBot > *priorAction ) override;
	virtual ActionResult< CNEOBot >	Update( CNEOBot *me, float interval ) override;
	virtual ActionResult< CNEOBot >	OnResume( CNEOBot *me, Action< CNEOBot > *interruptingAction ) override;

	virtual EventDesiredResult< CNEOBot > OnStuck( CNEOBot *me ) override;
	virtual EventDesiredResult< CNEOBot > OnMoveToSuccess( CNEOBot *me, const Path *path ) override;
	virtual EventDesiredResult< CNEOBot > OnMoveToFailure( CNEOBot *me, const Path *path, MoveToFailureType reason ) override;

	virtual const char *GetName( void ) const override	{ return "jgrEscort"; }

private:
	ChasePath m_chasePath;
};

#endif // NEO_BOT_JGR_ESCORT_H
