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

	// Starts LOS interception after scheduled delay when smoke particles are practically occluded
	void ActivateLOSBlocker();

	void Spawn() override {
		BaseClass::Spawn();
		SetSolid(SOLID_BBOX);
		SetCollisionBounds(m_mins, m_maxs);
		SetSolidFlags(FSOLID_CUSTOMRAYTEST | FSOLID_CUSTOMBOXTEST);
	}

	bool TestCollision(const Ray_t& ray, unsigned int fContentsMask, trace_t& tr) override
	{
		if (fContentsMask &= CONTENTS_BLOCKLOS)
		{
			tr.fraction = 0.f; // NEO NOTE (Adam) Shouldn't returning true be enough? Should we calculate correct value for the fraction?
			return true;
		}
		return false;
	}

	bool TestHitboxes(const Ray_t& ray, unsigned int fContentsMask, trace_t& tr) override
	{ // NEO NOTE (Adam) Shotguns used to use hulls for half of their pellets, not sure if this los blocker would have affected that, check if/when any weapons do again
		return false;
	}

	void SetBounds(const Vector& mins, const Vector& maxs) { m_mins = mins; m_maxs = maxs; }

private:
	Vector m_mins, m_maxs;
};

#endif // NEO_SMOKELOSBLOCKER_NEO_H
