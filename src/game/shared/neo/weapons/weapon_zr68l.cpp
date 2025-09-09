#include "cbase.h"
#include "weapon_zr68l.h"

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

IMPLEMENT_NETWORKCLASS_ALIASED(WeaponZR68L, DT_WeaponZR68L)

BEGIN_NETWORK_TABLE(CWeaponZR68L, DT_WeaponZR68L)
	DEFINE_NEO_BASE_WEP_NETWORK_TABLE
END_NETWORK_TABLE()

#ifdef CLIENT_DLL
BEGIN_PREDICTION_DATA(CWeaponZR68L)
	DEFINE_NEO_BASE_WEP_PREDICTION
END_PREDICTION_DATA()
#endif

NEO_IMPLEMENT_ACTTABLE(CWeaponZR68L)

LINK_ENTITY_TO_CLASS(weapon_zr68l, CWeaponZR68L);

PRECACHE_WEAPON_REGISTER(weapon_zr68l);

CWeaponZR68L::CWeaponZR68L()
{
	m_flSoonestAttack = gpGlobals->curtime;
	m_flAccuracyPenalty = 0;

	m_nNumShotsFired = 0;

	m_weaponSeeds = {
		"zr68lpx",
		"zr68lpy",
		"zr68lrx",
		"zr68lry",
	};
}

bool CWeaponZR68L::CanBePickedUpByClass(int classId)
{
	return classId != NEO_CLASS_JUGGERNAUT;
}