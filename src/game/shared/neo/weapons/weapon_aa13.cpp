#include "cbase.h"
#include "weapon_aa13.h"

#ifdef CLIENT_DLL
#include "c_neo_player.h"
#else
#include "neo_player.h"
#endif

#include "dt_recv.h"
#include "in_buttons.h"

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

#if defined(CLIENT_DLL) && !defined(CNEO_Player)
#define CNEO_Player C_NEO_Player
#endif

IMPLEMENT_NETWORKCLASS_ALIASED(WeaponAA13, DT_WeaponAA13)

BEGIN_NETWORK_TABLE(CWeaponAA13, DT_WeaponAA13)
	DEFINE_NEO_BASE_WEP_NETWORK_TABLE
END_NETWORK_TABLE()

#ifdef CLIENT_DLL
BEGIN_PREDICTION_DATA(CWeaponAA13)
	DEFINE_NEO_BASE_WEP_PREDICTION
END_PREDICTION_DATA()
#endif

NEO_IMPLEMENT_ACTTABLE(CWeaponAA13)

LINK_ENTITY_TO_CLASS(weapon_aa13, CWeaponAA13);

PRECACHE_WEAPON_REGISTER(weapon_aa13);

CWeaponAA13::CWeaponAA13(void)
{
	m_flSoonestAttack = gpGlobals->curtime;
	// m_flAccuracyPenalty = 0.0f;

	// m_nNumShotsFired = 0;

	m_weaponSeeds = {
		"aa13px",
		"aa13py",
		"aa13rx",
		"aa13ry",
	};
}

void CWeaponAA13::PrimaryAttack(void)
{
	// Combo of the neobasecombatweapon and the Supa7 attack
	if (ShootingIsPrevented())
	{
		return;
	}
	if (gpGlobals->curtime < m_flSoonestAttack)
	{
		return;
	}
	else if (gpGlobals->curtime - m_flLastAttackTime < GetFireRate())
	{
		return;
	}
	else if (m_iClip1 == 0)
	{
		if (!m_bFireOnEmpty)
		{
			CheckReload();
		}
		else
		{
			DryFire();
		}
		return;
	}

	// Only the player fires this way so we can cast
	auto* pPlayer = static_cast<CNEO_Player*>(GetOwner());

	if (!pPlayer)
	{
		return;
	}

	// MUST call sound before removing a round from the clip of a CMachineGun
	WeaponSound(SINGLE);

	pPlayer->DoMuzzleFlash();

	SendWeaponAnim(ACT_VM_PRIMARYATTACK);

	m_iClip1 -= 1;

	// player "shoot" animation
	pPlayer->DoAnimationEvent(PLAYERANIMEVENT_ATTACK_PRIMARY);

	Vector vecSrc = pPlayer->Weapon_ShootPosition();
	Vector vecAiming = pPlayer->GetAutoaimVector(AUTOAIM_10DEGREES);

	Vector vecSpread = GetBulletSpread();
	FireBulletsInfo_t info(5, vecSrc, vecAiming, vecSpread, MAX_TRACE_LENGTH, m_iPrimaryAmmoType);
	info.m_pAttacker = pPlayer;
	info.m_iTracerFreq = 0;

	m_flNextPrimaryAttack = m_flNextPrimaryAttack + GetFireRate();

	// Fire the bullets, and force the first shot to be perfectly accurate
	pPlayer->FireBullets(info);

	if (!m_iClip1 && pPlayer->GetAmmoCount(m_iPrimaryAmmoType) <= 0)
	{
		// HEV suit - indicate out of ammo condition
		pPlayer->SetSuitUpdate("!HEV_AMO0", FALSE, 0);
	}

	pPlayer->ViewPunchReset();
	AddViewKick();
}

bool CWeaponAA13::CanBePickedUpByClass(int classId)
{
	return classId != NEO_CLASS_JUGGERNAUT;
}