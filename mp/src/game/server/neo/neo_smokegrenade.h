#ifndef NEO_SMOKEGRENADE_NEO_H
#define NEO_SMOKEGRENADE_NEO_H
#ifdef _WIN32
#pragma once
#endif

#include "cbase.h"
#include "sdk/sdk_basegrenade_projectile.h"
#include "grenade_frag.h"
#include "Sprite.h"
#include "SpriteTrail.h"
#include "soundent.h"

#define NEO_SMOKE_GRENADE_MODEL "models/weapons/w_smokenade.mdl"

class CNEOGrenadeSmoke : public CBaseGrenadeProjectile
{
	DECLARE_CLASS(CNEOGrenadeSmoke, CBaseGrenadeProjectile);
#ifndef CLIENT_DLL
	DECLARE_DATADESC();
#endif

public:
	virtual void	Detonate(void);
	virtual void	Explode(trace_t* pTrace, int bitsDamageType) { Assert(false); }

	void	Spawn(void);
	int		OnTakeDamage(const CTakeDamageInfo& inputInfo);
	void	DelayThink();
	void	SetPunted(bool punt) { m_punted = punt; }
	bool	WasPunted(void) const { return m_punted; }
	void	OnPhysGunPickup(CBasePlayer* pPhysGunUser, PhysGunPickup_t reason);

	void	InputSetTimer(inputdata_t& inputdata);

private:
	bool TryDetonate(void);
	void SetupParticles(size_t number);

protected:
	bool	m_inSolid;
	bool	m_punted;
	bool	m_hasSettled;
	bool	m_hasBeenMadeNonSolid;

	float	m_flSmokeBloomTime;

private:
	Vector m_lastPos;
};

CBaseGrenadeProjectile* NEOSmokegrenade_Create(const Vector& position, const QAngle& angles, const Vector& velocity,
	const AngularImpulse& angVelocity, CBaseEntity* pOwner);

bool NEOSmokegrenade_WasPunted(const CBaseEntity* pEntity);

#endif // NEO_SMOKEGRENADE_NEO_H