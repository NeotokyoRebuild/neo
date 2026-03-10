#include "cbase.h"

#include "bot/neo_bot.h"
#include "bot/behavior/neo_bot_path_clear_breakable.h"
#include "neo_gamerules.h"

extern ConVar neo_bot_fire_weapon_allowed;

ConVar neo_bot_fire_at_breakable_weapon_min_time( "neo_bot_fire_at_breakable_weapon_min_time", "0.2", FCVAR_CHEAT,
	"Minimum time to fire at breakables", true, 0.0f, true, 60.0f );

//--------------------------------------------------------------------------------------------------------
CBaseEntity *GetBreakableInPath( CNEOBot *me )
{
	if ( !me || !me->IsAlive() )
	{
		return nullptr;
	}

	if ( me->GetNeoFlags() & NEO_FL_FREEZETIME )
	{
		return nullptr;
	}

	if ( !neo_bot_fire_weapon_allowed.GetBool() )
	{
		return nullptr;
	}

	// Only enter this behavior if there is no visible threat
	const CKnownEntity *threat = me->GetVisionInterface()->GetPrimaryKnownThreat();
	if ( threat && threat->GetEntity() && threat->IsVisibleRecently() )
	{
		return nullptr;
	}

	const PathFollower *path = me->GetCurrentPath();
	if ( !path || !path->IsValid() )
	{
		return nullptr;
	}

	const Path::Segment *goal = path->GetCurrentGoal();
	if ( !goal )
	{
		return nullptr;
	}

	// Trace forward along the path to look for breakables
	// We use a hull trace to match the bot's collision size
	trace_t tr;
	UTIL_TraceHull( me->GetAbsOrigin() + Vector( 0, 0, 10 ),
					goal->pos + Vector( 0, 0, 10 ),
					me->GetBodyInterface()->GetHullMins(),
					me->GetBodyInterface()->GetHullMaxs(),
					MASK_NPCSOLID,
					me->GetEntity(),
					COLLISION_GROUP_NONE,
					&tr );

	if ( tr.DidHit() && tr.m_pEnt )
	{
		if ( me->IsAbleToBreak( tr.m_pEnt ) )
		{
			return tr.m_pEnt;
		}
	}

	return nullptr;
}


//--------------------------------------------------------------------------------------------------------
CNEOBotPathClearBreakable::CNEOBotPathClearBreakable( CBaseEntity *breakable )
{
	m_hBreakable = breakable;
}


//--------------------------------------------------------------------------------------------------------
ActionResult< CNEOBot > CNEOBotPathClearBreakable::OnStart( CNEOBot *me, Action< CNEOBot > *priorAction )
{
	m_isWaitingForFullReload = false;
	m_bDidSwitchWeapon = false;
	m_giveUpTimer.Start( 5.0f );
	m_meleeTimeoutTimer.Start( 2.0f );

	if ( !m_hBreakable.Get() )
	{
		return Done( "No breakable target" );
	}

	// Prioritize weapons for clearing breakables:
	// 1. Knife (Slot 2)
	// 2. Secondary (Slot 1) if it has ammo
	// 3. Current/Primary (Slot 0) as fallback

	auto *myWeapon = static_cast<CNEOBaseCombatWeapon *>( me->GetActiveWeapon() );
	auto *knife = static_cast<CNEOBaseCombatWeapon *>( me->Weapon_GetSlot( 2 ) );
	auto *secondaryWep = static_cast<CNEOBaseCombatWeapon *>( me->Weapon_GetSlot( 1 ) );

	if ( knife && knife != myWeapon )
	{
		me->Weapon_Switch( knife );
		m_bDidSwitchWeapon = true;
	}
	else if ( secondaryWep && secondaryWep != myWeapon &&
			( ( secondaryWep->Clip1() + secondaryWep->m_iPrimaryAmmoCount ) > 0 ) )
	{
		me->Weapon_Switch( secondaryWep );
		m_bDidSwitchWeapon = true;
	}

	return Continue();
}


