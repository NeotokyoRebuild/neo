#include "cbase.h"
#include "weapon_balc.h"
#ifdef GAME_DLL
#include "grenade_ar2.h"
#endif

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

#define BALC_COOLING_RATE 0.35f
#define BALC_UNDERWATER_COOLING_RATE 0.1f
#define BALC_OVERHEAT_DURATION 5.0f
#define BALC_CHARGE_DURATION 1.0f
#define BALC_CHARGE_SHOT_RATE 0.35f
#define BALC_CHARGE_SHOT_MAX 2
#define BALC_CHARGE_SHOT_DAMAGE 270.0f

IMPLEMENT_NETWORKCLASS_ALIASED(WeaponBALC, DT_WeaponBALC)

BEGIN_NETWORK_TABLE(CWeaponBALC, DT_WeaponBALC)
#ifdef CLIENT_DLL
	RecvPropBool(RECVINFO(m_bOverheated)),
	RecvPropBool(RECVINFO(m_bCharging)),
	RecvPropBool(RECVINFO(m_bCharged)),
	RecvPropTime(RECVINFO(m_flOverheatStartTime)),
	RecvPropTime(RECVINFO(m_flChargeStartTime)),
#else
	SendPropBool(SENDINFO(m_bOverheated)),
	SendPropBool(SENDINFO(m_bCharging)),
	SendPropBool(SENDINFO(m_bCharged)),
	SendPropTime(SENDINFO(m_flOverheatStartTime)),
	SendPropTime(SENDINFO(m_flChargeStartTime)),
#endif
END_NETWORK_TABLE()

#ifdef CLIENT_DLL
BEGIN_PREDICTION_DATA(CWeaponBALC)
	DEFINE_PRED_FIELD(m_bOverheated, FIELD_BOOLEAN, FTYPEDESC_INSENDTABLE),
	DEFINE_PRED_FIELD(m_bCharging, FIELD_BOOLEAN, FTYPEDESC_INSENDTABLE),
	DEFINE_PRED_FIELD(m_bCharged, FIELD_BOOLEAN, FTYPEDESC_INSENDTABLE),
END_PREDICTION_DATA()
#endif

NEO_IMPLEMENT_ACTTABLE(CWeaponBALC)

LINK_ENTITY_TO_CLASS(weapon_balc, CWeaponBALC);

#ifdef GAME_DLL
BEGIN_DATADESC(CWeaponBALC)
	DEFINE_FIELD(m_bOverheated, FIELD_BOOLEAN),
	DEFINE_FIELD(m_bCharging, FIELD_BOOLEAN),
	DEFINE_FIELD(m_bCharged, FIELD_BOOLEAN),
END_DATADESC()
#endif

PRECACHE_WEAPON_REGISTER(weapon_balc);

// To avoid any overlap from last fire with m_flNextSecondaryAttack
COMPILE_TIME_ASSERT(BALC_CHARGE_DURATION > BALC_CHARGE_SHOT_RATE);

CWeaponBALC::CWeaponBALC()
{
	m_flSoonestAttack = gpGlobals->curtime;
	m_flAccuracyPenalty = 0;

	m_nNumShotsFired = 0;

	m_weaponSeeds = {
		"balcpx",
		"balcpy",
		"balcrx",
		"balcry",
	};

	m_bFiresUnderwater = true;
	m_bAltFiresUnderwater = true;

	m_bOverheated = false;
	m_bCharging = false;
	m_bCharged = false;
	m_flOverheatStartTime = 0.0f;
	m_flChargeStartTime = 0.0f;
}

void CWeaponBALC::Precache(void)
{
	PrecacheModel("models/weapons/ar2_grenade.mdl");

	BaseClass::Precache();
}

void CWeaponBALC::Spawn(void)
{
	BaseClass::Spawn();

#ifdef GAME_DLL
	SetThink(&CWeaponBALC::Think);
	SetNextThink(gpGlobals->curtime + GetCoolingRate());
#endif
}

bool CWeaponBALC::CanBePickedUpByClass(int classId)
{
	return classId == NEO_CLASS_JUGGERNAUT;
}

void CWeaponBALC::PrimaryAttack(void)
{
	if (m_bOverheated)
	{
		return;
	}

	BaseClass::PrimaryAttack();
}

