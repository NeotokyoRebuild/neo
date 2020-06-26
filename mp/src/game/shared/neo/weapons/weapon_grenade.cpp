#include "cbase.h"
#include "weapon_grenade.h"

#include "basegrenade_shared.h"

#ifdef GAME_DLL
#include "neo_grenade.h"
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

ConVar sv_neo_infinite_frag_grenades("sv_neo_infinite_frag_grenades", "0", FCVAR_CHEAT, "Should frag grenades use up ammo.", true, 0.0, true, 1.0);
ConVar sv_neo_grenade_throw_intensity("sv_neo_grenade_throw_intensity", "900.0", FCVAR_CHEAT, "How strong should regular grenade throws be.", true, 0.0, true, 9999.9); // 750 is the original NT impulse. Seems incorrect though, weight or phys difference?
ConVar sv_neo_grenade_lob_intensity("sv_neo_grenade_lob_intensity", "375.0", FCVAR_CHEAT, "How strong should underhand grenade lobs be.", true, 0.0, true, 9999.9); // No such thing in original, but chose half of 750
ConVar sv_neo_grenade_roll_intensity("sv_neo_grenade_roll_intensity", "375.0", FCVAR_CHEAT, "How strong should underhand grenade rolls be.", true, 0.0, true, 9999.9);
ConVar sv_neo_grenade_blast_damage("sv_neo_grenade_blast_damage", "100.0", FCVAR_CHEAT, "How much damage should a grenade blast deal.", true, 0.0, true, 999.9);
ConVar sv_neo_grenade_blast_radius("sv_neo_grenade_blast_radius", "250.0", FCVAR_CHEAT, "How large should the grenade blast radius be.", true, 0.0, true, 9999.9);
ConVar sv_neo_grenade_fuse_timer("sv_neo_grenade_fuse_timer", "2.16", FCVAR_CHEAT, "How long in seconds until a frag grenade explodes.", true, 0.1, true, 60.0); // Measured as 2.15999... in NT, ie. < 2.16

IMPLEMENT_NETWORKCLASS_ALIASED(WeaponGrenade, DT_WeaponGrenade)

BEGIN_NETWORK_TABLE(CWeaponGrenade, DT_WeaponGrenade)
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
BEGIN_PREDICTION_DATA(CWeaponGrenade)
	DEFINE_PRED_FIELD(m_bRedraw, FIELD_BOOLEAN, FTYPEDESC_INSENDTABLE),
	DEFINE_PRED_FIELD(m_fDrawbackFinished, FIELD_BOOLEAN, FTYPEDESC_INSENDTABLE),
	DEFINE_PRED_FIELD(m_AttackPaused, FIELD_INTEGER, FTYPEDESC_INSENDTABLE),
END_PREDICTION_DATA()
#endif

NEO_IMPLEMENT_ACTTABLE(CWeaponGrenade)

LINK_ENTITY_TO_CLASS(weapon_grenade, CWeaponGrenade);

#ifdef GAME_DLL
BEGIN_DATADESC(CWeaponGrenade)
	DEFINE_FIELD(m_bRedraw, FIELD_BOOLEAN),
	DEFINE_FIELD(m_fDrawbackFinished, FIELD_BOOLEAN),
	DEFINE_FIELD(m_AttackPaused, FIELD_INTEGER),
END_DATADESC()
#endif

PRECACHE_WEAPON_REGISTER(weapon_grenade);

#define RETHROW_DELAY 0.5

CWeaponGrenade::CWeaponGrenade()
{
	m_bRedraw = false;
}

void CWeaponGrenade::Precache(void)
{
	BaseClass::Precache();
}

bool CWeaponGrenade::Deploy(void)
{
	m_bRedraw = false;
	m_fDrawbackFinished = false;

	return BaseClass::Deploy();
}

// Output : Returns true on success, false on failure.
bool CWeaponGrenade::Holster(CBaseCombatWeapon *pSwitchingTo)
{
	m_bRedraw = false;
	m_fDrawbackFinished = false;

	return BaseClass::Holster(pSwitchingTo);
}

// Output : Returns true on success, false on failure.
bool CWeaponGrenade::Reload(void)
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

