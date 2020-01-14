#ifndef NEO_GRENADE_NEO_H
#define NEO_GRENADE_NEO_H
#ifdef _WIN32
#pragma once
#endif

#include "cbase.h"
#include "basegrenade_shared.h"
#include "grenade_frag.h"
#include "Sprite.h"
#include "SpriteTrail.h"
#include "soundent.h"

#define NEO_FRAG_GRENADE_WARN_TIME 1.5f

const float GRENADE_COEFFICIENT_OF_RESTITUTION = 0.2f;

extern ConVar sk_plr_dmg_fraggrenade, sk_npc_dmg_fraggrenade, sk_fraggrenade_radius;

#define NEO_FRAG_GRENADE_MODEL "models/weapons/w_frag_thrown.mdl"

class CNEOGrenadeFrag : public CBaseGrenade
{
	DECLARE_CLASS(CNEOGrenadeFrag, CBaseGrenade);
#ifndef CLIENT_DLL
	DECLARE_DATADESC();
#endif

public:
	void	Spawn(void);
	void	Precache(void);
	bool	CreateVPhysics(void);
	void	SetTimer(float detonateDelay, float warnDelay);
	void	SetVelocity(const Vector &velocity, const AngularImpulse &angVelocity);
	int		OnTakeDamage(const CTakeDamageInfo &inputInfo);
	void	DelayThink();
	void	VPhysicsUpdate(IPhysicsObject *pPhysics);
	void	SetPunted(bool punt) { m_punted = punt; }
	bool	WasPunted(void) const { return m_punted; }
	void	OnPhysGunPickup(CBasePlayer *pPhysGunUser, PhysGunPickup_t reason);

	void	InputSetTimer(inputdata_t &inputdata);

protected:
	bool	m_inSolid;
	bool	m_punted;
};

// This will hit only things that are in newCollisionGroup, but NOT in collisionGroupAlreadyChecked
class CNEOTraceFilterCollisionGroupDelta : public CTraceFilterEntitiesOnly
{
public:
	// It does have a base, but we'll never network anything below here..
	DECLARE_CLASS_NOBASE(CNEOTraceFilterCollisionGroupDelta);

	CNEOTraceFilterCollisionGroupDelta(const IHandleEntity *passentity, int collisionGroupAlreadyChecked, int newCollisionGroup)
		: m_pPassEnt(passentity), m_collisionGroupAlreadyChecked(collisionGroupAlreadyChecked), m_newCollisionGroup(newCollisionGroup)
	{
	}

	virtual bool ShouldHitEntity(IHandleEntity *pHandleEntity, int contentsMask)
	{
		if (!PassServerEntityFilter(pHandleEntity, m_pPassEnt))
		{
			return false;
		}

		CBaseEntity *pEntity = EntityFromEntityHandle(pHandleEntity);

		if (pEntity)
		{
			if (g_pGameRules->ShouldCollide(m_collisionGroupAlreadyChecked, pEntity->GetCollisionGroup()))
			{
				return false;
			}
			if (g_pGameRules->ShouldCollide(m_newCollisionGroup, pEntity->GetCollisionGroup()))
			{
				return true;
			}
		}

		return false;
	}

protected:
	const IHandleEntity *m_pPassEnt;
	int		m_collisionGroupAlreadyChecked;
	int		m_newCollisionGroup;
};

CBaseGrenade *NEOFraggrenade_Create(const Vector &position, const QAngle &angles, const Vector &velocity,
	const AngularImpulse &angVelocity, CBaseEntity *pOwner, float timer, bool combineSpawned);

bool NEOFraggrenade_WasPunted(const CBaseEntity *pEntity);

#endif // NEO_GRENADE_NEO_H