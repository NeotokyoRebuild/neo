#include "cbase.h"
#include "neo_gamerules.h"
#include "in_buttons.h"
#include "ammodef.h"

#ifdef CLIENT_DLL
	#include "c_neo_player.h"
#else
	#include "neo_player.h"
	#include "team.h"
	#include "neo_model_manager.h"
	#include "neo_ghost_spawn_point.h"
	#include "neo_ghost_cap_point.h"
	#include "neo/weapons/weapon_ghost.h"
	#include "neo/weapons/weapon_neobasecombatweapon.h"
	#include "eventqueue.h"
	#include "mapentities.h"
	#include "hl2mp_gameinterface.h"
#endif

REGISTER_GAMERULES_CLASS( CNEORules );

BEGIN_NETWORK_TABLE_NOBASE( CNEORules, DT_NEORules )
// NEO TODO (Rain): NEO specific game modes var (CTG/TDM/...)
#ifdef CLIENT_DLL
	RecvPropFloat(RECVINFO(m_flNeoNextRoundStartTime)),
	RecvPropFloat(RECVINFO(m_flNeoRoundStartTime)),
#else
	SendPropFloat(SENDINFO(m_flNeoNextRoundStartTime)),
	SendPropFloat(SENDINFO(m_flNeoRoundStartTime)),
#endif
END_NETWORK_TABLE()

LINK_ENTITY_TO_CLASS( neo_gamerules, CNEOGameRulesProxy );
IMPLEMENT_NETWORKCLASS_ALIASED( NEOGameRulesProxy, DT_NEOGameRulesProxy );

extern void respawn(CBaseEntity *pEdict, bool fCopyCorpse);

// NEO TODO (Rain): These are about 1 unit accurate currently, should probably double check view heights are 100% correct.
// Also need to offset on class by class basis, these reflect the assault class.
static NEOViewVectors g_NEOViewVectors(
	Vector( 0, 0, 72 ),	   //VEC_VIEW (m_vView)
							  
	Vector(-16, -16, 0 ),	  //VEC_HULL_MIN (m_vHullMin)
	Vector( 16,  16,  73 ),	  //VEC_HULL_MAX (m_vHullMax)
							  					
	Vector(-16, -16, 0 ),	  //VEC_DUCK_HULL_MIN (m_vDuckHullMin)
	Vector( 16,  16,  48 ),	  //VEC_DUCK_HULL_MAX	(m_vDuckHullMax)
	Vector( 0, 0, 53 ),		  //VEC_DUCK_VIEW		(m_vDuckView)
							  					
	Vector(-10, -10, -10 ),	  //VEC_OBS_HULL_MIN	(m_vObsHullMin)
	Vector( 10,  10,  10 ),	  //VEC_OBS_HULL_MAX	(m_vObsHullMax)
							  					
	Vector( 0, 0, 14 ),		  //VEC_DEAD_VIEWHEIGHT (m_vDeadViewHeight)

	Vector(-16, -16, 0 ),	  //VEC_CROUCH_TRACE_MIN (m_vCrouchTraceMin)
	Vector( 16,  16,  60 )	  //VEC_CROUCH_TRACE_MAX (m_vCrouchTraceMax)
);

#ifdef CLIENT_DLL
	void RecvProxy_NEORules( const RecvProp *pProp, void **pOut,
		void *pData, int objectID )
	{
		CNEORules *pRules = NEORules();
		Assert( pRules );
		*pOut = pRules;
	}

	BEGIN_RECV_TABLE( CNEOGameRulesProxy, DT_NEOGameRulesProxy )
		RecvPropDataTable( "neo_gamerules_data", 0, 0,
			&REFERENCE_RECV_TABLE( DT_NEORules ),
			RecvProxy_NEORules )
	END_RECV_TABLE()