void CWeaponGrenade::SecondaryAttack(void)
{
	if (ShootingIsPrevented())
	{
		return;
	}

	if (m_bRedraw || !HasPrimaryAmmo())
	{
		return;
	}

	auto pPlayer = ToBasePlayer(GetOwner());
	if (!pPlayer)
	{
		return;
	}

	if (m_AttackPaused != GRENADE_PAUSED_SECONDARY)
	{
		// Note that this is a secondary attack and prepare the grenade attack to pause.
		m_AttackPaused = GRENADE_PAUSED_SECONDARY;
		SendWeaponAnim(ACT_VM_PULLPIN);

		// Don't let weapon idle interfere in the middle of a throw!
		m_flTimeWeaponIdle = FLT_MAX;
		m_flNextPrimaryAttack = gpGlobals->curtime + RETHROW_DELAY;
	}
	// If I'm now out of ammo, switch away
	if (!HasPrimaryAmmo())
	{
		pPlayer->SwitchToNextBestWeapon(this);
	}
}

void CWeaponGrenade::PrimaryAttack(void)
{
	if (ShootingIsPrevented())
	{
		return;
	}

	if (m_bRedraw || !HasPrimaryAmmo())
	{
		return;
	}

	auto pPlayer = ToBasePlayer(GetOwner());
	if (!pPlayer)
	{
		return;
	}

	if (m_AttackPaused != GRENADE_PAUSED_PRIMARY)
	{
		// Note that this is a primary attack and prepare the grenade attack to pause.
		m_AttackPaused = GRENADE_PAUSED_PRIMARY;
		SendWeaponAnim(ACT_VM_PULLPIN);

		// Don't let weapon idle interfere in the middle of a throw!
		m_flTimeWeaponIdle = FLT_MAX;
		m_flNextPrimaryAttack = gpGlobals->curtime + RETHROW_DELAY;
	}
	// If I'm now out of ammo, switch away
	if (!HasPrimaryAmmo())
	{
		pPlayer->SwitchToNextBestWeapon(this);
	}
}

void CWeaponGrenade::DecrementAmmo(CBaseCombatCharacter *pOwner)
{
	pOwner->RemoveAmmo(1, m_iPrimaryAmmoType);
}

void CWeaponGrenade::ItemPostFrame(void)
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
		CBasePlayer *pOwner = ToBasePlayer(GetOwner());

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
void CWeaponGrenade::CheckThrowPosition(CBasePlayer *pPlayer, const Vector &vecEye, Vector &vecSrc)
{
	trace_t tr;

	UTIL_TraceHull(vecEye, vecSrc, -Vector(GRENADE_RADIUS + 2, GRENADE_RADIUS + 2, GRENADE_RADIUS + 2), Vector(GRENADE_RADIUS + 2, GRENADE_RADIUS + 2, GRENADE_RADIUS + 2),
		pPlayer->PhysicsSolidMaskForEntity(), pPlayer, pPlayer->GetCollisionGroup(), &tr);

	if (tr.DidHit())
	{
		vecSrc = tr.endpos;
	}
}

void NEODropPrimedFragGrenade(CNEO_Player *pPlayer, CBaseCombatWeapon *pGrenade)
{
	auto pWeaponFrag = dynamic_cast<CWeaponGrenade*>(pGrenade);

	if (pWeaponFrag)
	{
		pWeaponFrag->ThrowGrenade(pPlayer);
		pWeaponFrag->DecrementAmmo(pPlayer);
	}
}

