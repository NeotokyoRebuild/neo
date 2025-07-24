#pragma once

class CNEOBotScenarioMonitor : public Action< CNEOBot >
{
public:
	virtual Action< CNEOBot > *InitialContainedAction( CNEOBot *me );

	virtual ActionResult< CNEOBot >	OnStart( CNEOBot *me, Action< CNEOBot > *priorAction );
	virtual ActionResult< CNEOBot >	Update( CNEOBot *me, float interval );

	virtual const char *GetName( void ) const	{ return "ScenarioMonitor"; }

private:
	CountdownTimer m_ignoreLostFlagTimer;
	CountdownTimer m_lostFlagTimer;

	virtual Action< CNEOBot > *DesiredScenarioAndClassAction( CNEOBot *me );
};