//--------------------------------------------------------------------------------------------------------
ActionResult< CNEOBot > CNEOBotPathClearBreakable::Update( CNEOBot *me, float interval )
{
	CBaseEntity *breakable = m_hBreakable.Get();

	// If the breakable is destroyed or invalid, we're done
	if ( !breakable || !breakable->IsAlive() || breakable->GetHealth() <= 0 )
	{
		return Done( "Path is clear" );
	}

	if ( m_giveUpTimer.IsElapsed() )
	{
		return Done( "Give up timer elapsed" );
	}

	// Double check distance - if we've moved too far away or the breakable is too far, stop
	if ( me->GetRangeSquaredTo( breakable ) > Square( 500.0f ) )
	{
		return Done( "Breakable is too far" );
	}

	// If freeze time started, bail out
	if ( me->GetNeoFlags() & NEO_FL_FREEZETIME )
	{
		return Done( "Freeze time" );
	}

	// If a visible threat appeared, bail out and let the normal combat system handle it
	const CKnownEntity *threat = me->GetVisionInterface()->GetPrimaryKnownThreat();
	if ( threat && threat->GetEntity() && threat->IsVisibleRecently() )
	{
		return Done( "Threat appeared" );
	}

	auto *myWeapon = static_cast<CNEOBaseCombatWeapon *>( me->GetActiveWeapon() );
	if ( !myWeapon )
	{
		return Done( "No weapon" );
	}

	// Aim at the breakable
	me->GetBodyInterface()->AimHeadTowards(
		breakable->WorldSpaceCenter(), IBody::MANDATORY, 1.0f, nullptr,
		"Attacking breakable in path" );

	// Handle reloading if clip is empty
	bool bUsesClips = myWeapon->UsesClipsForAmmo1();
	if ( bUsesClips && myWeapon->Clip1() <= 0 )
	{
		me->ReleaseFireButton();
		me->PressReloadButton();
		m_isWaitingForFullReload = true;
	}

	if ( m_isWaitingForFullReload )
	{
		m_isWaitingForFullReload = ( bUsesClips && myWeapon->Clip1() < myWeapon->GetMaxClip1() );
	}

	// Fire at the breakable if aim is ready
	if ( !m_isWaitingForFullReload && me->GetBodyInterface()->IsHeadAimingOnTarget() )
	{
		// If we've been trying to melee for too long, switch to a firearm
		if ( m_meleeTimeoutTimer.IsElapsed() && ( myWeapon->GetNeoWepBits() & NEO_WEP_KNIFE ) )
		{
			auto *secondaryWep = static_cast<CNEOBaseCombatWeapon *>( me->Weapon_GetSlot( 1 ) );
			auto *primaryWeapon = static_cast<CNEOBaseCombatWeapon *>( me->Weapon_GetSlot( 0 ) );

			if ( secondaryWep && ( ( secondaryWep->Clip1() + secondaryWep->m_iPrimaryAmmoCount ) > 0 ) )
			{
				me->Weapon_Switch( secondaryWep );
				m_bDidSwitchWeapon = true;
			}
			else if ( primaryWeapon && ( ( primaryWeapon->Clip1() + primaryWeapon->m_iPrimaryAmmoCount ) > 0 ) )
			{
				me->Weapon_Switch( primaryWeapon );
				m_bDidSwitchWeapon = true;
			}
		}

		// Try to avoid hitting friends if clustered together
		me->AvoidBumpingFriends();
		// Prevent bot from missing breakable because of leaning
		me->ReleaseLeanLeftButton();
		me->ReleaseLeanRightButton();

		if ( me->IsContinuousFireWeapon( myWeapon ) )
		{
			me->PressFireButton( neo_bot_fire_at_breakable_weapon_min_time.GetFloat() );
		}
		else
		{
			if ( me->m_nButtons & IN_ATTACK )
			{
				me->ReleaseFireButton();
			}
			else
			{
				me->PressFireButton();
			}
		}
	}

	return Continue();
}


//--------------------------------------------------------------------------------------------------------
void CNEOBotPathClearBreakable::OnEnd( CNEOBot *me, Action< CNEOBot > *nextAction )
{
	// If we switched weapon, try to switch back to primary
	if ( m_bDidSwitchWeapon )
	{
		auto *myWeapon = static_cast<CNEOBaseCombatWeapon *>( me->GetActiveWeapon() );
		if ( !myWeapon ||
				( myWeapon->GetNeoWepBits() & ( NEO_WEP_MILSO | NEO_WEP_TACHI | NEO_WEP_KYLA | NEO_WEP_KNIFE | NEO_WEP_THROWABLE ) ) )
		{
			auto *primaryWeapon = static_cast<CNEOBaseCombatWeapon *>( me->Weapon_GetSlot( 0 ) );
			if ( primaryWeapon && ( primaryWeapon->Clip1() + primaryWeapon->m_iPrimaryAmmoCount ) > 0 )
			{
				me->Weapon_Switch( primaryWeapon );
			}
		}
	}
}