void CWeaponGrenade::ThrowGrenade(CBasePlayer *pPlayer)
{
	if (!sv_neo_infinite_frag_grenades.GetBool())
	{
		Assert(HasPrimaryAmmo());
		Assert(pPlayer);
		DecrementAmmo(pPlayer);
	}

#ifndef CLIENT_DLL
	Vector	vecEye = pPlayer->EyePosition();
	Vector	vForward, vRight;

	pPlayer->EyeVectors(&vForward, &vRight, NULL);
	Vector vecSrc = vecEye + vForward * 18.0f + vRight * 8.0f;
	CheckThrowPosition(pPlayer, vecEye, vecSrc);
	vForward.z += 0.1f;

	// Upwards at 1 + 0.1226 direction sampled from original NT frag spawn event to next tick pos delta.
	VectorMA(vForward, 0.1226, Vector(0, 0, 1), vForward);
	Assert(vForward.IsValid());

	Vector vecThrow;
	pPlayer->GetVelocity(&vecThrow, NULL);
	vecThrow += vForward * (pPlayer->IsAlive() ? sv_neo_grenade_throw_intensity.GetFloat() : 1.0f);
	Assert(vecThrow.IsValid());

	// Sampled angular impulses from original NT frags:
	// x: -584, 630, -1028, 967, -466, -535 (random, seems roughly in the same (-1200, 1200) range)
	// y: 0 (constant)
	// z: 600 (constant)
	// This SDK original impulse line: AngularImpulse(600, random->RandomInt(-1200, 1200), 0)

	CBaseGrenade *pGrenade = NEOFraggrenade_Create(vecSrc, vec3_angle, vecThrow, AngularImpulse(random->RandomInt(-1200, 1200), 0, 600), pPlayer, sv_neo_grenade_fuse_timer.GetFloat(), false);

	if (pGrenade)
	{
		Assert(pPlayer);
		if (!pPlayer->IsAlive())
		{
			IPhysicsObject *pPhysicsObject = pGrenade->VPhysicsGetObject();
			if (pPhysicsObject)
			{
				pPhysicsObject->SetVelocity(&vecThrow, NULL);
			}
		}

		pGrenade->SetDamage(sv_neo_grenade_blast_damage.GetFloat());
		pGrenade->SetDamageRadius(sv_neo_grenade_blast_radius.GetFloat());
	}
#endif

	m_bRedraw = true;

	WeaponSound(SINGLE);

	// player "shoot" animation
	pPlayer->SetAnimation(PLAYER_ATTACK1);

	// If I'm now out of ammo, switch away
	if (!HasPrimaryAmmo())
	{
		pPlayer->SwitchToNextBestWeapon(this);
	}
}

void CWeaponGrenade::LobGrenade(CBasePlayer *pPlayer)
{
	// Binds hack: we want grenade secondary attack to trigger on aim, not the attack2 bind.
	if (pPlayer->m_afButtonPressed & IN_ATTACK2)
	{
		return;
	}
	else if (!sv_neo_infinite_frag_grenades.GetBool())
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
	CBaseGrenade *pGrenade = NEOFraggrenade_Create(vecSrc, vec3_angle, vecThrow, AngularImpulse(200, random->RandomInt(-600, 600), 0), pPlayer, sv_neo_grenade_fuse_timer.GetFloat(), false);

	if (pGrenade)
	{
		pGrenade->SetDamage(sv_neo_grenade_blast_damage.GetFloat());
		pGrenade->SetDamageRadius(sv_neo_grenade_blast_radius.GetFloat());
	}
#endif

	WeaponSound(WPN_DOUBLE);

	// player "shoot" animation
	pPlayer->SetAnimation(PLAYER_ATTACK1);

	m_bRedraw = true;

	// If I'm now out of ammo, switch away
	if (!HasPrimaryAmmo())
	{
		pPlayer->SwitchToNextBestWeapon(this);
	}
}

void CWeaponGrenade::RollGrenade(CBasePlayer *pPlayer)
{
	// Binds hack: we want grenade secondary attack to trigger on aim, not the attack2 bind.
	if (pPlayer->m_afButtonPressed & IN_ATTACK2)
	{
		return;
	}
	else if (!sv_neo_infinite_frag_grenades.GetBool())
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
	CBaseGrenade *pGrenade = NEOFraggrenade_Create(vecSrc, orientation, vecThrow, rotSpeed, pPlayer, sv_neo_grenade_fuse_timer.GetFloat(), false);

	if (pGrenade)
	{
		pGrenade->SetDamage(sv_neo_grenade_blast_damage.GetFloat());
		pGrenade->SetDamageRadius(sv_neo_grenade_blast_radius.GetFloat());
	}

#endif

	WeaponSound(SPECIAL1);

	// player "shoot" animation
	pPlayer->SetAnimation(PLAYER_ATTACK1);

	m_bRedraw = true;

	// If I'm now out of ammo, switch away
	if (!HasPrimaryAmmo())
	{
		pPlayer->SwitchToNextBestWeapon(this);
	}
}

#ifndef CLIENT_DLL
void CWeaponGrenade::Operator_HandleAnimEvent(animevent_t *pEvent, CBaseCombatCharacter *pOperator)
{
	CBasePlayer *pOwner = ToBasePlayer(GetOwner());
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
