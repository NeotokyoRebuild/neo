#pragma once

#include "cbase.h"
#include "bot/neo_bot.h"
#include "bot/neo_bot_contextual_query_interface.h"

class CNEOBotRetreatFromGrenade : public Action< CNEOBot >, public CNEOBotContextualQueryInterface
{
public:
	CNEOBotRetreatFromGrenade( CBaseEntity *grenade = nullptr );

	static CBaseEntity *FindDangerousGrenade( CNEOBot *me );
	static Vector PredictGrenadeRest( CBaseEntity *grenade );
	static float GetGrenadeCoverDistance();

	virtual ActionResult< CNEOBot >	OnStart( CNEOBot *me, Action< CNEOBot > *priorAction );
	virtual ActionResult< CNEOBot >	Update( CNEOBot *me, float interval );

	virtual EventDesiredResult< CNEOBot > OnStuck( CNEOBot *me );
	virtual EventDesiredResult< CNEOBot > OnMoveToSuccess( CNEOBot *me, const Path *path );
	virtual EventDesiredResult< CNEOBot > OnMoveToFailure( CNEOBot *me, const Path *path, MoveToFailureType reason );

	virtual QueryResultType ShouldHurry( const INextBot *me ) const;					// are we in a hurry?
	QueryResultType ShouldWalk( const CNEOBot *me, const QueryResultType qShouldAimQuery ) const override;
	virtual QueryResultType ShouldRetreat( const CNEOBot *me ) const;
	QueryResultType ShouldAim( const CNEOBot *me, const bool bWepHasClip ) const override;

	virtual const char *GetName( void ) const	{ return "RetreatFromGrenade"; };

private:
	CHandle< CBaseEntity > m_grenade;

	PathFollower m_path;

	CountdownTimer m_expiryTimer;
	CountdownTimer m_repathTimer;

	CNavArea *m_coverArea;

	CNavArea *FindCoverArea( CNEOBot *me );
	bool IsTradingFire( const CNEOBot *me ) const;
};
