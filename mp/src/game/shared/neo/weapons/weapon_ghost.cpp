#include "cbase.h"
#include "weapon_ghost.h"
#include "neo_gamerules.h"

#ifdef CLIENT_DLL
#include <engine/ivdebugoverlay.h>
#include <engine/IEngineSound.h>
#include "filesystem.h"
#include "ui/neo_hud_ghostbeacon.h"
#endif

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

enum {
	GHOST_SOUND_EQUIP = 0,
	GHOST_SOUND_EQUIP2,
	GHOST_SOUND_EQUIP3,
	GHOST_SOUND_EQUIP4,
	GHOST_SOUND_EQUIP5,
	GHOST_SOUND_IDLE_LOOP,
	GHOST_SOUND_PICKUP,
	GHOST_SOUND_PING,
	NUM_GHOST_SOUNDS
};

const char *ghostSoundFiles[NUM_GHOST_SOUNDS] = {
	"gameplay/ghost_equip.wav",
	"gameplay/ghost_equip2.wav",
	"gameplay/ghost_equip3.wav",
	"gameplay/ghost_equip4.wav",
	"gameplay/ghost_equip5.wav",
	"gameplay/ghost_idle_loop.wav",
	"gameplay/ghost_pickup.wav",
	"gameplay/ghost_ping.wav",
};

IMPLEMENT_NETWORKCLASS_ALIASED(WeaponGhost, DT_WeaponGhost)

BEGIN_NETWORK_TABLE(CWeaponGhost, DT_WeaponGhost)
#ifdef CLIENT_DLL
RecvPropBool(RECVINFO(m_bShouldShowEnemies)),
RecvPropArray(RecvPropVector(RECVINFO(m_rvPlayerPositions[0])), m_rvPlayerPositions),
#else
SendPropBool(SENDINFO(m_bShouldShowEnemies)),
SendPropArray(SendPropVector(SENDINFO_ARRAY(m_rvPlayerPositions), -1, SPROP_COORD_MP_LOWPRECISION | SPROP_CHANGES_OFTEN, MIN_COORD_FLOAT, MAX_COORD_FLOAT), m_rvPlayerPositions),
#endif
END_NETWORK_TABLE()

#ifdef CLIENT_DLL
BEGIN_PREDICTION_DATA(CWeaponGhost)
DEFINE_PRED_FIELD(m_rvPlayerPositions, FIELD_VECTOR, FTYPEDESC_INSENDTABLE),
END_PREDICTION_DATA()
#endif

LINK_ENTITY_TO_CLASS(weapon_ghost, CWeaponGhost);
PRECACHE_WEAPON_REGISTER(weapon_ghost);

#ifdef GAME_DLL
// NEO TODO (Rain): make these sensible
acttable_t CWeaponGhost::m_acttable[] =
{
	{ ACT_IDLE,				ACT_IDLE_PISTOL,				false },
	{ ACT_RUN,				ACT_RUN_PISTOL,					false },
	{ ACT_CROUCHIDLE,		ACT_HL2MP_IDLE_CROUCH_PISTOL,	false },
	{ ACT_WALK_CROUCH,		ACT_HL2MP_WALK_CROUCH_PISTOL,	false },
	{ ACT_RANGE_ATTACK1,	ACT_RANGE_ATTACK_PISTOL,		false },
	{ ACT_RELOAD,			ACT_RELOAD_PISTOL,				false },
	{ ACT_JUMP,				ACT_HL2MP_JUMP_PISTOL,			false },
};
IMPLEMENT_ACTTABLE(CWeaponGhost);
#endif

CWeaponGhost::CWeaponGhost(void)
{
#ifdef CLIENT_DLL
	m_bHavePlayedGhostEquipSound = false;
	m_bHaveHolsteredTheGhost = false;

	for (int i = 0; i < ARRAYSIZE(m_pGhostBeacons); i++)
	{
		m_pGhostBeacons[i] = new CNEOHud_GhostBeacon("ghostBeacon");
	}
#endif

	// This is just always on for now.
	// Not sure if there's a reason to ever disable ghosting,
	// but might as well have the option.
	SetShowEnemies(true);

	ZeroGhostedPlayerLocArray();
}

