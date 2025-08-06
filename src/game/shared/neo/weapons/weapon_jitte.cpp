#include "cbase.h"
#include "weapon_jitte.h"

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

IMPLEMENT_NETWORKCLASS_ALIASED(WeaponJitte, DT_WeaponJitte)

BEGIN_NETWORK_TABLE(CWeaponJitte, DT_WeaponJitte)
	DEFINE_NEO_BASE_WEP_NETWORK_TABLE
END_NETWORK_TABLE()

#ifdef CLIENT_DLL
BEGIN_PREDICTION_DATA(CWeaponJitte)
	DEFINE_NEO_BASE_WEP_PREDICTION
END_PREDICTION_DATA()
#endif

NEO_IMPLEMENT_ACTTABLE(CWeaponJitte)

LINK_ENTITY_TO_CLASS(weapon_jitte, CWeaponJitte);

PRECACHE_WEAPON_REGISTER(weapon_jitte);

CWeaponJitte::CWeaponJitte()
{
	m_flSoonestAttack = gpGlobals->curtime;
	m_flAccuracyPenalty = 0;

	m_nNumShotsFired = 0;

	m_weaponSeeds = {
		"jittepx",
		"jittepy",
		"jitterx",
		"jittery",
	};
}

bool CWeaponJitte::CanBePickedUpByClass(int classId)
{
	return classId != NEO_CLASS_JUGGERNAUT;
}