#include "cbase.h"
#include "weapon_zr68c.h"

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

IMPLEMENT_NETWORKCLASS_ALIASED(WeaponZR68C, DT_WeaponZR68C)

BEGIN_NETWORK_TABLE(CWeaponZR68C, DT_WeaponZR68C)
	DEFINE_NEO_BASE_WEP_NETWORK_TABLE
END_NETWORK_TABLE()

#ifdef CLIENT_DLL
BEGIN_PREDICTION_DATA(CWeaponZR68C)
	DEFINE_NEO_BASE_WEP_PREDICTION
END_PREDICTION_DATA()
#endif

NEO_IMPLEMENT_ACTTABLE(CWeaponZR68C)

LINK_ENTITY_TO_CLASS(weapon_zr68c, CWeaponZR68C);

PRECACHE_WEAPON_REGISTER(weapon_zr68c);

CWeaponZR68C::CWeaponZR68C()
{
	m_flSoonestAttack = gpGlobals->curtime;
	m_flAccuracyPenalty = 0;

	m_nNumShotsFired = 0;

	m_weaponSeeds = {
		"z68cpx",
		"z68cpy",
		"z68crx",
		"z68cry",
	};
}

bool CWeaponZR68C::CanBePickedUpByClass(int classId)
{
	return classId != NEO_CLASS_JUGGERNAUT;
}