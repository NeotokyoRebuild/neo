#include "cbase.h"
#include "baseentity.h"
#include "filters.h"
#include "neo_player.h"
#include "tier0/memdbgon.h"

class CNEO_NPCTargetSystem : public CBaseEntity
{
public:
    DECLARE_CLASS(CNEO_NPCTargetSystem, CBaseEntity);
    DECLARE_DATADESC();

    CNEO_NPCTargetSystem();

    // KVs
    float m_flFOV;
    float m_flTopClip;
    float m_flBottomClip;
    float m_flMaxViewDistance;
    float m_flDeadzone;
    float m_flMiddleBoundsHalf;
    string_t m_iFilterName;
    bool m_bStartDisabled;
    // Damage trace
    string_t m_strDamageSourceName;
    float m_flDamage;
    float m_flFireRate;

    CBaseFilter* m_pFilter;
    EHANDLE m_hDamageSource;

    COutputEvent m_OnSpotted;
    COutputEvent m_OnRight;
    COutputEvent m_OnLeft;
    COutputEvent m_OnMiddle;
    COutputEvent m_OnMiddleIgnore;
    COutputEvent m_OnExit;
    COutputEvent m_OnExitMiddle;

    void InputEnable(inputdata_t& inputData);
    void InputDisable(inputdata_t& inputData);

    void Precache(void);
    void Spawn(void);
    void Think(void);

private:
    enum TargetZone_e
    {
        ZONE_NONE = 0,
        ZONE_LEFT,
        ZONE_MIDDLE,
        ZONE_RIGHT
    };

    TargetZone_e m_iLastZone;
    bool m_bTargetAcquired;
    bool m_bMiddleIgnoreActive;
    float m_flNextFireTime;
};

LINK_ENTITY_TO_CLASS(neo_npc_targetsystem, CNEO_NPCTargetSystem);

BEGIN_DATADESC(CNEO_NPCTargetSystem)
    DEFINE_KEYFIELD(m_flFOV, FIELD_FLOAT, "fov"),
    DEFINE_KEYFIELD(m_flTopClip, FIELD_FLOAT, "topclip"),
    DEFINE_KEYFIELD(m_flBottomClip, FIELD_FLOAT, "bottomclip"),
    DEFINE_KEYFIELD(m_flMaxViewDistance, FIELD_FLOAT, "maxviewdistance"),
    DEFINE_KEYFIELD(m_flDeadzone, FIELD_FLOAT, "deadzone"),
    DEFINE_KEYFIELD(m_flMiddleBoundsHalf, FIELD_FLOAT, "middlebounds"),
    DEFINE_KEYFIELD(m_iFilterName, FIELD_STRING, "filtername"),
    DEFINE_KEYFIELD(m_bStartDisabled, FIELD_BOOLEAN, "StartDisabled"),

    DEFINE_KEYFIELD(m_strDamageSourceName, FIELD_STRING, "damagesource"),
    DEFINE_KEYFIELD(m_flDamage, FIELD_FLOAT, "damage"),
    DEFINE_KEYFIELD(m_flFireRate, FIELD_FLOAT, "firerate"),

    DEFINE_INPUTFUNC(FIELD_VOID, "Enable", InputEnable),
    DEFINE_INPUTFUNC(FIELD_VOID, "Disable", InputDisable),

    DEFINE_OUTPUT(m_OnSpotted, "OnSpotted"),
    DEFINE_OUTPUT(m_OnRight, "OnRight"),
    DEFINE_OUTPUT(m_OnLeft, "OnLeft"),
    DEFINE_OUTPUT(m_OnMiddle, "OnMiddle"),
    DEFINE_OUTPUT(m_OnMiddleIgnore, "OnMiddleIgnore"),
    DEFINE_OUTPUT(m_OnExit, "OnExit"),
    DEFINE_OUTPUT(m_OnExitMiddle, "OnExitMiddle"),

    DEFINE_THINKFUNC(Think),
END_DATADESC()

