#pragma once

#include "neo_player_spawnpoint.h"

class CBasePlayer;
class CNEOSpawnPoint;
namespace NeoSpawnManager
{
	CNEOSpawnPoint* RequestSpawn(int team, CBasePlayer* player);

	void Init();
	void Deinit();
	void Register(CNEOSpawnPoint*);
	void Unregister(CNEOSpawnPoint*);
};
