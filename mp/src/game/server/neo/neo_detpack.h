#ifndef NEO_DETPACK_NEO_H
#define NEO_DETPACK_NEO_H
#ifdef _WIN32
#pragma once
#endif

#include "cbase.h"
#include "basegrenade_shared.h"
#include "grenade_frag.h"
#include "Sprite.h"
#include "SpriteTrail.h"
#include "soundent.h"

#define NEO_DEPLOYED_DET_WARN_TIME 1.5f

#define NEO_DEPLOYED_DETPACK_MODEL "models/weapons/w_detpack.mdl"
#define NEO_DEPLOYED_DETPACK_RADIUS 4.0f // Inches. NEO TODO (Rain): check correct value!

#define NEO_DETPACK_DAMAGE 200.0f
#define NEO_DETPACK_DAMAGE_RADIUS 400.0f

class CNEODeployedDetpack : public CBaseGrenade
{
	DECLARE_CLASS(CNEODeployedDetpack, CBaseGrenade);
#ifndef CLIENT_DLL
	DECLARE_DATADESC();
#endif

public:
	virtual void	Detonate(void);
	virtual void	Explode(trace_t* pTrace, int bitsDamageType);

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

	void	InputRemoteDetonate(inputdata_t& inputdata);

protected:
	bool	m_inSolid;
	bool	m_punted;
	bool	m_hasSettled;
	bool	m_hasBeenMadeNonSolid;
	bool	m_hasBeenTriggeredToDetonate;

private:
	bool TryDetonate(void);

private:
	Vector m_lastPos;
};

CBaseGrenade *NEODeployedDetpack_Create(const Vector& position, const QAngle& angles, const Vector& velocity,
	const AngularImpulse& angVelocity, CBaseEntity* pOwner);

bool NEODeployedDetpack_WasPunted(const CBaseEntity* pEntity);

#endif // NEO_DETPACK_NEO_H