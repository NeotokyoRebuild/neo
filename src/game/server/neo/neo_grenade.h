#ifndef NEO_GRENADE_NEO_H
#define NEO_GRENADE_NEO_H
#ifdef _WIN32
#pragma once
#endif

#include "cbase.h"
#include "sdk/sdk_basegrenade_projectile.h"
#include "grenade_frag.h"
#include "Sprite.h"
#include "SpriteTrail.h"
#include "soundent.h"

#define NEO_FRAG_GRENADE_WARN_TIME 1.5f

extern ConVar sk_plr_dmg_fraggrenade, sk_npc_dmg_fraggrenade, sk_fraggrenade_radius;

#define NEO_FRAG_GRENADE_MODEL "models/weapons/w_frag_thrown.mdl"

class CNEOGrenadeFrag : public CBaseGrenadeProjectile
{
	DECLARE_CLASS(CNEOGrenadeFrag, CBaseGrenadeProjectile);
#ifndef CLIENT_DLL
	DECLARE_DATADESC();
#endif

public:
	virtual void	Explode(trace_t* pTrace, int bitsDamageType);
	virtual	float	GetShakeAmplitude() { return 0; };

	void	Spawn(void);
	void	Precache(void);
	int		OnTakeDamage(const CTakeDamageInfo &inputInfo);
	void	DelayThink();
	void	SetPunted(bool punt) { m_punted = punt; }
	bool	WasPunted(void) const { return m_punted; }
	void	OnPhysGunPickup(CBasePlayer *pPhysGunUser, PhysGunPickup_t reason);

	void	InputSetTimer(inputdata_t &inputdata);

protected:
	bool	m_inSolid;
	bool	m_punted;
};

CBaseGrenadeProjectile *NEOFraggrenade_Create(const Vector &position, const QAngle &angles, const Vector &velocity,
	const AngularImpulse &angVelocity, CBaseEntity *pOwner, bool combineSpawned);

bool NEOFraggrenade_WasPunted(const CBaseEntity *pEntity);

#endif // NEO_GRENADE_NEO_H