#else
	void *SendProxy_NEORules( const SendProp *pProp,
		const void *pStructBase, const void *pData,
		CSendProxyRecipients *pRecipients, int objectID )
	{
		CNEORules *pRules = NEORules();
		Assert( pRules );
		return pRules;
	}

	BEGIN_SEND_TABLE(CNEOGameRulesProxy, DT_NEOGameRulesProxy)
		SendPropDataTable("neo_gamerules_data", 0,
		&REFERENCE_SEND_TABLE(DT_NEORules),
		SendProxy_NEORules)
		END_SEND_TABLE()
#endif

		// NEO NOTE (Rain): These are copied over from hl2mp gamerules implementation.
		//
		// shared ammo definition
		// JAY: Trying to make a more physical bullet response
#define BULLET_MASS_GRAINS_TO_LB(grains)	(0.002285*(grains)/16.0f)
#define BULLET_MASS_GRAINS_TO_KG(grains)	lbs2kg(BULLET_MASS_GRAINS_TO_LB(grains))

		// exaggerate all of the forces, but use real numbers to keep them consistent
#define BULLET_IMPULSE_EXAGGERATION			3.5
		// convert a velocity in ft/sec and a mass in grains to an impulse in kg in/s
#define BULLET_IMPULSE(grains, ftpersec)	((ftpersec)*12*BULLET_MASS_GRAINS_TO_KG(grains)*BULLET_IMPULSE_EXAGGERATION)


ConVar neo_round_timelimit("neo_round_timelimit", "2.75", FCVAR_REPLICATED, "Neo round timelimit, in minutes.",
	true, 0.0f, false, 600.0f);

extern CBaseEntity *g_pLastJinraiSpawn, *g_pLastNSFSpawn;

static const char *s_NeoPreserveEnts[] =
{
	"neo_gamerules",
	"info_player_attacker",
	"info_player_defender",
	"info_player_start",
	"neo_predicted_viewmodel",
	"neo_ghost_retrieval_point",
	"neo_ghostspawnpoint",

	// HL2MP inherited below
	"ai_network",
	"ai_hint",
	"hl2mp_gamerules",
	"team_manager",
	"player_manager",
	"env_soundscape",
	"env_soundscape_proxy",
	"env_soundscape_triggerable",
	"env_sun",
	"env_wind",
	"env_fog_controller",
	"func_brush",
	"func_wall",
	"func_buyzone",
	"func_illusionary",
	"infodecal",
	"info_projecteddecal",
	"info_node",
	"info_target",
	"info_node_hint",
	"info_player_deathmatch",
	"info_player_combine",
	"info_player_rebel",
	"info_map_parameters",
	"keyframe_rope",
	"move_rope",
	"info_ladder",
	"player",
	"point_viewcontrol",
	"scene_manager",
	"shadow_control",
	"sky_camera",
	"soundent",
	"trigger_soundscape",
	"viewmodel",
	"predicted_viewmodel",
	"worldspawn",
	"point_devshot_camera",
	"", // END Marker
};

// Purpose: Empty legacy event for backwards compatibility
// with server plugins relying on this callback.
// https://wiki.alliedmods.net/Neotokyo_Events
static inline void FireLegacyEvent_NeoRoundStart()
{
	IGameEvent *neoLegacy = gameeventmanager->CreateEvent("game_round_start");
	if (neoLegacy)
	{
		gameeventmanager->FireEvent(neoLegacy);
	}
}

// Purpose: Empty legacy event for backwards compatibility
// with server plugins relying on this callback.
// https://wiki.alliedmods.net/Neotokyo_Events
static inline void FireLegacyEvent_NeoRoundEnd()
{
	IGameEvent *neoLegacy = gameeventmanager->CreateEvent("game_round_end");
	if (neoLegacy)
	{
		gameeventmanager->FireEvent(neoLegacy);
	}
}

