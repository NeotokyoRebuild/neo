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
#include "sdk/sdk_basegrenade_projectile.h"
#endif

#include "effect_dispatch_data.h"

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

ConVar sv_neo_infinite_frag_grenades("sv_neo_infinite_frag_grenades", "0", FCVAR_REPLICATED | FCVAR_CHEAT, "Should frag grenades use up ammo.", true, 0.0, true, 1.0);
ConVar sv_neo_grenade_throw_intensity("sv_neo_grenade_throw_intensity", "750.0", FCVAR_REPLICATED | FCVAR_CHEAT, "How strong should regular grenade throws be.", true, 0.0, true, 9999.9);
ConVar sv_neo_grenade_blast_damage("sv_neo_grenade_blast_damage", "200.0", FCVAR_REPLICATED | FCVAR_CHEAT, "How much damage should a grenade blast deal.", true, 0.0, true, 999.9);
ConVar sv_neo_grenade_blast_radius("sv_neo_grenade_blast_radius", "250.0", FCVAR_REPLICATED | FCVAR_CHEAT, "How large should the grenade blast radius be.", true, 0.0, true, 9999.9);
ConVar sv_neo_grenade_fuse_timer("sv_neo_grenade_fuse_timer", "2.16", FCVAR_REPLICATED | FCVAR_CHEAT, "How long in seconds until a frag grenade explodes.", true, 0.1, true, 60.0); // Measured as 2.15999... in NT, ie. < 2.16
ConVar sv_neo_grenade_cor("sv_neo_grenade_cor", "0.45", FCVAR_CHEAT | FCVAR_REPLICATED, "Neotokyo frag coefficient of restitution", true, 0.0, true, 1.0);
ConVar sv_neo_grenade_gravity("sv_neo_grenade_gravity", "0.4", FCVAR_CHEAT | FCVAR_REPLICATED, "Grenade gravity", true, 0, true, 1);
ConVar sv_neo_grenade_friction("sv_neo_grenade_friction", "0.2", FCVAR_CHEAT | FCVAR_REPLICATED, "Grenade friction", true, 0, true, 1);

IMPLEMENT_NETWORKCLASS_ALIASED(WeaponGrenade, DT_WeaponGrenade)

BEGIN_NETWORK_TABLE(CWeaponGrenade, DT_WeaponGrenade)
#ifdef CLIENT_DLL
	RecvPropBool(RECVINFO(m_bRedraw)),
	RecvPropBool(RECVINFO(m_bDrawbackFinished)),
	RecvPropInt(RECVINFO(m_AttackPaused)),
#else
	SendPropBool(SENDINFO(m_bRedraw)),
	SendPropBool(SENDINFO(m_bDrawbackFinished)),
	SendPropInt(SENDINFO(m_AttackPaused)),
#endif
END_NETWORK_TABLE()

#ifdef CLIENT_DLL
BEGIN_PREDICTION_DATA(CWeaponGrenade)
	DEFINE_PRED_FIELD(m_bRedraw, FIELD_BOOLEAN, FTYPEDESC_INSENDTABLE),
	DEFINE_PRED_FIELD(m_bDrawbackFinished, FIELD_BOOLEAN, FTYPEDESC_INSENDTABLE),
	DEFINE_PRED_FIELD(m_AttackPaused, FIELD_INTEGER, FTYPEDESC_INSENDTABLE),
END_PREDICTION_DATA()
#endif

NEO_IMPLEMENT_ACTTABLE(CWeaponGrenade)

LINK_ENTITY_TO_CLASS(weapon_grenade, CWeaponGrenade);

#ifdef GAME_DLL
BEGIN_DATADESC(CWeaponGrenade)
	DEFINE_FIELD(m_bRedraw, FIELD_BOOLEAN),
	DEFINE_FIELD(m_bDrawbackFinished, FIELD_BOOLEAN),
	DEFINE_FIELD(m_AttackPaused, FIELD_INTEGER),
END_DATADESC()
#endif

PRECACHE_WEAPON_REGISTER(weapon_grenade);

#define RETHROW_DELAY 0.5

CWeaponGrenade::CWeaponGrenade()
{
	m_bRedraw = false;
	SetViewOffset(Vector(0, 0, 1.0));
}

void CWeaponGrenade::Precache(void)
{
	BaseClass::Precache();
}

bool CWeaponGrenade::Deploy(void)
{
	m_bRedraw = false;
	m_bDrawbackFinished = false;

	return BaseClass::Deploy();
}

// Output : Returns true on success, false on failure.
bool CWeaponGrenade::Holster(CBaseCombatWeapon *pSwitchingTo)
{
	m_bRedraw = false;
	m_bDrawbackFinished = false;
	m_AttackPaused = GRENADE_PAUSED_NO;

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
		m_flNextPrimaryAttack = m_flNextSecondaryAttack = m_flTimeWeaponIdle = gpGlobals->curtime + SequenceDuration();

		//Mark this as done
		m_bRedraw = false;
	}

	return true;
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

	if (m_flNextPrimaryAttack > gpGlobals->curtime)
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
		m_bDrawbackFinished = false;
	}
}

