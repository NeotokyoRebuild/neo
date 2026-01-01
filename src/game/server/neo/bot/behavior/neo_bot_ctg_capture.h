#ifndef NEO_BOT_CTG_CAPTURE_H
#define NEO_BOT_CTG_CAPTURE_H

#include "NextBotBehavior.h"
#include "bot/neo_bot.h"

//----------------------------------------------------------------------------------------------------------------
class CNEOBotCtgCapture : public Action<CNEOBot>
{
public:
	CNEOBotCtgCapture( CWeaponGhost *pObjective );
	virtual ~CNEOBotCtgCapture() { }

	virtual const char *GetName() const override { return "ctgCapture"; }

	virtual ActionResult<CNEOBot> OnStart( CNEOBot *me, Action<CNEOBot> *priorAction ) override;
	virtual ActionResult<CNEOBot> Update( CNEOBot *me, float interval ) override;

private:
	CHandle<CWeaponGhost> m_hObjective;
	CountdownTimer m_captureAttemptTimer;
	CountdownTimer m_repathTimer;
	PathFollower m_path;
};

#endif // NEO_BOT_CTG_CAPTURE_H
