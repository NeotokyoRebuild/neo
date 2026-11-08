#pragma once

#include "NextBotBehavior.h"
#include "bot/neo_bot.h"
#include "bot/neo_bot_ctg_carry_block.h"

extern ConVar neo_bot_transition_log;

// TEMP: lifecycle telemetry, kept outside NextBot so it can be removed independently.
// Target is the last existing primary-threat selection, not a fresh arbitration.
// Block is the role verdict, including zoneless fallback; unknown external reasons are empty.
class CNEOBotTransitionAction : public Action< CNEOBot >
{
public:
	ActionResult< CNEOBot > OnStart( CNEOBot *me, Action< CNEOBot > *priorAction ) override
	{
		LogTransition( me, "start", priorAction );
		return Continue();
	}

	void OnEnd( CNEOBot *me, Action< CNEOBot > *nextAction ) override
	{
		LogTransition( me, "end", nextAction );
	}

	ActionResult< CNEOBot > OnSuspend( CNEOBot *me, Action< CNEOBot > *interruptingAction ) override
	{
		LogTransition( me, "suspend", interruptingAction );
		return Continue();
	}

	ActionResult< CNEOBot > OnResume( CNEOBot *me, Action< CNEOBot > *interruptingAction ) override
	{
		LogTransition( me, "resume", interruptingAction );
		return Continue();
	}

protected:
	void LogTransition( CNEOBot *me, const char *event, Action< CNEOBot > *nextAction = nullptr )
	{
		const char *reason = "";
		if ( event[0] == 'e' || ( event[0] == 's' && event[1] == 'u' ) )
		{
			if ( m_transitionType == DONE || m_transitionAction == nextAction )
			{
				reason = m_transitionReason ? m_transitionReason : "";
			}
		}

		if ( event[0] == 'e' || ( event[0] == 's' && event[1] == 'u' ) )
		{
			m_lastTransitionReason = reason;
		}
		else
		{
			const CNEOBotTransitionAction *prior = dynamic_cast< const CNEOBotTransitionAction * >( nextAction );
			if ( prior )
			{
				reason = prior->m_lastTransitionReason;
			}
		}

		// SeekAndDestroy also serves other modes; only its named CTG derivatives are in scope.
		if ( neo_bot_transition_log.GetBool() && Q_stricmp( GetName(), "SeekAndDestroy" ) != 0 &&
			( Q_strnicmp( GetName(), "ctg", 3 ) == 0 || Q_stricmp( GetName(), "Attack" ) == 0 ) )
		{
			const CBaseEntity *target = me->GetVisionInterface()->GetTransitionTarget();

			Msg( "NEO_TRANS bot=%d t=%.3f ev=%s action=%s reason=\"%s\" carrier=%d block=%d target=%d\n",
				me->entindex(), gpGlobals->curtime, event, GetName(), reason,
				NEORules()->GetGhosterPlayer(), NEOBotCarryBlock::IsTeamHoldingGhost( me->GetTeamNumber() ) ? 1 : 0,
				target ? target->entindex() : -1 );
		}

		m_transitionReason = nullptr;
		m_transitionAction = nullptr;
		m_transitionType = CONTINUE;
		m_transitionPriority = RESULT_NONE;
	}

	ActionResult< CNEOBot > Continue() const
	{
		if ( m_transitionPriority == RESULT_NONE )
		{
			m_transitionReason = nullptr;
			m_transitionAction = nullptr;
			m_transitionType = CONTINUE;
		}
		return Action< CNEOBot >::Continue();
	}

	ActionResult< CNEOBot > Done( const char *reason = nullptr ) const
	{
		RememberTransition( DONE, nullptr, reason );
		return Action< CNEOBot >::Done( reason );
	}

	ActionResult< CNEOBot > ChangeTo( Action< CNEOBot > *action, const char *reason = nullptr ) const
	{
		RememberTransition( CHANGE_TO, action, reason );
		return Action< CNEOBot >::ChangeTo( action, reason );
	}

	ActionResult< CNEOBot > SuspendFor( Action< CNEOBot > *action, const char *reason = nullptr ) const
	{
		RememberTransition( SUSPEND_FOR, action, reason );
		return Action< CNEOBot >::SuspendFor( action, reason );
	}

	EventDesiredResult< CNEOBot > TryDone( EventResultPriorityType priority = RESULT_TRY, const char *reason = nullptr ) const
	{
		RememberEvent( DONE, nullptr, priority, reason );
		return Action< CNEOBot >::TryDone( priority, reason );
	}

	EventDesiredResult< CNEOBot > TryChangeTo( Action< CNEOBot > *action, EventResultPriorityType priority = RESULT_TRY, const char *reason = nullptr ) const
	{
		RememberEvent( CHANGE_TO, action, priority, reason );
		return Action< CNEOBot >::TryChangeTo( action, priority, reason );
	}

	EventDesiredResult< CNEOBot > TrySuspendFor( Action< CNEOBot > *action, EventResultPriorityType priority = RESULT_TRY, const char *reason = nullptr ) const
	{
		RememberEvent( SUSPEND_FOR, action, priority, reason );
		return Action< CNEOBot >::TrySuspendFor( action, priority, reason );
	}

private:
	void RememberTransition( ActionResultType type, Action< CNEOBot > *action, const char *reason ) const
	{
		m_transitionType = type;
		m_transitionAction = action;
		m_transitionReason = reason;
	}

	void RememberEvent( ActionResultType type, Action< CNEOBot > *action, EventResultPriorityType priority, const char *reason ) const
	{
		if ( priority >= m_transitionPriority )
		{
			RememberTransition( type, action, reason );
			m_transitionPriority = priority;
		}
	}

	const char *m_lastTransitionReason = "";
	mutable const char *m_transitionReason = nullptr;
	mutable Action< CNEOBot > *m_transitionAction = nullptr;
	mutable ActionResultType m_transitionType = CONTINUE;
	mutable EventResultPriorityType m_transitionPriority = RESULT_NONE;
};
