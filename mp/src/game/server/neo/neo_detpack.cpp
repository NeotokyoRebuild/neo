#include "cbase.h"
#include "neo_detpack.h"

#include "neo_tracefilter_collisiongroupdelta.h"

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

LINK_ENTITY_TO_CLASS(neo_deployed_detpack, CNEODeployedDetpack);

BEGIN_DATADESC(CNEODeployedDetpack)
	// Fields
	DEFINE_FIELD(m_inSolid, FIELD_BOOLEAN),
	DEFINE_FIELD(m_punted, FIELD_BOOLEAN),
	DEFINE_FIELD(m_hasSettled, FIELD_BOOLEAN),
	DEFINE_FIELD(m_hasBeenTriggeredToDetonate, FIELD_BOOLEAN),

	// Function pointers
	DEFINE_THINKFUNC(DelayThink),

	// Inputs
	DEFINE_INPUTFUNC(FIELD_VOID, "RemoteDetonate", InputRemoteDetonate),
END_DATADESC()

void CNEODeployedDetpack::Spawn(void)
{
	Precache();

	SetModel(NEO_DEPLOYED_DETPACK_MODEL);

	// NEO FIXME (Rain): these are doubly defined in weapon_detpack
#define NEO_DETPACK_DAMAGE 999.0f
#define NEO_DETPACK_DAMAGE_RADIUS 999.0f

	m_flDamage = NEO_DETPACK_DAMAGE;
	m_DmgRadius = NEO_DETPACK_DAMAGE_RADIUS;

	m_takedamage = DAMAGE_YES;
	m_iHealth = 1;

	SetSize(-Vector(NEO_DEPLOYED_DETPACK_RADIUS, NEO_DEPLOYED_DETPACK_RADIUS, NEO_DEPLOYED_DETPACK_RADIUS),
		Vector(NEO_DEPLOYED_DETPACK_RADIUS, NEO_DEPLOYED_DETPACK_RADIUS, NEO_DEPLOYED_DETPACK_RADIUS));
	SetCollisionGroup(COLLISION_GROUP_WEAPON);
	CreateVPhysics();

	AddSolidFlags(FSOLID_NOT_STANDABLE);

	m_punted = false;
	m_hasSettled = false;
	m_hasBeenMadeNonSolid = false;
	m_hasBeenTriggeredToDetonate = false;

	BaseClass::Spawn();

	m_lastPos = GetAbsOrigin();
}

bool CNEODeployedDetpack::CreateVPhysics()
{
	// Create the object in the physics system
	VPhysicsInitNormal(SOLID_BBOX, 0, false);
	return true;
}

void CNEODeployedDetpack::VPhysicsUpdate(IPhysicsObject* pPhysics)
{
	BaseClass::VPhysicsUpdate(pPhysics);
	Vector vel;
	AngularImpulse angVel;
	pPhysics->GetVelocity(&vel, &angVel);

	Vector start = GetAbsOrigin();
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

	const float GRENADE_COEFFICIENT_OF_RESTITUTION = 0.2f;

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
			CRecipientFilter filter;
			filter.AddRecipientsByPAS(GetAbsOrigin());
			EmitSound_t type;
			type.m_pSoundName = "Grenade.Bounce";

			EmitSound(filter, edict()->m_EdictIndex, type);
		}
	}
}

void CNEODeployedDetpack::Precache(void)
{
	// NEO TODO (Rain): confirm these are handled accordingly,
	// then should be able to refactor this out.

	//PrecacheModel(NEO_DEPLOYED_DETPACK_MODEL);
	//PrecacheScriptSound("???");
	BaseClass::Precache();
}

void CNEODeployedDetpack::SetTimer(float detonateDelay, float warnDelay)
{
	m_flDetonateTime = gpGlobals->curtime + detonateDelay;
	m_flWarnAITime = gpGlobals->curtime + warnDelay;
	SetThink(&CNEODeployedDetpack::DelayThink);
	SetNextThink(gpGlobals->curtime);
}

void CNEODeployedDetpack::OnPhysGunPickup(CBasePlayer* pPhysGunUser, PhysGunPickup_t reason)
{
	SetThrower(pPhysGunUser);
	m_bHasWarnedAI = true;

	BaseClass::OnPhysGunPickup(pPhysGunUser, reason);
}

bool CNEODeployedDetpack::TryDetonate(void)
{
	if (m_hasBeenTriggeredToDetonate || (gpGlobals->curtime > m_flDetonateTime))
	{
		Detonate();
		return true;
	}

	return false;
}

void CNEODeployedDetpack::Detonate(void)
{
	BaseClass::Detonate();

	SetThink(&CNEODeployedDetpack::SUB_Remove);
	AddEffects(EF_NODRAW);

	SetNextThink(gpGlobals->curtime + 1.0);
}

void CNEODeployedDetpack::DelayThink()
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

	if (m_hasSettled && !m_hasBeenMadeNonSolid)
	{
		SetTouch(NULL);
		SetSolid(SOLID_NONE);
		SetAbsVelocity(vec3_origin);
		SetMoveType(MOVETYPE_NONE);

		m_hasBeenMadeNonSolid = true;
	}

	SetNextThink(gpGlobals->curtime + 0.1);
}

void CNEODeployedDetpack::SetVelocity(const Vector& velocity, const AngularImpulse& angVelocity)
{
	IPhysicsObject* pPhysicsObject = VPhysicsGetObject();
	if (pPhysicsObject)
	{
		pPhysicsObject->AddVelocity(&velocity, &angVelocity);
	}
}

int CNEODeployedDetpack::OnTakeDamage(const CTakeDamageInfo& inputInfo)
{
	// Dets can't be detonated by damage.
	return 0;
}

void CNEODeployedDetpack::InputRemoteDetonate(inputdata_t& inputdata)
{
	m_hasBeenTriggeredToDetonate = true;
	DevMsg("CNEODeployedDetpack::InputRemoteDetonate triggered\n");
}

CBaseGrenade* NEODeployedDetpack_Create(const Vector& position, const QAngle& angles, const Vector& velocity,
	const AngularImpulse& angVelocity, CBaseEntity* pOwner)
{
	// Don't set the owner here, or the player can't interact with grenades he's thrown
	auto pDet = (CNEODeployedDetpack*)CBaseEntity::Create("neo_deployed_detpack", position, angles, pOwner);

	// Since we are inheriting from a grenade class, just make sure the fuse never blows due to timer.
	pDet->SetTimer(FLT_MAX, FLT_MAX);
	pDet->SetVelocity(velocity, angVelocity);
	pDet->SetThrower(ToBaseCombatCharacter(pOwner));
	pDet->m_takedamage = DAMAGE_EVENTS_ONLY;

	return pDet;
}

bool NEODeployedDetpack_WasPunted(const CBaseEntity* pEntity)
{
	auto pDet = dynamic_cast<const CNEODeployedDetpack*>(pEntity);
	return ((pDet != NULL) ? pDet->WasPunted() : false);
}
