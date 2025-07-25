#include "cbase.h"
#include "bot/neo_bot.h"
#include "bot/behavior/neo_bot_melee_attack.h"

#include "nav_mesh.h"

extern ConVar neo_bot_path_lookahead_range;

ConVar neo_bot_melee_attack_abandon_range( "neo_bot_melee_attack_abandon_range", "500", FCVAR_CHEAT, "If threat is farther away than this, bot will switch back to its primary weapon and attack" );


//---------------------------------------------------------------------------------------------
CNEOBotMeleeAttack::CNEOBotMeleeAttack( float giveUpRange )
{
	m_giveUpRange = giveUpRange < 0.0f ? neo_bot_melee_attack_abandon_range.GetFloat() : giveUpRange;
}


//---------------------------------------------------------------------------------------------
ActionResult< CNEOBot >	CNEOBotMeleeAttack::OnStart( CNEOBot *me, Action< CNEOBot > *priorAction )
{
	m_path.SetMinLookAheadDistance( me->GetDesiredPathLookAheadRange() );

	return Continue();
}


//---------------------------------------------------------------------------------------------
ActionResult< CNEOBot >	CNEOBotMeleeAttack::Update( CNEOBot *me, float interval )
{
	// bash the bad guys
	const CKnownEntity *threat = me->GetVisionInterface()->GetPrimaryKnownThreat();

	if ( threat == NULL )
	{
		return Done( "No threat" );
	}

	if ( me->IsDistanceBetweenGreaterThan( threat->GetLastKnownPosition(), m_giveUpRange ) )
	{
		// threat is too far away for melee
		return Done( "Threat is too far away for a melee attack" );
	}

	// switch to our melee weapon
	CNEOBaseCombatWeapon*meleeWeapon = me->GetBludgeonWeapon();

	if ( !meleeWeapon )
	{
		// misyl: TF nextbot is missing this check... Interesting.
		return Done( "Don't have a melee weapon!" );
	}

	me->Weapon_Switch( meleeWeapon );

	// actual head aiming is handled elsewhere

	// just keep swinging
	me->PressFireButton();

	// chase them down
	CNEOBotPathCost cost( me, FASTEST_ROUTE );
	m_path.Update( me, threat->GetEntity(), cost );

	return Continue();
}
