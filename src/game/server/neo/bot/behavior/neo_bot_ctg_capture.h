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
	CNavArea *m_previousKnownArea;
	CountdownTimer m_captureAttemptTimer;
	CountdownTimer m_repathTimer;
	CountdownTimer m_useTapTimer;
	CountdownTimer m_dislodgeTimer;
	bool m_bTriedDislodge = false;
	PathFollower m_path;

	static constexpr float CAPTURE_ATTEMPT_TIME = 3.0f;	// Per nav area, before the ghost counts as lodged
	static constexpr float USE_FACING_DOT = 0.9f;		// As JGR_CAPTURE_FACING_DOT
	static constexpr float BUTTON_TAP_HOLD = 0.1f;
	static constexpr float USE_TAP_INTERVAL = 0.3f;
	static constexpr float DISLODGE_TIME = 1.5f;		// How long to shoot a lodged ghost
};

#endif // NEO_BOT_CTG_CAPTURE_H
