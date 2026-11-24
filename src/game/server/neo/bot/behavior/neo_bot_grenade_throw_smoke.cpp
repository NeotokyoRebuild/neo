#include "cbase.h"
#include "neo_player.h"
#include "bot/neo_bot.h"
#include "bot/behavior/neo_bot_grenade_throw_smoke.h"
#include "weapon_neobasecombatweapon.h"
#include "nav_mesh.h"
#include "nav_pathfind.h"

extern ConVar sv_neo_bot_grenade_search_range;

static Vector FindSmokePathThrowPoint( CNEOBot *me, const Vector &vecThreatPos )
{
	CNavArea *pStartArea = me->GetLastKnownArea();
	const float flMaxPathLength = sv_neo_bot_grenade_search_range.GetFloat();
	if ( !pStartArea || flMaxPathLength <= 0.0f )
	{
		return vec3_invalid;
	}
	CNavArea *pGoalArea = TheNavMesh->GetNavArea( vecThreatPos );
	ShortestPathCost cost;
	if ( !pGoalArea || !NavAreaBuildPath( pStartArea, pGoalArea, nullptr, cost, nullptr, flMaxPathLength, me->GetTeamNumber() ) )
	{
		return vec3_invalid;
	}

	CUtlVector< CNavArea * > path;
	for ( CNavArea *pArea = pGoalArea; pArea && pArea != pStartArea; pArea = pArea->GetParent() )
	{
		path.AddToTail( pArea );
	}

	CNavArea *pTargetArea = pStartArea;
	for ( int i = path.Count() - 1; i >= 0; --i )
	{
		if ( !pStartArea->IsCompletelyVisible( path[i] ) )
		{
			break;
		}
		pTargetArea = path[i];
	}

	if ( pTargetArea == pStartArea )
	{
		return vec3_invalid;
	}
	const Vector &vecCenter = pTargetArea->GetCenter();
	return me->IsThrowLineClear( vecCenter ) ? vecCenter : vec3_invalid;
}

CNEOBotGrenadeThrowSmoke::CNEOBotGrenadeThrowSmoke( CNEOBaseCombatWeapon *pWeapon, const CKnownEntity *threat )
	: CNEOBotGrenadeThrow( pWeapon, threat )
{
}

ActionResult< CNEOBot >	CNEOBotGrenadeThrowSmoke::OnStart( CNEOBot *me, Action< CNEOBot > *priorAction )
{
	ActionResult< CNEOBot > result = CNEOBotGrenadeThrow::OnStart( me, priorAction );
	if ( result.IsDone() )
	{
		return result;
	}

	return Continue();
}

CNEOBotGrenadeThrow::ThrowTargetResult CNEOBotGrenadeThrowSmoke::UpdateGrenadeTargeting( CNEOBot *me, CNEOBaseCombatWeapon *pWeapon )
{
	Assert( m_vecThreatLastKnownPos != vec3_invalid );
	
	if (m_vecTarget == vec3_invalid)
	{
		const Vector vecThrowTarget = me->FindVisibleThrowPointNear( m_vecThreatLastKnownPos );

		if (vecThrowTarget != vec3_invalid)
		{
			m_vecTarget = vecThrowTarget;
		}
		else
		{
			m_vecTarget = FindSmokePathThrowPoint( me, m_vecThreatLastKnownPos );
			if ( m_vecTarget == vec3_invalid || UpdateThrowAim( me ) == THROW_AIM_UNREACHABLE )
			{
				m_vecTarget = me->GetAbsOrigin();
			}
		}
	}
	
	Assert( m_vecTarget != vec3_invalid );
	return THROW_TARGET_READY;
}

