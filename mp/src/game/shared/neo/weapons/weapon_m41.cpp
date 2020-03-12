#include "cbase.h"
#include "weapon_m41.h"

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

IMPLEMENT_NETWORKCLASS_ALIASED(WeaponM41, DT_WeaponM41)

BEGIN_NETWORK_TABLE(CWeaponM41, DT_WeaponM41)
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
BEGIN_PREDICTION_DATA(CWeaponM41)
DEFINE_PRED_FIELD(m_flSoonestAttack, FIELD_FLOAT, FTYPEDESC_INSENDTABLE),
DEFINE_PRED_FIELD(m_flLastAttackTime, FIELD_FLOAT, FTYPEDESC_INSENDTABLE),
DEFINE_PRED_FIELD(m_flAccuracyPenalty, FIELD_FLOAT, FTYPEDESC_INSENDTABLE),
DEFINE_PRED_FIELD(m_nNumShotsFired, FIELD_INTEGER, FTYPEDESC_INSENDTABLE),
END_PREDICTION_DATA()
#endif

LINK_ENTITY_TO_CLASS(weapon_m41, CWeaponM41);
PRECACHE_WEAPON_REGISTER(weapon_m41);

NEO_ACTTABLE(CWeaponM41);

CWeaponM41::CWeaponM41()
{
	m_flSoonestAttack = gpGlobals->curtime;
	m_flAccuracyPenalty = 0;

	m_nNumShotsFired = 0;
}

void CWeaponM41::DryFire()
{
	WeaponSound(EMPTY);
	SendWeaponAnim(ACT_VM_DRYFIRE);

	m_flNextPrimaryAttack = gpGlobals->curtime + SequenceDuration();
}

void CWeaponM41::PrimaryAttack()
{
	if (m_iClip1 == 0)
	{
		if (!m_bFireOnEmpty)
		{
			Reload();
		}
		else
		{
			WeaponSound(EMPTY);
			m_flNextPrimaryAttack = 0.2;
		}

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

	auto owner = ToBasePlayer(GetOwner());
	if (owner)
	{
		owner->ViewPunchReset();
	}

	BaseClass::PrimaryAttack();

	m_flAccuracyPenalty += M41_ACCURACY_SHOT_PENALTY_TIME;
}

void CWeaponM41::UpdatePenaltyTime()
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
			0.0f, M41_ACCURACY_MAXIMUM_PENALTY_TIME);
	}
}

void CWeaponM41::ItemPreFrame()
{
	UpdatePenaltyTime();

	BaseClass::ItemPreFrame();
}

void CWeaponM41::ItemBusyFrame()
{
	UpdatePenaltyTime();

	BaseClass::ItemBusyFrame();
}

void CWeaponM41::ItemPostFrame()
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

	if (m_iClip1 <= 0)
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

				m_flSoonestAttack = gpGlobals->curtime + M41_FASTEST_DRY_REFIRE_TIME;
			}
			else
			{
				m_flSoonestAttack = gpGlobals->curtime + M41_FASTEST_REFIRE_TIME;
			}
		}
	}
}

Activity CWeaponM41::GetPrimaryAttackActivity()
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

void CWeaponM41::AddViewKick()
{
	auto owner = ToBasePlayer(GetOwner());

	if (!owner)
	{
		return;
	}

	QAngle viewPunch;

	viewPunch.x = SharedRandomFloat("m41x", 0.25f, 0.5f);
	viewPunch.y = SharedRandomFloat("m41y", -0.6f, 0.6f);
	viewPunch.z = 0;

	owner->ViewPunch(viewPunch);
}
