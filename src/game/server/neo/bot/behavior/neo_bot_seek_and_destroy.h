#pragma once

#include "Path/NextBotChasePath.h"

class CSound;

//
// Roam around the map attacking enemies
//
class CNEOBotSeekAndDestroy : public Action< CNEOBot >
{
public:
	CNEOBotSeekAndDestroy( float duration = -1.0f );

	virtual ActionResult< CNEOBot >	OnStart( CNEOBot *me, Action< CNEOBot > *priorAction );
	virtual ActionResult< CNEOBot >	Update( CNEOBot *me, float interval );

	virtual ActionResult< CNEOBot >	OnResume( CNEOBot *me, Action< CNEOBot > *interruptingAction );

	virtual EventDesiredResult< CNEOBot > OnStuck( CNEOBot *me );
	virtual EventDesiredResult< CNEOBot > OnMoveToSuccess( CNEOBot *me, const Path *path );
	virtual EventDesiredResult< CNEOBot > OnMoveToFailure( CNEOBot *me, const Path *path, MoveToFailureType reason );

	virtual QueryResultType	ShouldRetreat( const INextBot *me ) const;					// is it time to retreat?
	virtual QueryResultType ShouldHurry( const INextBot *me ) const;					// are we in a hurry?

	virtual EventDesiredResult< CNEOBot > OnTerritoryCaptured( CNEOBot *me, int territoryID );
	virtual EventDesiredResult< CNEOBot > OnTerritoryLost( CNEOBot *me, int territoryID );
	virtual EventDesiredResult< CNEOBot > OnTerritoryContested( CNEOBot *me, int territoryID );

	virtual EventDesiredResult< CNEOBot > OnCommandApproach( CNEOBot *me, const Vector& pos, float range );

	virtual const char *GetName( void ) const	{ return "SeekAndDestroy"; };

protected:
	virtual ActionResult< CNEOBot > UpdateCommon( CNEOBot *me, float interval );

	virtual void RecomputeSeekPath( CNEOBot *me );

	static CSound* SearchGunfireSounds(CNEOBot* me, const Vector* currentGoalPos = nullptr);
	static const Vector& SearchGunfireLocation(CNEOBot* me, const Vector* currentGoalPos = nullptr);

	PathFollower m_path;
	CountdownTimer m_repathTimer;
	CountdownTimer m_soundSearchTimer;
	CountdownTimer m_itemStolenTimer;
	EHANDLE m_hTargetEntity;
	bool m_bGoingToTargetEntity = false;
	bool m_bInvestigateGunfire = true;
	Vector m_vGoalPos = vec3_origin;
	bool m_bTimerElapsed = false;
	bool m_bOverrideApproach = false;
	Vector m_vOverrideApproach = vec3_origin;

	CountdownTimer m_giveUpTimer;
};
