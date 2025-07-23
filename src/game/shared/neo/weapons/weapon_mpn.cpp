#include "cbase.h"
#include "weapon_mpn.h"

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

IMPLEMENT_NETWORKCLASS_ALIASED(WeaponMPN, DT_WeaponMPN)

BEGIN_NETWORK_TABLE(CWeaponMPN, DT_WeaponMPN)
	DEFINE_NEO_BASE_WEP_NETWORK_TABLE
END_NETWORK_TABLE()

#ifdef CLIENT_DLL
BEGIN_PREDICTION_DATA(CWeaponMPN)
	DEFINE_NEO_BASE_WEP_PREDICTION
END_PREDICTION_DATA()
#endif

NEO_IMPLEMENT_ACTTABLE(CWeaponMPN)

LINK_ENTITY_TO_CLASS(weapon_mpn_unsilenced, CWeaponMPN);

PRECACHE_WEAPON_REGISTER(weapon_mpn_unsilenced);

CWeaponMPN::CWeaponMPN()
{
	m_flSoonestAttack = gpGlobals->curtime;
	m_flAccuracyPenalty = 0;

	m_nNumShotsFired = 0;

	m_weaponSeeds = {
		"mpnpx",
		"mpnpy",
		"mpnrx",
		"mpnry",
	};
}

bool CWeaponMPN::CanBePickedUpByClass(int classId)
{
	return classId != NEO_CLASS_JUGGERNAUT;
}