#include "cbase.h"
#include "neo_smokegrenade.h"

#include "neo_tracefilter_collisiongroupdelta.h"
#include "particle_smokegrenade.h"

#include "mathlib/vector.h"

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

#define NADE_SOLID_TYPE SolidType_t::SOLID_BBOX

LINK_ENTITY_TO_CLASS(neo_grenade_smoke, CNEOGrenadeSmoke);

BEGIN_DATADESC(CNEOGrenadeSmoke)
// Fields
DEFINE_FIELD(m_inSolid, FIELD_BOOLEAN),
DEFINE_FIELD(m_punted, FIELD_BOOLEAN),
DEFINE_FIELD(m_hasSettled, FIELD_BOOLEAN),

// Function Pointers
DEFINE_THINKFUNC(DelayThink),

// Inputs
DEFINE_INPUTFUNC(FIELD_FLOAT, "SetTimer", InputSetTimer),
END_DATADESC()

ConVar sv_neo_smoke_bloom_duration("sv_neo_smoke_bloom_duration", "15", FCVAR_CHEAT, "How long should the smoke bloom be up, in seconds.", true, 0.0, true, 60.0);
ConVar sv_neo_smoke_bloom_radius("sv_neo_smoke_bloom_radius", "256", FCVAR_CHEAT, "Size of the smoke bloom radius, in Hammer units.", true, 1.0, true, 2048.0);
ConVar sv_neo_smoke_bloom_layers("sv_neo_smoke_bloom_layers", "5", FCVAR_CHEAT, "Amount of smoke layers atop each other.", true, 0.0, true, 32.0);

void CNEOGrenadeSmoke::Spawn(void)
{
	Precache();

	SetModel(NEO_SMOKE_GRENADE_MODEL);

	m_flDamage = 0;
	m_DmgRadius = 0;

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
	m_hasSettled = false;
	m_hasBeenMadeNonSolid = false;

	BaseClass::Spawn();

	m_lastPos = GetAbsOrigin();
}

bool CNEOGrenadeSmoke::CreateVPhysics()
{
	// Create the object in the physics system
	VPhysicsInitNormal(NADE_SOLID_TYPE, 0, false);
	return true;
}

extern ConVar sv_neo_frag_cor;
extern ConVar sv_neo_frag_showdebug;
extern ConVar sv_neo_frag_vphys_reawaken_vel;

void CNEOGrenadeSmoke::VPhysicsUpdate(IPhysicsObject* pPhysics)
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

	const Vector start = GetAbsOrigin();
	const Vector end = start + vel * gpGlobals->frametime;
	trace_t tr;

	UTIL_TraceLine(start, end, MASK_SOLID, GetThrower(), COLLISION_GROUP_PROJECTILE, &tr);

	if (tr.startsolid)
	{
		if (!m_inSolid)
		{
			BounceSound();

			vel *= -sv_neo_frag_cor.GetFloat(); // bounce backwards
			pPhysics->SetVelocity(&vel, NULL);

			if (sv_neo_frag_showdebug.GetBool())
			{
				DevMsg("Smoke re-bounced in solid\n");
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
				DevMsg("Smoke has settled; awoke CNEOGrenadeSmoke VPhys for handling\n");
			}
		}
	}
}

void CNEOGrenadeSmoke::SetTimer(float detonateDelay, float warnDelay)
{
	m_flDetonateTime = gpGlobals->curtime + detonateDelay;
	m_flWarnAITime = gpGlobals->curtime + warnDelay;
	SetThink(&CNEOGrenadeSmoke::DelayThink);
	SetNextThink(gpGlobals->curtime);
}

void CNEOGrenadeSmoke::OnPhysGunPickup(CBasePlayer* pPhysGunUser, PhysGunPickup_t reason)
{
	SetThrower(pPhysGunUser);
	m_bHasWarnedAI = true;

	BaseClass::OnPhysGunPickup(pPhysGunUser, reason);
}

void CNEOGrenadeSmoke::DelayThink()
{
	if (TryDetonate())
	{
		return;
	}

	if (!m_bHasWarnedAI && gpGlobals->curtime >= m_flWarnAITime)
	{
#if !defined( CLIENT_DLL )
		CSoundEnt::InsertSound(SOUND_DANGER, GetAbsOrigin(), 400, 1.5, this);
#endif
		m_bHasWarnedAI = true;
	}

	m_hasSettled = CloseEnough(m_lastPos, GetAbsOrigin(), 0.1f);
	m_lastPos = GetAbsOrigin();

	SetNextThink(gpGlobals->curtime + 0.1);
}

