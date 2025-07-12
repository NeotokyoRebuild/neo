#include "cbase.h"
#include "weapon_m41s.h"

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

IMPLEMENT_NETWORKCLASS_ALIASED(WeaponM41S, DT_WeaponM41S)

BEGIN_NETWORK_TABLE(CWeaponM41S, DT_WeaponM41S)
	DEFINE_NEO_BASE_WEP_NETWORK_TABLE
END_NETWORK_TABLE()

#ifdef CLIENT_DLL
BEGIN_PREDICTION_DATA(CWeaponM41S)
	DEFINE_NEO_BASE_WEP_PREDICTION
END_PREDICTION_DATA()
#endif

NEO_IMPLEMENT_ACTTABLE(CWeaponM41S)

LINK_ENTITY_TO_CLASS(weapon_m41s, CWeaponM41S);

PRECACHE_WEAPON_REGISTER(weapon_m41s);

CWeaponM41S::CWeaponM41S()
{
	m_flSoonestAttack = gpGlobals->curtime;
	m_flAccuracyPenalty = 0;

	m_nNumShotsFired = 0;

	m_weaponSeeds = {
		"m41spx",
		"m41spy",
		"m41srx",
		"m41sry",
	};
}

bool CWeaponM41S::CanBePickedUpByClass(int classId)
{
	return classId != NEO_CLASS_JUGGERNAUT;
}