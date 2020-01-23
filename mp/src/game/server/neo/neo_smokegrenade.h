#ifndef NEO_SMOKEGRENADE_NEO_H
#define NEO_SMOKEGRENADE_NEO_H
#ifdef _WIN32
#pragma once
#endif

#include "cbase.h"
#include "basegrenade_shared.h"
#include "grenade_frag.h"
#include "Sprite.h"
#include "SpriteTrail.h"
#include "soundent.h"

const float GRENADE_COEFFICIENT_OF_RESTITUTION = 0.2f;

#define NEO_SMOKE_GRENADE_MODEL "models/weapons/w_smokenade.mdl"

class CNEOGrenadeSmoke : public CBaseGrenade
{
	DECLARE_CLASS(CNEOGrenadeSmoke, CBaseGrenade);
#ifndef CLIENT_DLL
	DECLARE_DATADESC();
#endif

public:
	void	Spawn(void);
	void	Precache(void);
	bool	CreateVPhysics(void);
	void	SetTimer(float detonateDelay, float warnDelay);
	void	SetVelocity(const Vector& velocity, const AngularImpulse& angVelocity);
	int		OnTakeDamage(const CTakeDamageInfo& inputInfo);
	void	DelayThink();
	void	VPhysicsUpdate(IPhysicsObject* pPhysics);
	void	SetPunted(bool punt) { m_punted = punt; }
	bool	WasPunted(void) const { return m_punted; }
	void	OnPhysGunPickup(CBasePlayer* pPhysGunUser, PhysGunPickup_t reason);

	void	InputSetTimer(inputdata_t& inputdata);

protected:
	bool	m_inSolid;
	bool	m_punted;
};

CBaseGrenade* NEOSmokegrenade_Create(const Vector& position, const QAngle& angles, const Vector& velocity,
	const AngularImpulse& angVelocity, CBaseEntity* pOwner, float timer, bool combineSpawned);

bool NEOSmokegrenade_WasPunted(const CBaseEntity* pEntity);

#endif // NEO_SMOKEGRENADE_NEO_H