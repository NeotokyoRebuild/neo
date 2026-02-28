#include "cbase.h"
#include "neo_player.h"
#include "bot/neo_bot.h"
#include "bot/behavior/neo_bot_grenade_throw_smoke.h"
#include "bot/neo_bot_path_compute.h"
#include "weapon_neobasecombatweapon.h"

#include "nav_pathfind.h"

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
		const Vector& vecThrowTarget = FindEmergencePointAlongPath(me, me->GetAbsOrigin(), m_vecThreatLastKnownPos);

		if (vecThrowTarget != vec3_invalid)
		{
			m_vecTarget = vecThrowTarget;
		}
		else
		{
			// throw smoke at feet as fallback
			m_vecTarget = me->GetAbsOrigin();
		}
	}
	
	Assert( m_vecTarget != vec3_invalid );
	return THROW_TARGET_READY;
}

