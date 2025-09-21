#include "cbase.h"
#include "weapon_srms.h"

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

IMPLEMENT_NETWORKCLASS_ALIASED(WeaponSRM_S, DT_WeaponSRM_S)

BEGIN_NETWORK_TABLE(CWeaponSRM_S, DT_WeaponSRM_S)
	DEFINE_NEO_BASE_WEP_NETWORK_TABLE
END_NETWORK_TABLE()

#ifdef CLIENT_DLL
BEGIN_PREDICTION_DATA(CWeaponSRM_S)
	DEFINE_NEO_BASE_WEP_PREDICTION
END_PREDICTION_DATA()
#endif

NEO_IMPLEMENT_ACTTABLE(CWeaponSRM_S)

LINK_ENTITY_TO_CLASS(weapon_srm_s, CWeaponSRM_S);

PRECACHE_WEAPON_REGISTER(weapon_srm_s);

CWeaponSRM_S::CWeaponSRM_S()
{
	m_flSoonestAttack = gpGlobals->curtime;
	m_flAccuracyPenalty = 0;

	m_nNumShotsFired = 0;

	m_weaponSeeds = {
		"srmspx",
		"srmspy",
		"srmsrx",
		"srmsry",
	};
}

bool CWeaponSRM_S::CanBePickedUpByClass(int classId)
{
	return classId != NEO_CLASS_JUGGERNAUT;
}