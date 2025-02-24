#include "cbase.h"
#include "neo_grenade.h"

#include "neo_tracefilter_collisiongroupdelta.h"

#ifdef GAME_DLL
#include "gamestats.h"
#endif

#include "vcollide_parse.h"
#include "sdk/sdk_basegrenade_projectile.h"

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

ConVar sv_neo_frag_showdebug("sv_neo_frag_showdebug", "0", FCVAR_CHEAT, "Show frag collision debug", true, 0.0, true, 1.0);
ConVar sv_neo_frag_vphys_reawaken_vel("sv_neo_frag_vphys_reawaken_vel", "200", FCVAR_CHEAT);
extern ConVar sv_neo_grenade_cor;
extern ConVar sv_neo_grenade_gravity;
extern ConVar sv_neo_grenade_friction;
extern ConVar sv_neo_grenade_fuse_timer;
void CNEOGrenadeFrag::Spawn(void)
{
	Precache();
	SetModel(NEO_FRAG_GRENADE_MODEL);
	BaseClass::Spawn();

	SetElasticity(sv_neo_grenade_cor.GetFloat());
	SetGravity(sv_neo_grenade_gravity.GetFloat());
	SetFriction(sv_neo_grenade_friction.GetFloat());
	SetCollisionGroup(COLLISION_GROUP_WEAPON);
	SetDetonateTimerLength(FLT_MAX);

	SetThink(&CNEOGrenadeFrag::DelayThink);
	SetNextThink(gpGlobals->curtime);
}

void CNEOGrenadeFrag::Precache(void)
{
	BaseClass::Precache();
}

void CNEOGrenadeFrag::OnPhysGunPickup(CBasePlayer *pPhysGunUser, PhysGunPickup_t reason)
{
	SetThrower(pPhysGunUser);
	m_bHasWarnedAI = true;
	BaseClass::OnPhysGunPickup(pPhysGunUser, reason);
}

int CNEOGrenadeFrag::OnTakeDamage(const CTakeDamageInfo &inputInfo)
{
	// NEO nades can't be detonated by damage.
	return 0;
}

void CNEOGrenadeFrag::DelayThink()
{
	if (gpGlobals->curtime > GetDetonateTimerLength())
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

void CNEOGrenadeFrag::InputSetTimer(inputdata_t &inputdata)
{
	SetDetonateTimerLength(inputdata.value.Float());
}

CBaseGrenadeProjectile *NEOFraggrenade_Create(const Vector &position, const QAngle &angles, const Vector &velocity,
	const AngularImpulse &angVelocity, CBaseEntity *pOwner, bool combineSpawned)
{
	// Don't set the owner here, or the player can't interact with grenades he's thrown
	CNEOGrenadeFrag *pGrenade = (CNEOGrenadeFrag *)CBaseEntity::Create("neo_grenade_frag", position, angles, pOwner);

	pGrenade->SetAbsVelocity(velocity);
	pGrenade->ApplyLocalAngularVelocityImpulse(angVelocity);
	pGrenade->SetupInitialTransmittedGrenadeVelocity(velocity);

	// make NPCs afaid of it while in the air
	pGrenade->SetNextThink(gpGlobals->curtime);
	pGrenade->SetThrower(ToBaseCombatCharacter(pOwner));
	if (pOwner) pGrenade->ChangeTeam(pOwner->GetTeamNumber());
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
