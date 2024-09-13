#include "cbase.h"
#include "weapon_pz.h"

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

IMPLEMENT_NETWORKCLASS_ALIASED(WeaponPZ, DT_WeaponPZ)

BEGIN_NETWORK_TABLE(CWeaponPZ, DT_WeaponPZ)
	DEFINE_NEO_BASE_WEP_NETWORK_TABLE
END_NETWORK_TABLE()

#ifdef CLIENT_DLL
BEGIN_PREDICTION_DATA(CWeaponPZ)
	DEFINE_NEO_BASE_WEP_PREDICTION
END_PREDICTION_DATA()
#endif

NEO_IMPLEMENT_ACTTABLE(CWeaponPZ)

LINK_ENTITY_TO_CLASS(weapon_pz, CWeaponPZ);

PRECACHE_WEAPON_REGISTER(weapon_pz);

CWeaponPZ::CWeaponPZ()
{
	m_flSoonestAttack = gpGlobals->curtime;
	m_flAccuracyPenalty = 0;

	m_nNumShotsFired = 0;
}

bool CWeaponPZ::CanBePickedUpByClass(int classId)
{
	return classId != NEO_CLASS_RECON;
}

void CWeaponPZ::AddViewKick()
{
	auto owner = ToBasePlayer(GetOwner());

	if (!owner)
	{
		return;
	}

	QAngle viewPunch;

	viewPunch.x = SharedRandomFloat("pzpx", 0.25f, 0.5f);
	viewPunch.y = SharedRandomFloat("pzpy", -0.6f, 0.6f);
	viewPunch.z = 0;

	owner->ViewPunch(viewPunch);
}
