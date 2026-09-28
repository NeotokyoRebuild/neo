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
	void TryUseGhost( CNEOBot *me, const Vector &vecEye, const Vector &vecGhostCenter );
	void UpdateDislodge( CNEOBot *me, const Vector &vecEye, const Vector &vecGhostCenter, bool bThreatInView );

	CHandle<CWeaponGhost> m_hObjective;
	CNavArea *m_previousKnownArea;
	CountdownTimer m_captureAttemptTimer;
	CountdownTimer m_repathTimer;
	CountdownTimer m_useTapTimer;
	CountdownTimer m_useJumpTimer;
	CountdownTimer m_dislodgeTimer;
	bool m_bTriedDislodge;
	PathFollower m_path;
};

#endif // NEO_BOT_CTG_CAPTURE_H
