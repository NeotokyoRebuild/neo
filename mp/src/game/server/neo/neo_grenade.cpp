#include "cbase.h"
#include "neo_grenade.h"

#include "neo_tracefilter_collisiongroupdelta.h"

#ifdef GAME_DLL
#include "gamestats.h"
#endif

#include "vcollide_parse.h"

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

#define NADE_SOLID_TYPE SolidType_t::SOLID_BBOX

LINK_ENTITY_TO_CLASS(neo_grenade_frag, CNEOGrenadeFrag);

BEGIN_DATADESC(CNEOGrenadeFrag)
	// Fields
	DEFINE_FIELD(m_inSolid, FIELD_BOOLEAN),
	DEFINE_FIELD(m_punted, FIELD_BOOLEAN),

	// Function Pointers
	DEFINE_THINKFUNC(DelayThink),

	// Inputs
	DEFINE_INPUTFUNC(FIELD_FLOAT, "SetTimer", InputSetTimer),
END_DATADESC()

void CNEOGrenadeFrag::Spawn(void)
{
	Precache();

	SetModel(NEO_FRAG_GRENADE_MODEL);

	if (GetOwnerEntity() && GetOwnerEntity()->IsPlayer())
	{
		m_flDamage = sk_plr_dmg_fraggrenade.GetFloat();
		m_DmgRadius = sk_fraggrenade_radius.GetFloat();
	}
	else
	{
		m_flDamage = sk_npc_dmg_fraggrenade.GetFloat();
		m_DmgRadius = sk_fraggrenade_radius.GetFloat();
	}

	m_takedamage = DAMAGE_YES;
	m_iHealth = 1;

#define FRAG_RADIUS 4.0f
#define FRAG_COLLISION_RADIUS (FRAG_RADIUS * 2.0)

	SetSize(-Vector(FRAG_RADIUS, FRAG_RADIUS, FRAG_RADIUS), Vector(FRAG_RADIUS, FRAG_RADIUS, FRAG_RADIUS));
	SetCollisionBounds(-Vector(FRAG_COLLISION_RADIUS, FRAG_COLLISION_RADIUS, FRAG_COLLISION_RADIUS), Vector(FRAG_COLLISION_RADIUS, FRAG_COLLISION_RADIUS, FRAG_COLLISION_RADIUS));
	SetCollisionGroup(COLLISION_GROUP_PROJECTILE);
	CreateVPhysics();

	AddSolidFlags(FSOLID_NOT_STANDABLE);

	// We turn off VPhysics for the nade because we want to control it with a bounding box ourselves
	// for more consistent and controllable bouncing.
	// This gets re-enabled on the prop if it settles before exploding (see the VPhysicsUpdate).
	VPhysicsGetObject()->EnableCollisions(false);

	m_punted = false;

	BaseClass::Spawn();
}

bool CNEOGrenadeFrag::CreateVPhysics()
{
	// Create the object in the physics system
	VPhysicsInitNormal(NADE_SOLID_TYPE, 0, false);
	return true;
}

ConVar sv_neo_frag_cor("sv_neo_frag_cor", "0.5", FCVAR_CHEAT | FCVAR_REPLICATED, "Neotokyo frag coefficient of restitution", true, 0.0, true, 1.0);
ConVar sv_neo_frag_showdebug("sv_neo_frag_showdebug", "0", FCVAR_CHEAT, "Show frag collision debug", true, 0.0, true, 1.0);
ConVar sv_neo_frag_vphys_reawaken_vel("sv_neo_frag_vphys_reawaken_vel", "200", FCVAR_CHEAT);

void CNEOGrenadeFrag::VPhysicsUpdate(IPhysicsObject *pPhysics)
{
	if (sv_neo_frag_showdebug.GetBool())
	{
		if (NADE_SOLID_TYPE == SolidType_t::SOLID_BBOX)
		{
			DrawBBoxOverlay(0.1f);
		}
		else
		{
			DrawAbsBoxOverlay();
		}
	}

	BaseClass::VPhysicsUpdate(pPhysics);
	Vector vel;
	AngularImpulse angVel;
	pPhysics->GetVelocity(&vel, &angVel);

	Vector start = GetAbsOrigin();
	trace_t tr;

	UTIL_TraceLine(start, start + vel * gpGlobals->frametime, MASK_SOLID, GetThrower(), COLLISION_GROUP_PROJECTILE, &tr);

	// If we clipped in a solid, undo it so we don't go out of bounds
	if (tr.startsolid)
	{
		if (!m_inSolid)
		{
			BounceSound();

			vel *= -sv_neo_frag_cor.GetFloat(); // bounce backwards
			pPhysics->SetVelocity(&vel, NULL);

			if (sv_neo_frag_showdebug.GetBool())
			{
				DevMsg("Frag re-bounced in solid\n");
			}
		}
		m_inSolid = true;
		return;
	}

	m_inSolid = false;
	
	// Still bouncing around the world...
	if (tr.DidHit() || tr.DidHitNonWorldEntity() || tr.DidHitWorld())
	{
		BounceSound();

		Vector dir = vel;
		VectorNormalize(dir);

		if (tr.m_pEnt)
		{
			// send a tiny amount of damage so the character will react to getting bonked
			CTakeDamageInfo info(this, GetThrower(), pPhysics->GetMass() * vel, GetAbsOrigin(), 0.1f, DMG_CRUSH);
			tr.m_pEnt->TakeDamage(info);
		}

		// reflect velocity around normal
		vel = -2.0f * tr.plane.normal * DotProduct(vel, tr.plane.normal) + vel;

		// Absorb some of the impact
		vel *= sv_neo_frag_cor.GetFloat();
		angVel *= -0.5f;
		pPhysics->SetVelocity(&vel, &angVel);
	}
	// Else, have we slowed down sufficiently for the VPhys to take over and settle the model.
	// We do this because the BBox can't handle keeping the frag in bounds as robustly.
	// NEO TODO (Rain): bind this compare velocity to nade release velocity, so
	// it doesn't need manual adjustment.
	else if (vel.Length() <= sv_neo_frag_vphys_reawaken_vel.GetFloat())
	{
		if (!VPhysicsGetObject()->IsCollisionEnabled())
		{
			VPhysicsGetObject()->EnableCollisions(true);
			VPhysicsGetObject()->RecheckContactPoints();
			if (sv_neo_frag_showdebug.GetBool())
			{
				DevMsg("Frag has settled; awoke CNEOGrenadeFrag VPhys for handling\n");
			}
		}
	}
}

