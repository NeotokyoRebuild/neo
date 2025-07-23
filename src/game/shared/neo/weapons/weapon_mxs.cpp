#include "cbase.h"
#include "weapon_mxs.h"

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

IMPLEMENT_NETWORKCLASS_ALIASED(WeaponMX_S, DT_WeaponMX_S)

BEGIN_NETWORK_TABLE(CWeaponMX_S, DT_WeaponMX_S)
	DEFINE_NEO_BASE_WEP_NETWORK_TABLE
END_NETWORK_TABLE()

#ifdef CLIENT_DLL
BEGIN_PREDICTION_DATA(CWeaponMX_S)
	DEFINE_NEO_BASE_WEP_PREDICTION
END_PREDICTION_DATA()
#endif

NEO_IMPLEMENT_ACTTABLE(CWeaponMX_S)

LINK_ENTITY_TO_CLASS(weapon_mx_silenced, CWeaponMX_S);

PRECACHE_WEAPON_REGISTER(weapon_mx_silenced);

CWeaponMX_S::CWeaponMX_S()
{
	m_flSoonestAttack = gpGlobals->curtime;
	m_flAccuracyPenalty = 0;

	m_nNumShotsFired = 0;

	m_weaponSeeds = {
		"mxspx",
		"mxspy",
		"mxsrx",
		"mxsry",
	};
}

bool CWeaponMX_S::CanBePickedUpByClass(int classId)
{
	return classId != NEO_CLASS_JUGGERNAUT;
}