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
#include "eventqueue.h"
#endif

#include "effect_dispatch_data.h"
#include "in_buttons.h"

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

IMPLEMENT_NETWORKCLASS_ALIASED(WeaponDetpack, DT_WeaponDetpack)

BEGIN_NETWORK_TABLE(CWeaponDetpack, DT_WeaponDetpack)
#ifdef CLIENT_DLL
	RecvPropBool(RECVINFO(m_fDrawbackFinished)),
	RecvPropBool(RECVINFO(m_bWantsToThrowThisDetpack)),
	RecvPropBool(RECVINFO(m_bThisDetpackHasBeenThrown)),
	RecvPropBool(RECVINFO(m_bRemoteHasBeenTriggered)),
#else
	SendPropBool(SENDINFO(m_fDrawbackFinished)),
	SendPropBool(SENDINFO(m_bWantsToThrowThisDetpack)),
	SendPropBool(SENDINFO(m_bThisDetpackHasBeenThrown)),
	SendPropBool(SENDINFO(m_bRemoteHasBeenTriggered)),
#endif
END_NETWORK_TABLE()

#ifdef CLIENT_DLL
BEGIN_PREDICTION_DATA(CWeaponDetpack)
	DEFINE_PRED_FIELD(m_fDrawbackFinished, FIELD_BOOLEAN, FTYPEDESC_INSENDTABLE),
	DEFINE_PRED_FIELD(m_bWantsToThrowThisDetpack, FIELD_BOOLEAN, FTYPEDESC_INSENDTABLE),
	DEFINE_PRED_FIELD(m_bThisDetpackHasBeenThrown, FIELD_BOOLEAN, FTYPEDESC_INSENDTABLE),
	DEFINE_PRED_FIELD(m_bRemoteHasBeenTriggered, FIELD_BOOLEAN, FTYPEDESC_INSENDTABLE),
END_PREDICTION_DATA()
#endif

NEO_IMPLEMENT_ACTTABLE(CWeaponDetpack)

LINK_ENTITY_TO_CLASS(weapon_remotedet, CWeaponDetpack);

#ifdef GAME_DLL
BEGIN_DATADESC(CWeaponDetpack)
	DEFINE_FIELD(m_fDrawbackFinished, FIELD_BOOLEAN),
	DEFINE_FIELD(m_bWantsToThrowThisDetpack, FIELD_BOOLEAN),
	DEFINE_FIELD(m_bThisDetpackHasBeenThrown, FIELD_BOOLEAN),
	DEFINE_FIELD(m_bRemoteHasBeenTriggered, FIELD_BOOLEAN),
END_DATADESC()
#endif

PRECACHE_WEAPON_REGISTER(weapon_remotedet);

ConVar sv_neo_detpack_xp_limit("sv_neo_detpack_xp_limit", "4", FCVAR_REPLICATED,
	"How many XP are required to unlock the detpack for recons (or -1 for no limit).", true, -1.0, false, 0.0);

int CWeaponDetpack::GetNeoWepXPCost(const int neoClass) const
{
	return sv_neo_detpack_xp_limit.GetInt();
}

CWeaponDetpack::CWeaponDetpack()
{
	m_bWantsToThrowThisDetpack = false;
	m_bThisDetpackHasBeenThrown = false;
	m_bRemoteHasBeenTriggered = false;

#ifdef GAME_DLL
	m_pDetpack = NULL;
#endif
	SetViewOffset(Vector(0, 0, 1.0));
}

void CWeaponDetpack::Precache(void)
{
	BaseClass::Precache();
}

bool CWeaponDetpack::Deploy(void)
{
	const bool res = BaseClass::Deploy();

	m_fDrawbackFinished = false;

	if (m_bThisDetpackHasBeenThrown)
	{
		SendWeaponAnim(ACT_VM_DRAW_DEPLOYED);
	}

	return res;
}

// Output : Returns true on success, false on failure.
bool CWeaponDetpack::Holster(CBaseCombatWeapon* pSwitchingTo)
{

	m_fDrawbackFinished = false;

	if (m_bThisDetpackHasBeenThrown)
	{
		SendWeaponAnim(ACT_VM_PULLBACK);
	}
	else if (m_bWantsToThrowThisDetpack)
	{
		m_bWantsToThrowThisDetpack = false;
	}

	const bool res = BaseClass::Holster(pSwitchingTo);

	return res;
}

