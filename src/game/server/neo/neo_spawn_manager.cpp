#include "neo_spawn_manager.h"

#include <GameEventListener.h>
#include <ehandle.h>
#include <utlvector.h>

#include "neo_gamerules.h"
#include "neo_player_spawnpoint.h"

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

struct SpawnInfo
{
	CHandle<CNEOSpawnPoint> handle;
	bool isUsed; // Whether this spawn point has been spawned into for the current round.
};

class CBasePlayer;

class CNEO_SpawnManager : public CGameEventListener
{
public:
	CNEO_SpawnManager();
	CNEOSpawnPoint* RequestSpawn(int team, CNEO_Player *player);
	virtual void FireGameEvent(IGameEvent* event) override final;

	CUtlVector<SpawnInfo> m_spawns;
} manager;

CNEO_SpawnManager::CNEO_SpawnManager()
{
	m_spawns.EnsureCapacity(MAX_PLAYERS);
}

void CNEO_SpawnManager::FireGameEvent(IGameEvent* event)
{
	Assert(!V_strcmp("round_start", event->GetName()));
	for (int i = 0; i < manager.m_spawns.Count(); ++i)
	{
		auto& spawn = manager.m_spawns[i];
		if (!spawn.handle || !spawn.handle.IsValid())
			manager.m_spawns.Remove(i--);
		else
			spawn.isUsed = false;
	}
	manager.m_spawns.Shuffle();
}

namespace NeoSpawnManager
{
	void Init()
	{
		manager.ListenForGameEvent("round_start");
	}

	void Deinit()
	{
		manager.StopListeningForAllEvents();
	}

	CNEOSpawnPoint* RequestSpawn(int team, CNEO_Player *player)
	{
		// Nothing we can do to salvage this... This will fall back
		// to spawning at info_player_start or related logic in the caller.
		if (manager.m_spawns.IsEmpty())
			return nullptr;

		auto rules = NEORules();
		if (!rules)
		{
			Assert(false);
			return nullptr;
		}

		if (!rules->IsTeamplay())
		{
			// Random selection every time for non-teamplay
			manager.m_spawns.Shuffle();
		}

		CNEOSpawnPoint* backup = nullptr;

		bool bRestoreSpawn = (player->m_iNextRestore.flags & NEXT_ROUND_PLAYER_RESTORE_FLAG_SPAWN
				&& player->m_iNextRestore.iSpawnEntIdx >= 0);

		auto FindSpawn = [rules, team, player, &backup, bRestoreSpawn](const auto& spawn)->bool
			{
				if (!spawn.handle || !spawn.handle.IsValid())
				{
					Assert(false);
					return false;
				}

				const int spawnTeam = spawn.handle.Get()->GetOwningTeam();
				if (spawnTeam == TEAM_ANY)
				{
					if (team <= LAST_SHARED_TEAM)
						return false;
				}
				else if (team != spawnTeam)
				{
					return false;
				}

				// We know this spawn is valid and belongs to our team.
				// Save it as backup, just in case we can't find a good fresh spawn.
				backup = spawn.handle;

				if (spawn.isUsed)
					return false;

				if (!rules->IsSpawnPointValid(spawn.handle, player))
					return false;

				if (bRestoreSpawn)
				{
					return (spawn.handle->entindex() == player->m_iNextRestore.iSpawnEntIdx);
				}
				else
				{
					return true;
				}
			};

		int idx = manager.m_spawns.FindPredicate(FindSpawn);

		// Try again if it's restoring spawn but cannot find it
		if (bRestoreSpawn && idx == manager.m_spawns.InvalidIndex())
		{
			bRestoreSpawn = false;
			idx = manager.m_spawns.FindPredicate(FindSpawn);
		}

		player->m_iSpawnEntIdx = -1;

		if (idx == manager.m_spawns.InvalidIndex())
		{
			// If we didn't find any free capzones, at least return an overlapping one.
			// This can happen for maps with too few capzones for the amount of (re)spawns occurring.
			// Can be nullptr if we got >0 spawns but none of them were considered valid,
			// in which case it's up to the caller to handle.
			return backup;
		}

		if (!rules->CanRespawnAnyTime())
		{
			// We only care if it's been used before or not if there are no respawns
			manager.m_spawns[idx].isUsed = true;
		}

		auto handle = manager.m_spawns[idx].handle;
		player->m_iSpawnEntIdx = handle->entindex();
		return handle;
	}

	void Register(CNEOSpawnPoint* spawn)
	{
		manager.m_spawns.AddToTail({
			.handle{spawn},
			.isUsed{false}});
	}
	void Unregister(CNEOSpawnPoint* spawn)
	{
		auto idx = manager.m_spawns.FindPredicate([spawn](const auto& s)->bool
			{
				return spawn == s.handle;
			});
		if (idx != manager.m_spawns.InvalidIndex())
			manager.m_spawns.Remove(idx);
	}
}
