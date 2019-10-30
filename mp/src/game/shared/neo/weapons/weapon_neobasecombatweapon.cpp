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

#ifdef GAME_DLL
BEGIN_DATADESC( CNEOBaseCombatWeapon )
END_DATADESC()
#endif

BEGIN_PREDICTION_DATA( CNEOBaseCombatWeapon )
END_PREDICTION_DATA()

const char *GetWeaponByLoadoutId(int id)
{
	if (id < 0 || id >= NEO_WEP_LOADOUT_ID_COUNT)
	{
		Assert(false);
		return "";
	}

	const char* weps[] = {
		"weapon_mpn",
		"weapon_srm",
		"weapon_srms",
		"weapon_jitte",
		"weapon_jittescoped",
		"weapon_zr68c",
		"weapon_zr68s",
		"weapon_zr68l",
		"weapon_mx",
		"weapon_pz",
		"weapon_supa7",
		"weapon_m41",
		"weapon_m41l",
	};

	COMPILE_TIME_ASSERT(NEO_WEP_LOADOUT_ID_COUNT == ARRAYSIZE(weps));

	return weps[id];
}

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

bool CNEOBaseCombatWeapon::CanBeSelected(void)
{
	if (GetWeaponFlags() & ITEM_FLAG_NOAUTOSWITCHEMPTY)
	{
		return true;
	}

	return BaseClass::CanBeSelected();
}
