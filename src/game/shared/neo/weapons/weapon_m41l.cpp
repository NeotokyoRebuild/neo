#include "cbase.h"
#include "weapon_m41l.h"

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

IMPLEMENT_NETWORKCLASS_ALIASED(WeaponM41L, DT_WeaponM41L)

BEGIN_NETWORK_TABLE(CWeaponM41L, DT_WeaponM41L)
	DEFINE_NEO_BASE_WEP_NETWORK_TABLE
END_NETWORK_TABLE()

#ifdef CLIENT_DLL
BEGIN_PREDICTION_DATA(CWeaponM41L)
	DEFINE_NEO_BASE_WEP_PREDICTION
END_PREDICTION_DATA()
#endif

NEO_IMPLEMENT_ACTTABLE(CWeaponM41L)

LINK_ENTITY_TO_CLASS(weapon_m41l, CWeaponM41L);

PRECACHE_WEAPON_REGISTER(weapon_m41l);

CWeaponM41L::CWeaponM41L()
{
	m_flSoonestAttack = gpGlobals->curtime;
	m_flAccuracyPenalty = 0;

	m_nNumShotsFired = 0;

	m_weaponSeeds = {
		"m41lpx",
		"m41lpy",
		"m41lrx",
		"m41lry",
	};
}

bool CWeaponM41L::CanBePickedUpByClass(int classId)
{
	return classId != NEO_CLASS_JUGGERNAUT;
}