void CNEOGrenadeFrag::Precache(void)
{
	BaseClass::Precache();
}

void CNEOGrenadeFrag::SetTimer(float detonateDelay, float warnDelay)
{
	m_flDetonateTime = gpGlobals->curtime + detonateDelay;
	m_flWarnAITime = gpGlobals->curtime + warnDelay;
	SetThink(&CNEOGrenadeFrag::DelayThink);
	SetNextThink(gpGlobals->curtime);
}

void CNEOGrenadeFrag::OnPhysGunPickup(CBasePlayer *pPhysGunUser, PhysGunPickup_t reason)
{
	SetThrower(pPhysGunUser);
	m_bHasWarnedAI = true;

	BaseClass::OnPhysGunPickup(pPhysGunUser, reason);
}

void CNEOGrenadeFrag::DelayThink()
{
	if (gpGlobals->curtime > m_flDetonateTime)
	{
		Detonate();
		return;
	}

	if (!m_bHasWarnedAI && gpGlobals->curtime >= m_flWarnAITime)
	{
#if !defined( CLIENT_DLL )
		CSoundEnt::InsertSound(SOUND_DANGER, GetAbsOrigin(), 400, 1.5, this);
#endif
		m_bHasWarnedAI = true;
	}

	SetNextThink(gpGlobals->curtime + 0.1);
}

void CNEOGrenadeFrag::Explode(trace_t* pTrace, int bitsDamageType)
{
	BaseClass::Explode(pTrace, bitsDamageType);
#ifdef GAME_DLL
	auto pThrower = GetThrower();
	auto pPlayer = ToBasePlayer(pThrower);
	if (pPlayer)
	{
		// Use the thrower's position as the reported position
		Vector vecReported = pThrower ? pThrower->GetAbsOrigin() : vec3_origin;
		CTakeDamageInfo info(this, pThrower, GetBlastForce(), GetAbsOrigin(), m_flDamage, bitsDamageType, 0, &vecReported);
		gamestats->Event_WeaponHit(pPlayer, true, "weapon_grenade", info);
	}
#endif
}

void CNEOGrenadeFrag::SetVelocity(const Vector &velocity, const AngularImpulse &angVelocity)
{
	IPhysicsObject *pPhysicsObject = VPhysicsGetObject();
	if (pPhysicsObject)
	{
		pPhysicsObject->AddVelocity(&velocity, &angVelocity);
	}
}

int CNEOGrenadeFrag::OnTakeDamage(const CTakeDamageInfo &inputInfo)
{
	// NEO nades can't be detonated by damage.
	return 0;
}

void CNEOGrenadeFrag::InputSetTimer(inputdata_t &inputdata)
{
	SetTimer(inputdata.value.Float(), inputdata.value.Float() - NEO_FRAG_GRENADE_WARN_TIME);
}

CBaseGrenade *NEOFraggrenade_Create(const Vector &position, const QAngle &angles, const Vector &velocity,
	const AngularImpulse &angVelocity, CBaseEntity *pOwner, float timer, bool combineSpawned)
{
	// Don't set the owner here, or the player can't interact with grenades he's thrown
	CNEOGrenadeFrag *pGrenade = (CNEOGrenadeFrag *)CBaseEntity::Create("neo_grenade_frag", position, angles, pOwner);

	pGrenade->SetTimer(timer, timer - NEO_FRAG_GRENADE_WARN_TIME);
	pGrenade->SetVelocity(velocity, angVelocity);
	pGrenade->SetThrower(ToBaseCombatCharacter(pOwner));
	pGrenade->m_takedamage = DAMAGE_EVENTS_ONLY;

	return pGrenade;
}

bool NEOFraggrenade_WasPunted(const CBaseEntity *pEntity)
{
	const CNEOGrenadeFrag *pFrag = dynamic_cast<const CNEOGrenadeFrag *>(pEntity);
	if (pFrag)
	{
		return pFrag->WasPunted();
	}

	return false;
}