CNEORules::CNEORules()
{
#ifdef GAME_DLL	
	Q_strncpy(g_Teams[TEAM_JINRAI]->m_szTeamname.GetForModify(),
		TEAM_STR_JINRAI, MAX_TEAM_NAME_LENGTH);
	
	Q_strncpy(g_Teams[TEAM_NSF]->m_szTeamname.GetForModify(),
		TEAM_STR_NSF, MAX_TEAM_NAME_LENGTH);
		
	for (int i = 1; i <= gpGlobals->maxClients; i++)
	{
		CBasePlayer *player = UTIL_PlayerByIndex(i);
		if (player)
		{
			g_Teams[TEAM_JINRAI]->UpdateClientData(player);
			g_Teams[TEAM_NSF]->UpdateClientData(player);
		}
	}
#endif

	m_flNeoRoundStartTime = m_flNeoNextRoundStartTime = 0;

	ListenForGameEvent("round_start");
}

CNEORules::~CNEORules()
{
}

#ifdef GAME_DLL
void CNEORules::Precache()
{
	BaseClass::Precache();
}
#endif

// This is the HL2MP gamerules GetAmmoDef() global scope function copied over,
// because we want to implement it ourselves. This can be refactored out if/when
// we don't want to support HL2MP guns anymore.
CAmmoDef *GetAmmoDef_HL2MP()
{
	static CAmmoDef def;
	static bool bInitted = false;

	if (!bInitted)
	{
		bInitted = true;

		def.AddAmmoType("AR2", DMG_BULLET, TRACER_LINE_AND_WHIZ, 0, 0, 60, BULLET_IMPULSE(200, 1225), 0);
		def.AddAmmoType("AR2AltFire", DMG_DISSOLVE, TRACER_NONE, 0, 0, 3, 0, 0);
		def.AddAmmoType("Pistol", DMG_BULLET, TRACER_LINE_AND_WHIZ, 0, 0, 150, BULLET_IMPULSE(200, 1225), 0);
		def.AddAmmoType("SMG1", DMG_BULLET, TRACER_LINE_AND_WHIZ, 0, 0, 225, BULLET_IMPULSE(200, 1225), 0);
		def.AddAmmoType("357", DMG_BULLET, TRACER_LINE_AND_WHIZ, 0, 0, 12, BULLET_IMPULSE(800, 5000), 0);
		def.AddAmmoType("XBowBolt", DMG_BULLET, TRACER_LINE, 0, 0, 10, BULLET_IMPULSE(800, 8000), 0);
		def.AddAmmoType("Buckshot", DMG_BULLET | DMG_BUCKSHOT, TRACER_LINE, 0, 0, 30, BULLET_IMPULSE(400, 1200), 0);
		def.AddAmmoType("RPG_Round", DMG_BURN, TRACER_NONE, 0, 0, 3, 0, 0);
		def.AddAmmoType("SMG1_Grenade", DMG_BURN, TRACER_NONE, 0, 0, 3, 0, 0);
		def.AddAmmoType("Grenade", DMG_BURN, TRACER_NONE, 0, 0, 5, 0, 0);
		def.AddAmmoType("slam", DMG_BURN, TRACER_NONE, 0, 0, 5, 0, 0);
	}

	return &def;
}

CAmmoDef *GetAmmoDef()
{
	static CAmmoDef *def;
	static bool bInitted = false;

	if (!bInitted)
	{
		bInitted = true;

		// HL2MP ammo support
		def = GetAmmoDef_HL2MP();

		// NEO ammo support
		def->AddAmmoType("AMMO_10G_SHELL", DMG_BULLET | DMG_BUCKSHOT,
			TRACER_LINE, 0, 0, 30, BULLET_IMPULSE(400, 1200), 0);
	}

	return def;
}

void CNEORules::ClientSpawned(edict_t* pPlayer)
{
#ifdef CLIENT_DLL
	C_NEO_Player *player = C_NEO_Player::GetLocalNEOPlayer();
	if (player)
	{
		player->m_bShowClassMenu = true;
	}
#endif
}

