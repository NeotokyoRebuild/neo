#pragma once

#include "neo_player_spawnpoint.h"

class CNEO_Player;
class CNEOSpawnPoint;
namespace NeoSpawnManager
{
	CNEOSpawnPoint* RequestSpawn(int team, CNEO_Player *player);

	void Init();
	void Deinit();
	void Register(CNEOSpawnPoint*);
	void Unregister(CNEOSpawnPoint*);
};
