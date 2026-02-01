#ifndef NEO_BOT_CTG_CARRIER_H
#define NEO_BOT_CTG_CARRIER_H

#include "bot/neo_bot.h"
#include "Path/NextBotChasePath.h"

class CNEO_Player;

//--------------------------------------------------------------------------------------------------------
// Common ghost equipment usage routines. Also used by CNEOBotCommandFollow.
class CNEOBotGhostEquipmentHandler
{
public:
	CNEOBotGhostEquipmentHandler();
	void Update( CNEOBot *me );

private:
	void EquipBestWeaponForGhoster( CNEOBot *me );
	float GetUpdateInterval( CNEOBot *me ) const;
	void UpdateGhostCarrierCallout( CNEOBot *me, const CUtlVector<CNEO_Player*> &enemies );

	EHANDLE m_hCurrentFocusEnemy{nullptr};
	CountdownTimer m_enemyUpdateTimer;
	Vector m_enemyLastPos[MAX_PLAYERS_ARRAY_SAFE];
	CUtlVector<CNEO_Player*> m_enemies;
};

//--------------------------------------------------------------------------------------------------------
class CNEOBotCtgCarrier : public Action< CNEOBot >
{
public:
	CNEOBotCtgCarrier( void );

	virtual ActionResult< CNEOBot > OnStart( CNEOBot *me, Action< CNEOBot > *priorAction ) override;
	virtual ActionResult< CNEOBot > Update( CNEOBot *me, float interval ) override;
	virtual ActionResult< CNEOBot > OnResume( CNEOBot *me, Action< CNEOBot > *interruptingAction ) override;

	virtual EventDesiredResult< CNEOBot > OnStuck( CNEOBot *me ) override;
	virtual EventDesiredResult< CNEOBot > OnMoveToSuccess( CNEOBot *me, const Path *path ) override;
	virtual EventDesiredResult< CNEOBot > OnMoveToFailure( CNEOBot *me, const Path *path, MoveToFailureType reason ) override;

	virtual const char *GetName( void ) const override { return "ctgGhostCarrier"; }

private:
	PathFollower m_path;
	ChasePath m_chasePath;
	CountdownTimer m_aloneTimer;
	CountdownTimer m_repathTimer;
	
	CNEOBotGhostEquipmentHandler m_ghostEquipmentHandler;
	
	Vector m_closestCapturePoint;
	CUtlVector<CNEO_Player*> m_teammates;

	Vector GetNearestCapPoint( const CNEOBot *me ) const;
	void UpdateFollowPath( CNEOBot *me, const CUtlVector<CNEO_Player*> &teammates );
};

#endif // NEO_BOT_CTG_CARRIER_H