bool CNEORules::ShouldCollide(int collisionGroup0, int collisionGroup1)
{
	return BaseClass::ShouldCollide(collisionGroup0, collisionGroup1);
}

extern ConVar mp_chattime;

void CNEORules::Think(void)
{
	BaseClass::Think();

#ifdef GAME_DLL
	if (IsRoundOver())
	{
		// If the next round was not scheduled yet
		if (m_flNeoNextRoundStartTime == 0)
		{
			m_flNeoNextRoundStartTime = gpGlobals->curtime + mp_chattime.GetFloat();
			DevMsg("Round is over\n");
		}
		// Else if it's time to start the next round
		else if (gpGlobals->curtime >= m_flNeoNextRoundStartTime)
		{
			StartNextRound();
		}

		return;
	}
	// Note that exactly zero here means infinite round time.
	else if (GetRoundRemainingTime() < 0)
	{
		SetWinningTeam(TEAM_SPECTATOR, NEO_VICTORY_STALEMATE, false, false, true, false);
	}

	// Check if the ghost was capped during this Think
	int captorTeam, captorClient;
	for (int i = 0; i < m_pGhostCaps.Count(); i++)
	{
		auto pGhostCap = dynamic_cast<CNEOGhostCapturePoint*>(UTIL_EntityByIndex(m_pGhostCaps[i]));
		if (!pGhostCap)
		{
			Assert(false);
			continue;
		}

		// If a ghost was captured
		if (pGhostCap->IsGhostCaptured(captorTeam, captorClient))
		{
			// Turn off all capzones
			for (int i = 0; i < m_pGhostCaps.Count(); i++)
			{
				auto pGhostCap = dynamic_cast<CNEOGhostCapturePoint*>(UTIL_EntityByIndex(m_pGhostCaps[i]));
				pGhostCap->SetActive(false);
			}

			// And then announce team victory
			// NEO TODO (Rain): figure out the win reasons for Neo
			SetWinningTeam(captorTeam, 0, false, false, false, false);

			break;
		}
	}
#endif
}

// Return remaining time in seconds. Zero means there is no time limit.
float CNEORules::GetRoundRemainingTime()
{
	if (neo_round_timelimit.GetFloat() == 0)
	{
		return 0;
	}

	return (m_flNeoRoundStartTime + (neo_round_timelimit.GetFloat() * 60.0f)) - gpGlobals->curtime;
}

void CNEORules::FireGameEvent(IGameEvent* event)
{
	const char *type = event->GetName();

	if (Q_strcmp(type, "round_start") == 0)
	{
		m_flNeoRoundStartTime = gpGlobals->curtime;
		m_flNeoNextRoundStartTime = 0;
	}
}

