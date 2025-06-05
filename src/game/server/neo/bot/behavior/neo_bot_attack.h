#pragma once

#include "Path/NextBotChasePath.h"


//-------------------------------------------------------------------------------
class CNEOBotAttack : public Action< CNEOBot >
{
public:
	CNEOBotAttack( void );
	virtual ~CNEOBotAttack() { }

	virtual ActionResult< CNEOBot >	OnStart( CNEOBot *me, Action< CNEOBot > *priorAction );
	virtual ActionResult< CNEOBot >	Update( CNEOBot *me, float interval );

	virtual EventDesiredResult< CNEOBot > OnStuck( CNEOBot *me );
	virtual EventDesiredResult< CNEOBot > OnMoveToSuccess( CNEOBot *me, const Path *path );
	virtual EventDesiredResult< CNEOBot > OnMoveToFailure( CNEOBot *me, const Path *path, MoveToFailureType reason );

	virtual QueryResultType	ShouldRetreat( const INextBot *me ) const;							// is it time to retreat?
	virtual QueryResultType ShouldHurry( const INextBot *me ) const;					// are we in a hurry?

	virtual const char *GetName( void ) const	{ return "Attack"; };

private:
	PathFollower m_path;
	ChasePath m_chasePath;
	CountdownTimer m_repathTimer;
};
