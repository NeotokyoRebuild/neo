#include "cbase.h"
#include "neo_gamerules.h"
#include "bot/neo_bot.h"
//#include "item_healthkit.h"
#include "bot/behavior/neo_bot_get_health.h"

extern ConVar neo_bot_path_lookahead_range;
extern ConVar neo_bot_health_critical_ratio;
extern ConVar neo_bot_health_ok_ratio;

void neo_bot_health_ratio_change_callback(IConVar* var, const char* pOldValue [[maybe_unused]], float flOldValue)
{
	if (neo_bot_health_ok_ratio.GetFloat() != neo_bot_health_critical_ratio.GetFloat())
	{
		return;
	}

	float newValue = flOldValue;
	if (flOldValue == neo_bot_health_ok_ratio.GetFloat())
	{ // just in case so we don't loop infinitely if old value is also a match
		newValue = flOldValue + 0.01;
	}

	if (!Q_strcmp(var->GetName(), neo_bot_health_ok_ratio.GetName()))
	{
		neo_bot_health_ok_ratio.SetValue(newValue);
	}
	else if (!Q_strcmp(var->GetName(), neo_bot_health_critical_ratio.GetName()))
	{
		neo_bot_health_critical_ratio.SetValue(newValue);
	}
}

ConVar neo_bot_health_critical_ratio( "neo_bot_health_critical_ratio", "0.3", FCVAR_CHEAT, "", neo_bot_health_ratio_change_callback);
ConVar neo_bot_health_ok_ratio( "neo_bot_health_ok_ratio", "0.65", FCVAR_CHEAT, "", neo_bot_health_ratio_change_callback);

ConVar neo_bot_health_search_near_range( "neo_bot_health_search_near_range", "1000", FCVAR_CHEAT );
ConVar neo_bot_health_search_far_range( "neo_bot_health_search_far_range", "2000", FCVAR_CHEAT );

ConVar neo_bot_debug_health_scavenging( "neo_bot_debug_ammo_scavenging", "0", FCVAR_CHEAT );

//---------------------------------------------------------------------------------------------
class CHealthFilter : public INextBotFilter
{
public:
	CHealthFilter( CNEOBot *me )
	{
		m_me = me;
	}

	bool IsSelected( const CBaseEntity *constCandidate ) const
	{
		if ( !constCandidate )
			return false;

		CBaseEntity *candidate = const_cast< CBaseEntity * >( constCandidate );

		CClosestNEOPlayer close( candidate );
		ForEachPlayer( close );

		// if the closest player to this candidate object is an enemy, don't use it
		if ( close.m_closePlayer && m_me->IsEnemy( close.m_closePlayer ) )
			return false;

		// ignore non-existent ammo to ensure we collect nearby existing ammo
		if ( candidate->IsEffectActive( EF_NODRAW ) )
			return false;

		if ( candidate->ClassMatches( "item_healthkit" ) )
			return true;

		if ( candidate->ClassMatches( "item_healthvial" ) )
			return true;

		if ( candidate->ClassMatches( "item_healthcharger" ) )
			return true;

		if ( candidate->ClassMatches( "func_healthcharger" ) )
			return true;

		return false;
	}

	CNEOBot *m_me;
};


//---------------------------------------------------------------------------------------------
static CNEOBot *s_possibleBot = NULL;
static CHandle< CBaseEntity > s_possibleHealth = NULL;
static int s_possibleFrame = 0;


//---------------------------------------------------------------------------------------------
/** 
 * Return true if this Action has what it needs to perform right now
 */
