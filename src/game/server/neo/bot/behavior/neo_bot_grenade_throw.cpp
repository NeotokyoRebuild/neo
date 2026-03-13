#include "cbase.h"
#include "neo_player.h"
#include "bot/neo_bot.h"
#include "bot/behavior/neo_bot_grenade_throw.h"
#include "bot/behavior/neo_bot_retreat_from_grenade.h"
#include "weapon_neobasecombatweapon.h"
#include "weapon_grenade.h"
#include "weapon_smokegrenade.h"
#include "nav_mesh.h"
#include "nav_pathfind.h"
#include "bot/neo_bot_path_compute.h"

extern ConVar sv_neo_bot_grenade_frag_safety_range_multiplier;
extern ConVar sv_neo_grenade_blast_radius;

ConVar sv_neo_bot_grenade_debug_behavior("sv_neo_bot_grenade_debug_behavior", "0", FCVAR_CHEAT,
	"Draw debug overlays for bot grenade behavior", true, 0, true, 1);

ConVar sv_neo_bot_grenade_give_up_time("sv_neo_bot_grenade_give_up_time", "5.0", FCVAR_NONE,
	"Time in seconds before bot gives up on grenade throw", true, 1, false, 0);

ConVar sv_neo_bot_grenade_search_range("sv_neo_bot_grenade_search_range", "1000", FCVAR_CHEAT,
	"Travel distance range for bot to search for a grenade throw vantage point", true, 0, false, 0);

