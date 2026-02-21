#include "cbase.h"
#include "bot/neo_bot.h"
#include "neo_player.h"
#include "bot/behavior/neo_bot_throw_weapon_at_player.h"

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

//---------------------------------------------------------------------------------------------
CNEOBotThrowWeaponAtPlayer::CNEOBotThrowWeaponAtPlayer( CNEO_Player *pTargetPlayer )
{
	m_hTargetPlayer = pTargetPlayer;
}

//---------------------------------------------------------------------------------------------
ActionResult< CNEOBot >	CNEOBotThrowWeaponAtPlayer::OnStart( CNEOBot *me, Action< CNEOBot > *priorAction )
{
	if ( !m_hTargetPlayer )
	{
		return Done( "No target player to throw weapon at" );
	}

	return Continue();
}

//---------------------------------------------------------------------------------------------
ActionResult< CNEOBot >	CNEOBotThrowWeaponAtPlayer::Update( CNEOBot *me, float interval )
{
	CNEO_Player *pTarget = m_hTargetPlayer.Get();
	if ( !pTarget || !pTarget->IsAlive() )
	{
		return Done( "Target player lost or died" );
	}

	me->GetBodyInterface()->AimHeadTowards( pTarget->GetAbsOrigin(), IBody::MANDATORY, 0.1f, NULL, "Aiming at player's feet to throw weapon" );

	CBaseCombatWeapon *pPrimary = me->Weapon_GetSlot( 0 );
	if ( pPrimary )
	{
		if ( me->GetActiveWeapon() != pPrimary )
		{
			me->Weapon_Switch( pPrimary );
			return Continue(); // Wait for switch
		}
	}
	else
	{
		return Done( "No primary weapon to throw" );
	}

	if ( me->GetBodyInterface()->IsHeadAimingOnTarget() )
	{
		me->PressDropButton();
		return Done("Weapon dropped");
	}

	return Continue();
}

//---------------------------------------------------------------------------------------------
void CNEOBotThrowWeaponAtPlayer::OnEnd( CNEOBot *me, Action< CNEOBot > *nextAction )
{
}
