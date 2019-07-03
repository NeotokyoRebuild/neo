#include "cbase.h"
#include "weapon_neobasecombatweapon.h"

#include "in_buttons.h"

#ifdef GAME_DLL
#include "player.h"
#endif

#include "basecombatweapon_shared.h"

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

LINK_ENTITY_TO_CLASS( neobasecombatweapon, CNEOBaseCombatWeapon );

IMPLEMENT_NETWORKCLASS_ALIASED( NEOBaseCombatWeapon, DT_NEOBaseCombatWeapon )

BEGIN_NETWORK_TABLE( CNEOBaseCombatWeapon, DT_NEOBaseCombatWeapon )
END_NETWORK_TABLE()

#ifndef CLIENT_DLL

BEGIN_DATADESC( CNEOBaseCombatWeapon )
END_DATADESC()

#endif

BEGIN_PREDICTION_DATA( CNEOBaseCombatWeapon )
END_PREDICTION_DATA()

CNEOBaseCombatWeapon::CNEOBaseCombatWeapon( void )
{
	
}

void CNEOBaseCombatWeapon::Spawn()
{
	BaseClass::Spawn();

#ifdef GAME_DLL
	AddSpawnFlags(SF_NORESPAWN);
#endif
}

#ifdef CLIENT_DLL
extern ConVar cl_autoreload_when_empty;
#endif
bool CNEOBaseCombatWeapon::Reload( void )
{
	CBasePlayer *pOwner = ToBasePlayer( GetOwner() );
	if (!pOwner)
	{
		return false;
	}

	if (pOwner->m_afButtonPressed & IN_RELOAD)
	{
		return DefaultReload( GetMaxClip1(), GetMaxClip2(), ACT_VM_RELOAD );
	}

#ifdef CLIENT_DLL
	if (!ClientWantsAutoReload())
	{
		return false;
	}
#else
	if (!ClientWantsAutoReload(pOwner))
	{
		return false;
	}
#endif

	return DefaultReload( GetMaxClip1(), GetMaxClip2(), ACT_VM_RELOAD );
}