void CWeaponBALC::SecondaryAttack(void)
{
	if (ShootingIsPrevented() || !m_bCharged)
	{
		return;
	}

	if (gpGlobals->curtime < m_flSoonestAttack)
	{
		return;
	}

	auto pPlayer = ToNEOPlayer(GetOwner());
	if (!pPlayer)
	{
		Assert(false);
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

	SendWeaponAnim(GetPrimaryAttackActivity());
	SetWeaponIdleTime(gpGlobals->curtime + 2.0);
	pPlayer->DoAnimationEvent(PLAYERANIMEVENT_ATTACK_PRIMARY);

	WeaponSound(BURST);

#ifdef GAME_DLL
	const Vector vecSrc = pPlayer->Weapon_ShootPosition();
	Vector vecThrow;

	AngleVectors(pPlayer->EyeAngles() + pPlayer->GetPunchAngle(), &vecThrow);
	VectorScale(vecThrow, 2000.0f, vecThrow);

	QAngle angles;
	VectorAngles(vecThrow, angles);
	CGrenadeAR2 *pGrenade = (CGrenadeAR2*)Create("grenade_ar2", vecSrc, angles, pPlayer);
	pGrenade->SetAbsVelocity(vecThrow);

	pGrenade->SetLocalAngularVelocity(RandomAngle(-400, 400));
	pGrenade->SetMoveType(MOVETYPE_FLYGRAVITY, MOVECOLLIDE_FLY_BOUNCE);
	pGrenade->SetThrower(GetOwner());
	pGrenade->SetDamage(BALC_CHARGE_SHOT_DAMAGE);

	CSoundEnt::InsertSound(SOUND_COMBAT, GetAbsOrigin(), 1000, 0.2, GetOwner(), SOUNDENT_CHANNEL_WEAPON);
#endif
	const int iAmmoCost = int((GetDefaultClip1() + 10) / BALC_CHARGE_SHOT_MAX);
	m_iPrimaryAmmoCount = MAX(0, m_iPrimaryAmmoCount - iAmmoCost);

	m_flNextSecondaryAttack = m_flNextSecondaryAttack + BALC_CHARGE_SHOT_RATE;

	m_bCharging = false;
	m_bCharged = false;

	//View kick
	pPlayer->ViewPunchReset();
	AddViewKick();
}

void CWeaponBALC::ItemPostFrame(void)
{
	auto pOwner = ToBasePlayer(GetOwner());
	if (pOwner && !m_bOverheated && !ShootingIsPrevented() && !(pOwner->m_nButtons & IN_ATTACK))
	{
		if (pOwner->m_afButtonPressed & (IN_AIM | IN_ZOOM) && !m_bCharging)
		{
			m_bCharging = true;
			m_flChargeStartTime = gpGlobals->curtime;
			WeaponSound(SPECIAL3);
		}
		else if (pOwner->m_afButtonReleased & (IN_AIM | IN_ZOOM) && m_bCharging)
		{
			m_bCharging = false;
			m_bCharged = false;
			StopWeaponSound(SPECIAL3);
		}
	}
	else if (m_bCharging)
	{
		m_bCharging = false;
		m_bCharged = false;
	}

	if (m_bCharging)
	{
		const float flTimeCharged = gpGlobals->curtime - m_flChargeStartTime;
		if (flTimeCharged >= BALC_CHARGE_DURATION && !m_bCharged)
		{
			m_bCharged = true;
			m_flNextSecondaryAttack = gpGlobals->curtime;
		}
	}

	if (m_bOverheated)
	{
		const float flTimeOverheated = gpGlobals->curtime - m_flOverheatStartTime;
		if (flTimeOverheated >= BALC_OVERHEAT_DURATION)
		{
			SetPrimaryAmmoCount(GetDefaultClip1());
			m_bOverheated = false;
			WeaponSound(SPECIAL2);
		}
	}

	BaseClass::ItemPostFrame();

	if (m_iPrimaryAmmoCount <= 0 && !m_bOverheated)
	{
		m_bOverheated = true;
		m_flOverheatStartTime = gpGlobals->curtime;
		WeaponSound(SPECIAL1);
		return;
	}
}

// Reserve our think function for gun cooling. Should this be GAME_DLL only?
#ifdef GAME_DLL
void CWeaponBALC::Think(void)
{
	if (!m_bOverheated && GetPrimaryAmmoCount() < GetDefaultClip1())
	{
		SetPrimaryAmmoCount(GetPrimaryAmmoCount() + 1);
	}
	
	SetNextThink(gpGlobals->curtime + GetCoolingRate());
}

float CWeaponBALC::GetCoolingRate()
{
	auto pOwner = GetOwner();
	if ((pOwner && pOwner->GetWaterLevel() == 3) || GetWaterLevel() == 3)
	{
		return BALC_UNDERWATER_COOLING_RATE;
	}

	return BALC_COOLING_RATE;
}
#endif
