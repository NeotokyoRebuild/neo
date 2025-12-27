#ifndef NEO_BOT_CAPTURE_JGR_H
#define NEO_BOT_CAPTURE_JGR_H

#include "NextBotBehavior.h"
#include "bot/neo_bot.h"

//----------------------------------------------------------------------------------------------------------------
class CNEOBotJgrCapture : public Action<CNEOBot>
{
public:
	CNEOBotJgrCapture( CBaseEntity *pObjective );
	virtual ~CNEOBotJgrCapture() { }

	virtual const char *GetName() const override { return "jgrCapture"; }

	virtual ActionResult<CNEOBot> OnStart( CNEOBot *me, Action<CNEOBot> *priorAction ) override;
	virtual ActionResult<CNEOBot> Update( CNEOBot *me, float interval ) override;
	virtual void OnEnd( CNEOBot *me, Action<CNEOBot> *nextAction ) override;
	virtual ActionResult<CNEOBot> OnSuspend( CNEOBot *me, Action<CNEOBot> *interruptingAction ) override;
	virtual ActionResult<CNEOBot> OnResume( CNEOBot *me, Action<CNEOBot> *interruptingAction ) override;

private:
	CHandle<CBaseEntity> m_hObjective;
	CountdownTimer m_useAttemptTimer;
	PathFollower m_path;
	CountdownTimer m_repathTimer;
};

#endif // NEO_BOT_CAPTURE_JGR_H