#ifdef GAME_DLL
// Purpose: Spawns one ghost at a randomly chosen Neo ghost spawn point.
static inline void SpawnTheGhost()
{
	static int ghostEdict = -1;

	CBaseEntity *pEnt;

	CWeaponGhost *ghost = dynamic_cast<CWeaponGhost*>(UTIL_EntityByIndex(ghostEdict));

	bool spawnedGhostNow = false;

	// If we couldn't cast to ghost from existing edict
	if (!ghost)
	{
		pEnt = gEntList.FirstEnt();
		while (pEnt)
		{
			auto ghostTest = dynamic_cast<CWeaponGhost*>(pEnt);

			if (ghostTest)
			{
				ghost = ghostTest;
				break;
			}

			pEnt = gEntList.NextEnt(pEnt);
		}

		// If none of the entities were castable to a ghost
		if (!ghost)
		{
			ghost = dynamic_cast<CWeaponGhost*>(CreateEntityByName("weapon_ghost", -1));

			spawnedGhostNow = true;

			if (!ghost)
			{
				Warning("Failed to spawn a new ghost\n");
				Assert(false);

				return;
			}
		}
	}

	ghostEdict = ghost->edict()->m_EdictIndex;

	// Get the amount of ghost spawns available to us
	int numGhostSpawns = 0;

	pEnt = gEntList.FirstEnt();
	while (pEnt)
	{
		auto ghostSpawn = dynamic_cast<CNEOGhostSpawnPoint*>(pEnt);

		if (ghostSpawn)
		{
			numGhostSpawns++;
		}

		pEnt = gEntList.NextEnt(pEnt);
	}

	// We didn't have any spawns, spawn ghost at origin
	if (numGhostSpawns == 0)
	{
		Warning("No ghost spawns found! Spawning ghost at map origin, instead.\n");
		ghost->SetAbsOrigin(vec3_origin);
	}
	else
	{
		// Randomly decide on a ghost spawn point we want this time
		const int desiredSpawn = RandomInt(1, numGhostSpawns);
		int ghostSpawnIteration = 1;

		pEnt = gEntList.FirstEnt();
		// Second iteration, we pick the ghost spawn we want
		while (pEnt)
		{
			auto ghostSpawn = dynamic_cast<CNEOGhostSpawnPoint*>(pEnt);

			if (ghostSpawn)
			{
				if (ghostSpawnIteration++ == desiredSpawn)
				{
					ghost->SetAbsOrigin(ghostSpawn->GetAbsOrigin());
					break;
				}
			}

			pEnt = gEntList.NextEnt(pEnt);
		}
	}

	if (spawnedGhostNow)
	{
		DispatchSpawn(ghost);

		DevMsg("Spawned ghost at coords: %.1f %.1f %.1f\n",
			ghost->GetAbsOrigin().x,
			ghost->GetAbsOrigin().y,
			ghost->GetAbsOrigin().z);
	}
	else
	{
		DevMsg("Moved ghost to coords: %.1f %.1f %.1f\n",
			ghost->GetAbsOrigin().x,
			ghost->GetAbsOrigin().y,
			ghost->GetAbsOrigin().z);
	}
}

void CNEORules::StartNextRound()
{
	m_flNeoRoundStartTime = gpGlobals->curtime;
	m_flNeoNextRoundStartTime = 0;

	CleanUpMap();

	for (int i = 1; i <= gpGlobals->maxClients; i++)
	{
		CNEO_Player *pPlayer = (CNEO_Player*)UTIL_PlayerByIndex(i);

		if (!pPlayer)
		{
			continue;
		}

		if (pPlayer->GetActiveWeapon())
		{
			pPlayer->GetActiveWeapon()->Holster();
		}
		pPlayer->RemoveAllItems(true);
		respawn(pPlayer, false);
		pPlayer->Reset();

		pPlayer->SetTestMessageVisible(false);
	}

	m_flIntermissionEndTime = 0;
	m_flRestartGameTime = 0;
	m_bCompleteReset = false;

	IGameEvent * event = gameeventmanager->CreateEvent("round_start");
	if (event)
	{
		event->SetInt("fraglimit", 0);
		event->SetInt("priority", 6); // HLTV event priority, not transmitted

		event->SetString("objective", "DEATHMATCH");

		gameeventmanager->FireEvent(event);
	}

	FireLegacyEvent_NeoRoundEnd();
	FireLegacyEvent_NeoRoundStart();

	SpawnTheGhost();

	DevMsg("New round start here!\n");
}

bool CNEORules::IsRoundOver()
{
	// We don't want to start preparing for a new round
	// if the game has ended for the current map.
	if (g_fGameOver)
	{
		return false;
	}

	// Next round start has been scheduled, so current round must be over.
	if (m_flNeoNextRoundStartTime != 0)
	{
		return true;
	}

	return false;
}
#endif

void CNEORules::CreateStandardEntities(void)
{
	BaseClass::CreateStandardEntities();

#ifdef GAME_DLL
	g_pLastJinraiSpawn = NULL;
	g_pLastNSFSpawn = NULL;

#ifdef DBGFLAG_ASSERT
	CBaseEntity *pEnt =
#endif
		CBaseEntity::Create("neo_gamerules", vec3_origin, vec3_angle);
	Assert(pEnt);
#endif
}

