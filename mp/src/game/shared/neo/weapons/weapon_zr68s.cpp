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
}

void CWeaponZR68S::AddViewKick()
{
	auto owner = ToBasePlayer(GetOwner());

	if (!owner)
	{
		return;
	}

	QAngle viewPunch;

	viewPunch.x = SharedRandomFloat("zr68spx", 0.25f, 0.5f);
	viewPunch.y = SharedRandomFloat("zr68spy", -0.6f, 0.6f);
	viewPunch.z = 0;

	owner->ViewPunch(viewPunch);
}
