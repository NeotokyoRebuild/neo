#include "cbase.h"
#include "neo_grenade.h"

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

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

	SetSize(-Vector(4, 4, 4), Vector(4, 4, 4));
	SetCollisionGroup(COLLISION_GROUP_WEAPON);
	CreateVPhysics();

	AddSolidFlags(FSOLID_NOT_STANDABLE);

	m_punted = false;

	BaseClass::Spawn();
}

bool CNEOGrenadeFrag::CreateVPhysics()
{
	// Create the object in the physics system
	VPhysicsInitNormal(SOLID_BBOX, 0, false);
	return true;
}

void CNEOGrenadeFrag::VPhysicsUpdate(IPhysicsObject *pPhysics)
{
	BaseClass::VPhysicsUpdate(pPhysics);
	Vector vel;
	AngularImpulse angVel;
	pPhysics->GetVelocity(&vel, &angVel);

	Vector start = GetAbsOrigin();
	// find all entities that my collision group wouldn't hit, but COLLISION_GROUP_NONE would and bounce off of them as a ray cast
	CNEOTraceFilterCollisionGroupDelta filter(this, GetCollisionGroup(), COLLISION_GROUP_NONE);
	trace_t tr;

	// UNDONE: Hull won't work with hitboxes - hits outer hull.  But the whole point of this test is to hit hitboxes.
#if 0
	UTIL_TraceHull(start, start + vel * gpGlobals->frametime, CollisionProp()->OBBMins(), CollisionProp()->OBBMaxs(), CONTENTS_HITBOX | CONTENTS_MONSTER | CONTENTS_SOLID, &filter, &tr);
#else
	UTIL_TraceLine(start, start + vel * gpGlobals->frametime, CONTENTS_HITBOX | CONTENTS_MONSTER | CONTENTS_SOLID, &filter, &tr);
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
}

void CNEOGrenadeFrag::Precache(void)
{
	//PrecacheModel(GRENADE_MODEL);
	//PrecacheScriptSound("Grenade.Blip");

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
