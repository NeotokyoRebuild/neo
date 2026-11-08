#pragma once

#include "bot/behavior/neo_bot_seek_and_destroy.h"

//
// ATK defender with a teammate carrying the ghost: hunt the enemies the team knows about,
// pursue the nearest known enemy, and roam like seek-and-destroy when none is known
//
class CNEOBotCtgHunt : public CNEOBotSeekAndDestroy
{
public:
	virtual ActionResult< CNEOBot >	OnStart( CNEOBot *me, Action< CNEOBot > *priorAction ) override;
	virtual ActionResult< CNEOBot >	Update( CNEOBot *me, float interval ) override;

	virtual EventDesiredResult< CNEOBot > OnMoveToSuccess( CNEOBot *me, const Path *path ) override;

	virtual const char *GetName( void ) const override { return "ctgHunt"; }

protected:
	virtual void RecomputeSeekPath( CNEOBot *me ) override;

private:
	CNEO_Player *SelectHuntTarget( CNEOBot *me ) const;

	CHandle< CNEO_Player > m_hHuntTarget;
	CountdownTimer m_retargetTimer;
	bool m_bRepathPending = false;
};
