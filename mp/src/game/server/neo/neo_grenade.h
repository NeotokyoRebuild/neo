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

extern ConVar sk_plr_dmg_fraggrenade, sk_npc_dmg_fraggrenade, sk_fraggrenade_radius;

#define NEO_FRAG_GRENADE_MODEL "models/weapons/w_frag_thrown.mdl"

class CNEOGrenadeFrag : public CBaseGrenade
{
	DECLARE_CLASS(CNEOGrenadeFrag, CBaseGrenade);
#ifndef CLIENT_DLL
	DECLARE_DATADESC();
#endif

public:
	virtual void	Explode(trace_t* pTrace, int bitsDamageType);

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

CBaseGrenade *NEOFraggrenade_Create(const Vector &position, const QAngle &angles, const Vector &velocity,
	const AngularImpulse &angVelocity, CBaseEntity *pOwner, float timer, bool combineSpawned);

bool NEOFraggrenade_WasPunted(const CBaseEntity *pEntity);

#endif // NEO_GRENADE_NEO_H