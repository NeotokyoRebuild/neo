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

// NEO TODO (Rain): move this stuff to weapon scripts for all guns
#define	AA13_ACCURACY_SHOT_PENALTY_TIME		0.2f
#define	AA13_ACCURACY_MAXIMUM_PENALTY_TIME	1.5f	// Maximum penalty to deal out

#define	AA13_FASTEST_REFIRE_TIME 0.333f
#define	AA13_FASTEST_DRY_REFIRE_TIME 0.2f

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

Activity CWeaponAA13::GetPrimaryAttackActivity()
{
	if (m_nNumShotsFired < 1)
		return ACT_VM_PRIMARYATTACK;

	if (m_nNumShotsFired < 2)
		return ACT_VM_RECOIL1;

	if (m_nNumShotsFired < 3)
		return ACT_VM_RECOIL2;

	return ACT_VM_RECOIL3;
}

const Vector& CWeaponAA13::GetBulletSpread()
{
	static Vector cone = VECTOR_CONE_10DEGREES;
	return cone;
}

CWeaponAA13::CWeaponAA13(void)
{
	m_flSoonestAttack = gpGlobals->curtime;
	m_flAccuracyPenalty = 0.0f;

	m_nNumShotsFired = 0;
}

void CWeaponAA13::UpdatePenaltyTime()
{
	// Update the penalty time decay
	m_flAccuracyPenalty -= gpGlobals->frametime;
	m_flAccuracyPenalty = clamp(m_flAccuracyPenalty, 0.0f, AA13_ACCURACY_MAXIMUM_PENALTY_TIME);
}

void CWeaponAA13::ItemPostFrame()
{
	BaseClass::ItemPostFrame();

	if (m_bInReload)
	{
		return;
	}

	CNEO_Player *pOwner = ToNEOPlayer((GetOwner()));

	if (!pOwner)
	{
		return;
	}

	if (pOwner->m_nButtons & IN_ATTACK)
	{
		if (m_flSoonestAttack < gpGlobals->curtime)
		{
			if (m_iClip1 <= 0)
			{
				DryFire();

				m_flSoonestAttack = gpGlobals->curtime + AA13_FASTEST_DRY_REFIRE_TIME;
				return;
			}
			else
			{
				m_flSoonestAttack = gpGlobals->curtime + AA13_FASTEST_REFIRE_TIME;
			}
		}
	}
}

void CWeaponAA13::ItemPreFrame()
{
	UpdatePenaltyTime();

	BaseClass::ItemPreFrame();
}

void CWeaponAA13::ItemBusyFrame()
{
	UpdatePenaltyTime();

	BaseClass::ItemBusyFrame();
}

void CWeaponAA13::PrimaryAttack(void)
{
	CNEO_Player *pOwner = ToNEOPlayer((GetOwner()));

	if (!pOwner)
	{
		return;
	}

	// Can't shoot again yet
	if (gpGlobals->curtime - m_flLastAttackTime < AA13_FASTEST_REFIRE_TIME)
	{
		return;
	}

	if ((gpGlobals->curtime - m_flLastAttackTime) > 0.5f)
	{
		m_nNumShotsFired = 0;
	}
	else
	{
		m_nNumShotsFired++;
	}

	m_flLastAttackTime = gpGlobals->curtime;

	if (pOwner)
	{
		pOwner->ViewPunchReset();
	}

	BaseClass::PrimaryAttack();

	m_flAccuracyPenalty += AA13_ACCURACY_SHOT_PENALTY_TIME;
}

void CWeaponAA13::AddViewKick()
{
	CNEO_Player *pOwner = ToNEOPlayer((GetOwner()));

	if (!pOwner)
	{
		return;
	}

	QAngle viewPunch;

	viewPunch.x = SharedRandomFloat("aa13x", 0.33f, 0.5f);
	viewPunch.y = SharedRandomFloat("aa13y", -0.6f, 0.6f);
	viewPunch.z = 0.0f;

	pOwner->ViewPunch(viewPunch);
}

void CWeaponAA13::DryFire()
{
	WeaponSound(EMPTY);
	SendWeaponAnim(ACT_VM_DRYFIRE);

	m_flNextPrimaryAttack = gpGlobals->curtime + SequenceDuration();
}
