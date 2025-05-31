#include "neo_dm_spawn.h"

#include <dbg.h>
#include "cbase.h"
#include "inetchannelinfo.h"
#include "neo_player.h"
#include "neo_misc.h"
#include "util_shared.h"
#include "filesystem.h"
#include "utlbuffer.h"

#include "tier0/memdbgon.h"

static inline CUtlVector<DMSpawn::Info> gDMSpawnLocs;
static inline int gDMSpawnCur = 0;
static inline CUtlVector<bool> gDMSpawnUsed;
static inline int gDMSpawnUsedTally = 0;

namespace DMSpawn
{
bool HasDMSpawn()
{
	return !gDMSpawnLocs.IsEmpty();
}

Info GiveNextSpawn()
{
	if (gDMSpawnLocs.IsEmpty()) return Info{};

	// Try to give the next available spawn rather than just giving only a random number
	const int iSpawnTotal = gDMSpawnLocs.Size();
	if (gDMSpawnUsedTally >= iSpawnTotal)
	{
		gDMSpawnUsed.SetCount(iSpawnTotal);
		gDMSpawnUsedTally = 0;
	}

	// iChecked for sanity check so it never inf. loop
	static bool bCheckLeft = false; // Alternate check directions per spawning
	int iPickSpawn = RandomInt(0, iSpawnTotal - 1);
	for (int iChecked = 0; gDMSpawnUsed[iPickSpawn] && iChecked < iSpawnTotal; ++iChecked)
	{
		iPickSpawn = LoopAroundInArray(iPickSpawn + (bCheckLeft ? -1 : +1), iSpawnTotal);
	}
	bCheckLeft = !bCheckLeft;

	gDMSpawnUsed[iPickSpawn] = true;
	++gDMSpawnUsedTally;
	return gDMSpawnLocs[iPickSpawn];
}
}

static CNEO_Player *LoopbackPlayer()
{
	for (int i = 1; i <= gpGlobals->maxClients; ++i)
	{
		auto *pPlayer = static_cast<CNEO_Player *>(UTIL_PlayerByIndex(i));
		if (!pPlayer || pPlayer->IsBot())
		{
			continue;
		}
		INetChannelInfo *nci = engine->GetPlayerNetInfo(i);
		if (nci->IsLoopback())
		{
			return pPlayer;
		}
	}
	return nullptr;
}

static void DMSpawnComCallbackCreate()
{
	if (auto *player = LoopbackPlayer())
	{
		const Vector newLoc = player->GetAbsOrigin();
		const QAngle lookAngle = player->GetAbsAngles();
		bool bAlreadyApplied = false;
		for (const auto &spawn : gDMSpawnLocs)
		{
			if (spawn.pos == newLoc)
			{
				bAlreadyApplied = true;
				break;
			}
		}
		if (bAlreadyApplied)
		{
			Msg("DM Spawn already applied\n");
		}
		else
		{
			gDMSpawnLocs.AddToTail(DMSpawn::Info{newLoc, lookAngle.y});
			gDMSpawnUsed.AddToTail(false);
			Msg("DM Spawn: %.2f %.2f %.2f (%.2f) created\n", newLoc.x, newLoc.y, newLoc.z, lookAngle.y);
		}
	}
}

static void DMSpawnComCallbackRemoveAll()
{
	gDMSpawnLocs.RemoveAll();
	gDMSpawnUsed.RemoveAll();
}

static void DMSpawnComCallbackRemoveOne(const CCommand &command)
{
	static constexpr char USAGE_MSG[] = "Usage: sv_neo_dmspawn_removeone [spawn index]\n";
	if (command.ArgC() != 2)
	{
		Msg(USAGE_MSG);
		return;
	}
	const auto optIdx = StrToInt(command.Arg(1));
	if (!optIdx)
	{
		Msg(USAGE_MSG);
		return;
	}

	const int idx = *optIdx;
	if (!IN_BETWEEN_AR(0, idx, gDMSpawnLocs.Size()))
	{
		Msg("Error: Index %d is not within 0 to %d.\n", idx, gDMSpawnLocs.Size() - 1);
		return;
	}

	gDMSpawnLocs.Remove(idx);
	gDMSpawnUsed.Remove(idx);
	Msg("Removed spawn %d\n", idx);
}

static void DMSpawnComCallbackPrintLocs()
{
	for (int i = 0; i < gDMSpawnLocs.Size(); ++i)
	{
		const auto &spawn = gDMSpawnLocs[i];
		Msg("DM Spawn [%d]: %.2f %.2f %.2f (%.2f)\n", i, spawn.pos.x, spawn.pos.y, spawn.pos.z, spawn.lookY);
	}
}

static constexpr unsigned int MAGIC_NUMBER = 0x15AF73DA;
static constexpr int DMSPAWN_SERIAL_CURRENT = 1;

void DMSpawnComCallbackSave([[maybe_unused]] const CCommand &command)
{
	if (gDMSpawnLocs.IsEmpty())
	{
		return;
	}

	CUtlBuffer buf;
	buf.PutUnsignedInt(MAGIC_NUMBER);
	buf.PutInt(DMSPAWN_SERIAL_CURRENT);
	buf.PutInt(gDMSpawnLocs.Size());
	for (const auto &spawn : gDMSpawnLocs)
	{
		buf.PutFloat(spawn.pos.x);
		buf.PutFloat(spawn.pos.y);
		buf.PutFloat(spawn.pos.z);
		buf.PutFloat(spawn.lookY);
	}

	char szCurrentMapName[MAX_MAP_NAME + 1];
	V_strcpy_safe(szCurrentMapName, STRING(gpGlobals->mapname));
	filesystem->CreateDirHierarchy("maps/dm_locs");

	char szFName[512];
	V_sprintf_safe(szFName, "maps/dm_locs/%s.loc", szCurrentMapName);
	if (!filesystem->WriteFile(szFName, nullptr, buf))
	{
		Msg("Failed to write file: %s\n", szFName);
		return;
	}
	Msg("DMSpawn file saved: %s\n", szFName);
}

