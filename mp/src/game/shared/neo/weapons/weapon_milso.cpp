#include "cbase.h"
#include "weapon_milso.h"

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

IMPLEMENT_NETWORKCLASS_ALIASED(WeaponMilso, DT_WeaponMilso)

BEGIN_NETWORK_TABLE(CWeaponMilso, DT_WeaponMilso)
#ifdef CLIENT_DLL
RecvPropTime(RECVINFO(m_flSoonestPrimaryAttack)),
RecvPropTime(RECVINFO(m_flLastAttackTime)),
RecvPropFloat(RECVINFO(m_flAccuracyPenalty)),
RecvPropInt(RECVINFO(m_nNumShotsFired)),
#else
SendPropTime(SENDINFO(m_flSoonestPrimaryAttack)),
SendPropTime(SENDINFO(m_flLastAttackTime)),
SendPropFloat(SENDINFO(m_flAccuracyPenalty)),
SendPropInt(SENDINFO(m_nNumShotsFired)),
#endif
END_NETWORK_TABLE()

#ifdef CLIENT_DLL
BEGIN_PREDICTION_DATA(CWeaponMilso)
DEFINE_PRED_FIELD(m_flSoonestPrimaryAttack, FIELD_FLOAT, FTYPEDESC_INSENDTABLE),
DEFINE_PRED_FIELD(m_flLastAttackTime, FIELD_FLOAT, FTYPEDESC_INSENDTABLE),
DEFINE_PRED_FIELD(m_flAccuracyPenalty, FIELD_FLOAT, FTYPEDESC_INSENDTABLE),
DEFINE_PRED_FIELD(m_nNumShotsFired, FIELD_INTEGER, FTYPEDESC_INSENDTABLE),
END_PREDICTION_DATA()
#endif

LINK_ENTITY_TO_CLASS(weapon_milso, CWeaponMilso);
PRECACHE_WEAPON_REGISTER(weapon_milso);

NEO_ACTTABLE(CWeaponMilso);

CWeaponMilso::CWeaponMilso(void)
{
	m_flSoonestPrimaryAttack = gpGlobals->curtime;
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

void CWeaponMilso::PrimaryAttack(void)
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

	CBasePlayer *pOwner = ToBasePlayer(GetOwner());

	if (pOwner)
	{
		if (!m_iClip1 && !ClientWantsAutoReload(pOwner))
		{
			return;
		}

		// Do nothing if we hold fire
		if ((pOwner->m_nButtons & IN_ATTACK) &&
			(!(pOwner->m_afButtonPressed & IN_ATTACK)))
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

	if (pOwner)
	{
		// Each time the player fires the pistol, reset the view punch. This prevents
		// the aim from 'drifting off' when the player fires very quickly. This may
		// not be the ideal way to achieve this, but it's cheap and it works, which is
		// great for a feature we're evaluating. (sjb)
		pOwner->ViewPunchReset();
	}

	BaseClass::PrimaryAttack();

	// Add an accuracy penalty which can move past our maximum penalty time if we're really spastic
	m_flAccuracyPenalty += MILSO_ACCURACY_SHOT_PENALTY_TIME;
}

void CWeaponMilso::UpdatePenaltyTime(void)
{
	CBasePlayer *pOwner = ToBasePlayer(GetOwner());

	if (pOwner == NULL)
		return;

	// Check our penalty time decay
	if (((pOwner->m_nButtons & IN_ATTACK) == false) &&
		(m_flSoonestPrimaryAttack < gpGlobals->curtime))
	{
		m_flAccuracyPenalty -= gpGlobals->frametime;
		m_flAccuracyPenalty = clamp(m_flAccuracyPenalty, 0.0f,
			MILSO_ACCURACY_MAXIMUM_PENALTY_TIME);
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

float CWeaponMilso::GetFireRate(void)
{
	return MILSO_FASTEST_REFIRE_TIME;
}

void CWeaponMilso::ItemPostFrame(void)
{
	BaseClass::ItemPostFrame();

	if (m_bInReload)
	{
		return;
	}

	CBasePlayer *pOwner = ToBasePlayer(GetOwner());

	if (pOwner == NULL)
	{
		return;
	}

	if (m_iClip1 <= 0)
	{
		return;
	}

	if (pOwner->m_nButtons & IN_ATTACK)
	{
		if (m_flSoonestPrimaryAttack < gpGlobals->curtime)
		{
			if (m_iClip1 <= 0)
			{
				DryFire();

				m_flSoonestPrimaryAttack = gpGlobals->curtime + MILSO_FASTEST_DRY_REFIRE_TIME;
			}
			else
			{
				m_flSoonestPrimaryAttack = gpGlobals->curtime + MILSO_FASTEST_REFIRE_TIME;
			}
		}
	}
	else if (pOwner->m_afButtonReleased & IN_ATTACK)
	{
		m_flSoonestPrimaryAttack = gpGlobals->curtime + MILSO_FASTEST_REFIRE_TIME;
	}
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

	if (pPlayer == NULL)
		return;

	QAngle	viewPunch;

	viewPunch.x = SharedRandomFloat("pistolpax", 0.25f, 0.5f);
	viewPunch.y = SharedRandomFloat("pistolpay", -0.6f, 0.6f);
	viewPunch.z = 0.0f;

	//Add it to the view punch
	pPlayer->ViewPunch(viewPunch);
}
