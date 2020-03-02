#include "cbase.h"
#include "weapon_detpack.h"

#include "npcevent.h"

#include "basegrenade_shared.h"

#ifdef GAME_DLL
#include "neo_detpack.h"
#else
#define NEO_DEPLOYED_DETPACK_RADIUS 4.0f // Inches. NEO TODO (Rain): check correct value!
#endif

#ifdef CLIENT_DLL
#include "c_hl2mp_player.h"
#include "c_te_effect_dispatch.h"
#else
#include "hl2mp_player.h"
#include "te_effect_dispatch.h"
#include "grenade_frag.h"
#endif

#include "effect_dispatch_data.h"
#include "in_buttons.h"

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

// NEO FIXME (Rain): these are doubly defined in neo_detpack
#define NEO_DETPACK_DAMAGE 999.0f
#define NEO_DETPACK_DAMAGE_RADIUS 999.0f

NEO_ACTTABLE(CWeaponDetpack)

IMPLEMENT_NETWORKCLASS_ALIASED(WeaponDetpack, DT_WeaponDetpack)

BEGIN_NETWORK_TABLE(CWeaponDetpack, DT_WeaponDetpack)
#ifdef CLIENT_DLL
RecvPropBool(RECVINFO(m_bRedraw)),
RecvPropBool(RECVINFO(m_fDrawbackFinished)),
RecvPropInt(RECVINFO(m_AttackPaused)),
#else
SendPropBool(SENDINFO(m_bRedraw)),
SendPropBool(SENDINFO(m_fDrawbackFinished)),
SendPropInt(SENDINFO(m_AttackPaused)),
#endif
END_NETWORK_TABLE()

#ifdef CLIENT_DLL
BEGIN_PREDICTION_DATA(CWeaponDetpack)
DEFINE_PRED_FIELD(m_bRedraw, FIELD_BOOLEAN, FTYPEDESC_INSENDTABLE),
DEFINE_PRED_FIELD(m_fDrawbackFinished, FIELD_BOOLEAN, FTYPEDESC_INSENDTABLE),
DEFINE_PRED_FIELD(m_AttackPaused, FIELD_INTEGER, FTYPEDESC_INSENDTABLE),
END_PREDICTION_DATA()
#endif

LINK_ENTITY_TO_CLASS(weapon_remotedet, CWeaponDetpack);
PRECACHE_WEAPON_REGISTER(weapon_remotedet);

#define REDEPLOY_DELAY 0.5

CWeaponDetpack::CWeaponDetpack()
{
	m_bRedraw = false;
}

void CWeaponDetpack::Precache(void)
{
	BaseClass::Precache();
}

bool CWeaponDetpack::Deploy(void)
{
	m_bRedraw = false;
	m_fDrawbackFinished = false;

	return BaseClass::Deploy();
}

// Output : Returns true on success, false on failure.
bool CWeaponDetpack::Holster(CBaseCombatWeapon* pSwitchingTo)
{
	m_bRedraw = false;
	m_fDrawbackFinished = false;

	return BaseClass::Holster(pSwitchingTo);
}

// Output : Returns true on success, false on failure.
bool CWeaponDetpack::Reload(void)
{
	if (!HasPrimaryAmmo())
	{
		return false;
	}

	if ((m_bRedraw) && (m_flNextPrimaryAttack <= gpGlobals->curtime) && (m_flNextSecondaryAttack <= gpGlobals->curtime))
	{
		//Redraw the weapon
		SendWeaponAnim(ACT_VM_DRAW);

		//Update our times
		m_flNextPrimaryAttack = gpGlobals->curtime + SequenceDuration();
		m_flNextSecondaryAttack = gpGlobals->curtime + SequenceDuration();
		m_flTimeWeaponIdle = gpGlobals->curtime + SequenceDuration();

		//Mark this as done
		m_bRedraw = false;
	}

	return true;
}

void CWeaponDetpack::SecondaryAttack(void)
{
}

void CWeaponDetpack::PrimaryAttack(void)
{
	if (m_bRedraw)
	{
		return;
	}

	auto pPlayer = ToBasePlayer(GetOwner());

	if (!pPlayer)
	{
		return;
	}

	m_AttackPaused = DETPACK_DEPLOY_PAUSED_NO;
	SendWeaponAnim(ACT_VM_PULLPIN);

	m_flTimeWeaponIdle = FLT_MAX;
	m_flNextPrimaryAttack = gpGlobals->curtime + REDEPLOY_DELAY;

	// If I'm now out of ammo, switch away
	if (!HasPrimaryAmmo())
	{
		pPlayer->SwitchToNextBestWeapon(this);
	}
}

void CWeaponDetpack::DecrementAmmo(CBaseCombatCharacter* pOwner)
{
	pOwner->RemoveAmmo(1, m_iPrimaryAmmoType);
}

