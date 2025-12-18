#include "cbase.h"
#include "neo_player.h"
#include "bot/neo_bot.h"
#include "bot/behavior/neo_bot_seek_weapon.h"
#include "bot/neo_bot_path_compute.h"

//---------------------------------------------------------------------------------------------
CBaseEntity *FindNearestPrimaryWeapon( const Vector &searchOrigin, bool bShortCircuit )
{
	constexpr float flSearchRadius = 1000.0f;
	CBaseEntity *pClosestWeapon = NULL;
	float flClosestDistSq = FLT_MAX;

	// Iterate through all available weapons, looking for the nearest primary 
	CBaseEntity *pEntity = NULL;
	while ( ( pEntity = gEntList.FindEntityByClassnameWithin( pEntity, "weapon_*", searchOrigin, flSearchRadius ) ) != NULL )
	{
		CBaseCombatWeapon *pWeapon = pEntity->MyCombatWeaponPointer();
		if ( pWeapon && pWeapon->GetOwner() == NULL && pWeapon->HasAnyAmmo() && pWeapon->GetSlot() == 0 )
		{
			// Prioritize getting any primary class weapon that is NOT the ghost
			if ( FStrEq( pEntity->GetClassname(), "weapon_ghost" ) )
			{
				continue;
			}
			
			if ( bShortCircuit )
			{
				return pEntity;
			}

			float flDistSq = searchOrigin.DistToSqr( pEntity->GetAbsOrigin() );
			if ( flDistSq < flClosestDistSq )
			{
				flClosestDistSq = flDistSq;
				pClosestWeapon = pEntity;
			}
		}
	}

	return pClosestWeapon;
}

//---------------------------------------------------------------------------------------------
CNEOBotSeekWeapon::CNEOBotSeekWeapon( void )
{
	m_hTargetWeapon = NULL;
}

//---------------------------------------------------------------------------------------------
CBaseEntity *CNEOBotSeekWeapon::FindAndPathToWeapon( CNEOBot *me )
{
	CBaseEntity *pClosestWeapon = FindNearestPrimaryWeapon( me->GetAbsOrigin() );
	
	if (pClosestWeapon)
	{
		m_hTargetWeapon = pClosestWeapon;
		CNEOBotPathCompute(me, m_path, m_hTargetWeapon->GetAbsOrigin(), FASTEST_ROUTE);
	}
	else
	{
		// no weapon found
		m_hTargetWeapon = NULL;
		m_path.Invalidate();

	}

	return pClosestWeapon;
}

//---------------------------------------------------------------------------------------------
ActionResult< CNEOBot >	CNEOBotSeekWeapon::OnStart( CNEOBot *me, Action< CNEOBot > *priorAction )
{
	CBaseCombatWeapon *pPrimary = me->Weapon_GetSlot(0);
	if ( pPrimary )
	{
		if ( pPrimary->HasAnyAmmo() )
		{
			// so we don't exit this behavior still equipped with secondary
			me->Weapon_Switch(pPrimary);
		}
		return Done("Already have a primary weapon");
	}

	if ( !FindAndPathToWeapon(me) )
	{
		return Done("No replacement primary found");
	}

	// found a replacement, go towards it next frame
	return Continue();
}

//---------------------------------------------------------------------------------------------
ActionResult< CNEOBot >	CNEOBotSeekWeapon::Update( CNEOBot *me, float interval )
{
	CBaseCombatWeapon *pPrimary = me->Weapon_GetSlot(0);
	if ( pPrimary )
	{
		if ( pPrimary->HasAnyAmmo() )
		{
			// so we don't exit this behavior still equipped with secondary
			me->Weapon_Switch(pPrimary);
		}
		return Done("Acquired a primary weapon");
	}
	
	if (m_hTargetWeapon == NULL)
	{
		return Done("No weapon to seek");
	}

	if (!m_path.IsValid())
	{
		return Done("Path to weapon is invalid");
	}

	m_path.Update(me);

	return Continue();
}

//---------------------------------------------------------------------------------------------
ActionResult< CNEOBot > CNEOBotSeekWeapon::OnResume( CNEOBot *me, Action< CNEOBot > *interruptingAction )
{
	return OnStart(me, interruptingAction);
}

//---------------------------------------------------------------------------------------------
EventDesiredResult< CNEOBot > CNEOBotSeekWeapon::OnStuck( CNEOBot *me )
{
	FindAndPathToWeapon(me);
	return TryContinue();
}

//---------------------------------------------------------------------------------------------
EventDesiredResult< CNEOBot > CNEOBotSeekWeapon::OnMoveToSuccess( CNEOBot *me, const Path *path )
{
	// a weapon should have been picked up
	return TryDone();
}

//---------------------------------------------------------------------------------------------
EventDesiredResult< CNEOBot > CNEOBotSeekWeapon::OnMoveToFailure( CNEOBot *me, const Path *path, MoveToFailureType reason )
{
	FindAndPathToWeapon(me);
	return TryContinue();
}
