#include "cbase.h"
#include "weapon_mpns.h"

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

IMPLEMENT_NETWORKCLASS_ALIASED(WeaponMPN_S, DT_WeaponMPN_S)

BEGIN_NETWORK_TABLE(CWeaponMPN_S, DT_WeaponMPN_S)
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
BEGIN_PREDICTION_DATA(CWeaponMPN_S)
DEFINE_PRED_FIELD(m_flSoonestAttack, FIELD_FLOAT, FTYPEDESC_INSENDTABLE),
DEFINE_PRED_FIELD(m_flLastAttackTime, FIELD_FLOAT, FTYPEDESC_INSENDTABLE),
DEFINE_PRED_FIELD(m_flAccuracyPenalty, FIELD_FLOAT, FTYPEDESC_INSENDTABLE),
DEFINE_PRED_FIELD(m_nNumShotsFired, FIELD_INTEGER, FTYPEDESC_INSENDTABLE),
END_PREDICTION_DATA()
#endif

LINK_ENTITY_TO_CLASS(weapon_mpn, CWeaponMPN_S);
PRECACHE_WEAPON_REGISTER(weapon_mpn);

NEO_ACTTABLE(CWeaponMPN_S);

CWeaponMPN_S::CWeaponMPN_S()
{
	m_flSoonestAttack = gpGlobals->curtime;
	m_flAccuracyPenalty = 0;

	m_nNumShotsFired = 0;
}

void CWeaponMPN_S::DryFire()
{
	WeaponSound(EMPTY);
	SendWeaponAnim(ACT_VM_DRYFIRE);

	m_flNextPrimaryAttack = gpGlobals->curtime + SequenceDuration();
}

void CWeaponMPN_S::Spawn()
{
	BaseClass::Spawn();
}

bool CWeaponMPN_S::Deploy(void)
{
	return BaseClass::Deploy();
}

void CWeaponMPN_S::PrimaryAttack()
{
	auto pOwner = ToBasePlayer(GetOwner());

	// We don't have bullets, but player doesn't want auto-reload. Do nothing.
	if (m_iClip1 == 0 && !ClientWantsAutoReload(pOwner))
	{
		return;
	}

	if ((gpGlobals->curtime - m_flLastAttackTime) > 0.5f)
	{
		m_nNumShotsFired = 0;
	}
	else
	{
		++m_nNumShotsFired;
	}

	m_flLastAttackTime = gpGlobals->curtime;

	if (pOwner)
	{
		pOwner->ViewPunchReset();
	}

	BaseClass::PrimaryAttack();

	m_flAccuracyPenalty += MPN_S_ACCURACY_SHOT_PENALTY_TIME;
}

void CWeaponMPN_S::UpdatePenaltyTime()
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
			0.0f, MPN_S_ACCURACY_MAXIMUM_PENALTY_TIME);
	}
}

void CWeaponMPN_S::ItemPreFrame()
{
	UpdatePenaltyTime();

	BaseClass::ItemPreFrame();
}

void CWeaponMPN_S::ItemBusyFrame()
{
	UpdatePenaltyTime();

	BaseClass::ItemBusyFrame();
}

void CWeaponMPN_S::ItemPostFrame()
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

				m_flSoonestAttack = gpGlobals->curtime + MPN_S_FASTEST_DRY_REFIRE_TIME;
			}
			else
			{
				m_flSoonestAttack = gpGlobals->curtime + MPN_S_FASTEST_REFIRE_TIME;
			}
		}
	}
}

float CWeaponMPN_S::GetFireRate()
{
	return MPN_S_FASTEST_REFIRE_TIME;
}

Activity CWeaponMPN_S::GetPrimaryAttackActivity()
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

void CWeaponMPN_S::AddViewKick()
{
	auto owner = ToBasePlayer(GetOwner());

	if (!owner)
	{
		return;
	}

	QAngle viewPunch;

	viewPunch.x = SharedRandomFloat("mpnsx", 0.25f, 0.5f);
	viewPunch.y = SharedRandomFloat("mpnsy", -0.6f, 0.6f);
	viewPunch.z = 0;

	owner->ViewPunch(viewPunch);
}