void DMSpawnComCallbackLoad([[maybe_unused]] const CCommand &command)
{
	gDMSpawnLocs.RemoveAll();

	char szCurrentMapName[MAX_MAP_NAME + 1];
	V_strcpy_safe(szCurrentMapName, STRING(gpGlobals->mapname));

	CUtlBuffer buf(0, 0, CUtlBuffer::READ_ONLY);
	char szFName[512];
	V_sprintf_safe(szFName, "maps/dm_locs/%s.loc", szCurrentMapName);
	if (!filesystem->ReadFile(szFName, nullptr, buf))
	{
		Msg("DMSpawn file not found: %s", szFName);
		return;
	}

	if (buf.GetUnsignedInt() != MAGIC_NUMBER)
	{
		return;
	}

	const int version = buf.GetInt();
	const int locsSize = buf.GetInt();
	for (int i = 0; i < locsSize && buf.IsValid(); ++i)
	{
		DMSpawn::Info spawn = {};
		spawn.pos.x = buf.GetFloat();
		spawn.pos.y = buf.GetFloat();
		spawn.pos.z = buf.GetFloat();
		spawn.lookY = buf.GetFloat();
		gDMSpawnLocs.AddToTail(spawn);
	}
	gDMSpawnUsed.SetCount(gDMSpawnLocs.Size());
	Msg("DMSpawn file loaded: %s", szFName);
}

static void DMSpawnComCallbackTeleportNext()
{
	if (gDMSpawnLocs.IsEmpty()) return;
	gDMSpawnCur = LoopAroundInArray(gDMSpawnCur + 1, gDMSpawnLocs.Size());
	if (auto *pPlayer = LoopbackPlayer())
	{
		const auto &spawn = gDMSpawnLocs[gDMSpawnCur];
		const QAngle spawnAngle{0, spawn.lookY, 0};
		pPlayer->SetAbsOrigin(spawn.pos);
		pPlayer->SetAbsAngles(spawnAngle);
		pPlayer->SnapEyeAngles(spawnAngle);
		Msg("Teleported to spawn %d: %.2f %.2f %.2f (%.2f)\n", gDMSpawnCur, spawn.pos.x, spawn.pos.y, spawn.pos.z, spawn.lookY);
	}
}

static void DMSpawnComCallbackMapinfo()
{
	char szCurrentMapName[MAX_MAP_NAME + 1];
	V_strcpy_safe(szCurrentMapName, STRING(gpGlobals->mapname));

	int iEntCount = 0;
	CBaseEntity *entDMSpawn = nullptr;
	while ((entDMSpawn = gEntList.FindEntityByClassname(entDMSpawn, "info_player_deathmatch")))
	{
		++iEntCount;
	}

	Msg("Deathmatch spawns for: %s\ndmspawn spawns count: %d\ninfo_player_deathmatch count: %d\nAllow deathmatch: %s",
		szCurrentMapName, gDMSpawnLocs.Size(), iEntCount, (iEntCount > 0 || !gDMSpawnLocs.IsEmpty()) ? "YES" : "NO");
}

ConCommand sv_neo_dmspawn_create("sv_neo_dmspawn_create", &DMSpawnComCallbackCreate, "DMSpawn - Create a new spawn", FCVAR_USERINFO | FCVAR_CHEAT);
ConCommand sv_neo_dmspawn_removeallspawns("sv_neo_dmspawn_removeallspawns", &DMSpawnComCallbackRemoveAll, "DMSpawn - Remove all spawns", FCVAR_USERINFO | FCVAR_CHEAT);
ConCommand sv_neo_dmspawn_removeone("sv_neo_dmspawn_removeone", &DMSpawnComCallbackRemoveOne, "DMSpawn - Remove one spawn by a given index", FCVAR_USERINFO | FCVAR_CHEAT);
ConCommand sv_neo_dmspawn_printlocs("sv_neo_dmspawn_printlocs", &DMSpawnComCallbackPrintLocs, "DMSpawn - Print locations and Y-angle of all spawns", FCVAR_USERINFO | FCVAR_CHEAT);
ConCommand sv_neo_dmspawn_save("sv_neo_dmspawn_save", &DMSpawnComCallbackSave, "DMSpawn - Save spawn file to filesystem", FCVAR_USERINFO | FCVAR_CHEAT);
ConCommand sv_neo_dmspawn_load("sv_neo_dmspawn_load", &DMSpawnComCallbackLoad, "DMSpawn - Load spawn file from filesystem", FCVAR_USERINFO | FCVAR_CHEAT);
ConCommand sv_neo_dmspawn_teleportnext("sv_neo_dmspawn_teleportnext", &DMSpawnComCallbackTeleportNext, "DMSpawn - Teleport to the next spawn", FCVAR_USERINFO | FCVAR_CHEAT);
ConCommand sv_neo_dmspawn_mapinfo("sv_neo_dmspawn_mapinfo", &DMSpawnComCallbackMapinfo, "DMSpawn - Map deathmatch spawns check and information", FCVAR_USERINFO);
