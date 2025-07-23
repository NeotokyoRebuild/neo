#include "cbase.h"
#include "weapon_srm.h"

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

IMPLEMENT_NETWORKCLASS_ALIASED(WeaponSRM, DT_WeaponSRM)

BEGIN_NETWORK_TABLE(CWeaponSRM, DT_WeaponSRM)
	DEFINE_NEO_BASE_WEP_NETWORK_TABLE
END_NETWORK_TABLE()

#ifdef CLIENT_DLL
BEGIN_PREDICTION_DATA(CWeaponSRM)
	DEFINE_NEO_BASE_WEP_PREDICTION
END_PREDICTION_DATA()
#endif

NEO_IMPLEMENT_ACTTABLE(CWeaponSRM)

LINK_ENTITY_TO_CLASS(weapon_srm, CWeaponSRM);

PRECACHE_WEAPON_REGISTER(weapon_srm);

CWeaponSRM::CWeaponSRM()
{
	m_flSoonestAttack = gpGlobals->curtime;
	m_flAccuracyPenalty = 0;

	m_nNumShotsFired = 0;

	m_weaponSeeds = {
		"srmpx",
		"srmpy",
		"srmrx",
		"srmry",
	};
}

bool CWeaponSRM::CanBePickedUpByClass(int classId)
{
	return classId != NEO_CLASS_JUGGERNAUT;
}