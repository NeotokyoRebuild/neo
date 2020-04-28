#include "cbase.h"
#include "weapon_zr68l.h"

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

IMPLEMENT_NETWORKCLASS_ALIASED(WeaponZR68L, DT_WeaponZR68L)

BEGIN_NETWORK_TABLE(CWeaponZR68L, DT_WeaponZR68L)
	DEFINE_NEO_BASE_WEP_NETWORK_TABLE
END_NETWORK_TABLE()

#ifdef CLIENT_DLL
BEGIN_PREDICTION_DATA(CWeaponZR68L)
	DEFINE_NEO_BASE_WEP_PREDICTION
END_PREDICTION_DATA()
#endif

NEO_IMPLEMENT_ACTTABLE(CWeaponZR68L)

LINK_ENTITY_TO_CLASS(weapon_zr68l, CWeaponZR68L);

PRECACHE_WEAPON_REGISTER(weapon_zr68l);

CWeaponZR68L::CWeaponZR68L()
{
	m_flSoonestAttack = gpGlobals->curtime;
	m_flAccuracyPenalty = 0;

	m_nNumShotsFired = 0;
}

void CWeaponZR68L::DryFire()
{
	WeaponSound(EMPTY);
	SendWeaponAnim(ACT_VM_DRYFIRE);

	m_flNextPrimaryAttack = gpGlobals->curtime + SequenceDuration();
}

void CWeaponZR68L::Spawn()
{
	BaseClass::Spawn();
}

bool CWeaponZR68L::Deploy(void)
{
	return BaseClass::Deploy();
}

void CWeaponZR68L::PrimaryAttack()
{
	if ((gpGlobals->curtime - m_flLastAttackTime) > 0.5f)
	{
		m_nNumShotsFired = 0;
	}
	else
	{
		m_nNumShotsFired++;
	}

	m_flLastAttackTime = gpGlobals->curtime;

	auto pOwner = ToBasePlayer(GetOwner());
	if (pOwner)
	{
		pOwner->ViewPunchReset();
	}

	BaseClass::PrimaryAttack();

	m_flAccuracyPenalty += ZR68L_ACCURACY_SHOT_PENALTY_TIME;
}

void CWeaponZR68L::UpdatePenaltyTime()
{
	m_flAccuracyPenalty -= gpGlobals->frametime;
	m_flAccuracyPenalty = clamp(m_flAccuracyPenalty,
		0.0f, ZR68L_ACCURACY_MAXIMUM_PENALTY_TIME);
}

void CWeaponZR68L::ItemPreFrame()
{
	UpdatePenaltyTime();

	BaseClass::ItemPreFrame();
}

void CWeaponZR68L::ItemBusyFrame()
{
	UpdatePenaltyTime();

	BaseClass::ItemBusyFrame();
}

void CWeaponZR68L::ItemPostFrame()
{
	BaseClass::ItemPostFrame();

	if (m_bInReload)
	{
		return;
	}

	auto owner = ToBasePlayer(GetOwner());

	if (!owner)
	{
		return;
	}

	if (owner->m_nButtons & IN_ATTACK)
	{
		if (m_flSoonestAttack < gpGlobals->curtime)
		{
			if (m_iClip1 <= 0)
			{
				DryFire();

				m_flSoonestAttack = gpGlobals->curtime + ZR68L_FASTEST_DRY_REFIRE_TIME;
			}
			else
			{
				m_flSoonestAttack = gpGlobals->curtime + ZR68L_FASTEST_REFIRE_TIME;
			}
		}
	}
}

float CWeaponZR68L::GetFireRate()
{
	return ZR68L_FASTEST_REFIRE_TIME;
}

Activity CWeaponZR68L::GetPrimaryAttackActivity()
{
	if (m_nNumShotsFired < 1)
	{
		return ACT_VM_PRIMARYATTACK;
	}

	if (m_nNumShotsFired < 2)
	{
		return ACT_VM_RECOIL1;
	}

	if (m_nNumShotsFired < 3)
	{
		return ACT_VM_RECOIL2;
	}

	return ACT_VM_RECOIL3;
}

void CWeaponZR68L::AddViewKick()
{
	auto owner = ToBasePlayer(GetOwner());

	if (!owner)
	{
		return;
	}

	QAngle viewPunch;

	viewPunch.x = SharedRandomFloat("zr68lx", 0.25f, 0.5f);
	viewPunch.y = SharedRandomFloat("zr68ly", -0.6f, 0.6f);
	viewPunch.z = 0;

	owner->ViewPunch(viewPunch);
}
