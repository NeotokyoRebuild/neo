#pragma once

#include "bot/behavior/neo_bot_seek_and_destroy.h"
#include "bot/neo_bot_ctg_defense.h"

//
// A zoneless team with a teammate carrying the ghost: hunt the enemies the team knows about,
// pursue the nearest known enemy, and roam like seek-and-destroy when none is known
//
class CNEOBotCtgHunt : public CNEOBotSeekAndDestroy
{
public:
	virtual ActionResult< CNEOBot >	OnStart( CNEOBot *me, Action< CNEOBot > *priorAction ) override;
	virtual ActionResult< CNEOBot >	Update( CNEOBot *me, float interval ) override;

	virtual EventDesiredResult< CNEOBot > OnMoveToSuccess( CNEOBot *me, const Path *path ) override;
	virtual EventDesiredResult< CNEOBot > OnInjured( CNEOBot *me, const CTakeDamageInfo &info ) override;
	virtual void OnEnd( CNEOBot *me, Action< CNEOBot > *nextAction ) override;
	virtual QueryResultType ShouldHurry( const INextBot *me ) const override;
	virtual QueryResultType IsHindrance( const INextBot *me, CBaseEntity *blocker ) const override;

	virtual const char *GetName( void ) const override { return "ctgHunt"; }

protected:
	virtual bool ShouldHoldFire( CNEOBot *me, const CKnownEntity *threat ) override;
	virtual void RecomputeSeekPath( CNEOBot *me ) override;

private:
	CNEO_Player *SelectHuntTarget( CNEOBot *me ) const;

	bool TryPlanWatch( CNEOBot *me, CNEO_Player *pTarget, const CKnownEntity *known, const WatchArea_t *pAssigned );
	void UpdateWatch( CNEOBot *me );
	void DropWatch( CNEOBot *me, const char *pszReason );

	CHandle< CNEO_Player > m_hHuntTarget;
	CountdownTimer m_retargetTimer;
	bool m_bRepathPending = false;

	// Route watch: wait beside the target's predicted route to our carrier
	WatchArea_t m_watch;
	bool m_bHasWatch = false;
	bool m_bHolding = false;
	CHandle< CNEO_Player > m_hHoldFireThreat;
	bool m_bHurtByUnseen = false;
	float m_flHoldFireLastSeen = 0.0f;
	float m_flHoldStart = 0.0f;
	float m_flLastReport = 0.0f;
	CountdownTimer m_watchStaggerTimer;
};
