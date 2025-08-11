#include "cbase.h"
#include "util.h"
#include "neo_smokelineofsightblocker.h"

// NOTE: This has to be the last file included!
#include "tier0/memdbgon.h"

// Assume most existing traceline checks were not designed with blocking smoke in mind
// e.g. Jeff the tank in nt_rogue_ctg should see through smoke
bool CNEOSmokeLineOfSightBlocker::m_bNextEntitySeesThroughSmoke{ true };

LINK_ENTITY_TO_CLASS(env_smokelineofsightblocker, CNEOSmokeLineOfSightBlocker);
BEGIN_DATADESC(CNEOSmokeLineOfSightBlocker)
    DEFINE_FIELD(m_mins, FIELD_VECTOR),
    DEFINE_FIELD(m_maxs, FIELD_VECTOR),
END_DATADESC()

ConVar sv_neo_smoke_blocker_duration("sv_neo_smoke_blocker_duration", "20.5", FCVAR_CHEAT, "Time when LOS block should disappear", true, 0.0, true, 30.0);

// Activation scheduled by CNEOGrenadeSmoke::SetupParticles
void CNEOSmokeLineOfSightBlocker::ActivateLOSBlocker()
{
	// Start blocking LOS
	Spawn();
	// Active blocking duration ends after smoke can be seen through by a player
	float duration = sv_neo_smoke_blocker_duration.GetFloat();
	SetThink(&CNEOSmokeLineOfSightBlocker::SUB_Remove);
	SetNextThink(gpGlobals->curtime + duration);
}
