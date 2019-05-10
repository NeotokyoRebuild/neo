#include "cbase.h"
#include "weapon_neobasecombatweapon.h"

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