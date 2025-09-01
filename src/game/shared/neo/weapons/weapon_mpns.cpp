#include "cbase.h"
#include "weapon_mpns.h"

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

IMPLEMENT_NETWORKCLASS_ALIASED(WeaponMPN_S, DT_WeaponMPN_S)

BEGIN_NETWORK_TABLE(CWeaponMPN_S, DT_WeaponMPN_S)
	DEFINE_NEO_BASE_WEP_NETWORK_TABLE
END_NETWORK_TABLE()

#ifdef CLIENT_DLL
BEGIN_PREDICTION_DATA(CWeaponMPN_S)
	DEFINE_NEO_BASE_WEP_PREDICTION
END_PREDICTION_DATA()
#endif

NEO_IMPLEMENT_ACTTABLE(CWeaponMPN_S)

LINK_ENTITY_TO_CLASS(weapon_mpn, CWeaponMPN_S);

PRECACHE_WEAPON_REGISTER(weapon_mpn);

CWeaponMPN_S::CWeaponMPN_S()
{
	m_flSoonestAttack = gpGlobals->curtime;
	m_flAccuracyPenalty = 0;

	m_nNumShotsFired = 0;

	m_weaponSeeds = {
		"mpnspx",
		"mpnspy",
		"mpnsrx",
		"mpnsry",
	};
}

bool CWeaponMPN_S::CanBePickedUpByClass(int classId)
{
	return classId != NEO_CLASS_JUGGERNAUT;
}