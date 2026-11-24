#include "cbase.h"
#include "neo_player.h"
#include "bot/neo_bot.h"
#include "bot/behavior/neo_bot_grenade_throw_smoke.h"
#include "bot/neo_bot_path_reservation.h"
#include "weapon_neobasecombatweapon.h"
#include "nav_mesh.h"

extern ConVar sv_neo_grenade_fuse_timer;
extern ConVar sv_neo_smoke_blocker_delay;
extern ConVar sv_neo_smoke_blocker_duration;
extern ConVar sv_neo_smoke_blocker_size;

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

	if ( m_vecTarget == vec3_invalid )
	{
		m_vecTarget = me->FindVisibleThrowPointNear( m_vecThreatLastKnownPos );
		m_pTargetArea = m_vecTarget == vec3_invalid ? nullptr : TheNavMesh->GetNavArea( m_vecTarget );

		if ( m_vecTarget == vec3_invalid || IsThrowTargetReserved( me, m_pTargetArea, GRENADE_HAZARD_SMOKE_OR_DEADLY ) )
		{
			// throw smoke at feet as fallback
			m_vecTarget = me->GetAbsOrigin();
			m_pTargetArea = TheNavMesh->GetNavArea( m_vecTarget );
		}
	}

	Assert( m_vecTarget != vec3_invalid );
	if ( IsThrowTargetReserved( me, m_pTargetArea, GRENADE_HAZARD_SMOKE_OR_DEADLY ) )
	{
		return THROW_TARGET_CANCEL;
	}

	return THROW_TARGET_READY;
}

void CNEOBotGrenadeThrowSmoke::OnThrowReleased( CNEOBot *me )
{
	const float flExpireTime = gpGlobals->curtime + sv_neo_grenade_fuse_timer.GetFloat()
		+ sv_neo_smoke_blocker_delay.GetFloat() + sv_neo_smoke_blocker_duration.GetFloat();
	const Vector vecHalfSize( sv_neo_smoke_blocker_size.GetInt() );
	const Extent footprint = { m_vecTarget - vecHalfSize, m_vecTarget + vecHalfSize };
	const int team = me->GetTeamNumber();
	constexpr bool bPropagatePVS = false;
	CNEOBotPathReservationSystem *pReservations = CNEOBotPathReservations();

	if ( m_pTargetArea )
	{
		pReservations->AddSmokeHazard( m_pTargetArea->GetID(), flExpireTime, team, bPropagatePVS );
	}

	auto reserveFootprint = [&]( const CNavArea *pArea )
	{
		if ( pArea != m_pTargetArea )
		{
			pReservations->AddSmokeHazard( pArea->GetID(), flExpireTime, team, bPropagatePVS );
		}
		return true;
	};
	TheNavMesh->ForAllAreasOverlappingExtent( reserveFootprint, footprint );
}

