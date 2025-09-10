#pragma once

#include "Path/NextBotChasePath.h"

class CNEOBotMoveToVantagePoint : public Action< CNEOBot >
{
public:
	CNEOBotMoveToVantagePoint( float maxTravelDistance = 2000.0f );
	virtual ~CNEOBotMoveToVantagePoint() { }

	virtual ActionResult< CNEOBot >	OnStart( CNEOBot *me, Action< CNEOBot > *priorAction );
	virtual ActionResult< CNEOBot >	Update( CNEOBot *me, float interval );

	virtual EventDesiredResult< CNEOBot > OnStuck( CNEOBot *me );
	virtual EventDesiredResult< CNEOBot > OnMoveToSuccess( CNEOBot *me, const Path *path );
	virtual EventDesiredResult< CNEOBot > OnMoveToFailure( CNEOBot *me, const Path *path, MoveToFailureType reason );

	virtual const char *GetName( void ) const	{ return "MoveToVantagePoint"; };

private:
	float m_maxTravelDistance;
	PathFollower m_path;
	CountdownTimer m_repathTimer;
	CNavArea *m_vantageArea;
};
