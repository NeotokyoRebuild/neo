#include "cbase.h"
#include "weapon_smokegrenade.h"

#include "basegrenade_shared.h"

#ifdef GAME_DLL
#include "neo_smokegrenade.h"
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

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

extern ConVar sv_neo_grenade_throw_intensity;
extern ConVar sv_neo_grenade_lob_intensity;
extern ConVar sv_neo_grenade_roll_intensity;

ConVar sv_neo_infinite_smoke_grenades("sv_neo_infinite_smoke_grenades", "0", FCVAR_CHEAT, "Should smoke grenades use up ammo.", true, 0.0, true, 1.0);

NEO_ACTTABLE(CWeaponSmokeGrenade)

IMPLEMENT_NETWORKCLASS_ALIASED(WeaponSmokeGrenade, DT_WeaponSmokeGrenade)

BEGIN_NETWORK_TABLE(CWeaponSmokeGrenade, DT_WeaponSmokeGrenade)
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
BEGIN_PREDICTION_DATA(CWeaponSmokeGrenade)
DEFINE_PRED_FIELD(m_bRedraw, FIELD_BOOLEAN, FTYPEDESC_INSENDTABLE),
DEFINE_PRED_FIELD(m_fDrawbackFinished, FIELD_BOOLEAN, FTYPEDESC_INSENDTABLE),
DEFINE_PRED_FIELD(m_AttackPaused, FIELD_INTEGER, FTYPEDESC_INSENDTABLE),
END_PREDICTION_DATA()
#endif

LINK_ENTITY_TO_CLASS(weapon_smokegrenade, CWeaponSmokeGrenade);
PRECACHE_WEAPON_REGISTER(weapon_smokegrenade);

#define RETHROW_DELAY 0.5

CWeaponSmokeGrenade::CWeaponSmokeGrenade()
{
	m_bRedraw = false;
}

void CWeaponSmokeGrenade::Precache(void)
{
	BaseClass::Precache();
}

bool CWeaponSmokeGrenade::Deploy(void)
{
	m_bRedraw = false;
	m_fDrawbackFinished = false;

	return BaseClass::Deploy();
}

// Output : Returns true on success, false on failure.
bool CWeaponSmokeGrenade::Holster(CBaseCombatWeapon* pSwitchingTo)
{
	m_bRedraw = false;
	m_fDrawbackFinished = false;

	return BaseClass::Holster(pSwitchingTo);
}

// Output : Returns true on success, false on failure.
bool CWeaponSmokeGrenade::Reload(void)
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

void CWeaponSmokeGrenade::SecondaryAttack(void)
{
	if (m_bRedraw)
		return;

	if (!HasPrimaryAmmo())
		return;

	CBasePlayer* pPlayer = ToBasePlayer(GetOwner());

	if (pPlayer == NULL)
		return;

	// Note that this is a secondary attack and prepare the grenade attack to pause.
	m_AttackPaused = GRENADE_PAUSED_SECONDARY;
	SendWeaponAnim(ACT_VM_PULLPIN);

	// Don't let weapon idle interfere in the middle of a throw!
	m_flTimeWeaponIdle = FLT_MAX;
	m_flNextPrimaryAttack = gpGlobals->curtime + RETHROW_DELAY;

	// If I'm now out of ammo, switch away
	if (!HasPrimaryAmmo())
	{
		pPlayer->SwitchToNextBestWeapon(this);
	}
}

void CWeaponSmokeGrenade::PrimaryAttack(void)
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

	// Note that this is a primary attack and prepare the grenade attack to pause.
	m_AttackPaused = GRENADE_PAUSED_PRIMARY;
	SendWeaponAnim(ACT_VM_PULLPIN);

	m_flTimeWeaponIdle = FLT_MAX;
	m_flNextPrimaryAttack = gpGlobals->curtime + RETHROW_DELAY;

	// If I'm now out of ammo, switch away
	if (!HasPrimaryAmmo())
	{
		pPlayer->SwitchToNextBestWeapon(this);
	}
}

void CWeaponSmokeGrenade::DecrementAmmo(CBaseCombatCharacter* pOwner)
{
	pOwner->RemoveAmmo(1, m_iPrimaryAmmoType);
}