bool CNEOBotGetHealth::IsPossible( CNEOBot *me )
{
	VPROF_BUDGET( "CNEOBotGetHealth::IsPossible", "NextBot" );

	float healthRatio = (float)me->GetHealth() / (float)me->GetMaxHealth();

#ifdef _DEBUG
	Assert(neo_bot_health_ok_ratio.GetFloat() != neo_bot_health_critical_ratio.GetFloat()); // Change callback should ensure this isn't the case, but not sure if its triggered immediately
#endif // _DEBUG
	float t = ( healthRatio - neo_bot_health_critical_ratio.GetFloat() ) / ( neo_bot_health_ok_ratio.GetFloat() - neo_bot_health_critical_ratio.GetFloat() );
	t = clamp( t, 0.0f, 1.0f );

	if ( me->GetFlags() & FL_ONFIRE )
	{
		// on fire - get health now
		t = 0.0f;
	}

	// the more we are hurt, the farther we'll travel to get health
	float searchRange = neo_bot_health_search_far_range.GetFloat() + t * ( neo_bot_health_search_near_range.GetFloat() - neo_bot_health_search_far_range.GetFloat() );

	CBaseEntity* healthkit = NULL;
	CUtlVector< CHandle< CBaseEntity > > hHealthKits;
	while ( ( healthkit = gEntList.FindEntityByClassname( healthkit, "*_health*" ) ) != NULL )
	{
		hHealthKits.AddToTail( healthkit );
	}

	CHealthFilter healthFilter( me );
	CUtlVector< CHandle< CBaseEntity > > hReachableHealthKits;
	me->SelectReachableObjects( hHealthKits, &hReachableHealthKits, healthFilter, me->GetLastKnownArea(), searchRange );

	CBaseEntity* closestHealth = hReachableHealthKits.Size() > 0 ? hReachableHealthKits[0] : NULL;

	if ( !closestHealth )
	{
		if ( me->IsDebugging( NEXTBOT_BEHAVIOR ) )
		{
			Warning( "%3.2f: No health nearby\n", gpGlobals->curtime );
		}
		return false;
	}

	CNEOBotPathCost cost( me, FASTEST_ROUTE );
	PathFollower path;
	if ( !path.Compute( me, closestHealth->WorldSpaceCenter(), cost ) || !path.IsValid() || path.GetResult() != Path::COMPLETE_PATH )
	{
		if ( me->IsDebugging( NEXTBOT_BEHAVIOR ) )
		{
			Warning( "%3.2f: No path to health!\n", gpGlobals->curtime );
		}
		return false;
	}

	s_possibleBot = me;
	s_possibleHealth = closestHealth;
	s_possibleFrame = gpGlobals->framecount;

	return true;
}

//---------------------------------------------------------------------------------------------
ActionResult< CNEOBot >	CNEOBotGetHealth::OnStart( CNEOBot *me, Action< CNEOBot > *priorAction )
{
	VPROF_BUDGET( "CNEOBotGetHealth::OnStart", "NextBot" );

	m_path.SetMinLookAheadDistance( me->GetDesiredPathLookAheadRange() );

	// if IsPossible() has already been called, use its cached data
	if ( s_possibleFrame != gpGlobals->framecount || s_possibleBot != me )
	{
		if ( !IsPossible( me ) || s_possibleHealth == NULL )
		{
			return Done( "Can't get health" );
		}
	}

	m_healthKit = s_possibleHealth;
	m_isGoalCharger = m_healthKit->ClassMatches( "*charger*" );

	CNEOBotPathCost cost( me, SAFEST_ROUTE );
	if ( !m_path.Compute( me, m_healthKit->WorldSpaceCenter(), cost ) )
	{
		return Done( "No path to health!" );
	}

	return Continue();
}


//---------------------------------------------------------------------------------------------
ActionResult< CNEOBot >	CNEOBotGetHealth::Update( CNEOBot *me, float interval )
{
	if ( m_healthKit == NULL || ( m_healthKit->IsEffectActive( EF_NODRAW ) ) )
	{
		return Done( "Health kit I was going for has been taken" );
	}

	if ( me->GetHealth() >= me->GetMaxHealth() )
	{
		return Done( "I've been healed" );
	}

	if ( NEORules()->IsTeamplay() )
	{
		// if the closest player to the item we're after is an enemy, give up

		CClosestNEOPlayer close( m_healthKit );
		ForEachPlayer( close );
		if ( close.m_closePlayer && me->IsEnemy( close.m_closePlayer ) )
			return Done( "An enemy is closer to it" );
	}

	if ( !m_path.IsValid() )
	{
		// this can occur if we overshoot the health kit's location
		// because it is momentarily gone
		CNEOBotPathCost cost( me, SAFEST_ROUTE );
		if ( !m_path.Compute( me, m_healthKit->WorldSpaceCenter(), cost ) )
		{
			return Done( "No path to health!" );
		}
	}

	m_path.Update( me );

	// may need to switch weapons (ie: engineer holding toolbox now needs to heal and defend himself)
	const CKnownEntity *threat = me->GetVisionInterface()->GetPrimaryKnownThreat();
	me->EquipBestWeaponForThreat( threat );

	return Continue();
}


//---------------------------------------------------------------------------------------------
EventDesiredResult< CNEOBot > CNEOBotGetHealth::OnStuck( CNEOBot *me )
{
	return TryDone( RESULT_CRITICAL, "Stuck trying to reach health kit" );
}


//---------------------------------------------------------------------------------------------
EventDesiredResult< CNEOBot > CNEOBotGetHealth::OnMoveToSuccess( CNEOBot *me, const Path *path )
{
	return TryContinue();
}


//---------------------------------------------------------------------------------------------
EventDesiredResult< CNEOBot > CNEOBotGetHealth::OnMoveToFailure( CNEOBot *me, const Path *path, MoveToFailureType reason )
{
	return TryDone( RESULT_CRITICAL, "Failed to reach health kit" );
}


//---------------------------------------------------------------------------------------------
// We are always hurrying if we need to collect health
QueryResultType CNEOBotGetHealth::ShouldHurry( const INextBot *me ) const
{
	return ANSWER_YES;
}
