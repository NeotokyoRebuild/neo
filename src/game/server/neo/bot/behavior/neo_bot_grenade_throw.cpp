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

	// NEOJANK: The bots struggle throw to grenades with PressFireButton due to control quirks
	// As a workaround, we decompose the action into different phases of the bot behavior
	// Part 1: Primary attack which actually only pulls the pin
	// Part 2: See below ItemPostFrame() which throws the grenade and triggers throw animation
	if ( !m_bPinPulled )
	{
		// Initiate pull pin animation
		pWep->PrimaryAttack();
		m_pinPullTimer.Start( 0.5f ); // RETHROW_DELAY
		m_bPinPulled = true;
	}

	if ( !m_pinPullTimer.IsElapsed() )
	{
		return Continue();
	}

	// Continue to call UpdateGrenadeTargeting after THROW_TARGET_READY result
	// as subclasses may decide in the last second that a throw is too dangerous (CANCEL)
	switch ( UpdateGrenadeTargeting( me, pWep ) )
	{
	case THROW_TARGET_CANCEL:
		return Done( "Grenade throw aborted" );

	case THROW_TARGET_WAIT:
		return Continue();

	case THROW_TARGET_READY:
		// Wait until we are aiming at the target
		if ( m_vecTarget == vec3_invalid )
		{
			return Done( "Invalid target coordinate" );
		}
		me->GetBodyInterface()->AimHeadTowards( m_vecTarget, IBody::MANDATORY, 0.2f, nullptr, "Aiming grenade" );

		Vector vecForward;
		me->EyeVectors( &vecForward );
		Vector vecToTarget = m_vecTarget - me->GetEntity()->EyePosition();
		vecToTarget.NormalizeInPlace();
		if ( vecForward.Dot( vecToTarget ) < 0.99f )
		{
			return Continue();
		}

		// NEOJANK Part 2: Throw the grenade with ItemPostFrame
		pWep->ItemPostFrame(); // includes ThrowGrenade() among other triggers like animation
		return Done( "Grenade throw sequence finished" );
	default:
		Assert( false );
		return Done( "Unknown grenade throw outcome" );
	}
}

//---------------------------------------------------------------------------------------------
void CNEOBotGrenadeThrow::OnEnd( CNEOBot *me, Action< CNEOBot > *nextAction )
{
	me->PopRequiredWeapon();
}
