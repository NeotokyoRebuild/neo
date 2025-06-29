#pragma once

class CNEOBotGetHealth : public Action< CNEOBot >
{
public:
	static bool IsPossible( CNEOBot *me );	// Return true if this Action has what it needs to perform right now

	virtual ActionResult< CNEOBot >	OnStart( CNEOBot *me, Action< CNEOBot > *priorAction );
	virtual ActionResult< CNEOBot >	Update( CNEOBot *me, float interval );

	virtual EventDesiredResult< CNEOBot > OnStuck( CNEOBot *me );
	virtual EventDesiredResult< CNEOBot > OnMoveToSuccess( CNEOBot *me, const Path *path );
	virtual EventDesiredResult< CNEOBot > OnMoveToFailure( CNEOBot *me, const Path *path, MoveToFailureType reason );

	virtual QueryResultType ShouldHurry( const INextBot *me ) const;					// are we in a hurry?

	virtual const char *GetName( void ) const	{ return "GetHealth"; };

private:
	PathFollower m_path;
	CHandle< CBaseEntity > m_healthKit;

	bool m_isGoalCharger = false;
};
