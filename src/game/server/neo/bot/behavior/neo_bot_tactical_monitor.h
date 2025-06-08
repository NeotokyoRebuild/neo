#pragma once

class CObjectTeleporter;

class CNEOBotTacticalMonitor : public Action< CNEOBot >
{
public:
	virtual Action< CNEOBot >* InitialContainedAction(CNEOBot* me);

	virtual ActionResult< CNEOBot >	OnStart(CNEOBot* me, Action< CNEOBot >* priorAction);
	virtual ActionResult< CNEOBot >	Update(CNEOBot* me, float interval);

	virtual EventDesiredResult< CNEOBot > OnNavAreaChanged(CNEOBot* me, CNavArea* newArea, CNavArea* oldArea);
	virtual EventDesiredResult< CNEOBot > OnOtherKilled(CNEOBot* me, CBaseCombatCharacter* victim, const CTakeDamageInfo& info);

	// @note Tom Bui: Currently used for the training stuff, but once we get that interface down, we will turn that
	// into a proper API
	virtual EventDesiredResult< CNEOBot > OnCommandString(CNEOBot* me, const char* command);

	virtual const char* GetName(void) const { return "TacticalMonitor"; }

private:
	CountdownTimer m_maintainTimer;

	CountdownTimer m_acknowledgeAttentionTimer;
	CountdownTimer m_acknowledgeRetryTimer;
	CountdownTimer m_attentionTimer;

#if 0
	CountdownTimer m_stickyBombCheckTimer;
	void MonitorArmedStickyBombs(CNEOBot* me);
#endif

	void AvoidBumpingEnemies(CNEOBot* me);
};