int CNEORules::WeaponShouldRespawn(CBaseCombatWeapon *pWeapon)
{
	return GR_NONE;
}

const char *CNEORules::GetGameDescription(void)
{
	//DevMsg("Querying CNEORules game description\n");
	return BaseClass::GetGameDescription();
}

const CViewVectors *CNEORules::GetViewVectors() const
{
	return &g_NEOViewVectors;
}

const NEOViewVectors* CNEORules::GetNEOViewVectors() const
{
	return &g_NEOViewVectors;
}

float CNEORules::GetMapRemainingTime()
{
	return BaseClass::GetMapRemainingTime();
}

#ifndef CLIENT_DLL
static inline void RemoveGhosts()
{
	return;

	CBaseEntity *pEnt = gEntList.FirstEnt();
	while (pEnt)
	{
		auto ghost = dynamic_cast<CWeaponGhost*>(pEnt);

		if (ghost)
		{
			auto owner = ghost->GetPlayerOwner();

			if (owner && owner->GetActiveWeapon() &&
				(Q_strcmp(owner->GetActiveWeapon()->GetName(), ghost->GetName()) == 0))
			{
				owner->ClearActiveWeapon();
			}

			ghost->Delete();
		}

		pEnt = gEntList.NextEnt(pEnt);
	}
}

extern bool FindInList(const char **pStrings, const char *pToFind);

void CNEORules::CleanUpMap()

{
	// Recreate all the map entities from the map data (preserving their indices),
	// then remove everything else except the players.

	// Get rid of all entities except players.
	CBaseEntity *pCur = gEntList.FirstEnt();
	while (pCur)
	{
		CNEOBaseCombatWeapon *pWeapon = dynamic_cast<CNEOBaseCombatWeapon*>(pCur);
		if (pWeapon)
		{
			UTIL_Remove(pCur);
		}
		// remove entities that has to be restored on roundrestart (breakables etc)
		else if (!FindInList(s_NeoPreserveEnts, pCur->GetClassname()))
		{
			UTIL_Remove(pCur);
		}

		pCur = gEntList.NextEnt(pCur);
	}

	// Really remove the entities so we can have access to their slots below.
	gEntList.CleanupDeleteList();

	// Cancel all queued events to avoid any delayed inputs affecting nextround
	g_EventQueue.Clear();

	// Now reload the map entities.
	class CNEOMapEntityFilter : public IMapEntityFilter
	{
	public:
		virtual bool ShouldCreateEntity(const char *pClassname)
		{
			// Don't recreate the preserved entities.
			if (!FindInList(s_NeoPreserveEnts, pClassname))
			{
				return true;
			}
			else
			{
				// Increment our iterator since it's not going to call CreateNextEntity for this ent.
				if (m_iIterator != g_MapEntityRefs.InvalidIndex())
					m_iIterator = g_MapEntityRefs.Next(m_iIterator);

				return false;
			}
		}


		virtual CBaseEntity* CreateNextEntity(const char *pClassname)
		{
			if (m_iIterator == g_MapEntityRefs.InvalidIndex())
			{
				// This shouldn't be possible. When we loaded the map, it should have used 
				// CCSMapLoadEntityFilter, which should have built the g_MapEntityRefs list
				// with the same list of entities we're referring to here.
				Assert(false);
				return NULL;
			}
			else
			{
				CMapEntityRef &ref = g_MapEntityRefs[m_iIterator];
				m_iIterator = g_MapEntityRefs.Next(m_iIterator);	// Seek to the next entity.

				if (ref.m_iEdict == -1 || engine->PEntityOfEntIndex(ref.m_iEdict))
				{
					// Doh! The entity was delete and its slot was reused.
					// Just use any old edict slot. This case sucks because we lose the baseline.
					return CreateEntityByName(pClassname);
				}
				else
				{
					// Cool, the slot where this entity was is free again (most likely, the entity was 
					// freed above). Now create an entity with this specific index.
					return CreateEntityByName(pClassname, ref.m_iEdict);
				}
			}
		}

	public:
		int m_iIterator; // Iterator into g_MapEntityRefs.
	};
	CNEOMapEntityFilter filter;
	filter.m_iIterator = g_MapEntityRefs.Head();

	// DO NOT CALL SPAWN ON info_node ENTITIES!

	MapEntity_ParseAllEntities(engine->GetMapEntitiesString(), &filter, true);



	//RemoveGhosts();
	ResetGhostCapPoints();

	//BaseClass::CleanUpMap();
}