void CNEOGrenadeSmoke::SetVelocity(const Vector& velocity, const AngularImpulse& angVelocity)
{
	IPhysicsObject* pPhysicsObject = VPhysicsGetObject();
	if (pPhysicsObject)
	{
		pPhysicsObject->AddVelocity(&velocity, &angVelocity);
	}
}

int CNEOGrenadeSmoke::OnTakeDamage(const CTakeDamageInfo& inputInfo)
{
	// NEO smokes can't be detonated by damage.
	return 0;
}

void CNEOGrenadeSmoke::InputSetTimer(inputdata_t& inputdata)
{
	SetTimer(inputdata.value.Float(), inputdata.value.Float());
}

bool CNEOGrenadeSmoke::TryDetonate(void)
{
	if (m_hasSettled || (gpGlobals->curtime > m_flDetonateTime))
	{
		Detonate();
		return true;
	}

	return false;
}

void CNEOGrenadeSmoke::SetupParticles(size_t number)
{
	for (size_t i = 0; i < number; ++i)
	{
		auto ptr = dynamic_cast<ParticleSmokeGrenade*>(CreateEntityByName(PARTICLESMOKEGRENADE_ENTITYNAME));
		Assert(ptr);
		if (ptr)
		{
			Vector vForward;
			AngleVectors(GetLocalAngles(), &vForward);
			vForward.z = 0;
			VectorNormalize(vForward);

			ptr->SetLocalOrigin(GetLocalOrigin());
			ptr->SetFadeTime((sv_neo_smoke_bloom_duration.GetFloat() - 5), sv_neo_smoke_bloom_duration.GetFloat());
			ptr->Activate();
			ptr->SetLifetime(sv_neo_smoke_bloom_duration.GetFloat()); // This call passes on responsibility for the memory to the particle thinkfunc
			ptr->FillVolume();
		}
	}
}

void CNEOGrenadeSmoke::Detonate(void)
{
#if(0)
	Vector randVec;
	randVec.Random((-sv_neo_smoke_bloom_radius.GetFloat() * 0.05f), (sv_neo_smoke_bloom_radius.GetFloat() * 0.05f));
	UTIL_Smoke(GetAbsOrigin() + randVec, sv_neo_smoke_bloom_radius.GetFloat(), 1.0f);
#endif

	if (!m_hasBeenMadeNonSolid)
	{
		m_hasBeenMadeNonSolid = true;

		SetupParticles(sv_neo_smoke_bloom_layers.GetInt());
		m_flSmokeBloomTime = gpGlobals->curtime;

		CRecipientFilter filter;
		filter.AddRecipientsByPAS(GetAbsOrigin());
		EmitSound_t type;
		type.m_pSoundName = "Smoke.Explode";
		EmitSound(filter, edict()->m_EdictIndex, type);

		SetTouch(NULL);
		SetSolid(SOLID_NONE);
		SetAbsVelocity(vec3_origin);
		SetMoveType(MOVETYPE_NONE);
	}
	else if (gpGlobals->curtime - m_flSmokeBloomTime >= sv_neo_smoke_bloom_duration.GetFloat())
	{
		SetThink(&CNEOGrenadeSmoke::SUB_Remove);
		AddEffects(EF_NODRAW);
	}
	
	SetNextThink(gpGlobals->curtime + 1.0f);
}

CBaseGrenade* NEOSmokegrenade_Create(const Vector& position, const QAngle& angles, const Vector& velocity,
	const AngularImpulse& angVelocity, CBaseEntity* pOwner)
{
	// Don't set the owner here, or the player can't interact with grenades he's thrown
	CNEOGrenadeSmoke* pGrenade = (CNEOGrenadeSmoke*)CBaseEntity::Create("neo_grenade_smoke", position, angles, pOwner);

	// We won't know when to detonate until the smoke has settled.
	pGrenade->SetTimer(FLT_MAX, FLT_MAX);
	pGrenade->SetVelocity(velocity, angVelocity);
	pGrenade->SetThrower(ToBaseCombatCharacter(pOwner));
	pGrenade->m_takedamage = DAMAGE_EVENTS_ONLY;

	return pGrenade;
}

bool NEOSmokegrenade_WasPunted(const CBaseEntity* pEntity)
{
	auto pSmoke = dynamic_cast<const CNEOGrenadeSmoke*>(pEntity);
	if (pSmoke)
	{
		return pSmoke->WasPunted();
	}

	return false;
}
