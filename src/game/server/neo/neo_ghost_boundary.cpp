#include "neo_ghost_boundary.h"
#include "neo_gamerules.h"
#include "weapon_ghost.h"

#define THINK_INTERVAL 0.05f
#define GHOST_ANGLES QAngle(15, 0, 270) // Angles the ghost will reset to (laid on its back)

//##############################
//  Trigger Weapon
//##############################

LINK_ENTITY_TO_CLASS(neo_trigger_weapon, CNEO_TriggerWeapon);

BEGIN_DATADESC(CNEO_TriggerWeapon)
    DEFINE_THINKFUNC(Think),
END_DATADESC()

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
// What's here is fine for the ghost boundary but using this trigger for weapons will spam StartTouch every think
void CNEO_TriggerWeapon::CheckForWeapon()
{
    Vector mins, maxs;
    CollisionProp()->WorldSpaceAABB(&mins, &maxs);

    CBaseEntity* pEntList[128];
    int count = UTIL_EntitiesInBox(pEntList, ARRAYSIZE(pEntList), mins, maxs, 0); // Low accuracy

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
//  Ghost Boundary
//##############################

LINK_ENTITY_TO_CLASS(neo_ghost_boundary, CNEO_GhostBoundary);

BEGIN_DATADESC(CNEO_GhostBoundary)
    DEFINE_THINKFUNC(Think),
END_DATADESC()

void CNEO_GhostBoundary::Think()
{
    BaseClass::Think();

    if (NEORules()->GetGameType() == NEO_GAME_TYPE_JGR)
    {
        // In JGR, this is only true when the juggernaut is in it's item form, so there is no player
        if (NEORules()->JuggernautItemExists())
        {
            return;
        }

        int iJuggernautIndex = NEORules()->GetJuggernautPlayer();
        if (iJuggernautIndex == 0)
        {
            return;
        }

        CBasePlayer* pPlayer = UTIL_PlayerByIndex(iJuggernautIndex);
        if (pPlayer->GetFlags() & FL_ONGROUND)
        {
            m_vecLastObjectivePos = pPlayer->GetAbsOrigin();
        }
    }
    else
    {
        if (!NEORules()->GhostExists())
        {
            return;
        }

        int iGhosterIndex = NEORules()->GetGhosterPlayer();
        if (iGhosterIndex == 0)
        {
            return;
        }

        CBasePlayer* pPlayer = UTIL_PlayerByIndex(iGhosterIndex);
        if (pPlayer->GetFlags() & FL_ONGROUND)
        {
            m_vecLastObjectivePos = pPlayer->GetAbsOrigin();
        }
    }
}

void CNEO_GhostBoundary::StartTouch(CBaseEntity *pOther)
{
    BaseClass::StartTouch(pOther);

    if (NEORules()->GetGameType() == NEO_GAME_TYPE_JGR)
    {
        if ((NEORules()->m_vecPreviousJuggernautSpawn == vec3_origin) && (m_vecLastObjectivePos == vec3_origin))
        {
            return; // Don't bother teleporting if there's no juggernaut spawn and no jgr player pos
        }

        CNEO_Juggernaut *pDroppedJuggernaut = dynamic_cast<CNEO_Juggernaut*>(pOther);
        if (pDroppedJuggernaut)
        {
            pDroppedJuggernaut->SetAbsOrigin(m_vecLastObjectivePos);
        }
    }
    else
    {
        Vector vecGhostSpawn = NEORules()->m_vecPreviousGhostSpawn;
        if ((vecGhostSpawn == vec3_origin) && (m_vecLastObjectivePos == vec3_origin))
        {
            return; // Don't bother teleporting if there's no ghost spawn and no ghoster pos
        }

        auto pDroppedGhost = (pOther && pOther->IsBaseCombatWeapon() && static_cast<CNEOBaseCombatWeapon*>(pOther)->IsGhost())
            ? static_cast<CWeaponGhost*>(pOther) : nullptr;
        if (pDroppedGhost)
        {
            if (m_vecLastObjectivePos == vec3_origin)
            {
                pDroppedGhost->VPhysicsGetObject()->SetPosition(vecGhostSpawn, GHOST_ANGLES, true);
                // Don't freeze it, ghosts aren't frozen when they spawn
            }
            else
            {
                pDroppedGhost->VPhysicsGetObject()->SetPosition(m_vecLastObjectivePos, GHOST_ANGLES, true);
                pDroppedGhost->VPhysicsGetObject()->EnableMotion(false);
            }

            return;
        }

        CNEO_Player* pNEOPlayer = ToNEOPlayer(pOther);
        if (pNEOPlayer && pNEOPlayer->IsCarryingGhost())
        {
            CBaseCombatWeapon* pHeldGhost = pNEOPlayer->Weapon_OwnsThisType("weapon_ghost");
            pNEOPlayer->Weapon_Drop(pHeldGhost);

            pHeldGhost->VPhysicsGetObject()->SetPosition(m_vecLastObjectivePos, GHOST_ANGLES, true);
            pHeldGhost->VPhysicsGetObject()->EnableMotion(false);

            return;
        }
    }
}
