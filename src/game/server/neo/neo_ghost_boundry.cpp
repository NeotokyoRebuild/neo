#include "neo_ghost_boundry.h"
#include "neo_gamerules.h"
#include "weapon_ghost.h"

//##############################
//  Trigger Weapon
//##############################

LINK_ENTITY_TO_CLASS(neo_trigger_weapon, CNEO_TriggerWeapon);

BEGIN_DATADESC(CNEO_TriggerWeapon)
    DEFINE_THINKFUNC(Think),
END_DATADESC()

#define THINK_INTERVAL 0.05f
#define GHOST_ANGLES QAngle(15, 0, 270)

void CNEO_TriggerWeapon::Spawn()
{
    BaseClass::Spawn();
    InitTrigger();

    SetNextThink(gpGlobals->curtime + THINK_INTERVAL);
}

void CNEO_TriggerWeapon::Think()
{
    CheckForWeapon();

    SetNextThink(gpGlobals->curtime + THINK_INTERVAL);
}

// Weapons have the FSOLID_TRIGGER solidflag which is why they can't be used in triggers.
// If we remove it weapons can't be picked up by walking over them.

// NEO TODO DG: When there is a use for it improve this so it functions as expected- like other triggers.
// What's here is fine for the ghost boundry but using this trigger for weapons will spam StartTouch every think
void CNEO_TriggerWeapon::CheckForWeapon()
{
    Vector mins, maxs;
    CollisionProp()->WorldSpaceAABB(&mins, &maxs);

    CBaseEntity* pEntList[128];
    int count = UTIL_EntitiesInBox(pEntList, sizeof(pEntList), mins, maxs, null); // Low accuracy

    for (int i = 0; i < count; ++i)
    {
        CBaseEntity* pEnt = pEntList[i];

        if (!pEnt->IsBaseCombatWeapon())
        {
            continue;
        }

        if (pEnt->GetOwnerEntity())
        {
            continue;
        }

        if (IsEntityInside(pEnt)) // High(er) accuracy
        {
            StartTouch(pEnt);
        }
    }
}

bool CNEO_TriggerWeapon::IsEntityInside(CBaseEntity* pEnt)
{
    trace_t tr;
    Vector point = pEnt->WorldSpaceCenter();
    UTIL_TraceModel(point, point, pEnt->CollisionProp()->OBBMins(), pEnt->CollisionProp()->OBBMaxs(), this, COLLISION_GROUP_NONE, &tr);

    return tr.startsolid; // If the trace started inside the trigger or not
}


//##############################
//  Ghost Boundry
//##############################

LINK_ENTITY_TO_CLASS(neo_ghost_boundry, CNEO_GhostBoundry);

BEGIN_DATADESC(CNEO_GhostBoundry)
    DEFINE_THINKFUNC(Think),
END_DATADESC()

void CNEO_GhostBoundry::Think()
{
    CheckForWeapon();

    if (!NEORules()->GhostExists())
    {
        SetNextThink(gpGlobals->curtime + THINK_INTERVAL);
        return;
    }

    int iGhosterIndex = NEORules()->GetGhosterPlayer();
    if (iGhosterIndex == 0)
    {
        SetNextThink(gpGlobals->curtime + THINK_INTERVAL);
        return;
    }

    CBasePlayer* pPlayer = UTIL_PlayerByIndex(iGhosterIndex);
    if (pPlayer->GetFlags() & FL_ONGROUND)
    {
        m_vecLastGhosterPos = pPlayer->GetAbsOrigin();
    }

    SetNextThink(gpGlobals->curtime + THINK_INTERVAL);
}

void CNEO_GhostBoundry::StartTouch(CBaseEntity *pOther)
{
    BaseClass::StartTouch(pOther);

    Vector vecGhostSpawn = NEORules()->m_vecPreviousGhostSpawn;
    if ((vecGhostSpawn == vec3_origin) && (m_vecLastGhosterPos == vec3_origin))
    {
        return; // Don't bother teleporting if there's no ghost spawn and no ghoster pos
    }

    CWeaponGhost *pDroppedGhost = dynamic_cast<CWeaponGhost*>(pOther);
    if (pDroppedGhost)
    {
        if (m_vecLastGhosterPos == vec3_origin)
        {
            pDroppedGhost->VPhysicsGetObject()->SetPosition(vecGhostSpawn, GHOST_ANGLES, true);
            // Don't freeze it, ghosts aren't frozen when they spawn
        }
        else
        {
            pDroppedGhost->VPhysicsGetObject()->SetPosition(m_vecLastGhosterPos, GHOST_ANGLES, true);
            pDroppedGhost->VPhysicsGetObject()->EnableMotion(false);
        }

        return;
    }

    CNEO_Player *pNEOPlayer = dynamic_cast<CNEO_Player*>(pOther);
    if (pNEOPlayer && pNEOPlayer->IsCarryingGhost())
    {
        CBaseCombatWeapon* pHeldGhost = pNEOPlayer->Weapon_OwnsThisType("weapon_ghost");
        pNEOPlayer->Weapon_Drop(pHeldGhost);

        pHeldGhost->VPhysicsGetObject()->SetPosition(m_vecLastGhosterPos, GHOST_ANGLES, true);
        pHeldGhost->VPhysicsGetObject()->EnableMotion(false);

        return;
    }
}