void CWeaponSmokeGrenade::ItemPostFrame(void)
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
			case GRENADE_PAUSED_PRIMARY:
				if (!(pOwner->m_nButtons & IN_ATTACK))
				{
					ThrowGrenade(pOwner);

					SendWeaponAnim(ACT_VM_THROW);
					m_fDrawbackFinished = false;
					m_AttackPaused = GRENADE_PAUSED_NO;
				}
				break;

			case GRENADE_PAUSED_SECONDARY:
				if (!(pOwner->m_nButtons & IN_ATTACK2))
				{
					//See if we're ducking
					if (pOwner->m_nButtons & IN_DUCK)
					{
						RollGrenade(pOwner);
						//Send the weapon animation
						SendWeaponAnim(ACT_VM_THROW);
					}
					else
					{
						LobGrenade(pOwner);
						//Send the weapon animation
						SendWeaponAnim(ACT_VM_THROW);
					}

					m_fDrawbackFinished = false;
					m_AttackPaused = GRENADE_PAUSED_NO;
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
void CWeaponSmokeGrenade::CheckThrowPosition(CBasePlayer* pPlayer, const Vector& vecEye, Vector& vecSrc)
{
	trace_t tr;

	UTIL_TraceHull(vecEye, vecSrc, -Vector(GRENADE_RADIUS + 2, GRENADE_RADIUS + 2, GRENADE_RADIUS + 2), Vector(GRENADE_RADIUS + 2, GRENADE_RADIUS + 2, GRENADE_RADIUS + 2),
		pPlayer->PhysicsSolidMaskForEntity(), pPlayer, pPlayer->GetCollisionGroup(), &tr);

	if (tr.DidHit())
	{
		vecSrc = tr.endpos;
	}
}

void NEODropPrimedSmokeGrenade(CNEO_Player* pPlayer, CBaseCombatWeapon* pSmokeGrenade)
{
	auto pWeaponSmoke = dynamic_cast<CWeaponSmokeGrenade*>(pSmokeGrenade);

	if (pWeaponSmoke)
	{
		pWeaponSmoke->ThrowGrenade(pPlayer);
		pWeaponSmoke->DecrementAmmo(pPlayer);
	}
}

void CWeaponSmokeGrenade::ThrowGrenade(CBasePlayer* pPlayer)
{
	if (!sv_neo_infinite_smoke_grenades.GetBool())
	{
		Assert(HasPrimaryAmmo());
		DecrementAmmo(pPlayer);
	}

#ifndef CLIENT_DLL
	Vector	vecEye = pPlayer->EyePosition();
	Vector	vForward, vRight;

	pPlayer->EyeVectors(&vForward, &vRight, NULL);
	Vector vecSrc = vecEye + vForward * 18.0f + vRight * 8.0f;
	CheckThrowPosition(pPlayer, vecEye, vecSrc);
	//	vForward[0] += 0.1f;
	vForward[2] += 0.1f;

	Vector vecThrow;
	pPlayer->GetVelocity(&vecThrow, NULL);
	vecThrow += vForward * (pPlayer->IsAlive() ? sv_neo_grenade_throw_intensity.GetFloat() : 1.0f);
	Assert(vecThrow.IsValid());
	CBaseGrenade* pGrenade = NEOSmokegrenade_Create(vecSrc, vec3_angle, vecThrow, AngularImpulse(600, random->RandomInt(-1200, 1200), 0), pPlayer);

	if (pGrenade)
	{
		Assert(pPlayer);
		if (!pPlayer->IsAlive())
		{
			IPhysicsObject* pPhysicsObject = pGrenade->VPhysicsGetObject();
			if (pPhysicsObject)
			{
				pPhysicsObject->SetVelocity(&vecThrow, NULL);
			}
		}

		pGrenade->SetDamage(0);
		pGrenade->SetDamageRadius(0);
	}
#endif

	m_bRedraw = true;

	// player "shoot" animation
	pPlayer->SetAnimation(PLAYER_ATTACK1);
}

void CWeaponSmokeGrenade::LobGrenade(CBasePlayer* pPlayer)
{
	// Binds hack: we want grenade secondary attack to trigger on aim, not the attack2 bind.
	if (pPlayer->m_afButtonPressed & IN_ATTACK2)
	{
		return;
	}
	else if (!sv_neo_infinite_smoke_grenades.GetBool())
	{
		Assert(HasPrimaryAmmo());
		DecrementAmmo(pPlayer);
	}

#ifndef CLIENT_DLL
	Vector	vecEye = pPlayer->EyePosition();
	Vector	vForward, vRight;

	pPlayer->EyeVectors(&vForward, &vRight, NULL);
	Vector vecSrc = vecEye + vForward * 18.0f + vRight * 8.0f + Vector(0, 0, -8);
	CheckThrowPosition(pPlayer, vecEye, vecSrc);

	Vector vecThrow;
	pPlayer->GetVelocity(&vecThrow, NULL);
	vecThrow += vForward * sv_neo_grenade_lob_intensity.GetFloat() + Vector(0, 0, 50);
	CBaseGrenade* pGrenade = NEOSmokegrenade_Create(vecSrc, vec3_angle, vecThrow, AngularImpulse(200, random->RandomInt(-600, 600), 0), pPlayer);

	if (pGrenade)
	{
		pGrenade->SetDamage(0);
		pGrenade->SetDamageRadius(0);
	}
#endif

	WeaponSound(WPN_DOUBLE);

	// player "shoot" animation
	pPlayer->SetAnimation(PLAYER_ATTACK1);

	m_bRedraw = true;
}

void CWeaponSmokeGrenade::RollGrenade(CBasePlayer* pPlayer)
{
	// Binds hack: we want grenade secondary attack to trigger on aim, not the attack2 bind.
	if (pPlayer->m_afButtonPressed & IN_ATTACK2)
	{
		return;
	}
	else if (!sv_neo_infinite_smoke_grenades.GetBool())
	{
		Assert(HasPrimaryAmmo());
		DecrementAmmo(pPlayer);
	}

#ifndef CLIENT_DLL
	// BUGBUG: Hardcoded grenade width of 4 - better not change the model :)
	Vector vecSrc;
	pPlayer->CollisionProp()->NormalizedToWorldSpace(Vector(0.5f, 0.5f, 0.0f), &vecSrc);
	vecSrc.z += GRENADE_RADIUS;

	Vector vecFacing = pPlayer->BodyDirection2D();
	// no up/down direction
	vecFacing.z = 0;
	VectorNormalize(vecFacing);
	trace_t tr;
	UTIL_TraceLine(vecSrc, vecSrc - Vector(0, 0, 16), MASK_PLAYERSOLID, pPlayer, COLLISION_GROUP_NONE, &tr);
	if (tr.fraction != 1.0)
	{
		// compute forward vec parallel to floor plane and roll grenade along that
		Vector tangent;
		CrossProduct(vecFacing, tr.plane.normal, tangent);
		CrossProduct(tr.plane.normal, tangent, vecFacing);
	}
	vecSrc += (vecFacing * 18.0);
	CheckThrowPosition(pPlayer, pPlayer->WorldSpaceCenter(), vecSrc);

	Vector vecThrow;
	pPlayer->GetVelocity(&vecThrow, NULL);
	vecThrow += vecFacing * sv_neo_grenade_roll_intensity.GetFloat();
	// put it on its side
	QAngle orientation(0, pPlayer->GetLocalAngles().y, -90);
	// roll it
	AngularImpulse rotSpeed(0, 0, 720);
	CBaseGrenade* pGrenade = NEOSmokegrenade_Create(vecSrc, orientation, vecThrow, rotSpeed, pPlayer);

	if (pGrenade)
	{
		pGrenade->SetDamage(0);
		pGrenade->SetDamageRadius(0);
	}

#endif

	WeaponSound(SPECIAL1);

	// player "shoot" animation
	pPlayer->SetAnimation(PLAYER_ATTACK1);

	m_bRedraw = true;
}

#ifndef CLIENT_DLL
void CWeaponSmokeGrenade::Operator_HandleAnimEvent(animevent_t* pEvent, CBaseCombatCharacter* pOperator)
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
		ThrowGrenade(pOwner);
		DecrementAmmo(pOwner);
		fThrewGrenade = true;
		break;

	case EVENT_WEAPON_THROW2:
		RollGrenade(pOwner);
		DecrementAmmo(pOwner);
		fThrewGrenade = true;
		break;

	case EVENT_WEAPON_THROW3:
		LobGrenade(pOwner);
		DecrementAmmo(pOwner);
		fThrewGrenade = true;
		break;

	default:
		BaseClass::Operator_HandleAnimEvent(pEvent, pOperator);
		break;
	}

	if (fThrewGrenade)
	{
		m_flNextPrimaryAttack = gpGlobals->curtime + RETHROW_DELAY;
		m_flNextSecondaryAttack = gpGlobals->curtime + RETHROW_DELAY;
		m_flTimeWeaponIdle = FLT_MAX; //NOTE: This is set once the animation has finished up!
	}
}
#endif
