#include "cbase.h"
#include "neo_detpack.h"
#include "explode.h"
#include "neo_tracefilter_collisiongroupdelta.h"

#ifdef GAME_DLL
#include "gamestats.h"
#endif

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

#define NADE_SOLID_TYPE SolidType_t::SOLID_BBOX

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

extern ConVar sv_neo_grenade_cor;
extern ConVar sv_neo_grenade_gravity;
extern ConVar sv_neo_grenade_friction;
void CNEODeployedDetpack::Spawn(void)
{
	Precache();
	SetModel(NEO_DEPLOYED_DETPACK_MODEL);
	BaseClass::Spawn();

	SetElasticity(sv_neo_grenade_cor.GetFloat());
	SetGravity(sv_neo_grenade_gravity.GetFloat());
	SetFriction(sv_neo_grenade_friction.GetFloat());
	SetCollisionGroup(COLLISION_GROUP_WEAPON);

	m_flDamage = NEO_DETPACK_DAMAGE;
	m_DmgRadius = NEO_DETPACK_DAMAGE_RADIUS;
	m_takedamage = DAMAGE_YES;
	m_iHealth = 1;
	m_punted = false;
	m_hasSettled = false;
	m_hasBeenMadeNonSolid = false;
	m_hasBeenTriggeredToDetonate = false;

	m_lastPos = GetAbsOrigin();
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
	SetDetonateTimerLength(detonateDelay);
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
		// Clear think function
		SetThink(NULL);
		return;
	}

	SetNextThink(gpGlobals->curtime + 0.1);
}

void CNEODeployedDetpack::Explode(trace_t* pTrace, int bitsDamageType)
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
		gamestats->Event_WeaponHit(pPlayer, true, "weapon_remotedet", info);
	}
#endif
}

bool CNEODeployedDetpack::TryDetonate(void)
{
	if (m_hasBeenTriggeredToDetonate || (gpGlobals->curtime > GetDetonateTimerLength()))
	{
		Detonate();
		return true;
	}

	return false;
}

void CNEODeployedDetpack::Detonate(void)
{
	// Make sure we've stopped playing bounce tick sounds upon explosion
	if (!m_hasSettled)
	{
		m_hasSettled = true;
	}
	BaseClass::Detonate();

	SetThink(&CBaseEntity::SUB_Remove);
	SetNextThink(gpGlobals->curtime);
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
	ExplosionCreate(GetAbsOrigin() + Vector(0, 0, 16), GetAbsAngles(), GetThrower(), GetDamage(), GetDamageRadius(),
					SF_ENVEXPLOSION_NOSPARKS | SF_ENVEXPLOSION_NODLIGHTS | SF_ENVEXPLOSION_NOSMOKE | SF_ENVEXPLOSION_NOSOUND, 0.0f, this);
	m_hasBeenTriggeredToDetonate = true;
	DevMsg("CNEODeployedDetpack::InputRemoteDetonate triggered\n");
	EmitSound("weapon_remotedet.npc_single");
	UTIL_Remove(this);
}

CBaseGrenadeProjectile* NEODeployedDetpack_Create(const Vector& position, const QAngle& angles, const Vector& velocity,
	const AngularImpulse& angVelocity, CBaseEntity* pOwner)
{
	// Don't set the owner here, or the player can't interact with grenades he's thrown
	auto pDet = (CNEODeployedDetpack*)CBaseEntity::Create("neo_deployed_detpack", position, angles, pOwner);

	pDet->SetAbsVelocity(velocity);
	pDet->ApplyLocalAngularVelocityImpulse(angVelocity);
	pDet->SetupInitialTransmittedGrenadeVelocity(velocity);

	// Since we are inheriting from a grenade class, just make sure the fuse never blows due to timer.
	pDet->SetTimer(FLT_MAX, FLT_MAX);
	pDet->SetThrower(ToBaseCombatCharacter(pOwner));
	if (pOwner) pDet->ChangeTeam(pOwner->GetTeamNumber());
	pDet->m_takedamage = DAMAGE_EVENTS_ONLY;

	return pDet;
}

bool NEODeployedDetpack_WasPunted(const CBaseEntity* pEntity)
{
	auto pDet = dynamic_cast<const CNEODeployedDetpack*>(pEntity);
	return ((pDet != NULL) ? pDet->WasPunted() : false);
}
