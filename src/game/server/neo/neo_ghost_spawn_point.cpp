#include "cbase.h"
#include "neo_ghost_spawn_point.h"

#include "neo_gamerules.h"

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

LINK_ENTITY_TO_CLASS(neo_ghostspawnpoint, CNEOGhostSpawnPoint);

LINK_ENTITY_TO_CLASS(neo_juggernautspawnpoint, CNEOJuggernautSpawnPoint);

void CNEOGhostSpawnPoint::Spawn()
{
	BaseClass::Spawn();

	NEORules()->m_ghostSpawns.AddToTail(this);
}
