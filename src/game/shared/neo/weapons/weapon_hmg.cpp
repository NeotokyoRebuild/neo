#include "cbase.h"
#include "weapon_hmg.h"

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

IMPLEMENT_NETWORKCLASS_ALIASED(WeaponHMG, DT_WeaponHMG)

BEGIN_NETWORK_TABLE(CWeaponHMG, DT_WeaponHMG)
	DEFINE_NEO_BASE_WEP_NETWORK_TABLE
END_NETWORK_TABLE()

#ifdef CLIENT_DLL
BEGIN_PREDICTION_DATA(CWeaponHMG)
	DEFINE_NEO_BASE_WEP_PREDICTION
END_PREDICTION_DATA()
#endif

NEO_IMPLEMENT_ACTTABLE(CWeaponHMG)

LINK_ENTITY_TO_CLASS(weapon_hmg, CWeaponHMG);

PRECACHE_WEAPON_REGISTER(weapon_hmg);

#define HMG_COOLING_RATE 0.35f
#define HMG_OVERHEAT_DURATION 5.0f

CWeaponHMG::CWeaponHMG()
{
	m_flSoonestAttack = gpGlobals->curtime;
	m_flAccuracyPenalty = 0;

	m_nNumShotsFired = 0;

	m_weaponSeeds = {
		"hmgpx",
		"hmgpy",
		"hmgrx",
		"hmgry",
	};
}

void CWeaponHMG::Spawn(void)
{
	BaseClass::Spawn();

	SetThink(&CWeaponHMG::Think);
	SetNextThink(gpGlobals->curtime + HMG_COOLING_RATE);
}

bool CWeaponHMG::CanBePickedUpByClass(int classId)
{
	return classId == NEO_CLASS_JUGGERNAUT;
}

void CWeaponHMG::PrimaryAttack(void)
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
		return;
	}
}

void CWeaponHMG::Think(void)
{
	if (m_bOverheated)
	{
		float flTimeOverheated = gpGlobals->curtime - m_flOverheatStartTime;
		if (flTimeOverheated >= HMG_OVERHEAT_DURATION)
		{
			SetPrimaryAmmoCount(GetDefaultClip1());
			m_bOverheated = false;
		}
	}
	else if (GetPrimaryAmmoCount() < GetDefaultClip1())
	{
		SetPrimaryAmmoCount(GetPrimaryAmmoCount() + 1);
	}

	SetNextThink(gpGlobals->curtime + HMG_COOLING_RATE);
}