void CWeaponGrenade::DecrementAmmo(CBaseCombatCharacter *pOwner)
{
	m_iPrimaryAmmoCount -= 1;
}

void CWeaponGrenade::ItemPostFrame(void)
{
	if (!HasPrimaryAmmo() && (GetIdealActivity() == ACT_VM_IDLE || GetIdealActivity() == ACT_VM_DRAW)) {
		// Finished Throwing Animation, switch to next weapon and destroy this one
#ifdef GAME_DLL
		CBasePlayer* pOwner = ToBasePlayer(GetOwner());
		if (pOwner)
		{
			pOwner->SwitchToNextBestWeapon(this);
			pOwner->Weapon_Drop(this, NULL, NULL);
		}
		UTIL_Remove(this);
#endif
		return;
	}

	if (!m_bDrawbackFinished)
	{
		if ((m_flNextPrimaryAttack <= gpGlobals->curtime) && (m_flNextSecondaryAttack <= gpGlobals->curtime))
		{
			m_bDrawbackFinished = true;
		}
	}

	if (m_bDrawbackFinished)
	{
		auto pOwner = static_cast<CNEO_Player*>(GetOwner());

		if (pOwner)
		{
			switch (m_AttackPaused)
			{
			case GRENADE_PAUSED_PRIMARY:
				if (!(pOwner->m_nButtons & IN_ATTACK) && gpGlobals->curtime >= m_flNextPrimaryAttack)
				{
					ThrowGrenade(pOwner);

					SendWeaponAnim(ACT_VM_THROW);
					m_flNextPrimaryAttack = m_flNextSecondaryAttack = m_flTimeWeaponIdle = gpGlobals->curtime + SequenceDuration();
					m_bDrawbackFinished = false;
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

void CWeaponGrenade::ThrowGrenade(CNEO_Player *pPlayer, bool isAlive, CBaseEntity *pAttacker)
{
	if (!sv_neo_infinite_frag_grenades.GetBool())
	{
		Assert(HasPrimaryAmmo());
		Assert(pPlayer);
		DecrementAmmo(pPlayer);
	}

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

	float flVel = (90 - angThrow.x) * 6;

	if (flVel > sv_neo_grenade_throw_intensity.GetFloat())
		flVel = sv_neo_grenade_throw_intensity.GetFloat();

	AngleVectors(angThrow, &vForward, &vRight, &vUp);

	Vector vecSrc = pPlayer->GetAbsOrigin() + pPlayer->GetViewOffset();

	vecSrc += vForward * 16;

	Vector vecThrow = vForward * flVel + pPlayer->GetAbsVelocity();

	CBaseGrenadeProjectile *pGrenade = NEOFraggrenade_Create(vecSrc, vec3_angle, vecThrow, AngularImpulse(600, random->RandomInt(-1200, 1200), 0), ((!(pPlayer->IsAlive()) || !isAlive) && pAttacker) ? pAttacker : pPlayer, false);

	if (pGrenade)
	{
		pGrenade->SetDamage(sv_neo_grenade_blast_damage.GetFloat());
		pGrenade->SetDamageRadius(sv_neo_grenade_blast_radius.GetFloat());
		pGrenade->SetDetonateTimerLength(sv_neo_grenade_fuse_timer.GetFloat());
	}
#endif

	WeaponSound(SINGLE);

	// player "shoot" animation
	pPlayer->DoAnimationEvent(PLAYERANIMEVENT_ATTACK_PRIMARY);

	m_bRedraw = true;
}

bool CWeaponGrenade::CanDrop()
{
	auto owner = GetOwner(); 
	return owner && !owner->IsAlive();
}

void CWeaponGrenade::Drop(const Vector& vecVelocity)
{
	BaseClass::Drop(vecVelocity);
}

bool CWeaponGrenade::CanBePickedUpByClass(int classId)
{
	return classId != NEO_CLASS_JUGGERNAUT;
}

#ifndef CLIENT_DLL
void CWeaponGrenade::Operator_HandleAnimEvent(animevent_t *pEvent, CBaseCombatCharacter *pOperator)
{
	auto pOwner = static_cast<CNEO_Player*>(GetOwner());
	Assert(pOwner);

	bool fThrewGrenade = false;

	switch (pEvent->event)
	{
	case EVENT_WEAPON_SEQUENCE_FINISHED:
		m_bDrawbackFinished = true;
		break;

	case EVENT_WEAPON_THROW:
		ThrowGrenade(pOwner);
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
	}
}
#endif