void CNEORules::CheckRestartGame()
{
	BaseClass::CheckRestartGame();
}

inline void CNEORules::ResetGhostCapPoints()
{
	m_pGhostCaps.Purge();

	int numGhostCaps = 0;

	CBaseEntity *pEnt = gEntList.FirstEnt();

	// First iteration, pre-size our dynamic array to avoid extra copies
	while (pEnt)
	{
		auto ghostCap = dynamic_cast<CNEOGhostCapturePoint*>(pEnt);

		if (ghostCap)
		{
			numGhostCaps++;
		}

		pEnt = gEntList.NextEnt(pEnt);
	}

	//m_pGhostCaps.SetSize(numGhostCaps);

	if (numGhostCaps > 0)
	{
		// Second iteration, populate with cap points
		pEnt = gEntList.FirstEnt();
		while (pEnt)
		{
			auto ghostCap = dynamic_cast<CNEOGhostCapturePoint*>(pEnt);

			if (ghostCap)
			{
				ghostCap->ResetCaptureState();
				m_pGhostCaps.AddToTail(ghostCap->entindex());
				ghostCap->SetActive(true);
				ghostCap->Think_CheckMyRadius();
			}

			pEnt = gEntList.NextEnt(pEnt);
		}
	}
}

void CNEORules::RestartGame()
{
	// bounds check
	if (mp_timelimit.GetInt() < 0)
	{
		mp_timelimit.SetValue(0);
	}
	m_flGameStartTime = gpGlobals->curtime;
	if (!IsFinite(m_flGameStartTime.Get()))
	{
		Warning("Trying to set a NaN game start time\n");
		m_flGameStartTime.GetForModify() = 0.0f;
	}

	CleanUpMap();

	// now respawn all players
	for (int i = 1; i <= gpGlobals->maxClients; i++)
	{
		CNEO_Player *pPlayer = (CNEO_Player*)UTIL_PlayerByIndex(i);

		if (!pPlayer)
			continue;

		if (pPlayer->GetActiveWeapon())
		{
			pPlayer->GetActiveWeapon()->Holster();
		}
		pPlayer->RemoveAllItems(true);
		respawn(pPlayer, false);
		pPlayer->Reset();

		pPlayer->SetTestMessageVisible(false);
	}

	// Respawn entities (glass, doors, etc..)

	CTeam *pJinrai = GetGlobalTeam(TEAM_JINRAI);
	CTeam *pNSF = GetGlobalTeam(TEAM_NSF);

	if (pJinrai)
	{
		pJinrai->SetScore(0);
	}

	if (pNSF)
	{
		pNSF->SetScore(0);
	}

	m_flIntermissionEndTime = 0;
	m_flRestartGameTime = 0.0;
	m_bCompleteReset = false;

	IGameEvent * event = gameeventmanager->CreateEvent("round_start");
	if (event)
	{
		event->SetInt("fraglimit", 0);
		event->SetInt("priority", 6); // HLTV event priority, not transmitted

		event->SetString("objective", "DEATHMATCH");

		gameeventmanager->FireEvent(event);
	}

	FireLegacyEvent_NeoRoundStart();

	SpawnTheGhost();
}
#endif

