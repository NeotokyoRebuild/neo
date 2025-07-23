#include "cbase.h"
#include "weapon_zr68s.h"

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

IMPLEMENT_NETWORKCLASS_ALIASED(WeaponZR68S, DT_WeaponZR68S)

BEGIN_NETWORK_TABLE(CWeaponZR68S, DT_WeaponZR68S)
	DEFINE_NEO_BASE_WEP_NETWORK_TABLE
END_NETWORK_TABLE()

#ifdef CLIENT_DLL
BEGIN_PREDICTION_DATA(CWeaponZR68S)
	DEFINE_NEO_BASE_WEP_PREDICTION
END_PREDICTION_DATA()
#endif

NEO_IMPLEMENT_ACTTABLE(CWeaponZR68S)

LINK_ENTITY_TO_CLASS(weapon_zr68s, CWeaponZR68S);

PRECACHE_WEAPON_REGISTER(weapon_zr68s);

CWeaponZR68S::CWeaponZR68S()
{
	m_flSoonestAttack = gpGlobals->curtime;
	m_flAccuracyPenalty = 0;

	m_nNumShotsFired = 0;

	m_weaponSeeds = {
		"zr68spx",
		"zr68spy",
		"zr68srx",
		"zr68sry",
	};
}

bool CWeaponZR68S::CanBePickedUpByClass(int classId)
{
	return classId != NEO_CLASS_JUGGERNAUT;
}