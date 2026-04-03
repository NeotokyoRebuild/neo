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
	CNEOSpawnPoint* RequestSpawn(int team, CBasePlayer* player);
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

	CNEOSpawnPoint* RequestSpawn(int team, CBasePlayer* player)
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

		CNEOSpawnPoint* backup = nullptr;
		auto idx = manager.m_spawns.FindPredicate(
			[rules, team, player, &backup](const auto& spawn)->bool
			{
				if (!spawn.handle || !spawn.handle.IsValid())
				{
					Assert(false);
					return false;
				}

				if (team != spawn.handle.Get()->GetOwningTeam())
					return false;

				// We know this spawn is valid and belongs to our team.
				// Save it as backup, just in case we can't find a good fresh spawn.
				backup = spawn.handle;

				if (spawn.isUsed)
					return false;

				if (!rules->IsSpawnPointValid(spawn.handle, player))
					return false;

				return true;
			});

		if (idx == manager.m_spawns.InvalidIndex())
		{
			// If we didn't find any free capzones, at least return an overlapping one.
			// This can happen for maps with too few capzones for the amount of (re)spawns occurring.
			// Can be nullptr if we got >0 spawns but none of them were considered valid,
			// in which case it's up to the caller to handle.
			return backup;
		}

		manager.m_spawns[idx].isUsed = true;
		return manager.m_spawns[idx].handle;
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