// This entity is a built-for-purpose recreation of the HT tank's "vision"
CNEO_NPCTargetSystem::CNEO_NPCTargetSystem() :
    m_iLastZone(ZONE_NONE),
    m_bTargetAcquired(false),
    m_bMiddleIgnoreActive(false),
    m_pFilter(nullptr),
    m_hDamageSource(nullptr),
    m_flNextFireTime(0),
    m_bStartDisabled(false)
{
    m_flFOV = 90.0f;
    m_flTopClip = 60.0f;
    m_flBottomClip = -100.0f;
    m_flMaxViewDistance = 800.0f;
    m_flDeadzone = 100.0f;
    m_flMiddleBoundsHalf = 20.0f;
    m_iFilterName = NULL_STRING;

    m_strDamageSourceName = NULL_STRING;
    m_flDamage = 10.0f;
    m_flFireRate = 1.0f;
}

void CNEO_NPCTargetSystem::Precache(void)
{
    BaseClass::Precache();
}

void CNEO_NPCTargetSystem::Spawn(void)
{
    Precache();

    if (m_iFilterName != NULL_STRING)
    {
        m_pFilter = dynamic_cast<CBaseFilter*>(gEntList.FindEntityByName(nullptr, m_iFilterName));
    }

    if (m_strDamageSourceName != NULL_STRING)
    {
        m_hDamageSource = gEntList.FindEntityByName(nullptr, m_strDamageSourceName);
    }

    SetThink(&CNEO_NPCTargetSystem::Think);
    if (m_bStartDisabled)
    {
        SetNextThink(TICK_NEVER_THINK);
    }
    else
    {
        SetNextThink(gpGlobals->curtime + 0.05f);
    }
}

