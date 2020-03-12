#include "cbase.h"
#include "weapon_zr68s.h"

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

IMPLEMENT_NETWORKCLASS_ALIASED(WeaponZR68S, DT_WeaponZR68S)

BEGIN_NETWORK_TABLE(CWeaponZR68S, DT_WeaponZR68S)
#ifdef CLIENT_DLL
RecvPropTime(RECVINFO(m_flSoonestAttack)),
RecvPropTime(RECVINFO(m_flLastAttackTime)),
RecvPropFloat(RECVINFO(m_flAccuracyPenalty)),
RecvPropInt(RECVINFO(m_nNumShotsFired)),
#else
SendPropTime(SENDINFO(m_flSoonestAttack)),
SendPropTime(SENDINFO(m_flLastAttackTime)),
SendPropFloat(SENDINFO(m_flAccuracyPenalty)),
SendPropInt(SENDINFO(m_nNumShotsFired)),
#endif
END_NETWORK_TABLE()

#ifdef CLIENT_DLL
BEGIN_PREDICTION_DATA(CWeaponZR68S)
DEFINE_PRED_FIELD(m_flSoonestAttack, FIELD_FLOAT, FTYPEDESC_INSENDTABLE),
DEFINE_PRED_FIELD(m_flLastAttackTime, FIELD_FLOAT, FTYPEDESC_INSENDTABLE),
DEFINE_PRED_FIELD(m_flAccuracyPenalty, FIELD_FLOAT, FTYPEDESC_INSENDTABLE),
DEFINE_PRED_FIELD(m_nNumShotsFired, FIELD_INTEGER, FTYPEDESC_INSENDTABLE),
END_PREDICTION_DATA()
#endif

LINK_ENTITY_TO_CLASS(weapon_zr68s, CWeaponZR68S);
PRECACHE_WEAPON_REGISTER(weapon_zr68s);

NEO_ACTTABLE(CWeaponZR68S);

CWeaponZR68S::CWeaponZR68S()
{
	m_flSoonestAttack = gpGlobals->curtime;
	m_flAccuracyPenalty = 0;

	m_nNumShotsFired = 0;
}

void CWeaponZR68S::DryFire()
{
	WeaponSound(EMPTY);
	SendWeaponAnim(ACT_VM_DRYFIRE);

	m_flNextPrimaryAttack = gpGlobals->curtime + SequenceDuration();
}

void CWeaponZR68S::Spawn()
{
	BaseClass::Spawn();
}

bool CWeaponZR68S::Deploy(void)
{
	return BaseClass::Deploy();
}

void CWeaponZR68S::PrimaryAttack()
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

	m_flAccuracyPenalty += ZR68S_ACCURACY_SHOT_PENALTY_TIME;
}

void CWeaponZR68S::UpdatePenaltyTime()
{
	auto owner = ToBasePlayer(GetOwner());

	if (!owner)
	{
		return;
	}

	if (((owner->m_nButtons & IN_ATTACK) == false) &&
		(m_flSoonestAttack < gpGlobals->curtime))
	{
		m_flAccuracyPenalty -= gpGlobals->frametime;
		m_flAccuracyPenalty = clamp(m_flAccuracyPenalty,
			0.0f, ZR68S_ACCURACY_MAXIMUM_PENALTY_TIME);
	}
}

void CWeaponZR68S::ItemPreFrame()
{
	UpdatePenaltyTime();

	BaseClass::ItemPreFrame();
}

void CWeaponZR68S::ItemBusyFrame()
{
	UpdatePenaltyTime();

	BaseClass::ItemBusyFrame();
}

void CWeaponZR68S::ItemPostFrame()
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

				m_flSoonestAttack = gpGlobals->curtime + ZR68S_FASTEST_DRY_REFIRE_TIME;
			}
			else
			{
				m_flSoonestAttack = gpGlobals->curtime + ZR68S_FASTEST_REFIRE_TIME;
			}
		}
	}
}

Activity CWeaponZR68S::GetPrimaryAttackActivity()
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

void CWeaponZR68S::AddViewKick()
{
	auto owner = ToBasePlayer(GetOwner());

	if (!owner)
	{
		return;
	}

	QAngle viewPunch;

	viewPunch.x = SharedRandomFloat("zr68sx", 0.25f, 0.5f);
	viewPunch.y = SharedRandomFloat("zr68sy", -0.6f, 0.6f);
	viewPunch.z = 0;

	owner->ViewPunch(viewPunch);
}
