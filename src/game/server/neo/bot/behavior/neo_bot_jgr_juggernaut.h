#ifndef NEO_BOT_JGR_JUGGERNAUT_H
#define NEO_BOT_JGR_JUGGERNAUT_H
#pragma once

#include "bot/behavior/neo_bot_seek_and_destroy.h"

class CNEOBotJgrJuggernaut : public CNEOBotSeekAndDestroy
{
public:
	CNEOBotJgrJuggernaut( float duration = -1.0f ) : CNEOBotSeekAndDestroy( duration ), m_bStuckCrouchPhase( false ) {}

	virtual ActionResult< CNEOBot >	OnStart( CNEOBot *me, Action< CNEOBot > *priorAction );
	virtual ActionResult< CNEOBot >	Update( CNEOBot *me, float interval );
	virtual const char *GetName( void ) const { return "jgrJuggernaut"; }

	virtual EventDesiredResult< CNEOBot > OnStuck( CNEOBot *me );

	virtual QueryResultType	ShouldWalk( const CNEOBot *me, const QueryResultType qShouldAimQuery ) const;

protected:
	virtual void RecomputeSeekPath( CNEOBot *me );

	CountdownTimer m_stuckTimer;
	bool m_bStuckCrouchPhase;

private:
	CUtlVector<CHandle<CBaseEntity>> m_jgrSpawns;
	CountdownTimer m_soundSearchTimer;
};
#endif // NEO_BOT_JGR_JUGGERNAUT_H