#ifdef GAME_DLL
bool CNEORules::ClientConnected(edict_t *pEntity, const char *pszName, const char *pszAddress, char *reject, int maxrejectlen)
{
	return BaseClass::ClientConnected(pEntity, pszName, pszAddress,
		reject, maxrejectlen);
#if(0)
	const bool canJoin = BaseClass::ClientConnected(pEntity, pszName, pszAddress,
		reject, maxrejectlen);

	if (canJoin)
	{
	}

	return canJoin;
#endif
}
#endif

void CNEORules::ClientSettingsChanged(CBasePlayer *pPlayer)
{
#ifndef CLIENT_DLL
	CNEO_Player *pNEOPlayer = ToNEOPlayer(pPlayer);

	if (!pNEOPlayer)
	{
		return;
	}

	CNEOModelManager *mm = CNEOModelManager::Instance();

	const char *pCurrentModel = modelinfo->GetModelName(pNEOPlayer->GetModel());
	const char *pTargetModel = mm->GetPlayerModel(
		(NeoSkin)pNEOPlayer->GetSkin(),
		(NeoClass)pNEOPlayer->GetClass(),
		pNEOPlayer->GetTeamNumber());

	if (V_stricmp(pCurrentModel, pTargetModel))
	{
		pNEOPlayer->SetPlayerTeamModel();
	}

	// We're skipping calling the base CHL2MPRules method here
	CTeamplayRules::ClientSettingsChanged(pPlayer);
#endif
}

#ifdef GAME_DLL
void CNEORules::SetWinningTeam(int team, int iWinReason, bool bForceMapReset, bool bSwitchTeams, bool bDontAddScore, bool bFinal)
{
	if (IsRoundOver())
	{
		return;
	}

	char victoryMsg[128];

	if (iWinReason == NEO_VICTORY_GHOST_CAPTURE)
	{
		V_sprintf_safe(victoryMsg, "Team %s wins by capturing the ghost!\n", (team == TEAM_JINRAI ? "Jinrai" : "NSF"));
	}
	else if (iWinReason == NEO_VICTORY_TEAM_ELIMINATION)
	{
		V_sprintf_safe(victoryMsg, "Team %s wins by eliminating the other team!\n", (team == TEAM_JINRAI ? "Jinrai" : "NSF"));
	}
	else if (iWinReason == NEO_VICTORY_TIMEOUT_WIN_BY_NUMBERS)
	{
		V_sprintf_safe(victoryMsg, "Team %s wins by numbers!\n", (team == TEAM_JINRAI ? "Jinrai" : "NSF"));
	}
	else if (iWinReason == NEO_VICTORY_FORFEIT)
	{
		V_sprintf_safe(victoryMsg, "Team %s wins by forfeit!\n", (team == TEAM_JINRAI ? "Jinrai" : "NSF"));
	}
	else if (iWinReason == NEO_VICTORY_STALEMATE)
	{
		V_sprintf_safe(victoryMsg, "TIE\n");
	}
	else
	{
		V_sprintf_safe(victoryMsg, "Unknown Neotokyo victory reason %i\n", iWinReason);
		Warning(victoryMsg);
		Assert(false);
	}

	for (int i = 1; i <= gpGlobals->maxClients; i++)
	{
		auto player = UTIL_PlayerByIndex(i);

		if (player)
		{
			engine->ClientPrintf(player->edict(), victoryMsg);

			if (team == TEAM_JINRAI)
			{
				player->EmitSound("HUD.JinraiWin");
			}
			else if (team == TEAM_NSF)
			{
				player->EmitSound("HUD.NSFWin");
			}
			else
			{
				player->EmitSound("HUD.Draw");
			}
		}
	}
	
	if (bForceMapReset)
	{
		RestartGame();
	}
	else
	{
		m_flNeoNextRoundStartTime = gpGlobals->curtime + mp_chattime.GetFloat();

		if (!bDontAddScore)
		{
			auto winningTeam = GetGlobalTeam(team);
			winningTeam->AddScore(1);
		}
	}
}
#endif