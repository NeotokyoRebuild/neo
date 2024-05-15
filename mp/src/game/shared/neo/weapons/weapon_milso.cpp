#include "cbase.h"
#include "weapon_milso.h"

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

IMPLEMENT_NETWORKCLASS_ALIASED(WeaponMilso, DT_WeaponMilso)

BEGIN_NETWORK_TABLE(CWeaponMilso, DT_WeaponMilso)
	DEFINE_NEO_BASE_WEP_NETWORK_TABLE
END_NETWORK_TABLE()

#ifdef CLIENT_DLL
BEGIN_PREDICTION_DATA(CWeaponMilso)
	DEFINE_NEO_BASE_WEP_PREDICTION
END_PREDICTION_DATA()
#endif

NEO_IMPLEMENT_ACTTABLE(CWeaponMilso)

LINK_ENTITY_TO_CLASS(weapon_milso, CWeaponMilso);

PRECACHE_WEAPON_REGISTER(weapon_milso);

CWeaponMilso::CWeaponMilso()
{
	m_flSoonestAttack = gpGlobals->curtime;
	m_flAccuracyPenalty = 0.0f;

	m_fMinRange1 = 24;
	m_fMaxRange2 = 1500;
	m_fMinRange2 = 24;
	m_fMaxRange2 = 200;

	m_bFiresUnderwater = true;
}

void CWeaponMilso::Precache(void)
{
	BaseClass::Precache();
}

void CWeaponMilso::DryFire(void)
{
	WeaponSound(EMPTY);
	SendWeaponAnim(ACT_VM_DRYFIRE);

	m_flNextPrimaryAttack = gpGlobals->curtime + SequenceDuration();
}

void CWeaponMilso::UpdatePenaltyTime(void)
{
	CBasePlayer *pOwner = ToBasePlayer(GetOwner());

	if (pOwner == NULL)
		return;

	// Check our penalty time decay
	if ((pOwner->m_nButtons & IN_ATTACK) == false)
	{
		if (m_flSoonestAttack < gpGlobals->curtime)
		{
			m_flAccuracyPenalty -= gpGlobals->frametime;
			m_flAccuracyPenalty = clamp(m_flAccuracyPenalty, 0.0f, GetMaxAccuracyPenalty());
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

void CWeaponMilso::ItemPreFrame(void)
{
	UpdatePenaltyTime();

	BaseClass::ItemPreFrame();
}

void CWeaponMilso::ItemBusyFrame(void)
{
	UpdatePenaltyTime();

	BaseClass::ItemBusyFrame();
}

void CWeaponMilso::ItemPostFrame(void)
{
	BaseClass::ItemPostFrame();
}

Activity CWeaponMilso::GetPrimaryAttackActivity(void)
{
	if (m_nNumShotsFired < 1)
		return ACT_VM_PRIMARYATTACK;

	if (m_nNumShotsFired < 2)
		return ACT_VM_RECOIL1;

	if (m_nNumShotsFired < 3)
		return ACT_VM_RECOIL2;

	return ACT_VM_RECOIL3;
}

void CWeaponMilso::AddViewKick(void)
{
	CBasePlayer *pPlayer = ToBasePlayer(GetOwner());

	if (!pPlayer)
	{
		return;
	}

	const QAngle viewPunch{
		SharedRandomFloat("milsopax", 0.25f, 0.5f),
		SharedRandomFloat("milsopay", -0.6f, 0.6f),
		0.0f
	};

	// Add it to the view punch
	pPlayer->ViewPunch(viewPunch);
}
