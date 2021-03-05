#include "cbase.h"
#include "weapon_kyla.h"

#include "npcevent.h"
#include "in_buttons.h"

#ifdef CLIENT_DLL
#include "c_neo_player.h"
#else
#include "neo_player.h"
#endif

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

IMPLEMENT_NETWORKCLASS_ALIASED(WeaponKyla, DT_WeaponKyla);

BEGIN_NETWORK_TABLE(CWeaponKyla, DT_WeaponKyla)
END_NETWORK_TABLE()

#ifdef CLIENT_DLL
BEGIN_PREDICTION_DATA(CWeaponKyla)
END_PREDICTION_DATA()
#endif

NEO_IMPLEMENT_ACTTABLE(CWeaponKyla)

LINK_ENTITY_TO_CLASS(weapon_kyla, CWeaponKyla);

PRECACHE_WEAPON_REGISTER(weapon_kyla);

CWeaponKyla::CWeaponKyla(void)
{
	m_bReloadsSingly = false;
	m_bFiresUnderwater = false;
}

void CWeaponKyla::PrimaryAttack(void)
{
	if (ShootingIsPrevented())
	{
		return;
	}

	if (m_iClip1 == 0)
	{
		if (!m_bFireOnEmpty)
		{
			CheckReload();
		}
		else
		{
			WeaponSound(EMPTY);
			m_flNextPrimaryAttack = gpGlobals->curtime + GetFastestDryRefireTime();
		}

		return;
	}

	// Only the player fires this way so we can cast
	CBasePlayer* pPlayer = ToBasePlayer(GetOwner());

	if (!pPlayer)
	{
		return;
	}
	WeaponSound(SINGLE);
	pPlayer->DoMuzzleFlash();

	SendWeaponAnim(ACT_VM_PRIMARYATTACK);
	pPlayer->SetAnimation(PLAYER_ATTACK1);

	m_flNextPrimaryAttack = gpGlobals->curtime + GetFireRate();

	m_iClip1--;

	Vector vecSrc = pPlayer->Weapon_ShootPosition();
	Vector vecAiming = pPlayer->GetAutoaimVector(AUTOAIM_5DEGREES);

	FireBulletsInfo_t info(1, vecSrc, vecAiming, vec3_origin, MAX_TRACE_LENGTH, m_iPrimaryAmmoType);
	info.m_pAttacker = pPlayer;

	// Fire the bullets, and force the first shot to be perfectly accuracy
	pPlayer->FireBullets(info);

	// HL2 revolver does this view jerk, but it doesn't play nice with prediction.
	// We could enable it with shared random vals and taking .z lean into account,
	// but disabling entirely for now since the view punch seems sufficient.
#if(0)
#ifndef CLIENT_DLL
	//Disorient the player
	QAngle angles = pPlayer->GetLocalAngles();

	angles.x += random->RandomInt(-1, 1);
	angles.y += random->RandomInt(-1, 1);
	angles.z = 0;

	//pPlayer->SnapEyeAngles(angles);
#endif
#endif

	const float punchIntensity = 1.0f;

	pPlayer->ViewPunch(QAngle(
		-punchIntensity,
		SharedRandomFloat("kylaPunch", -punchIntensity, punchIntensity),
		0));

	if (!m_iClip1 && pPlayer->GetAmmoCount(m_iPrimaryAmmoType) <= 0)
	{
		// HEV suit - indicate out of ammo condition
		pPlayer->SetSuitUpdate("!HEV_AMO0", FALSE, 0);
	}
}
