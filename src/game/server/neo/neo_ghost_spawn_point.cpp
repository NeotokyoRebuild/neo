#include "cbase.h"
#include "neo_ghost_spawn_point.h"

#include "neo_gamerules.h"

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

BEGIN_DATADESC(CNEOObjectiveSpawnPoint)
	DEFINE_KEYFIELD(m_bStartDisabled, FIELD_BOOLEAN, "StartDisabled"),

	DEFINE_INPUTFUNC(FIELD_VOID, "Enable", InputEnable),
	DEFINE_INPUTFUNC(FIELD_VOID, "Disable", InputDisable),

	DEFINE_OUTPUT(m_OnSpawnedHere, "OnSpawnedHere"),
END_DATADESC()

void CNEOObjectiveSpawnPoint::Spawn()
{
	BaseClass::Spawn();

	if (m_bStartDisabled)
	{
		m_bDisabled = true;
	}
}

LINK_ENTITY_TO_CLASS(neo_ghostspawnpoint, CNEOGhostSpawnPoint);

LINK_ENTITY_TO_CLASS(neo_juggernautspawnpoint, CNEOJuggernautSpawnPoint);

void CNEOGhostSpawnPoint::Spawn()
{
	BaseClass::Spawn();

	if (!m_bDisabled)
	{
		NEORules()->m_ghostSpawns.AddToTail(this);
	}
}

void CNEOJuggernautSpawnPoint::Spawn()
{
	BaseClass::Spawn();

	if (!m_bDisabled)
	{
		NEORules()->m_jgrSpawns.AddToTail(this);
	}
}