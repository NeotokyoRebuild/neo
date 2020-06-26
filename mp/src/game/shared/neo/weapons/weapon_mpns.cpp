#include "cbase.h"
#include "weapon_mpns.h"

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

IMPLEMENT_NETWORKCLASS_ALIASED(WeaponMPN_S, DT_WeaponMPN_S)

BEGIN_NETWORK_TABLE(CWeaponMPN_S, DT_WeaponMPN_S)
	DEFINE_NEO_BASE_WEP_NETWORK_TABLE
END_NETWORK_TABLE()

#ifdef CLIENT_DLL
BEGIN_PREDICTION_DATA(CWeaponMPN_S)
	DEFINE_NEO_BASE_WEP_PREDICTION
END_PREDICTION_DATA()
#endif

NEO_IMPLEMENT_ACTTABLE(CWeaponMPN_S)

LINK_ENTITY_TO_CLASS(weapon_mpn, CWeaponMPN_S);

PRECACHE_WEAPON_REGISTER(weapon_mpn);

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
		m_flAccuracyPenalty = clamp(m_flAccuracyPenalty, 0.0f, GetMaxAccuracyPenalty());
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

	if (owner->m_nButtons & IN_ATTACK)
	{
		if (m_flSoonestAttack < gpGlobals->curtime)
		{
			if (m_iClip1 <= 0)
			{
				DryFire();

				m_flSoonestAttack = gpGlobals->curtime + GetFastestDryRefireTime();
			}
			else
			{
				m_flSoonestAttack = gpGlobals->curtime + GetFireRate();
			}
		}
	}
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
	auto pOwner = ToBasePlayer(GetOwner());

	if (!pOwner)
	{
		return;
	}

	const QAngle viewPunch{
		SharedRandomFloat("mpnspx", 0.25f, 0.5f),
		SharedRandomFloat("mpnspy", -0.6f, 0.6f),
		0.0f
	};

	pOwner->ViewPunch(viewPunch);
}