void CNEO_NPCTargetSystem::Think(void)
{
    bool bFoundTarget = false;

    float flBestLateral = FLT_MAX;
    float flBestForward = 0.0f;
    Vector vBestLateral;
    CBasePlayer* pBestTarget = nullptr;

    const float flMaxViewDistanceSqr = m_flMaxViewDistance * m_flMaxViewDistance;
    const float flTanFOV = tan(DEG2RAD(m_flFOV * 0.5f));

    Vector vecEye = EyePosition();

    Vector vecForward, vecRight;
    AngleVectors(GetAbsAngles(), &vecForward, &vecRight, nullptr);

    for (int i = 1; i <= gpGlobals->maxClients; i++)
    {
        CBasePlayer* pPlayer = UTIL_PlayerByIndex(i);
        if (!pPlayer || !pPlayer->IsAlive())
            continue;

        if (m_pFilter && !m_pFilter->PassesFilter(this, pPlayer))
            continue;

        CNEO_Player* pNEOPlayer = static_cast<CNEO_Player*>(pPlayer);
        if (pNEOPlayer->m_bInThermOpticCamo)
            continue;

        // The player's eye position is used for detection.
        // The entity might be able to see the player's body up to their neck and they won't be detected
        Vector vecPlayerEye = pPlayer->EyePosition();
        Vector vecTargetPos = vecPlayerEye - vecEye;

        Vector vecTarget2DPos = vecTargetPos;
        vecTarget2DPos.z = 0;

        float flDistanceSqr = vecTarget2DPos.LengthSqr();
        if (flDistanceSqr > flMaxViewDistanceSqr) // Ignore if out of range
            continue;

        float flZDiff = vecPlayerEye.z - vecEye.z;
        if (flZDiff > m_flTopClip || flZDiff < m_flBottomClip) // Ignore if head is out of high/low bounds
            continue;

        if (!FVisible(pPlayer, MASK_BLOCKLOS, nullptr)) // Draw a trace to check if we can actually see the player
            continue;

        float flForwardDist = DotProduct(vecTarget2DPos, vecForward);
        if (flForwardDist <= 0) // Is the player behind?
            continue;

        // How close is this player to the center of our view
        Vector vLateral = vecTarget2DPos - (flForwardDist * vecForward);
        float flLateral = vLateral.Length();

        if (flLateral > flTanFOV * flForwardDist) // Ignore if they are outside our FOV
            continue;

        // The player is valid. But let's focus the one closest to us & our line of fire!!
        if (flLateral < flBestLateral)
        {
            flBestLateral = flLateral;
            flBestForward = flForwardDist;
            vBestLateral = vLateral;
            pBestTarget = pPlayer;
            bFoundTarget = true;
        }
    }

    // Determine the zone this valid player is in
    enum TargetZone_e iNewZone = ZONE_NONE;
    if (bFoundTarget)
    {
        if (flBestForward >= m_flDeadzone && flBestLateral <= m_flMiddleBoundsHalf) // If the player is outside the deadzone and inside the middle volume
        {
            iNewZone = ZONE_MIDDLE;
        }
        else if (flBestLateral > m_flMiddleBoundsHalf)
        {
            float flDotRight = DotProduct(vBestLateral, vecRight);
            iNewZone = (flDotRight > 0.0f) ? ZONE_RIGHT : ZONE_LEFT; // If the dot product is negative, the playa is on the left side
        }
    }

    bool bMiddleIgnore = bFoundTarget && (flBestLateral <= m_flMiddleBoundsHalf); // Its just the middle zone ignoring the deadzone.

    // Fire outputs
    if (bFoundTarget)
    {
        if (!m_bTargetAcquired)
        {
            m_OnSpotted.FireOutput(this, this);
            m_bTargetAcquired = true;
        }

        if (bMiddleIgnore && !m_bMiddleIgnoreActive)
        {
            m_OnMiddleIgnore.FireOutput(this, this);
            m_bMiddleIgnoreActive = true;
        }
        else if (!bMiddleIgnore && m_bMiddleIgnoreActive)
        {
            m_bMiddleIgnoreActive = false;
        }

        if (iNewZone != m_iLastZone) // If the zone has changed
        {
            if (m_iLastZone == ZONE_MIDDLE && iNewZone != ZONE_MIDDLE)
            {
                m_OnExitMiddle.FireOutput(this, this);
            }
            switch (iNewZone)
            {
            case ZONE_MIDDLE:
                m_OnMiddle.FireOutput(this, this);
                break;
            case ZONE_LEFT:
                m_OnLeft.FireOutput(this, this);
                break;
            case ZONE_RIGHT:
                m_OnRight.FireOutput(this, this);
                break;
            default:
                break;
            }
        }
    }
    else
    {
        if (m_bTargetAcquired)
        {
            if (m_iLastZone == ZONE_MIDDLE)
            {
                m_OnExitMiddle.FireOutput(this, this);
            }
            m_OnExit.FireOutput(this, this);
            m_bTargetAcquired = false;
        }
        // Reset state when no target is found
        m_bMiddleIgnoreActive = false;
    }

    // Optional - damage the target
    if (m_hDamageSource && bFoundTarget && (iNewZone == ZONE_MIDDLE) && pBestTarget)
    {
        if (gpGlobals->curtime >= m_flNextFireTime)
        {
            Vector vecDamageSrc = m_hDamageSource ? m_hDamageSource->WorldSpaceCenter() : vecEye;
            trace_t tr;
            UTIL_TraceLine(vecDamageSrc, pBestTarget->EyePosition(), MASK_SHOT, this, COLLISION_GROUP_NONE, &tr);

            CBaseEntity* pDamageTarget = (tr.m_pEnt != nullptr) ? tr.m_pEnt : pBestTarget;

            if (pDamageTarget && pDamageTarget->m_takedamage != DAMAGE_NO) // is the second check neseccary
            {
                CTakeDamageInfo info(this, this, m_flDamage, DMG_BULLET);
                info.SetDamagePosition(this->WorldSpaceCenter());
                info.SetDamageForce(Vector(0, 0, 1)); // These are just to fill the fields. Probably never used here
                pDamageTarget->TakeDamage(info);
            }

            if (m_flFireRate > 0)
                m_flNextFireTime = gpGlobals->curtime + (1.0f / m_flFireRate);
        }
    }

    m_iLastZone = iNewZone;
    SetNextThink(gpGlobals->curtime + 0.05f);
}

void CNEO_NPCTargetSystem::InputEnable(inputdata_t & inputData)
{
    SetNextThink(gpGlobals->curtime + 0.05f);
}

void CNEO_NPCTargetSystem::InputDisable(inputdata_t& inputData)
{
    SetNextThink(TICK_NEVER_THINK);
}