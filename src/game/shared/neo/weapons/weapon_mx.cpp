#include "cbase.h"
#include "weapon_mx.h"

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

IMPLEMENT_NETWORKCLASS_ALIASED(WeaponMX, DT_WeaponMX)

BEGIN_NETWORK_TABLE(CWeaponMX, DT_WeaponMX)
	DEFINE_NEO_BASE_WEP_NETWORK_TABLE
END_NETWORK_TABLE()

#ifdef CLIENT_DLL
BEGIN_PREDICTION_DATA(CWeaponMX)
	DEFINE_NEO_BASE_WEP_PREDICTION
END_PREDICTION_DATA()
#endif

NEO_IMPLEMENT_ACTTABLE(CWeaponMX)

LINK_ENTITY_TO_CLASS(weapon_mx, CWeaponMX);

PRECACHE_WEAPON_REGISTER(weapon_mx);

CWeaponMX::CWeaponMX()
{
	m_flSoonestAttack = gpGlobals->curtime;
	m_flAccuracyPenalty = 0;

	m_nNumShotsFired = 0;

	m_weaponSeeds = {
		"mxpx",
		"mxpy",
		"mxrx",
		"mxry",
	};
}

bool CWeaponMX::CanBePickedUpByClass(int classId)
{
	return classId != NEO_CLASS_JUGGERNAUT;
}