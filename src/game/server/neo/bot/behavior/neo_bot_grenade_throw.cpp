#include "cbase.h"
#include "neo_player.h"
#include "bot/neo_bot.h"
#include "bot/behavior/neo_bot_grenade_throw.h"
#include "weapon_neobasecombatweapon.h"
#include "nav_mesh.h"
#include "nav_pathfind.h"

//---------------------------------------------------------------------------------------------
CNEOBotGrenadeThrow::CNEOBotGrenadeThrow( CNEOBaseCombatWeapon *pWeapon, const CKnownEntity *threat )
{
	m_hGrenadeWeapon = pWeapon;
	m_vecTarget = vec3_invalid;

	if ( threat )
	{
		m_hThreatGrenadeTarget = threat->GetEntity();
		m_vecThreatLastKnownPos = threat->GetLastKnownPosition();
	}
	else
	{
		m_hThreatGrenadeTarget = nullptr;
		m_vecThreatLastKnownPos = vec3_invalid;
	}
}

//---------------------------------------------------------------------------------------------
// Used to anticipate the emergence point from cover ahead of a path
// Is calculated in reverse of path to find the farthest point from bot
// (assuming "familiar" position is closer to the bot than the "obscured" position)
Vector CNEOBotGrenadeThrow::FindEmergencePointAlongPath( CNEOBot *me, const Vector &familiarPos, const Vector &obscuredPos )
{
	CNavArea *familiarArea = TheNavMesh->GetNavArea( familiarPos );
	CNavArea *obscuredArea = TheNavMesh->GetNavArea( obscuredPos );

	if ( familiarArea && obscuredArea )
	{
		ShortestPathCost cost;
		Vector vecGoal = obscuredPos;
		if ( NavAreaBuildPath( familiarArea, obscuredArea, &vecGoal, cost ) )
		{
			// search backwards from obscured position to find the first point visible to me
			for ( CNavArea *area = obscuredArea; area; area = area->GetParent() )
			{
				Vector vecTest = area->GetCenter();

				if ( me->IsLineOfFireClear( vecTest, CNEOBot::LINE_OF_FIRE_FLAGS_SHOTGUN ) )
				{
					return vecTest;
				}

				if ( area == familiarArea )
				{
					break;
				}
			}
		}
	}

	return vec3_invalid;
}

//---------------------------------------------------------------------------------------------
ActionResult< CNEOBot >	CNEOBotGrenadeThrow::OnStart( CNEOBot *me, Action< CNEOBot > *priorAction )
{
	if ( !m_hThreatGrenadeTarget.Get() )
	{
		return Done( "Targeted Threat is null" );
	}

	if ( !m_hThreatGrenadeTarget->IsAlive() )
	{
		return Done( "Targeted threat is dead" );
	}

	if ( m_hGrenadeWeapon.Get() )
	{
		me->PushRequiredWeapon( m_hGrenadeWeapon );
	}
	
	m_giveUpTimer.Start( 5.0f );
	m_bPinPulled = false;

	return Continue();
}

//---------------------------------------------------------------------------------------------
ActionResult< CNEOBot >	CNEOBotGrenadeThrow::Update( CNEOBot *me, float interval )
{
	CNEOBaseCombatWeapon *pWep = m_hGrenadeWeapon.Get();
	if ( !pWep )
	{
		return Done( "Do not have grenade" );
	}

	if ( m_vecThreatLastKnownPos == vec3_invalid )
	{
		return Done( "Targeted threat position invalid" );
	}

	if ( !m_hThreatGrenadeTarget.Get() )
	{
		return Done( "Targeted threat is null" );
	}

	if ( !m_hThreatGrenadeTarget->IsAlive() )
	{
		return Done( "Targeted threat is dead" );
	}

	if ( m_giveUpTimer.IsElapsed() )
	{
		return Done( "Grenade throw timed out" );
	}

	if ( me->GetActiveWeapon() != pWep )
	{
		// Still waiting to switch
		me->Weapon_Switch( pWep );
		return Continue();
	}

	// Wait for weapon switch to complete fully
	if ( pWep->m_flNextPrimaryAttack > gpGlobals->curtime )
	{
		return Continue();
	}

	ThrowTargetResult result = UpdateGrenadeTargeting( me, pWep );

	// Subclasses may decide in the last second that a throw is too dangerous (CANCEL)
	if ( result == THROW_TARGET_CANCEL )
	{
		return Done( "Grenade throw aborted" );
	}

	bool bAimOnTarget = false;

	if ( result == THROW_TARGET_READY )
	{
		if ( m_vecTarget == vec3_invalid )
		{
			return Done( "Invalid target coordinate" );
		}
		
		me->GetBodyInterface()->AimHeadTowards( m_vecTarget, IBody::MANDATORY, 0.2f, nullptr, "Aiming grenade" );

		// NEOJANK: The bots struggle to throw grenades with PressFireButton due to control quirks.
		// As a workaround, we decompose the action into different phases of the bot behavior.
		// This also allows us to run aiming logic in parallel with the pin-pull animation.

		// PrimaryAttack which actually only pulls the pin
		if (!m_bPinPulled)
		{
			// Initiate pull pin animation
			pWep->PrimaryAttack();
			m_bPinPulled = true;
		}

		Vector vecForward;
		me->EyeVectors( &vecForward );
		Vector vecToTarget = m_vecTarget - me->GetEntity()->EyePosition();
		vecToTarget.NormalizeInPlace();
		
		if ( vecForward.Dot( vecToTarget ) >= 0.95f )
		{
			bAimOnTarget = true;
		}
	}

	// Check Weapon Readiness
	// m_flNextPrimaryAttack is set by PrimaryAttack() to block firing during the pin-pull anim.
	bool bWeaponReady = ( pWep->m_flNextPrimaryAttack <= gpGlobals->curtime );

	// Execute Throw
	// Only throw if both aimed correctly AND the weapon ready animation has finished.
	if ( bWeaponReady && bAimOnTarget )
	{
		// Throw the grenade with ItemPostFrame
		pWep->ItemPostFrame(); // includes ThrowGrenade() among other triggers like animation
		return Done( "Grenade throw sequence finished" );
	}
	
	return Continue();
}

//---------------------------------------------------------------------------------------------
void CNEOBotGrenadeThrow::OnEnd( CNEOBot *me, Action< CNEOBot > *nextAction )
{
	me->PopRequiredWeapon();
	const CKnownEntity *threat = me->GetVisionInterface()->GetPrimaryKnownThreat();
	me->EquipBestWeaponForThreat( threat );
}