#ifdef CLIENT_DLL
CWeaponGhost::~C_WeaponGhost(void)
{
	for (int i = 0; i < ARRAYSIZE(m_pGhostBeacons); i++)
	{
		if (m_pGhostBeacons[i])
		{
			delete m_pGhostBeacons[i];
		}
	}
}
#endif

inline void CWeaponGhost::ZeroGhostedPlayerLocArray(void)
{
#ifdef CLIENT_DLL
	for (int i = 0; i < m_rvPlayerPositions->Length(); i++)
	{
		m_rvPlayerPositions[i].Zero();
	}
#else

	for (int i = 0; i < m_rvPlayerPositions.Count(); i++)
	{
		m_rvPlayerPositions.Set(i, Vector(0, 0, 0));
	}
	NetworkStateChanged();
#endif
}

void CWeaponGhost::ItemPreFrame(void)
{
	// NEO TODO (Rain): it's probably acceptable to move this scope to Tick for perf
	{
		// Only show enemies if we are ghosting
		if (m_bShouldShowEnemies)
		{
#ifdef CLIENT_DLL
			ShowEnemies();
			HandleGhostEquip();
#else
			// We only need to update this while someone is ghosting
			UpdateNetworkedEnemyLocations();
#endif
		}
	}
}

#ifdef CLIENT_DLL
inline void CWeaponGhost::HandleGhostEquip(void)
{
	if (!m_bHavePlayedGhostEquipSound)
	{
		PlayGhostSound();
		m_bHavePlayedGhostEquipSound = true;
		m_bHaveHolsteredTheGhost = false;
	}
}

inline void CWeaponGhost::HandleGhostUnequip(void)
{
	if (!m_bHaveHolsteredTheGhost)
	{
		HideEnemies();
		StopGhostSound();
		m_bHaveHolsteredTheGhost = true;
		m_bHavePlayedGhostEquipSound = false;
	}
}

// Consider calling HandleGhostEquip instead.
inline void CWeaponGhost::PlayGhostSound(float volume)
{
	const int randomEquipSoundIndex = RandomInt(GHOST_SOUND_EQUIP, GHOST_SOUND_EQUIP5);
	enginesound->EmitAmbientSound(ghostSoundFiles[randomEquipSoundIndex], volume);
}

// Consider calling HandleGhostUnequip instead.
inline void CWeaponGhost::StopGhostSound(void)
{
	CUtlVector<SndInfo_t> sounds;
	enginesound->GetActiveSounds(sounds);

	char filename[MAX_PATH];
	for (int i = 0; i < sounds.Size(); i++)
	{
		if (!g_pFullFileSystem->String(sounds[i].m_filenameHandle,
			filename, sizeof(filename)))
		{
			continue;
		}

		if (!*filename)
		{
			continue;
		}

		// NEO HACK/TODO (Rain): track sound guid so we can avoid expensive string compare
#ifdef _WIN32
		if (Q_stricmp(filename, "gameplay\\ghost_equip") != -1)
#elif defined(LINUX)
		if (Q_stricmp(filename, "gameplay/ghost_equip") != -1)
#else
#error Unimplemented
#endif
		{
			enginesound->StopSoundByGuid(sounds[i].m_nGuid);
			break;
		}
	}
}
#endif

void CWeaponGhost::Equip(CBaseCombatCharacter *pOwner)
{
	BaseClass::Equip(pOwner);
}

void CWeaponGhost::ItemHolsterFrame(void)
{
	BaseClass::ItemHolsterFrame();

#ifdef CLIENT_DLL
	HandleGhostUnequip(); // is there some callback, so we don't have to keep calling this?
#endif
}

void CWeaponGhost::PrimaryAttack(void)
{
}

void CWeaponGhost::SetShowEnemies(bool enabled)
{
#ifdef GAME_DLL
	m_bShouldShowEnemies.GetForModify() = enabled;
#else
	m_bShouldShowEnemies = enabled;
#endif
}

