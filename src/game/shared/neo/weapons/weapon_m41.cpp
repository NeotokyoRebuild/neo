#include "cbase.h"
#include "weapon_m41.h"

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

IMPLEMENT_NETWORKCLASS_ALIASED(WeaponM41, DT_WeaponM41)

BEGIN_NETWORK_TABLE(CWeaponM41, DT_WeaponM41)
	DEFINE_NEO_BASE_WEP_NETWORK_TABLE
END_NETWORK_TABLE()

#ifdef CLIENT_DLL
BEGIN_PREDICTION_DATA(CWeaponM41)
	DEFINE_NEO_BASE_WEP_PREDICTION
END_PREDICTION_DATA()
#endif

NEO_IMPLEMENT_ACTTABLE(CWeaponM41)

LINK_ENTITY_TO_CLASS(weapon_m41, CWeaponM41);

PRECACHE_WEAPON_REGISTER(weapon_m41);

CWeaponM41::CWeaponM41()
{
	m_flSoonestAttack = gpGlobals->curtime;
	m_flAccuracyPenalty = 0;

	m_nNumShotsFired = 0;

	m_weaponSeeds = {
		"m41px",
		"m41py",
		"m41rx",
		"m41ry",
	};
}

bool CWeaponM41::CanBePickedUpByClass(int classId)
{
	return classId != NEO_CLASS_JUGGERNAUT;
}