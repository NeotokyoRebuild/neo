#ifndef NEO_BOT_RETREAT_FROM_APC_H
#define NEO_BOT_RETREAT_FROM_APC_H
#ifdef _WIN32
#pragma once
#endif

#include "neo_bot_behavior.h"

//------------------------------------------------------------------------------------------------------------------------------
class CNEOBotRetreatFromAPC : public Action< CNEOBot >
{
public:
	CNEOBotRetreatFromAPC( CBaseEntity *pAPC );

	virtual ActionResult< CNEOBot >	OnStart( CNEOBot *me, Action< CNEOBot > *priorAction );
	virtual ActionResult< CNEOBot >	Update( CNEOBot *me, float interval );

	virtual EventDesiredResult< CNEOBot > OnStuck( CNEOBot *me );
	virtual EventDesiredResult< CNEOBot > OnMoveToSuccess( CNEOBot *me, const Path *path );
	virtual EventDesiredResult< CNEOBot > OnMoveToFailure( CNEOBot *me, const Path *path, MoveToFailureType reason );

	virtual const char *GetName( void ) const	{ return "RetreatFromAPC"; }

	virtual QueryResultType ShouldHurry( const INextBot *me ) const;
	virtual QueryResultType ShouldWalk( const CNEOBot *me, const QueryResultType qShouldAimQuery ) const;
	virtual QueryResultType ShouldRetreat( const CNEOBot *me ) const;
	virtual QueryResultType ShouldAim( const CNEOBot *me, const bool bWepHasClip ) const;

private:
	CNavArea *FindCoverArea( CNEOBot *me );

	EHANDLE m_hAPC;
	CNavArea *m_coverArea;
	PathFollower m_path;
	CountdownTimer m_repathTimer;
};

#endif // NEO_BOT_RETREAT_FROM_APC_H