enum {
	NEO_GHOST_ONLY_ENEMIES = 0,
	NEO_GHOST_ONLY_PLAYABLE_TEAMS,
	NEO_GHOST_ANY_TEAMS
};
ConVar neo_ghost_debug_ignore_teams("neo_ghost_debug_ignore_teams", "2", FCVAR_CHEAT | FCVAR_REPLICATED,
	"Debug ghost team filter. If 0, only ghost the enemy team. If 1, ghost both playable teams. If 2, ghost any team, including spectator and unassigned.", true, 0.0, true, 2.0);
ConVar neo_ghost_debug_spew("neo_ghost_debug_spew", "0", FCVAR_CHEAT | FCVAR_REPLICATED,
	"Whether to spew debug logs to console about ghosting positions and the data PVS/server origin.", true, 0.0, true, 1.0);
ConVar neo_ghost_debug_hudinfo("neo_ghost_debug_hudinfo", "1", FCVAR_CHEAT | FCVAR_REPLICATED,
	"Whether to overlay debug text information to screen about ghosting targets.", true, 0.0, true, 1.0);

#ifdef CLIENT_DLL
void CWeaponGhost::HideEnemies(void)
{
	for (int i = 1; i <= gpGlobals->maxClients; i++)
	{
		HideBeacon(i);
	}
}

// Purpose: Iterate through all enemies and give ghoster their position info,
// either via client's own PVS information or networked by the server when needed.
void CWeaponGhost::ShowEnemies(void)
{
	C_NEO_Player *player = (C_NEO_Player*)GetOwner();
	if (!player)
	{
		return;
	}

	for (int i = 1; i <= gpGlobals->maxClients; i++)
	{
		HideBeacon(i);

		auto otherPlayer = ToBasePlayer( ClientEntityList().GetEnt( i ) );

		// Only ghost valid clients that aren't ourselves
		if (!otherPlayer || otherPlayer == player)
		{
			continue;
		}

		if (otherPlayer->GetTeamNumber() != TEAM_JINRAI && otherPlayer->GetTeamNumber() != TEAM_NSF)
		{
			// We don't want to ghost spectators or unassigned players
			if (neo_ghost_debug_ignore_teams.GetInt() < NEO_GHOST_ANY_TEAMS)
			{
				continue;
			}
		}

		if (player->GetTeamNumber() == otherPlayer->GetTeamNumber())
		{
			// We don't want to ghost our own team
			if (neo_ghost_debug_ignore_teams.GetInt() < NEO_GHOST_ONLY_PLAYABLE_TEAMS)
			{
				continue;
			}
		}

		const bool isInPVS = otherPlayer->IsVisible();

		// If it's in my PVS already
		if (isInPVS)
		{
			ShowBeacon(i, otherPlayer->GetAbsOrigin());

			if (neo_ghost_debug_spew.GetBool())
			{
				DevMsg("Ghosting enemy from my PVS: %f %f %f\n",
					otherPlayer->GetAbsOrigin().x,
					otherPlayer->GetAbsOrigin().y,
					otherPlayer->GetAbsOrigin().z);
			}
			
			if (neo_ghost_debug_hudinfo.GetBool())
			{
				Debug_ShowPos(otherPlayer->GetAbsOrigin(), isInPVS);
			}
		}
		// Else, the server will provide us with this enemy's position info
		else
		{
			ShowBeacon(i, m_rvPlayerPositions[i]);

			if (neo_ghost_debug_spew.GetBool())
			{
				DevMsg("Ghosting enemy from server pos: %f %f %f\n",
					m_rvPlayerPositions[i].x,
					m_rvPlayerPositions[i].y,
					m_rvPlayerPositions[i].z);
			}
			
			if (neo_ghost_debug_hudinfo.GetBool())
			{
				Debug_ShowPos(m_rvPlayerPositions[i], isInPVS);
			}
		}
	}
}

inline void CWeaponGhost::HideBeacon(int clientIndex)
{
	m_pGhostBeacons[clientIndex]->SetVisible(false);
}

using vgui::surface;

extern ConVar neo_ghost_beacon_scale_baseline;

