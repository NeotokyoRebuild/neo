#include "cbase.h"
#include "neo_detpack.h"

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

void CNEODeployedDetpack::Spawn(void)
{
	Precache();

	SetModel(NEO_DEPLOYED_DETPACK_MODEL);

	m_flDamage = NEO_DETPACK_DAMAGE;
	m_DmgRadius = NEO_DETPACK_DAMAGE_RADIUS;

	m_takedamage = DAMAGE_YES;
	m_iHealth = 1;

#define NEO_DET_COLLISION_RADIUS (NEO_DEPLOYED_DETPACK_RADIUS * 2.0)

	SetSize(-Vector(NEO_DEPLOYED_DETPACK_RADIUS, NEO_DEPLOYED_DETPACK_RADIUS, NEO_DEPLOYED_DETPACK_RADIUS),
		Vector(NEO_DEPLOYED_DETPACK_RADIUS, NEO_DEPLOYED_DETPACK_RADIUS, NEO_DEPLOYED_DETPACK_RADIUS));
	SetCollisionBounds(-Vector(NEO_DET_COLLISION_RADIUS, NEO_DET_COLLISION_RADIUS, NEO_DET_COLLISION_RADIUS),
		Vector(NEO_DET_COLLISION_RADIUS, NEO_DET_COLLISION_RADIUS, NEO_DET_COLLISION_RADIUS));
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
	m_hasBeenTriggeredToDetonate = false;

	BaseClass::Spawn();

	m_lastPos = GetAbsOrigin();
}

bool CNEODeployedDetpack::CreateVPhysics()
{
	// Create the object in the physics system
	VPhysicsInitNormal(NADE_SOLID_TYPE, 0, false);
	return true;
}

extern ConVar sv_neo_frag_cor;
extern ConVar sv_neo_frag_showdebug;
extern ConVar sv_neo_frag_vphys_reawaken_vel;

void CNEODeployedDetpack::VPhysicsUpdate(IPhysicsObject* pPhysics)
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
				DevMsg("Det re-bounced in solid\n");
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
				DevMsg("Det has settled; awoke CNEODeployedDetpack VPhys for handling\n");
			}
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
	if (m_hasBeenTriggeredToDetonate || (gpGlobals->curtime > m_flDetonateTime))
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

	SetThink(&CNEODeployedDetpack::SUB_Remove);
	SetNextThink(gpGlobals->curtime);
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