void CWeaponDetpack::PrimaryAttack(void)
{
	if (ShootingIsPrevented())
	{
		return;
	}

	auto* pPlayer = static_cast<CNEO_Player*>(GetOwner());
	if (!pPlayer)
	{
		Assert(false);
		return;
	}
	else if (m_bRemoteHasBeenTriggered)
	{
		return;
	}

	if ((m_bThisDetpackHasBeenThrown) && (gpGlobals->curtime > m_flNextPrimaryAttack))
	{
		m_bRemoteHasBeenTriggered = true;
#if(0)
		DevMsg("Pulling remote trigger\n");
#endif
		SendWeaponAnim(ACT_VM_PRIMARYATTACK_DEPLOYED);
		pPlayer->DoAnimationEvent(PLAYERANIMEVENT_ATTACK_PRIMARY);
		m_flNextPrimaryAttack = gpGlobals->curtime + (SequenceDuration() * 0.5);
		m_flTimeWeaponIdle = FLT_MAX;
	}
	else
	{
		if (!m_bWantsToThrowThisDetpack)
		{
			m_bWantsToThrowThisDetpack = true;
			WeaponSound(SINGLE);
#if(0)
			DevMsg("Preparing primary attack\n");
#endif
			SendWeaponAnim(ACT_VM_PRIMARYATTACK);
			m_bLowered = false;
			m_flNextPrimaryAttack = gpGlobals->curtime + SequenceDuration();
			m_flTimeWeaponIdle = gpGlobals->curtime + SequenceDuration();
		}
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
		auto pOwner = static_cast<CNEO_Player*>(GetOwner());

		if ((m_bRemoteHasBeenTriggered) && (gpGlobals->curtime > m_flNextPrimaryAttack))
		{
#ifdef GAME_DLL
			if (m_pDetpack)
			{
#if(0)
				DevMsg("REMOTE ATK\n");
#endif
				g_EventQueue.AddEvent(m_pDetpack, "RemoteDetonate", 0, GetOwner(), GetOwner());
				// m_pDetpack->Detonate();
				m_pDetpack = NULL;
			}
			else
			{
				if (pOwner)
				{
					if (pOwner->GetActiveWeapon() == this)
					{
						if (!pOwner->SwitchToNextBestWeapon(this))
						{
							pOwner->Weapon_Drop(this);
						}
						else
						{
							pOwner->GetActiveWeapon()->SendWeaponAnim(ACT_VM_DRAW);
						}
					}
				}
				UTIL_Remove(this);
			}
#endif
		}
		if (pOwner && m_bWantsToThrowThisDetpack)
		{
			if (!m_bThisDetpackHasBeenThrown)
			{
				if (gpGlobals->curtime > m_flNextPrimaryAttack)
				{
					m_bThisDetpackHasBeenThrown = true;
					TossDetpack(ToBasePlayer(pOwner));
					pOwner->DoAnimationEvent(PLAYERANIMEVENT_ATTACK_PRIMARY);
					m_flNextPrimaryAttack = gpGlobals->curtime + SequenceDuration();
					m_fDrawbackFinished = false;
				}
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

bool CWeaponDetpack::CanDrop()
{
	auto owner = GetOwner();
	return m_bThisDetpackHasBeenThrown && owner && !GetOwner()->IsAlive();
}

void CWeaponDetpack::TossDetpack(CBasePlayer* pPlayer)
{
	Assert(HasPrimaryAmmo());
	Assert(pPlayer);
	DecrementAmmo(pPlayer);

#if(0)
	DevMsg("TossDetpack\n");
#endif

#ifndef CLIENT_DLL
	QAngle angThrow = pPlayer->LocalEyeAngles();

	Vector vForward, vRight, vUp;

	if (angThrow.x < 90)
		angThrow.x = -10 + angThrow.x * ((90 + 10) / 90.0);
	else
	{
		angThrow.x = 360.0f - angThrow.x;
		angThrow.x = -10 + angThrow.x * -((90 - 10) / 90.0);
	}

	float flVel = 0;

	AngleVectors(angThrow, &vForward, &vRight, &vUp);

	Vector vecSrc = pPlayer->GetAbsOrigin() + pPlayer->GetViewOffset();

	vecSrc += vForward * 16;

	Vector vecThrow = pPlayer->GetAbsVelocity();
	m_pDetpack = static_cast<CNEODeployedDetpack*>(NEODeployedDetpack_Create(vecSrc, vec3_angle, vecThrow, AngularImpulse(600, random->RandomInt(-1200, 1200), 0), pPlayer));

	if (m_pDetpack)
	{
		m_pDetpack->SetDamage(NEO_DETPACK_DAMAGE);
		m_pDetpack->SetDamageRadius(NEO_DETPACK_DAMAGE_RADIUS);
	}
	else
	{
		Assert(false);
	}
#endif
	if (GetOwner()->IsPlayer()) // NEO NOTE (Adam) if else taken from CBaseCombatWeapon::Equip, this must be what was fixing the viewmodel previously after dropping and picking up the detremote
	{
		SetModel(GetViewModel());
	}
	else
	{
		SetModel(GetWorldModel());
	}
	Precache();
	DefaultDeploy((char*)GetViewModel(), (char*)GetModel(), ACT_VM_DRAW_DEPLOYED, (char*)GetAnimPrefix());

	pPlayer->SetAnimation(PLAYER_ATTACK1);
}

bool CWeaponDetpack::CanBePickedUpByClass(int classId)
{
	return classId != NEO_CLASS_JUGGERNAUT;
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
		m_flNextPrimaryAttack = gpGlobals->curtime + 1.0;
		m_flNextSecondaryAttack = gpGlobals->curtime + 1.0;
		m_flTimeWeaponIdle = FLT_MAX; //NOTE: This is set once the animation has finished up!
	}
}
#endif