void CWeaponDetpack::ItemPostFrame(void)
{
	if (!m_fDrawbackFinished)
	{
		if ((m_flNextPrimaryAttack <= gpGlobals->curtime) && (m_flNextSecondaryAttack <= gpGlobals->curtime))
		{
			m_fDrawbackFinished = true;
		}
	}

	if (m_fDrawbackFinished)
	{
		CBasePlayer* pOwner = ToBasePlayer(GetOwner());

		if (pOwner)
		{
			switch (m_AttackPaused)
			{
			case DETPACK_DEPLOY_PAUSED_PRIMARY:
				if (!(pOwner->m_nButtons & IN_ATTACK))
				{
					TossDetpack(pOwner);

					SendWeaponAnim(ACT_VM_THROW);
					m_fDrawbackFinished = false;
					m_AttackPaused = DETPACK_DEPLOY_PAUSED_NO;
				}
				break;
			default:
				if (m_bRedraw)
				{
					Reload();
				}
				break;
			}
		}
	}

	BaseClass::ItemPostFrame();
}

// Check a throw from vecSrc.  If not valid, move the position back along the line to vecEye
void CWeaponDetpack::CheckTossPosition(CBasePlayer* pPlayer, const Vector& vecEye, Vector& vecSrc)
{
	trace_t tr;

	UTIL_TraceHull(vecEye, vecSrc,
		-Vector(NEO_DEPLOYED_DETPACK_RADIUS + 2, NEO_DEPLOYED_DETPACK_RADIUS + 2, NEO_DEPLOYED_DETPACK_RADIUS + 2),
		Vector(NEO_DEPLOYED_DETPACK_RADIUS + 2, NEO_DEPLOYED_DETPACK_RADIUS + 2, NEO_DEPLOYED_DETPACK_RADIUS + 2),
		pPlayer->PhysicsSolidMaskForEntity(), pPlayer, pPlayer->GetCollisionGroup(), &tr);

	if (tr.DidHit())
	{
		vecSrc = tr.endpos;
	}
}

void CWeaponDetpack::TossDetpack(CBasePlayer* pPlayer)
{
	Assert(HasPrimaryAmmo());
	Assert(pPlayer);
	DecrementAmmo(pPlayer);

#ifndef CLIENT_DLL
	Vector	vecEye = pPlayer->EyePosition();
	Vector	vForward, vRight;

	pPlayer->EyeVectors(&vForward, &vRight, NULL);
	Vector vecSrc = vecEye + vForward * 18.0f + vRight * 8.0f;
	CheckTossPosition(pPlayer, vecEye, vecSrc);
	//	vForward[0] += 0.1f;
	vForward[2] += 0.1f;

	Vector vecToss;
	pPlayer->GetVelocity(&vecToss, NULL);
	vecToss += vForward /* * (pPlayer->IsAlive() ? 1.0f : 1.0f)*/;
	Assert(vecToss.IsValid());
	CBaseGrenade* pDet = NEODeployedDetpack_Create(vecSrc, vec3_angle, vecToss, AngularImpulse(600, random->RandomInt(-1200, 1200), 0), pPlayer);

	if (pDet)
	{
		Assert(pPlayer);
		if (!pPlayer->IsAlive())
		{
			auto pPhysicsObject = pDet->VPhysicsGetObject();
			if (pPhysicsObject)
			{
				pPhysicsObject->SetVelocity(&vecToss, NULL);
			}
		}

		pDet->SetDamage(NEO_DETPACK_DAMAGE);
		pDet->SetDamageRadius(NEO_DETPACK_DAMAGE_RADIUS);
	}
#endif

	m_bRedraw = true;

	WeaponSound(SINGLE);

	pPlayer->SetAnimation(PLAYER_ATTACK1);
}

#ifndef CLIENT_DLL
void CWeaponDetpack::Operator_HandleAnimEvent(animevent_t* pEvent, CBaseCombatCharacter* pOperator)
{
	CBasePlayer* pOwner = ToBasePlayer(GetOwner());
	Assert(pOwner);

	bool fThrewGrenade = false;

	switch (pEvent->event)
	{
	case EVENT_WEAPON_SEQUENCE_FINISHED:
		m_fDrawbackFinished = true;
		break;

	case EVENT_WEAPON_THROW:
		TossDetpack(pOwner);
		DecrementAmmo(pOwner);
		fThrewGrenade = true;
		break;

	default:
		BaseClass::Operator_HandleAnimEvent(pEvent, pOperator);
		break;
	}

	if (fThrewGrenade)
	{
		m_flNextPrimaryAttack = gpGlobals->curtime + REDEPLOY_DELAY;
		m_flNextSecondaryAttack = gpGlobals->curtime + REDEPLOY_DELAY;
		m_flTimeWeaponIdle = FLT_MAX; //NOTE: This is set once the animation has finished up!
	}
}
#endif
