#include "cbase.h"
#include "weapon_m41s.h"

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

IMPLEMENT_NETWORKCLASS_ALIASED(WeaponM41S, DT_WeaponM41S)

BEGIN_NETWORK_TABLE(CWeaponM41S, DT_WeaponM41S)
	DEFINE_NEO_BASE_WEP_NETWORK_TABLE
END_NETWORK_TABLE()

#ifdef CLIENT_DLL
BEGIN_PREDICTION_DATA(CWeaponM41S)
	DEFINE_NEO_BASE_WEP_PREDICTION
END_PREDICTION_DATA()
#endif

NEO_IMPLEMENT_ACTTABLE(CWeaponM41S)

LINK_ENTITY_TO_CLASS(weapon_m41s, CWeaponM41S);

PRECACHE_WEAPON_REGISTER(weapon_m41s);

CWeaponM41S::CWeaponM41S()
{
	m_flSoonestAttack = gpGlobals->curtime;
	m_flAccuracyPenalty = 0;

	m_nNumShotsFired = 0;
}

void CWeaponM41S::DryFire()
{
	WeaponSound(EMPTY);
	SendWeaponAnim(ACT_VM_DRYFIRE);

	m_flNextPrimaryAttack = gpGlobals->curtime + SequenceDuration();
}

void CWeaponM41S::UpdatePenaltyTime()
{
	auto owner = ToBasePlayer(GetOwner());

	if (!owner)
	{
		return;
	}

	if ((owner->m_nButtons & IN_ATTACK) == false)
	{
		if (m_flSoonestAttack < gpGlobals->curtime)
		{
			m_flAccuracyPenalty -= gpGlobals->frametime;
			m_flAccuracyPenalty = clamp(m_flAccuracyPenalty,
				0.0f, GetMaxAccuracyPenalty());
		}
	}
	else
	{
		m_flSoonestAttack = gpGlobals->curtime + GetFireRate();
	}

	if (m_flSoonestAttack > gpGlobals->curtime)
	{
		m_flSoonestAttack -= (gpGlobals->curtime - m_flLastAttackTime);
	}
}

void CWeaponM41S::ItemPreFrame()
{
	UpdatePenaltyTime();

	BaseClass::ItemPreFrame();
}

void CWeaponM41S::ItemBusyFrame()
{
	UpdatePenaltyTime();

	BaseClass::ItemBusyFrame();
}

void CWeaponM41S::ItemPostFrame()
{
	BaseClass::ItemPostFrame();
}

Activity CWeaponM41S::GetPrimaryAttackActivity()
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

void CWeaponM41S::AddViewKick()
{
	auto pOwner = ToBasePlayer(GetOwner());

	if (!pOwner)
	{
		return;
	}

	const QAngle viewPunch {
		SharedRandomFloat("m41spx", 0.25f, 0.5f),
		SharedRandomFloat("m41spy", -0.6f, 0.6f),
		0.0f
	};

	pOwner->ViewPunch(viewPunch);
}
