#include "cbase.h"
#include "neo_smokegrenade.h"
#include "neo_smokelineofsightblocker.h"

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

ConVar sv_neo_smoke_bloom_duration("sv_neo_smoke_bloom_duration", "25", FCVAR_CHEAT, "How long should the smoke bloom be up, in seconds.", true, 0.0, true, 60.0);
ConVar sv_neo_smoke_bloom_layers("sv_neo_smoke_bloom_layers", "2", FCVAR_CHEAT, "Amount of smoke layers atop each other.", true, 0.0, true, 32.0);
ConVar sv_neo_smoke_blocker_size("sv_neo_smoke_blocker_size", "140", FCVAR_CHEAT, "Size of each LOS blocker per smoke particle.", true, 0.0, true, 200.0);
ConVar sv_neo_smoke_blocker_duration("sv_neo_smoke_blocker_duration", "22", FCVAR_CHEAT, "Time when LOS block should disappear", true, 0.0, true, 30.0);
extern ConVar sv_neo_grenade_cor;
extern ConVar sv_neo_grenade_gravity;
extern ConVar sv_neo_grenade_friction;
void CNEOGrenadeSmoke::Spawn(void)
{
	Precache();
	SetModel(NEO_SMOKE_GRENADE_MODEL);
	BaseClass::Spawn();

	m_flDamage = 0;
	m_DmgRadius = 0;
	m_takedamage = DAMAGE_YES;
	m_iHealth = 1;

	m_punted = false;
	m_hasSettled = false;
	m_hasBeenMadeNonSolid = false;
	m_lastPos = GetAbsOrigin();

	SetElasticity(sv_neo_grenade_cor.GetFloat());
	SetGravity(sv_neo_grenade_gravity.GetFloat());
	SetFriction(sv_neo_grenade_friction.GetFloat());
	SetCollisionGroup(COLLISION_GROUP_WEAPON);
	SetDetonateTimerLength(FLT_MAX);

	SetThink(&CNEOGrenadeSmoke::DelayThink);
	SetNextThink(gpGlobals->curtime);
}

void CNEOGrenadeSmoke::OnPhysGunPickup(CBasePlayer* pPhysGunUser, PhysGunPickup_t reason)
{
	SetThrower(pPhysGunUser);
	m_bHasWarnedAI = true;

	BaseClass::OnPhysGunPickup(pPhysGunUser, reason);
}

int CNEOGrenadeSmoke::OnTakeDamage(const CTakeDamageInfo& inputInfo)
{
	// NEO smokes can't be detonated by damage.
	return 0;
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


bool CNEOGrenadeSmoke::TryDetonate(void)
{
	if (m_hasSettled && (gpGlobals->curtime > GetDetonateTimerLength()))
	{
		Detonate();
		return true;
	}

	return false;
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
		SetNextThink(TICK_NEVER_THINK);
		return;
	}
	
	SetNextThink(gpGlobals->curtime + 1.0f);
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
			constexpr int smokeFadeStartTime = 17;
			constexpr int smokeFadeEndTime = 25;
			ptr->SetFadeTime(smokeFadeStartTime, smokeFadeEndTime);
			ptr->Activate();
			ptr->SetLifetime(sv_neo_smoke_bloom_duration.GetFloat()); // This call passes on responsibility for the memory to the particle thinkfunc
			ptr->FillVolume();
		}

		// Currently redundant to create a blocker for each particle, but will be handy for dynamic flood fill later
		auto blocker = static_cast<CNEOSmokeLineOfSightBlocker*>(CreateEntityByName(SMOKELINEOFSIGHTBLOCKER_ENTITYNAME));
		int blockerSize = sv_neo_smoke_blocker_size.GetInt();
		if (blocker) {
			blocker->SetAbsOrigin(ptr->GetAbsOrigin());
			blocker->SetBounds(Vector(-blockerSize, -blockerSize, -blockerSize), Vector(blockerSize, blockerSize, blockerSize));
			blocker->Spawn();
			// There is no need for a delay to add the LOS blocker, because it takes a moment for bots to trigger another visibility check
			blocker->SetThink(&CNEOSmokeLineOfSightBlocker::SUB_Remove);
			// sv_neo_smoke_blocker_duration tuned to be the second after smoke can be seen through though not completely dissipated
			blocker->SetNextThink(gpGlobals->curtime + sv_neo_smoke_blocker_duration.GetFloat());
		}
	}
}

void CNEOGrenadeSmoke::InputSetTimer(inputdata_t& inputdata)
{
	SetDetonateTimerLength(inputdata.value.Float());
}

CBaseGrenadeProjectile* NEOSmokegrenade_Create(const Vector& position, const QAngle& angles, const Vector& velocity,
	const AngularImpulse& angVelocity, CBaseEntity* pOwner)
{
	// Don't set the owner here, or the player can't interact with grenades he's thrown
	CNEOGrenadeSmoke* pGrenade = (CNEOGrenadeSmoke*)CBaseEntity::Create("neo_grenade_smoke", position, angles, pOwner);

	pGrenade->SetAbsVelocity(velocity);
	pGrenade->ApplyLocalAngularVelocityImpulse(angVelocity);
	pGrenade->SetupInitialTransmittedGrenadeVelocity(velocity);

	// We won't know when to detonate until the smoke has settled.
	pGrenade->SetNextThink(gpGlobals->curtime);
	pGrenade->SetThrower(ToBaseCombatCharacter(pOwner));
	if (pOwner) pGrenade->ChangeTeam(pOwner->GetTeamNumber());
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
