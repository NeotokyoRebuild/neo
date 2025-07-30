#ifndef NEO_SMOKELOSBLOCKER_NEO_H
#define NEO_SMOKELOSBLOCKER_NEO_H
#ifdef _WIN32
#pragma once
#endif

#include "cbase.h"

#define SMOKELINEOFSIGHTBLOCKER_ENTITYNAME	"env_smokelineofsightblocker"


class CNEOSmokeLineOfSightBlocker : public CBaseEntity {
public:
	DECLARE_CLASS(CNEOSmokeLineOfSightBlocker, CBaseEntity);
	DECLARE_DATADESC();

	void Spawn() override {
		BaseClass::Spawn();
		SetSolid(SOLID_BBOX);
		SetCollisionBounds(m_mins, m_maxs);

		// For making trace lines hit
		RemoveSolidFlags(FSOLID_NOT_SOLID);
		// For allowing players and projectiles to pass through
		SetCollisionGroup(COLLISION_GROUP_DEBRIS);
	}

	// NEO Jank: Have TraceLines that include CONTENTS_HITBOX in the mask ignore this entity
	// or else players in/beyond smoke become immune to damage from projectiles despite being hit.
	virtual bool ShouldCollide(int collisionGroup, int contentsMask) const override
	{
		// Traces using CONTENTS_HITBOX determine if the target was hit, determining if damage is applied.
		if (contentsMask & CONTENTS_HITBOX)
			return false;

		// Fall back to normal collision behavior
		return CBaseEntity::ShouldCollide(collisionGroup, contentsMask);
	}

	void SetBounds(const Vector& mins, const Vector& maxs) { m_mins = mins; m_maxs = maxs; }

private:
	Vector m_mins, m_maxs;
};

#endif // NEO_SMOKELOSBLOCKER_NEO_H