//---------------------------------------------------------------------------------------------
CNEOBotGrenadeThrow::CNEOBotGrenadeThrow( CNEOBaseCombatWeapon *pWeapon, const CKnownEntity *threat )
{
	m_hGrenadeWeapon = pWeapon;
	m_bVantagePointBlocked = false;
	m_vantageArea = nullptr;
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
const Vector& CNEOBotGrenadeThrow::FindEmergencePointAlongPath( const CNEOBot *me, const Vector &familiarPos, const Vector &obscuredPos )
{
	CNavArea *familiarArea = TheNavMesh->GetNavArea( familiarPos );
	if ( !familiarArea )
	{
		return vec3_invalid;
	}

	CNavArea *obscuredArea = TheNavMesh->GetNavArea( obscuredPos );
	if ( !obscuredArea )
	{
		return vec3_invalid;
	}

	ShortestPathCost cost;
	const Vector& vecGoal = obscuredPos;
	if ( NavAreaBuildPath( familiarArea, obscuredArea, &vecGoal, cost ) )
	{
		// search backwards from obscured position to find the first point visible to me
		for ( CNavArea *area = obscuredArea; area; area = area->GetParent() )
		{
			// DEBUG: Draw emergence path
			// Color: Yellow (255, 255, 0) to distinguish path analysis
			if ( sv_neo_bot_grenade_debug_behavior.GetBool() )
			{
				if ( area->GetParent() )
				{
					NDebugOverlay::HorzArrow( area->GetCenter(), area->GetParent()->GetCenter(), 2.0f, 255, 255, 0, 255, true, 2.0f );
				}
				else
				{
					NDebugOverlay::Cross3D( area->GetCenter(), 16.0f, 255, 255, 0, true, 2.0f );
				}
			}

			const Vector& vecTest = area->GetCenter();

			if ( me->IsLineOfFireClear( vecTest, CNEOBot::LINE_OF_FIRE_FLAGS_SHOTGUN ) )
			{
				// DEBUG: Draw emergence point
				if ( sv_neo_bot_grenade_debug_behavior.GetBool() )
				{
					NDebugOverlay::Box( vecTest, Vector(-16,-16,-16), Vector(16,16,16), 255, 255, 0, 50, 2.0f );
				}
				return vecTest;
			}

			if ( area == familiarArea )
			{
				return vec3_invalid;
			}
		}
	}

	return vec3_invalid;
}


//---------------------------------------------------------------------------------------------
class CFindVantagePointTargetPos : public ISearchSurroundingAreasFunctor
{
public:
	CFindVantagePointTargetPos( CNEOBot *me, const Vector &targetPos, CBaseEntity *pThreat )
	{
		m_me = me;
		m_targetPos = targetPos;
		m_vantageArea = nullptr;
		m_threatArea = nullptr;
		m_targetArea = TheNavMesh->GetNavArea( m_targetPos );

		if ( pThreat )
		{
			m_threatArea = TheNavMesh->GetNavArea( pThreat->GetAbsOrigin() );
		}

		if ( !m_threatArea )
		{
			const CKnownEntity *primaryThreat = me->GetVisionInterface()->GetPrimaryKnownThreat();
			if ( primaryThreat )
			{
				m_threatArea = primaryThreat->GetLastKnownArea();
			}
		}

		float flRadius = sv_neo_grenade_blast_radius.GetFloat() * sv_neo_bot_grenade_frag_safety_range_multiplier.GetFloat();
		m_flSafetyRadiusSq = flRadius * flRadius;
	}

	virtual bool operator() ( CNavArea* baseArea, CNavArea* priorArea, float travelDistanceSoFar )
	{
		CNavArea* area = (CNavArea*)baseArea;

		if (!m_targetArea)
		{
			return false; // can't search
		}

		if ( area->IsPotentiallyVisible( m_targetArea ) )
		{
			// nearby area from which we can see the last known position
			m_vantageArea = area;
			return false; // stop searching
		}

		return true; // continue searching
	}

	// return true if 'adjArea' should be included in the ongoing search
	virtual bool ShouldSearch( CNavArea *adjArea, CNavArea *currentArea, float travelDistanceSoFar ) 
	{
		if ( travelDistanceSoFar > sv_neo_bot_grenade_search_range.GetFloat() )
		{
			return false;
		}

		// For considering areas off to the side of current area
		constexpr float distanceThresholdRatio = 0.8f;

		if ( m_threatArea )
		{
			// The adjacent area to search should not be farther from the threat
			float adjThreatDistance = ( m_threatArea->GetCenter() - adjArea->GetCenter() ).LengthSqr();
			float curThreatDistance = ( m_threatArea->GetCenter() - currentArea->GetCenter() ).LengthSqr();
			if ( adjThreatDistance * distanceThresholdRatio > curThreatDistance )
			{
				return false; // Candidate adjacent area veers farther from threat
			}

			// The adjacent area to search should not be closer than the safety range
			if ( adjThreatDistance < m_flSafetyRadiusSq )
			{
				// NEO JANK: While this range is based on frag grenades,
				// it's still likely a bad idea to throw smoke grenades this close to the threat
				// Though logic at time of comment just throws smoke safely from behind cover
				return false; // Candidate adjacent area is too close to threat
			}
		}

		// allow falling off ledges, but don't jump up - too slow
		return ( currentArea->ComputeAdjacentConnectionHeightChange( adjArea ) < m_me->GetLocomotionInterface()->GetStepHeight() );
	}

	CNEOBot *m_me;
	Vector m_targetPos;
	CNavArea *m_targetArea;
	CNavArea *m_vantageArea;
	const CNavArea *m_threatArea;
	float m_flSafetyRadiusSq;
};


//---------------------------------------------------------------------------------------------
ActionResult< CNEOBot >	CNEOBotGrenadeThrow::OnStart( CNEOBot *me, Action< CNEOBot > *priorAction )
{
	CNEOBaseCombatWeapon *pWep = m_hGrenadeWeapon.Get();
	if ( pWep )
	{
		Assert( ( pWep->GetNeoWepBits() & NEO_WEP_FRAG_GRENADE ) || ( pWep->GetNeoWepBits() & NEO_WEP_SMOKE_GRENADE ) );
		me->PushRequiredWeapon( m_hGrenadeWeapon );
	}
	else
	{
		return Done( "Grenade input invalid" );
	}

	if ( !m_hThreatGrenadeTarget.Get() )
	{
		return Done( "Targeted Threat is null" );
	}

	if ( !m_hThreatGrenadeTarget->IsAlive() )
	{
		return Done( "Targeted threat is dead" );
	}
	
	m_giveUpTimer.Start( sv_neo_bot_grenade_give_up_time.GetFloat() );
	m_bPinPulled = false;

	m_PathFollower.SetMinLookAheadDistance( me->GetDesiredPathLookAheadRange() );
	if ( m_vecThreatLastKnownPos != vec3_invalid )
	{
		CNEOBotPathCompute( me, m_PathFollower, m_vecThreatLastKnownPos, FASTEST_ROUTE );
	}

	// Stop distracting looking or weapon handling behavior that could interrupt the throw
	me->StopLookingAroundForEnemies(); // reduce distractions for manual look aiming
	me->SetAttribute( CNEOBot::IGNORE_ENEMIES ); // suppress reaction to swap back to firearm

	return Continue();
}


//---------------------------------------------------------------------------------------------
ActionResult< CNEOBot >	CNEOBotGrenadeThrow::Update( CNEOBot *me, float interval )
{
	Assert( me );

	CNEOBaseCombatWeapon *pWep = m_hGrenadeWeapon.Get();
	if ( !pWep )
	{
		return Done( "Do not have grenade" );
	}

	if ( !pWep->HasPrimaryAmmo() )
	{
		return Done( "Weapon slot out of ammo" );
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

	if ( gpGlobals->curtime - me->GetLastDamageTime() < 0.5f )
	{
		return Done( "Actively getting hurt, too dangerous to throw grenade" );
	}

	if ( sv_neo_bot_grenade_debug_behavior.GetBool() && m_hThreatGrenadeTarget.Get() )
	{
		// DEBUG Colors: Red = Frag, Gray = Smoke, Purple = Unknown
		const Vector& vecStart = me->WorldSpaceCenter();
		const Vector& vecVictim = m_hThreatGrenadeTarget->GetAbsOrigin();
		int r = 255, g = 0, b = 255; // Default purple just in case
		if ( pWep->GetNeoWepBits() & NEO_WEP_FRAG_GRENADE )
		{
			r = 255; g = 0; b = 0; // Red
		}
		else if ( pWep->GetNeoWepBits() & NEO_WEP_SMOKE_GRENADE )
		{
			r = 128; g = 128; b = 128; // Gray
		}
		// Draw box around the bot that is considering the throw
		NDebugOverlay::Box( me->GetAbsOrigin(), me->WorldAlignMins(), me->WorldAlignMaxs(), r, g, b, 30, 0.1f );
		// Arrow from bot to threat being targeted
		NDebugOverlay::HorzArrow( vecStart, vecVictim, 2.0f, r, g, b, 255, true, 0.1f );
	}

	me->EnableCloak(3.0f);

	if ( me->GetActiveWeapon() != pWep )
	{
		// Still waiting to switch
		me->Weapon_Switch( pWep );
		return Continue();
	}

	// NEOJANK: The bots struggle to throw grenades with PressFireButton due to control quirks.
	// As a workaround, we decompose the action into different phases of the bot behavior.
	// This also allows us to run aiming logic in parallel with the pin-pull animation.
	if (!m_bPinPulled)
	{
		// Wait for weapon switch to complete fully
		if ( pWep->m_flNextPrimaryAttack > gpGlobals->curtime )
		{
			return Continue();
		}
		// Just play the animation. Do NOT call PrimaryAttack, as that sets up the weapon to 
		// auto-throw via ItemPostFrame, which we want to control manually here.
		pWep->SendWeaponAnim( ACT_VM_PULLPIN );
		pWep->m_flTimeWeaponIdle = FLT_MAX; // Don't let idle anims interrupt us
		m_bPinPulled = true;
	}

	ThrowTargetResult result = UpdateGrenadeTargeting( me, pWep );

	// Subclasses may decide in the last second that a throw is too dangerous
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

		Vector vecForward;
		me->EyeVectors( &vecForward );
		Vector vecToTarget = m_vecTarget - me->EyePosition();
		vecToTarget.NormalizeInPlace();
		
		float flAimThreshold;
		switch( me->GetDifficulty() )
		{
		case CNEOBot::EXPERT:
			flAimThreshold = 0.98f;
			break;
		case CNEOBot::HARD:
			flAimThreshold = 0.97f;
			break;
		case CNEOBot::NORMAL:
			flAimThreshold = 0.96f;
			break;
		case CNEOBot::EASY:
		default:
			flAimThreshold = 0.95f;
			break;
		}

		if ( vecForward.Dot( vecToTarget ) >= flAimThreshold )
		{
			if ( me->IsLineOfFireClear( m_vecTarget, CNEOBot::LINE_OF_FIRE_FLAGS_SHOTGUN ) )
			{
				bAimOnTarget = true;
			}
			else
			{
				// Blocked by an obstruction
				if ( sv_neo_bot_grenade_debug_behavior.GetBool() )
				{
					NDebugOverlay::Line( me->EyePosition(), m_vecTarget, 0, 0, 255, true, 0.1f );
				}

				m_PathFollower.Update( me );

				if ( m_repathTimer.IsElapsed() )
				{
					m_repathTimer.Start( RandomFloat( 0.3f, 0.5f ) );
					CNEOBotPathCompute( me, m_PathFollower, m_vecTarget, FASTEST_ROUTE );
				}
			}
		}
	}
	else if ( result == THROW_TARGET_WAIT )
	{
		m_PathFollower.Update( me );

		// Watch for threats emerging from known position
		if ( m_vecThreatLastKnownPos != vec3_invalid )
		{
			me->GetBodyInterface()->AimHeadTowards(
				m_vecThreatLastKnownPos, IBody::IMPORTANT, 0.1f, nullptr,
				"Looking towards last known threat position while moving to vantage point");
		}

		if ( m_vantageArea && me->GetLastKnownArea() == m_vantageArea )
		{
			// Arrived at vantage point but still blocked - fallback to chase
			m_bVantagePointBlocked = true;
			m_vantageArea = nullptr;
		}

		if ( m_repathTimer.IsElapsed() )
		{
			m_repathTimer.Start( RandomFloat( 0.3f, 0.5f ) );

			if ( !m_vantageArea && !m_bVantagePointBlocked )
			{
				CFindVantagePointTargetPos find( me, m_vecThreatLastKnownPos, m_hThreatGrenadeTarget.Get() );
				SearchSurroundingAreas( me->GetLastKnownArea(), find, sv_neo_bot_grenade_search_range.GetFloat() );
				m_vantageArea = find.m_vantageArea;

				if ( !m_vantageArea )
				{
					return Done( "Failed to find a vantage area to throw grenade from" );
				}
			}

			bool bGoingToVantagePoint = false;
			if ( m_vantageArea )
			{
				bGoingToVantagePoint = CNEOBotPathCompute( me, m_PathFollower, m_vantageArea->GetCenter(), FASTEST_ROUTE );
			}

			if ( !bGoingToVantagePoint )
			{
				// Fallback to direct chase behavior if vantage point is blocked
				if ( m_vantageArea )
				{
					m_vantageArea = nullptr;
					m_bVantagePointBlocked = true;
				}
				CNEOBotPathCompute( me, m_PathFollower, m_vecTarget != vec3_invalid ? m_vecTarget : m_vecThreatLastKnownPos, FASTEST_ROUTE );
			}
		}
	}
	
	// DEBUG: Targeting decisions
	if ( sv_neo_bot_grenade_debug_behavior.GetBool() )
	{
		const Vector& vecStart = me->WorldSpaceCenter();
		// Color: Orange (255, 128, 0) is the final ballistics target
		if ( m_vecTarget != vec3_invalid )
		{
			NDebugOverlay::HorzArrow( vecStart, m_vecTarget, 2.0f, 255, 128, 0, 255, true, 2.0f );
			NDebugOverlay::Box( m_vecTarget, Vector(-16,-16,-16), Vector(16,16,16), 255, 128, 0, 30, 2.0f );
		}

		if ( m_vantageArea )
		{
			NDebugOverlay::Box( m_vantageArea->GetCenter(), Vector(-16,-16,0), Vector(16,16,16), 0, 255, 0, 30, 0.1f );
			NDebugOverlay::Line( me->EyePosition(), m_vantageArea->GetCenter(), 0, 255, 0, true, 0.1f );
		}
	}

	// NEO JANK: Force the throwing phase if aimed.
	// We ignore bWeaponReady/m_flNextPrimaryAttack because we are manually controlling the throw
	// and want to bypass the weapon's internal scheduled throw to ensure it happens NOW.
	if ( bAimOnTarget )
	{
		// Play first person throw animation
		pWep->SendWeaponAnim( ACT_VM_THROW );

		if ( pWep->GetNeoWepBits() & NEO_WEP_FRAG_GRENADE )
		{
			static_cast<CWeaponGrenade*>(pWep)->ThrowGrenade( me );
		}
		else if ( pWep->GetNeoWepBits() & NEO_WEP_SMOKE_GRENADE )
		{
			static_cast<CWeaponSmokeGrenade*>(pWep)->ThrowGrenade( me );
		}
		else
		{
			DevWarning( "CNEOBotGrenadeThrow: Unknown grenade type!\n" );
			Assert(0);
		}

		// Would exit immediately for smoke, but enemies tend to frag back so look for one now
		return ChangeTo( new CNEOBotRetreatFromGrenade( nullptr ), "Retreating after throw" );
	}
	
	return Continue();
}

//---------------------------------------------------------------------------------------------
void CNEOBotGrenadeThrow::OnEnd( CNEOBot *me, Action< CNEOBot > *nextAction )
{
	// Restore looking and weapon handling behaviors
	me->PopRequiredWeapon();
	me->StartLookingAroundForEnemies();
	me->ClearAttribute( CNEOBot::IGNORE_ENEMIES );
	const CKnownEntity *threat = me->GetVisionInterface()->GetPrimaryKnownThreat();
	me->EquipBestWeaponForThreat( threat );
}

//---------------------------------------------------------------------------------------------
QueryResultType CNEOBotGrenadeThrow::ShouldRetreat( const INextBot *me ) const
{
	// Avoid tactical monitor interrupting the grenade throw behavior
	return ANSWER_NO;
}

//---------------------------------------------------------------------------------------------
ActionResult<CNEOBot> CNEOBotGrenadeThrow::OnSuspend( CNEOBot *me, Action<CNEOBot> *interruptingAction )
{
	return Done( "OnSuspend: Cancel out of grenade throw behavior, situation will likely become stale." );
	// OnEnd will get called after Done
}

//---------------------------------------------------------------------------------------------
ActionResult<CNEOBot> CNEOBotGrenadeThrow::OnResume( CNEOBot *me, Action<CNEOBot> *interruptingAction )
{
	return Done( "OnResume: Cancel out of grenade throw behavior, situation is likely stale." );
	// OnEnd will get called after Done
}
