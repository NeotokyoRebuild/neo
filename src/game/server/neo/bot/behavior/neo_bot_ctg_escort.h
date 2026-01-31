#ifndef NEO_BOT_CTG_ESCORT_H
#define NEO_BOT_CTG_ESCORT_H

#include "bot/neo_bot.h"
#include "Path/NextBotChasePath.h"

//--------------------------------------------------------------------------------------------------------
class CNEOBotCtgEscort : public Action< CNEOBot >
{
public:
	CNEOBotCtgEscort( void );

	virtual ActionResult< CNEOBot >	OnStart( CNEOBot *me, Action< CNEOBot > *priorAction ) override;
	virtual ActionResult< CNEOBot >	Update( CNEOBot *me, float interval ) override;
	virtual ActionResult< CNEOBot >	OnResume( CNEOBot *me, Action< CNEOBot > *interruptingAction ) override;

	virtual EventDesiredResult< CNEOBot > OnStuck( CNEOBot *me ) override;
	virtual EventDesiredResult< CNEOBot > OnMoveToSuccess( CNEOBot *me, const Path *path ) override;
	virtual EventDesiredResult< CNEOBot > OnMoveToFailure( CNEOBot *me, const Path *path, MoveToFailureType reason ) override;

	virtual const char *GetName( void ) const override { return "ctgEscort"; }

private:
	PathFollower m_path;
	ChasePath m_chasePath;
	CountdownTimer m_repathTimer;
	CountdownTimer m_lostSightOfCarrierTimer;

	enum EscortRole
	{
		ROLE_SCREEN,
		ROLE_SCOUT,
		ROLE_BODYGUARD,
	};
	EscortRole m_role;
	
	EscortRole UpdateRoleAssignment( CNEOBot *me, CNEO_Player *pGhostCarrier, const Vector &vecGoalPos );
	void UpdateGoalPosition( CNEOBot *me, CNEO_Player *pGhostCarrier );

	Vector m_vecGoalPos;
	bool m_bHasGoal;
};

#endif // NEO_BOT_CTG_ESCORT_H
