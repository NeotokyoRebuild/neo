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

    void SetBounds(const Vector &mins, const Vector &maxs) { m_mins = mins; m_maxs = maxs; }

private:
    Vector m_mins, m_maxs;
};

#endif // NEO_SMOKELOSBLOCKER_NEO_H
