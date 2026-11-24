#include "cbase.h"
#include "neo_player.h"
#include "bot/neo_bot.h"
#include "bot/behavior/neo_bot_grenade_throw_smoke.h"
#include "neo_smokelineofsightblocker.h"
#include "weapon_neobasecombatweapon.h"

namespace
{
bool IsActiveSmoke( CBaseEntity *pSmoke )
{
	return pSmoke && pSmoke->IsSolid() && !pSmoke->IsMarkedForDeletion();
}

bool IsObservedActiveSmoke( CNEOBot *me, CBaseEntity *pSmoke )
{
	return IsActiveSmoke( pSmoke ) &&
		( pSmoke->CollisionProp()->IsPointInBounds( me->GetAbsOrigin() ) ||
			me->FVisible( pSmoke->GetAbsOrigin(), MASK_VISIBLE ) );
}

bool IsPointInObservedSmoke( CNEOBot *me, const Vector &vecPoint )
{
	CBaseEntity *pSmoke = nullptr;
	while ( ( pSmoke = gEntList.FindEntityByClassname( pSmoke, SMOKELINEOFSIGHTBLOCKER_ENTITYNAME ) ) )
	{
		if ( pSmoke->CollisionProp()->IsPointInBounds( vecPoint ) && IsObservedActiveSmoke( me, pSmoke ) )
		{
			return true;
		}
	}
	return false;
}
} // namespace

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
			// throw smoke at feet as fallback
			m_vecTarget = me->GetAbsOrigin();
		}
	}
	
	Assert( m_vecTarget != vec3_invalid );
	// A cloud observed while aiming can already cover the target.
	if ( IsPointInObservedSmoke( me, m_vecTarget ) )
	{
		return THROW_TARGET_CANCEL;
	}
	return THROW_TARGET_READY;
}