inline void CWeaponGhost::ShowBeacon(int clientIndex, const Vector &pos)
{
	if (!m_pGhostBeacons[clientIndex])
	{
		Assert(false);
		return;
	}

	Vector dir = GetOwner()->EyePosition() - pos;

	// NEO TODO (Rain): make server authoritative cvar in shared code
	const float maxGhostRangeMeters = 45.0f;

	const float distance = dir.Length2D();
	// NEO TODO (Rain): Make sure this conversion is actually accurate
	const float distMeters = (distance / METERS_PER_INCH) / 1000.0f;

	// Server will never give us info beyond the ghost range,
	// so this only happens if there was a target in PVS beyond range.
	// (NEO FIXME (Rain): actually it does, there is no check in place yet)
	if (distMeters > maxGhostRangeMeters)
	{
		return;
	}

	const float maxDistInHammerUnits = (maxGhostRangeMeters / METERS_PER_INCH);

	Assert(maxDistInHammerUnits > 0);

	const float scaling = clamp((distance / maxDistInHammerUnits),
		0.25f * neo_ghost_beacon_scale_baseline.GetFloat(),
		0.75f * neo_ghost_beacon_scale_baseline.GetFloat());
	
#if(0)
	DevMsg("Dist was: %.1f meters; beacon texture scaling: %f\n",
		distMeters, scaling);
#endif

	const int heightOffset = 32;
	Vector temp(pos.x, pos.y, pos.z + heightOffset);
	int x, y;
	GetVectorInScreenSpace(temp, x, y); // this is pixels from top-left

	m_pGhostBeacons[clientIndex]->SetGhostTargetPos(x, y, scaling, distMeters);
	m_pGhostBeacons[clientIndex]->SetVisible(true);
}

void CWeaponGhost::Debug_ShowPos(const Vector &pos, bool pvs)
{
	int x, y;
	GetVectorInScreenSpace(pos, x, y);

	// Whether target originated from client PVS or was sent by server
	if (pvs)
	{
		debugoverlay->AddTextOverlay(pos, gpGlobals->frametime, "GHOST TARGET (PVS)");
	}
	else
	{
		debugoverlay->AddTextOverlay(pos, gpGlobals->frametime, "GHOST TARGET (SERVER)");
	}
}
#endif

#ifdef GAME_DLL
// Purpose: Send enemy player locations to clients for ghost usage outside their PVS.
//
// NEO TODO/FIXME (Rain): This stuff will get networked once per ghost;
// this can be inefficient in the unlikely event of multiple ghosts at play at once.
void CWeaponGhost::UpdateNetworkedEnemyLocations(void)
{
	// FIXME/HACK: we never deallocate, move this to class member variable
	const int pvsMaxSize = (engine->GetClusterCount() / 8);
	Assert(pvsMaxSize > 0);
	static unsigned char *pvs = new unsigned char[pvsMaxSize];

	CNEO_Player *player = (CNEO_Player*)GetOwner();
	if (!player)
	{
		return;
	}

	const int cluster = engine->GetClusterForOrigin(player->GetAbsOrigin());
	const int pvsSize = engine->GetPVSForCluster(cluster, pvsMaxSize, pvs);
	Assert(pvsSize > 0);

	for (int i = 1; i <= gpGlobals->maxClients; i++)
	{
		CNEO_Player *otherPlayer = (CNEO_Player*)UTIL_PlayerByIndex(i);

		// We're only interested in valid players that aren't the ghost owner.
		if (!otherPlayer || otherPlayer == player)
		{
			continue;
		}

		// If the other player is already in ghoster's PVS, we can skip them.
		else if (engine->CheckOriginInPVS(otherPlayer->GetAbsOrigin(), pvs, pvsSize))
		{
			continue;
		}

		vec_t absPos[3] = { otherPlayer->GetAbsOrigin().x, otherPlayer->GetAbsOrigin().y, otherPlayer->GetAbsOrigin().z };

		m_rvPlayerPositions.Set(i, otherPlayer->GetAbsOrigin());
		m_rvPlayerPositions.GetForModify(i).CopyToArray(absPos);
	}
}
#endif
