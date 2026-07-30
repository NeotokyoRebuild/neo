#ifndef NEO_BOT_CAPTURE_JGR_H
#define NEO_BOT_CAPTURE_JGR_H

#include "NextBotBehavior.h"
#include "bot/neo_bot.h"

//----------------------------------------------------------------------------------------------------------------
class CNEOBotJgrCapture : public Action<CNEOBot>
{
public:
	CNEOBotJgrCapture( CNEO_Juggernaut *pObjective );
	virtual ~CNEOBotJgrCapture() { }

	virtual const char *GetName() const override { return "jgrCapture"; }

	virtual ActionResult<CNEOBot> OnStart( CNEOBot *me, Action<CNEOBot> *priorAction ) override;
	virtual ActionResult<CNEOBot> Update( CNEOBot *me, float interval ) override;
	virtual void OnEnd( CNEOBot *me, Action<CNEOBot> *nextAction ) override;
	virtual ActionResult<CNEOBot> OnSuspend( CNEOBot *me, Action<CNEOBot> *interruptingAction ) override;
	virtual ActionResult<CNEOBot> OnResume( CNEOBot *me, Action<CNEOBot> *interruptingAction ) override;

	static void LookAwayFrom( CNEOBot *me, CBaseEntity *pTarget );

private:
	void StopMoving( CNEOBot *me );

	bool m_bStrafeRight = false;
	CHandle<CNEO_Juggernaut> m_hObjective;
	CountdownTimer m_useAttemptTimer;
	PathFollower m_path;
	CountdownTimer m_repathTimer;
};

#endif // NEO_BOT_CAPTURE_JGR_H
