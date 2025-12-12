#pragma once

#include "cbase.h"
#include "bot/neo_bot.h"

class CNEOBotRetreatFromGrenade : public Action< CNEOBot >
{
public:
	CNEOBotRetreatFromGrenade( CBaseEntity *grenade, float hideDuration = -1.0f );
	CNEOBotRetreatFromGrenade( CBaseEntity *grenade, Action< CNEOBot > *actionToChangeToOnceCoverReached );

	virtual ActionResult< CNEOBot >	OnStart( CNEOBot *me, Action< CNEOBot > *priorAction );
	virtual ActionResult< CNEOBot >	Update( CNEOBot *me, float interval );

	virtual EventDesiredResult< CNEOBot > OnStuck( CNEOBot *me );
	virtual EventDesiredResult< CNEOBot > OnMoveToSuccess( CNEOBot *me, const Path *path );
	virtual EventDesiredResult< CNEOBot > OnMoveToFailure( CNEOBot *me, const Path *path, MoveToFailureType reason );

	virtual QueryResultType ShouldHurry( const INextBot *me ) const;					// are we in a hurry?

	virtual const char *GetName( void ) const	{ return "RetreatFromGrenade"; };

private:
	CHandle< CBaseEntity > m_grenade;
	float m_hideDuration;
	Action< CNEOBot > *m_actionToChangeToOnceCoverReached;

	PathFollower m_path;
	CountdownTimer m_repathTimer;

	CNavArea *m_coverArea;
	CountdownTimer m_waitInCoverTimer;

	CNavArea *FindCoverArea( CNEOBot *me );
};
