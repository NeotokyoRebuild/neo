#include "cbase.h"
#include "weapon_balc.h"

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

IMPLEMENT_NETWORKCLASS_ALIASED(WeaponBALC, DT_WeaponBALC)

BEGIN_NETWORK_TABLE(CWeaponBALC, DT_WeaponBALC)
	DEFINE_NEO_BASE_WEP_NETWORK_TABLE
END_NETWORK_TABLE()

#ifdef CLIENT_DLL
BEGIN_PREDICTION_DATA(CWeaponBALC)
	DEFINE_NEO_BASE_WEP_PREDICTION
END_PREDICTION_DATA()
#endif

NEO_IMPLEMENT_ACTTABLE(CWeaponBALC)

LINK_ENTITY_TO_CLASS(weapon_balc, CWeaponBALC);

PRECACHE_WEAPON_REGISTER(weapon_balc);

#define BALC_COOLING_RATE 0.35f
#define BALC_OVERHEAT_DURATION 5.0f

CWeaponBALC::CWeaponBALC()
{
	m_flSoonestAttack = gpGlobals->curtime;
	m_flAccuracyPenalty = 0;

	m_nNumShotsFired = 0;

	m_weaponSeeds = {
		"balcpx",
		"balcpy",
		"balcrx",
		"balcry",
	};
}

void CWeaponBALC::Spawn(void)
{
	BaseClass::Spawn();

	SetThink(&CWeaponBALC::Think);
	SetNextThink(gpGlobals->curtime + BALC_COOLING_RATE);
}

bool CWeaponBALC::CanBePickedUpByClass(int classId)
{
	return classId == NEO_CLASS_JUGGERNAUT;
}

void CWeaponBALC::PrimaryAttack(void)
{
	if (m_bOverheated)
	{
		return;
	}

	BaseClass::PrimaryAttack();

	if (m_iPrimaryAmmoCount <= 0 && !m_bOverheated)
	{
		m_bOverheated = true;
		m_flOverheatStartTime = gpGlobals->curtime;
		WeaponSound(SPECIAL1);
		return;
	}
}

void CWeaponBALC::Think(void)
{
	if (m_bOverheated)
	{
		float flTimeOverheated = gpGlobals->curtime - m_flOverheatStartTime;
		if (flTimeOverheated >= BALC_OVERHEAT_DURATION)
		{
			SetPrimaryAmmoCount(GetDefaultClip1());
			m_bOverheated = false;
			WeaponSound(SPECIAL2);
		}
	}
	else if (GetPrimaryAmmoCount() < GetDefaultClip1())
	{
		SetPrimaryAmmoCount(GetPrimaryAmmoCount() + 1);
	}

	SetNextThink(gpGlobals->curtime + BALC_COOLING_RATE);
}