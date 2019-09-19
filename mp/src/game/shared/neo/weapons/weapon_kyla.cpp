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

BEGIN_PREDICTION_DATA(CWeaponKyla)
END_PREDICTION_DATA()

LINK_ENTITY_TO_CLASS(weapon_kyla, CWeaponKyla);
PRECACHE_WEAPON_REGISTER(weapon_kyla);

#ifdef GAME_DLL
acttable_t CWeaponKyla::m_acttable[] =
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

	{ ACT_CROUCHIDLE, ACT_HL2MP_IDLE_CROUCH_PISTOL, true },
	{ ACT_WALK_CROUCH, ACT_HL2MP_WALK_CROUCH_PISTOL, true },
	{ ACT_RANGE_ATTACK1, ACT_RANGE_ATTACK_PISTOL, true }
};
IMPLEMENT_ACTTABLE(CWeaponKyla);
#endif

CWeaponKyla::CWeaponKyla(void)
{
	m_bReloadsSingly = false;
	m_bFiresUnderwater = false;
}

void CWeaponKyla::PrimaryAttack(void)
{
	// Only the player fires this way so we can cast
	CBasePlayer *pPlayer = ToBasePlayer(GetOwner());

	if (!pPlayer)
	{
		return;
	}

	if (m_iClip1 <= 0)
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

	WeaponSound(SINGLE);
	pPlayer->DoMuzzleFlash();

	SendWeaponAnim(ACT_VM_PRIMARYATTACK);
	pPlayer->SetAnimation(PLAYER_ATTACK1);

	m_flNextPrimaryAttack = gpGlobals->curtime + 0.35;

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