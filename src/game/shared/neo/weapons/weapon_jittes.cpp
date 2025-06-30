#include "cbase.h"
#include "weapon_jittes.h"

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

IMPLEMENT_NETWORKCLASS_ALIASED(WeaponJitteS, DT_WeaponJitteS)

BEGIN_NETWORK_TABLE(CWeaponJitteS, DT_WeaponJitteS)
	DEFINE_NEO_BASE_WEP_NETWORK_TABLE
END_NETWORK_TABLE()

#ifdef CLIENT_DLL
BEGIN_PREDICTION_DATA(CWeaponJitteS)
	DEFINE_NEO_BASE_WEP_PREDICTION
END_PREDICTION_DATA()
#endif

NEO_IMPLEMENT_ACTTABLE(CWeaponJitteS)

LINK_ENTITY_TO_CLASS(weapon_jittescoped, CWeaponJitteS);

PRECACHE_WEAPON_REGISTER(weapon_jittescoped);

CWeaponJitteS::CWeaponJitteS()
{
	m_flSoonestAttack = gpGlobals->curtime;
	m_flAccuracyPenalty = 0;

	m_nNumShotsFired = 0;

	m_weaponSeeds = {
		"jittespx",
		"jittespy",
		"jittesrx",
		"jittesry",
	};
}

bool CWeaponJitteS::CanBePickedUpByClass(int classId)
{
	return classId != NEO_CLASS_JUGGERNAUT;
}