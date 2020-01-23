#include "cbase.h"
#include "neo_smokegrenade.h"

#include "neo_tracefilter_collisiongroupdelta.h"

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

LINK_ENTITY_TO_CLASS(neo_grenade_smoke, CNEOGrenadeSmoke);

BEGIN_DATADESC(CNEOGrenadeSmoke)
// Fields
DEFINE_FIELD(m_inSolid, FIELD_BOOLEAN),
DEFINE_FIELD(m_punted, FIELD_BOOLEAN),

// Function Pointers
DEFINE_THINKFUNC(DelayThink),

// Inputs
DEFINE_INPUTFUNC(FIELD_FLOAT, "SetTimer", InputSetTimer),
END_DATADESC()

void CNEOGrenadeSmoke::Spawn(void)
{
	Precache();

	SetModel(NEO_SMOKE_GRENADE_MODEL);

	m_flDamage = 0;
	m_DmgRadius = 0;

	m_takedamage = DAMAGE_YES;
	m_iHealth = 1;

	SetSize(-Vector(4, 4, 4), Vector(4, 4, 4));
	SetCollisionGroup(COLLISION_GROUP_WEAPON);
	CreateVPhysics();

	AddSolidFlags(FSOLID_NOT_STANDABLE);

	m_punted = false;
	m_hasSettled = false;
	m_hasBeenMadeNonSolid = false;
	m_lastPos = GetAbsOrigin();

	BaseClass::Spawn();
}

bool CNEOGrenadeSmoke::CreateVPhysics()
{
	// Create the object in the physics system
	VPhysicsInitNormal(SOLID_BBOX, 0, false);
	return true;
}

void CNEOGrenadeSmoke::VPhysicsUpdate(IPhysicsObject* pPhysics)
{
	BaseClass::VPhysicsUpdate(pPhysics);
	Vector vel;
	AngularImpulse angVel;
	pPhysics->GetVelocity(&vel, &angVel);

	const Vector start = GetAbsOrigin();
	const Vector end = start + vel * gpGlobals->frametime;
	// find all entities that my collision group wouldn't hit, but COLLISION_GROUP_NONE would and bounce off of them as a ray cast
	CNEOTraceFilterCollisionGroupDelta filter(this, GetCollisionGroup(), COLLISION_GROUP_NONE);
	trace_t tr;

	// UNDONE: Hull won't work with hitboxes - hits outer hull.  But the whole point of this test is to hit hitboxes.
#if 0
	UTIL_TraceHull(start, end, CollisionProp()->OBBMins(), CollisionProp()->OBBMaxs(), CONTENTS_HITBOX | CONTENTS_MONSTER | CONTENTS_SOLID, &filter, &tr);
#else
	UTIL_TraceLine(start, end, CONTENTS_HITBOX | CONTENTS_MONSTER | CONTENTS_SOLID, &filter, &tr);
#endif
	if (tr.startsolid)
	{
		if (!m_inSolid)
		{
			// UNDONE: Do a better contact solution that uses relative velocity?
			vel *= -GRENADE_COEFFICIENT_OF_RESTITUTION; // bounce backwards
			pPhysics->SetVelocity(&vel, NULL);
		}
		m_inSolid = true;
		return;
	}
	m_inSolid = false;
	if (tr.DidHit())
	{
		Vector dir = vel;
		VectorNormalize(dir);
		// send a tiny amount of damage so the character will react to getting bonked
		CTakeDamageInfo info(this, GetThrower(), pPhysics->GetMass() * vel, GetAbsOrigin(), 0.1f, DMG_CRUSH);
		tr.m_pEnt->TakeDamage(info);

		// reflect velocity around normal
		vel = -2.0f * tr.plane.normal * DotProduct(vel, tr.plane.normal) + vel;

		// absorb 80% in impact
		vel *= GRENADE_COEFFICIENT_OF_RESTITUTION;
		angVel *= -0.5f;
		pPhysics->SetVelocity(&vel, &angVel);
	}

	if (!m_hasSettled)
	{
		UTIL_TraceLine(start, end, CONTENTS_HITBOX | CONTENTS_SOLID, 0, COLLISION_GROUP_DEBRIS, &tr);
		if (tr.DidHit())
		{
			DevMsg("Hit\n");

			CRecipientFilter filter;
			filter.AddRecipientsByPAS(GetAbsOrigin());
			EmitSound_t type;
			type.m_pSoundName = "Grenade.Bounce";

			EmitSound(filter, edict()->m_EdictIndex, type);
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

static inline bool IsRoughlySameVector(const Vector& v1, const Vector& v2)
{
	Assert(v1.IsValid() && v2.IsValid());
	const float threshold = 0.1f;
	return (abs(v1.x - v2.x) <= threshold) && (abs(v1.y - v2.y) <= threshold) && (abs(v1.z - v2.z) <= threshold);
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

	m_hasSettled = IsRoughlySameVector(m_lastPos, GetAbsOrigin());
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

#define NEO_SMOKE_BLOOM_TIME 15.0f

void CNEOGrenadeSmoke::Detonate(void)
{
	//DevMsg("Smoke detonated!\n");

	const size_t numSmokePuffs = 3;
	for (size_t i = 0; i < numSmokePuffs; i++)
	{
		const float randScale = 32.0f;
		Vector randVec;
		RandomSeed(gpGlobals->framecount);
		int min = RandomInt(-randScale, randScale);
		RandomSeed(gpGlobals->framecount * (i + 1));
		int max = MAX(min, min + RandomInt(-randScale, randScale));
		RandomSeed((gpGlobals->framecount * (i + 1)) % 2);
		randVec.Random(min, max);

		UTIL_Smoke(GetAbsOrigin() + randVec, 128.0f, 1.0f);
	}

	if (!m_hasBeenMadeNonSolid)
	{
		m_hasBeenMadeNonSolid = true;
		m_flSmokeBloomTime = gpGlobals->curtime;

		CRecipientFilter filter;
		filter.AddRecipientsByPAS(GetAbsOrigin());
		EmitSound_t type;
		type.m_pSoundName = "Smoke.Explode";
		EmitSound(filter, edict()->m_EdictIndex, type);

		SetTouch(NULL);
		SetSolid(SOLID_NONE);
		SetAbsVelocity(vec3_origin);
	}
	else if (gpGlobals->curtime - m_flSmokeBloomTime >= NEO_SMOKE_BLOOM_TIME)
	{
		SetThink(&CNEOGrenadeSmoke::SUB_Remove);
		AddEffects(EF_NODRAW);
	}
	
	SetNextThink(gpGlobals->curtime + 0.01f);
}

#if(0)
void CNEOGrenadeSmoke::BounceSound(void)
{
	Assert(m_iszBounceSound != NULL_STRING);

	CRecipientFilter filter;
	filter.AddRecipientsByPAS(GetAbsOrigin());
	EmitSound_t type;
	type.m_pSoundName = m_iszBounceSound.ToCStr();

	EmitSound(filter, edict()->m_EdictIndex, type);
}
#endif

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
