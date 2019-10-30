#include "cbase.h"
#include "weapon_mpn.h"

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

IMPLEMENT_NETWORKCLASS_ALIASED(WeaponMPN, DT_WeaponMPN)

BEGIN_NETWORK_TABLE(CWeaponMPN, DT_WeaponMPN)
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
BEGIN_PREDICTION_DATA(CWeaponMPN)
DEFINE_PRED_FIELD(m_flSoonestAttack, FIELD_FLOAT, FTYPEDESC_INSENDTABLE),
DEFINE_PRED_FIELD(m_flLastAttackTime, FIELD_FLOAT, FTYPEDESC_INSENDTABLE),
DEFINE_PRED_FIELD(m_flAccuracyPenalty, FIELD_FLOAT, FTYPEDESC_INSENDTABLE),
DEFINE_PRED_FIELD(m_nNumShotsFired, FIELD_INTEGER, FTYPEDESC_INSENDTABLE),
END_PREDICTION_DATA()
#endif

LINK_ENTITY_TO_CLASS(weapon_mpn, CWeaponMPN);
PRECACHE_WEAPON_REGISTER(weapon_mpn);

#ifdef GAME_DLL
acttable_t CWeaponMPN::m_acttable[] =
{
	{ ACT_MP_STAND_IDLE, ACT_IDLE, true },
	{ ACT_MP_RUN, ACT_RUN, true },
	{ ACT_MP_WALK, ACT_WALK, true },
	{ ACT_MP_JUMP, ACT_HOP, true },

	{ ACT_MP_AIRWALK, ACT_HOVER, true },
	{ ACT_MP_SWIM, ACT_SWIM, true },

	{ ACT_LEAP, ACT_LEAP, true },
	{ ACT_DIESIMPLE, ACT_DIESIMPLE, true },

	{ ACT_MP_CROUCH_IDLE, ACT_CROUCHIDLE, true },
	{ ACT_MP_CROUCHWALK, ACT_WALK_CROUCH, true },

	{ ACT_MP_RELOAD_STAND, ACT_RELOAD, true },

	{ ACT_CROUCHIDLE, ACT_HL2MP_IDLE_CROUCH_AR2, true },
	{ ACT_WALK_CROUCH, ACT_HL2MP_WALK_CROUCH_AR2, true },
	{ ACT_RANGE_ATTACK1, ACT_RANGE_ATTACK_AR1, true }
};
IMPLEMENT_ACTTABLE(CWeaponMPN);
#endif

CWeaponMPN::CWeaponMPN()
{
	m_flSoonestAttack = gpGlobals->curtime;
	m_flAccuracyPenalty = 0;

	m_nNumShotsFired = 0;
}

void CWeaponMPN::DryFire()
{
	WeaponSound(EMPTY);
	SendWeaponAnim(ACT_VM_DRYFIRE);

	m_flNextPrimaryAttack = gpGlobals->curtime + SequenceDuration();
}

void CWeaponMPN::Spawn()
{
	BaseClass::Spawn();
}

bool CWeaponMPN::Deploy(void)
{
	return BaseClass::Deploy();
}

void CWeaponMPN::PrimaryAttack()
{
	auto owner = ToBasePlayer(GetOwner());

	if (owner)
	{
		if (!m_iClip1 && !ClientWantsAutoReload(GetOwner()))
		{
			return;
		}
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

	if (owner)
	{
		owner->ViewPunchReset();
	}

	BaseClass::PrimaryAttack();

	m_flAccuracyPenalty += MPN_ACCURACY_SHOT_PENALTY_TIME;
}

void CWeaponMPN::UpdatePenaltyTime()
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
			0.0f, MPN_ACCURACY_MAXIMUM_PENALTY_TIME);
	}
}

void CWeaponMPN::ItemPreFrame()
{
	UpdatePenaltyTime();

	BaseClass::ItemPreFrame();
}

void CWeaponMPN::ItemBusyFrame()
{
	UpdatePenaltyTime();

	BaseClass::ItemBusyFrame();
}

void CWeaponMPN::ItemPostFrame()
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

				m_flSoonestAttack = gpGlobals->curtime + MPN_FASTEST_DRY_REFIRE_TIME;
			}
			else
			{
				m_flSoonestAttack = gpGlobals->curtime + MPN_FASTEST_REFIRE_TIME;
			}
		}
	}
}

float CWeaponMPN::GetFireRate()
{
	return MPN_FASTEST_REFIRE_TIME;
}

Activity CWeaponMPN::GetPrimaryAttackActivity()
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

bool CWeaponMPN::Reload()
{
	bool fRet = BaseClass::Reload();

	if (fRet)
	{
		WeaponSound(RELOAD);
		m_flAccuracyPenalty = 0;
	}

	return fRet;
}

void CWeaponMPN::AddViewKick()
{
	auto owner = ToBasePlayer(GetOwner());

	if (!owner)
	{
		return;
	}

	QAngle viewPunch;

	viewPunch.x = SharedRandomFloat("mpnx", 0.25f, 0.5f);
	viewPunch.y = SharedRandomFloat("mpny", -0.6f, 0.6f);
	viewPunch.z = 0;

	owner->ViewPunch(viewPunch);
}