#include "cbase.h"
#include "neo_gamerules.h"
#include "in_buttons.h"
#include "ammodef.h"

#include "takedamageinfo.h"

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
	#include "player_resource.h"
#endif

ConVar mp_neo_preround_freeze_time("mp_neo_preround_freeze_time", "10", FCVAR_REPLICATED, "The pre-round freeze time, in seconds.", true, 0.0, false, 0);
ConVar mp_neo_latespawn_max_time("mp_neo_latespawn_max_time", "15", FCVAR_REPLICATED, "How many seconds late are players still allowed to spawn.", true, 0.0, false, 0);

REGISTER_GAMERULES_CLASS( CNEORules );

BEGIN_NETWORK_TABLE_NOBASE( CNEORules, DT_NEORules )
// NEO TODO (Rain): NEO specific game modes var (CTG/TDM/...)
#ifdef CLIENT_DLL
	RecvPropFloat(RECVINFO(m_flNeoNextRoundStartTime)),
	RecvPropFloat(RECVINFO(m_flNeoRoundStartTime)),
	RecvPropInt(RECVINFO(m_nRoundStatus)),
	RecvPropInt(RECVINFO(m_iRoundNumber)),
#else
	SendPropFloat(SENDINFO(m_flNeoNextRoundStartTime)),
	SendPropFloat(SENDINFO(m_flNeoRoundStartTime)),
	SendPropInt(SENDINFO(m_nRoundStatus)),
	SendPropInt(SENDINFO(m_iRoundNumber)),
#endif
END_NETWORK_TABLE()

LINK_ENTITY_TO_CLASS( neo_gamerules, CNEOGameRulesProxy );
IMPLEMENT_NETWORKCLASS_ALIASED( NEOGameRulesProxy, DT_NEOGameRulesProxy );

extern void respawn(CBaseEntity *pEdict, bool fCopyCorpse);

// NEO TODO (Rain): check against a test map
static NEOViewVectors g_NEOViewVectors(
	Vector( 0, 0, 58 ),	   //VEC_VIEW (m_vView) // 57 == vanilla recon, 58 == vanilla assault (default), 60 == vanilla support. Use the shareddefs.h macro VEC_VIEW_NEOSCALE to access per player.
							  
	Vector(-16, -16, 0 ),	  //VEC_HULL_MIN (m_vHullMin)
	Vector(16, 16, NEO_ASSAULT_PLAYERMODEL_HEIGHT),	  //VEC_HULL_MAX (m_vHullMax). 66 == vanilla recon, 67 == vanilla assault (default), 72 == vanilla support. Use relevant VEC_... macros in shareddefs for class height adjusted per player access.
							  					
	Vector(-16, -16, 0 ),	  //VEC_DUCK_HULL_MIN (m_vDuckHullMin)
	Vector( 16,  16, NEO_ASSAULT_PLAYERMODEL_DUCK_HEIGHT),	  //VEC_DUCK_HULL_MAX	(m_vDuckHullMax)
	Vector( 0, 0, 45 ),		  //VEC_DUCK_VIEW		(m_vDuckView)
							  					
	Vector(-10, -10, -10 ),	  //VEC_OBS_HULL_MIN	(m_vObsHullMin)
	Vector( 10,  10,  10 ),	  //VEC_OBS_HULL_MAX	(m_vObsHullMax)
							  					
	Vector( 0, 0, 14 ),		  //VEC_DEAD_VIEWHEIGHT (m_vDeadViewHeight)

	Vector(-16, -16, 0 ),	  //VEC_CROUCH_TRACE_MIN (m_vCrouchTraceMin)
	Vector(16, 16, 60)	  //VEC_CROUCH_TRACE_MAX (m_vCrouchTraceMax)
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


ConVar neo_score_limit("neo_score_limit", "7", FCVAR_REPLICATED, "Neo score limit.", true, 0.0f, true, 99.0f);

ConVar neo_round_timelimit("neo_round_timelimit", "2.75", FCVAR_REPLICATED, "Neo round timelimit, in minutes.",
	true, 0.0f, false, 600.0f);

ConVar neo_sv_ignore_wep_xp_limit("neo_sv_ignore_wep_xp_limit", "0", FCVAR_CHEAT | FCVAR_REPLICATED, "If true, allow equipping any loadout regardless of player XP.",
	true, 0.0f, true, 1.0f);

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
#if(0) // NEO TODO (Rain): unimplemented
	IGameEvent *neoLegacy = gameeventmanager->CreateEvent("game_round_start");
	if (neoLegacy)
	{
		gameeventmanager->FireEvent(neoLegacy);
	}
	else
	{
		Assert(neoLegacy);
	}
#endif
}

// Purpose: Empty legacy event for backwards compatibility
// with server plugins relying on this callback.
// https://wiki.alliedmods.net/Neotokyo_Events
static inline void FireLegacyEvent_NeoRoundEnd()
{
#if(0) // NEO TODO (Rain): unimplemented
	IGameEvent *neoLegacy = gameeventmanager->CreateEvent("game_round_end");
	if (neoLegacy)
	{
		gameeventmanager->FireEvent(neoLegacy);
	}
	else
	{
		Assert(neoLegacy);
	}
#endif
}

CNEORules::CNEORules()
{
#ifdef GAME_DLL
	m_bNextClientIsFakeClient = false;

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

	ResetGhostCapPoints();
#endif

	m_flNeoRoundStartTime = m_flNeoNextRoundStartTime = 0;
	SetRoundStatus(NeoRoundStatus::Idle);
	m_iRoundNumber = 0;

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

ConVar	sk_max_neo_ammo("sk_max_neo_ammo", "10000", FCVAR_REPLICATED);
ConVar	sk_plr_dmg_neo("sk_plr_dmg_neo", "0", FCVAR_REPLICATED);
ConVar	sk_npc_dmg_neo("sk_npc_dmg_neo", "0", FCVAR_REPLICATED);

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

		def.AddAmmoType("AR2", DMG_BULLET, TRACER_LINE_AND_WHIZ, sk_plr_dmg_neo.GetName(), sk_npc_dmg_neo.GetName(), sk_max_neo_ammo.GetName(), BULLET_IMPULSE(200, 1225), 0);
		def.AddAmmoType("AR2AltFire", DMG_DISSOLVE, TRACER_NONE, sk_plr_dmg_neo.GetName(), sk_npc_dmg_neo.GetName(), sk_max_neo_ammo.GetName(), 0, 0);
		def.AddAmmoType("Pistol", DMG_BULLET, TRACER_LINE_AND_WHIZ, sk_plr_dmg_neo.GetName(), sk_npc_dmg_neo.GetName(), sk_max_neo_ammo.GetName(), BULLET_IMPULSE(200, 1225), 0);
		def.AddAmmoType("SMG1", DMG_BULLET, TRACER_LINE_AND_WHIZ, sk_plr_dmg_neo.GetName(), sk_npc_dmg_neo.GetName(), sk_max_neo_ammo.GetName(), BULLET_IMPULSE(200, 1225), 0);
		def.AddAmmoType("357", DMG_BULLET, TRACER_LINE_AND_WHIZ, sk_plr_dmg_neo.GetName(), sk_npc_dmg_neo.GetName(), sk_max_neo_ammo.GetName(), BULLET_IMPULSE(800, 5000), 0);
		def.AddAmmoType("XBowBolt", DMG_BULLET, TRACER_LINE, sk_plr_dmg_neo.GetName(), sk_npc_dmg_neo.GetName(), sk_max_neo_ammo.GetName(), BULLET_IMPULSE(800, 8000), 0);
		def.AddAmmoType("Buckshot", DMG_BULLET | DMG_BUCKSHOT, TRACER_LINE, sk_plr_dmg_neo.GetName(), sk_npc_dmg_neo.GetName(), sk_max_neo_ammo.GetName(), BULLET_IMPULSE(400, 1200), 0);
		def.AddAmmoType("RPG_Round", DMG_BURN, TRACER_NONE, sk_plr_dmg_neo.GetName(), sk_npc_dmg_neo.GetName(), sk_max_neo_ammo.GetName(), 0, 0);
		def.AddAmmoType("SMG1_Grenade", DMG_BURN, TRACER_NONE, sk_plr_dmg_neo.GetName(), sk_npc_dmg_neo.GetName(), sk_max_neo_ammo.GetName(), 0, 0);
		def.AddAmmoType("Grenade", DMG_BURN, TRACER_NONE, sk_plr_dmg_neo.GetName(), sk_npc_dmg_neo.GetName(), sk_max_neo_ammo.GetName(), 0, 0);
		def.AddAmmoType("slam", DMG_BURN, TRACER_NONE, sk_plr_dmg_neo.GetName(), sk_npc_dmg_neo.GetName(), sk_max_neo_ammo.GetName(), 0, 0);
	}

	return &def;
}

// This set of macros initializes a static CAmmoDef, and asserts its size stays within range. See usage in the GetAmmoDef() below.
#ifndef DEBUG
#define NEO_AMMO_DEF_START() static CAmmoDef *def; static bool bInitted = false; if (!bInitted) \
{ \
	bInitted = true; \
	def = GetAmmoDef_HL2MP()
#else
#define NEO_AMMO_DEF_START() static CAmmoDef *def; static bool bInitted = false; if (!bInitted) \
{ \
	bInitted = true; \
	def = GetAmmoDef_HL2MP(); \
	size_t numAmmos = def->m_nAmmoIndex;\
	Assert(numAmmos <= MAX_AMMO_TYPES)
#endif

#ifndef DEBUG
#define ADD_NEO_AMMO_TYPE(TypeName, DmgFlags, TracerEnum, PhysForceImpulse) def->AddAmmoType(#TypeName, DmgFlags, TracerEnum, sk_plr_dmg_neo.GetName(), sk_npc_dmg_neo.GetName(), sk_max_neo_ammo.GetName(), PhysForceImpulse, 0)
#else
#define ADD_NEO_AMMO_TYPE(TypeName, DmgFlags, TracerEnum, PhysForceImpulse) def->AddAmmoType(#TypeName, DmgFlags, TracerEnum, sk_plr_dmg_neo.GetName(), sk_npc_dmg_neo.GetName(), sk_max_neo_ammo.GetName(), PhysForceImpulse, 0);++numAmmos
#endif

#ifndef DEBUG
#define NEO_AMMO_DEF_END(); }
#else
#define NEO_AMMO_DEF_END(); Assert(numAmmos <= MAX_AMMO_TYPES); }
#endif

#define NEO_AMMO_DEF_RETURNVAL() def

CAmmoDef *GetAmmoDef()
{
	NEO_AMMO_DEF_START();
		ADD_NEO_AMMO_TYPE(AMMO_10G_SHELL, DMG_BULLET | DMG_BUCKSHOT, TRACER_LINE, BULLET_IMPULSE(400, 1200));
		ADD_NEO_AMMO_TYPE(AMMO_GRENADE, DMG_BLAST, TRACER_NONE, 0);
		ADD_NEO_AMMO_TYPE(AMMO_SMOKEGRENADE, DMG_BLAST, TRACER_NONE, 0);
		ADD_NEO_AMMO_TYPE(AMMO_DETPACK, DMG_BLAST, TRACER_NONE, 0);
		ADD_NEO_AMMO_TYPE(AMMO_PRI, DMG_BULLET, TRACER_LINE_AND_WHIZ, BULLET_IMPULSE(400, 1200));
		ADD_NEO_AMMO_TYPE(AMMO_SMAC, DMG_BULLET, TRACER_LINE_AND_WHIZ, BULLET_IMPULSE(400, 1200));
	NEO_AMMO_DEF_END();

	return NEO_AMMO_DEF_RETURNVAL();
}

void CNEORules::ClientSpawned(edict_t* pPlayer)
{
#if(0)
#ifdef CLIENT_DLL
	C_NEO_Player *player = C_NEO_Player::GetLocalNEOPlayer();
	if (player)
	{
		DevMsg("SPAWNED: %d vs %d\n", player->index, pPlayer->m_EdictIndex);
		player->m_bShowClassMenu = true;
	}
#endif
#endif
}

bool CNEORules::ShouldCollide(int collisionGroup0, int collisionGroup1)
{
	return BaseClass::ShouldCollide(collisionGroup0, collisionGroup1);
}

extern ConVar mp_chattime;

#ifdef GAME_DLL
void CNEORules::ChangeLevel(void)
{
	SetRoundStatus(NeoRoundStatus::Idle);
	m_iRoundNumber = 0;
	m_flNeoRoundStartTime = 0;
	m_flNeoNextRoundStartTime = 0;

	BaseClass::ChangeLevel();
}
#endif

bool CNEORules::CheckGameOver(void)
{
	// Note that this changes the level as side effect
	const bool gameOver = BaseClass::CheckGameOver();

	if (gameOver)
	{
		SetRoundStatus(NeoRoundStatus::Idle);
		m_iRoundNumber = 0;
		m_flNeoRoundStartTime = 0;
		m_flNeoNextRoundStartTime = 0;
	}

	return gameOver;
}

void CNEORules::Think(void)
{
#ifdef GAME_DLL
	if (m_nRoundStatus == NeoRoundStatus::Idle && gpGlobals->curtime > m_flNeoNextRoundStartTime)
	{
		StartNextRound();
		return;
	}

	if (g_fGameOver)   // someone else quit the game already
	{
		// check to see if we should change levels now
		if (m_flIntermissionEndTime < gpGlobals->curtime)
		{
			if (!m_bChangelevelDone)
			{
				ChangeLevel(); // intermission is over
				m_bChangelevelDone = true;
			}
		}

		return;
	}

	if (neo_score_limit.GetInt() != 0)
	{
#ifdef DEBUG
		float neoScoreLimitMin = -1.0f;
		AssertOnce(neo_score_limit.GetMin(neoScoreLimitMin));
		AssertOnce(neoScoreLimitMin >= 0);
#endif
		COMPILE_TIME_ASSERT((TEAM_JINRAI < TEAM_NSF) && (TEAM_JINRAI == (TEAM_NSF - 1)));
		for (int team = TEAM_JINRAI; team <= TEAM_NSF; ++team)
		{
			if (GetGlobalTeam(team)->GetRoundsWon() >= neo_score_limit.GetInt())
			{
				UTIL_CenterPrintAll(team == TEAM_JINRAI ? "JINRAI WINS THE MATCH\n" : "NSF WINS THE MATCH\n");
				SetRoundStatus(NeoRoundStatus::PostRound);
				GoToIntermission();
				return;
			}
		}
	}
#endif

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
			SetWinningTeam(captorTeam, NEO_VICTORY_GHOST_CAPTURE, false, true, false, false);

			for (int i = 1; i <= gpGlobals->maxClients; i++)
			{
				if (i == captorClient)
				{
					AwardRankUp(i);
					continue;
				}

				auto player = UTIL_PlayerByIndex(i);
				if (player && player->GetTeamNumber() == captorTeam &&
					player->IsAlive())
				{
					AwardRankUp(i);
				}
			}

			break;
		}
	}

	if (m_nRoundStatus == NeoRoundStatus::PreRoundFreeze)
	{
		if (IsRoundOver())
		{
			SetRoundStatus(NeoRoundStatus::PostRound);
		}
		else
		{
			if (gpGlobals->curtime > m_flNeoRoundStartTime + mp_neo_preround_freeze_time.GetFloat())
			{
				SetRoundStatus(NeoRoundStatus::RoundLive);
			}
		}
	}
	else if (m_nRoundStatus != NeoRoundStatus::RoundLive)
	{
		if (!IsRoundOver())
		{
			if (GetGlobalTeam(TEAM_JINRAI)->GetAliveMembers() > 0 && GetGlobalTeam(TEAM_NSF)->GetAliveMembers() > 0)
			{
				SetRoundStatus(NeoRoundStatus::RoundLive);
			}
		}
	}
	else
	{
		if (m_nRoundStatus == NeoRoundStatus::RoundLive)
		{
			COMPILE_TIME_ASSERT(TEAM_JINRAI == 2 && TEAM_NSF == 3);
			for (int team = TEAM_JINRAI; team <= TEAM_NSF; ++team)
			{
				if (GetGlobalTeam(team)->GetAliveMembers() == 0)
				{
					SetWinningTeam(GetOpposingTeam(team), NEO_VICTORY_TEAM_ELIMINATION, false, true, false, false);
				}
			}
		}
	}
#endif
}

void CNEORules::AwardRankUp(int client)
{
	auto player = UTIL_PlayerByIndex(client);
	if (player)
	{
		AwardRankUp(static_cast<CNEO_Player*>(player));
	}
}

#ifdef CLIENT_DLL
void CNEORules::AwardRankUp(C_NEO_Player *pClient)
#else
void CNEORules::AwardRankUp(CNEO_Player *pClient)
#endif
{
	if (!pClient)
	{
		return;
	}

	const int ranks[] = { 0, 4, 10, 20 };
	for (int i = 0; i < ARRAYSIZE(ranks); i++)
	{
		if (pClient->m_iXP.Get() < ranks[i])
		{
			pClient->m_iXP.GetForModify() = ranks[i];
			return;
		}
	}

	// If we're beyond max rank, just award +1 point.
	pClient->m_iXP.GetForModify()++;
}

// Return remaining time in seconds. Zero means there is no time limit.
float CNEORules::GetRoundRemainingTime()
{
	if (neo_round_timelimit.GetFloat() == 0 || m_nRoundStatus == NeoRoundStatus::Idle)
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
	CBaseEntity* pEnt;

	// Get the amount of ghost spawns available to us
	int numGhostSpawns = 0;

	pEnt = gEntList.FirstEnt();
	while (pEnt)
	{
		if (dynamic_cast<CNEOGhostSpawnPoint*>(pEnt))
		{
			numGhostSpawns++;
		}

		pEnt = gEntList.NextEnt(pEnt);
	}

	// No ghost spawns and this map isn't named "_ctg". Probably not a CTG map.
	if (numGhostSpawns == 0 && (V_stristr(GameRules()->MapName(), "_ctg") == 0))
	{
		return;
	}

	static int ghostEdict = -1;

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

			if (!ghost)
			{
				Assert(false);
				Warning("Failed to spawn a new ghost\n");
				return;
			}

			spawnedGhostNow = true;
		}
	}

	if (spawnedGhostNow)
	{
		int dispatchRes = DispatchSpawn(ghost);
		if (dispatchRes != 0)
		{
			Assert(false);
			return;
		}

		ghostEdict = ghost->edict()->m_EdictIndex;
		ghost->NetworkStateChanged();
	}

	Assert(UTIL_IsValidEntity(ghost));
	Assert(ghostEdict == ghost->edict()->m_EdictIndex);

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
					if (ghost->GetOwner())
					{
						Assert(false);
						ghost->GetOwner()->Weapon_Detach(ghost);
					}

					if (!ghostSpawn->GetAbsOrigin().IsValid())
					{
						ghost->SetAbsOrigin(vec3_origin);
						Warning("Failed to get ghost spawn coords; spawning ghost at map origin instead!\n");
						Assert(false);
					}
					else
					{
						ghost->SetAbsOrigin(ghostSpawn->GetAbsOrigin());
					}
					
					break;
				}
			}

			pEnt = gEntList.NextEnt(pEnt);
		}
	}

	if (spawnedGhostNow)
	{
		DevMsg("Spawned ghost at coords:\n\t%.1f %.1f %.1f\n",
			ghost->GetAbsOrigin().x,
			ghost->GetAbsOrigin().y,
			ghost->GetAbsOrigin().z);
	}
	else
	{
		DevMsg("Moved ghost to coords:\n\t%.1f %.1f %.1f\n",
			ghost->GetAbsOrigin().x,
			ghost->GetAbsOrigin().y,
			ghost->GetAbsOrigin().z);
	}
}

void CNEORules::StartNextRound()
{
	if (GetGlobalTeam(TEAM_JINRAI)->GetNumPlayers() == 0 || GetGlobalTeam(TEAM_NSF)->GetNumPlayers() == 0)
	{
		UTIL_CenterPrintAll("Waiting for players on both teams.\n"); // NEO TODO (Rain): actual message
		SetRoundStatus(NeoRoundStatus::Idle);
		m_flNeoNextRoundStartTime = gpGlobals->curtime + 10.0f;
		return;
	}

	m_flNeoRoundStartTime = gpGlobals->curtime;
	m_flNeoNextRoundStartTime = 0;

	CleanUpMap();

	SetRoundStatus(NeoRoundStatus::PreRoundFreeze);

	char RoundMsg[11];
	COMPILE_TIME_ASSERT(sizeof(RoundMsg) == sizeof("Round 99\n\0"));
	V_sprintf_safe(RoundMsg, "Round %d\n", Min(99, ++m_iRoundNumber));
	UTIL_CenterPrintAll(RoundMsg);

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
		if (gpGlobals->curtime < m_flNeoRoundStartTime + mp_neo_preround_freeze_time.GetFloat())
		{
			pPlayer->AddFlag(FL_GODMODE);
			pPlayer->AddNeoFlag(NEO_FL_FREEZETIME);
		}

		pPlayer->m_bInAim = false;
		pPlayer->m_bInThermOpticCamo = false;
		pPlayer->m_bInVision = false;

		pPlayer->SetTestMessageVisible(false);
	}

	m_flIntermissionEndTime = 0;
	m_flRestartGameTime = 0;
	m_bCompleteReset = false;

	IGameEvent *event = gameeventmanager->CreateEvent("round_start");
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

bool CNEORules::IsRoundOver() const
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
		Assert((m_flNeoNextRoundStartTime < 0) == false);
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

	// NEO TODO (Rain): get a neo_game_config so we can specify better
	if (IsTeamplay())
	{
		return "Capture the Ghost";
	}

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
#if(0)
static inline void RemoveGhosts()
{
	Assert(false);
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
#endif

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
		else
		{
			// NEO FIXME (Rain): decals won't clean on world (non-ent) surfaces.
			// Is this the right place to call in? Gamemode related?
			//pCur->RemoveAllDecals();
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
}

void CNEORules::CheckRestartGame()
{
	BaseClass::CheckRestartGame();
}

void CNEORules::ResetGhostCapPoints()
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

		pPlayer->m_iXP.GetForModify() = 0;

		pPlayer->SetTestMessageVisible(false);
	}

	// Respawn entities (glass, doors, etc..)

	CTeam *pJinrai = GetGlobalTeam(TEAM_JINRAI);
	CTeam *pNSF = GetGlobalTeam(TEAM_NSF);
	Assert(pJinrai && pNSF);

	pJinrai->SetScore(0);
	pJinrai->SetRoundsWon(0);
	pNSF->SetScore(0);
	pNSF->SetRoundsWon(0);

	m_flIntermissionEndTime = 0;
	m_flRestartGameTime = 0.0;
	m_bCompleteReset = false;
	m_iRoundNumber = 0;
	m_flNeoNextRoundStartTime = FLT_MAX;
	m_flNeoRoundStartTime = FLT_MAX;

	SetRoundStatus(NeoRoundStatus::Idle);

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

ConVar snd_victory_volume("snd_victory_volume", "0.33", FCVAR_ARCHIVE | FCVAR_DONTRECORD | FCVAR_USERINFO, "Loudness of the victory jingle (0-1).", true, 0.0, true, 1.0);

#ifdef GAME_DLL
extern ConVar snd_musicvolume;
void CNEORules::SetWinningTeam(int team, int iWinReason, bool bForceMapReset, bool bSwitchTeams, bool bDontAddScore, bool bFinal)
{
	if (IsRoundOver())
	{
		return;
	}

	SetRoundStatus(NeoRoundStatus::PostRound);

	auto winningTeam = GetGlobalTeam(team);

	if (bForceMapReset)
	{
		RestartGame();
	}
	else
	{
		m_flNeoNextRoundStartTime = gpGlobals->curtime + mp_chattime.GetFloat();

		if (!bDontAddScore)
		{
			winningTeam->IncrementRoundsWon();
		}
	}

	char victoryMsg[128];
	bool gotMatchWinner = false;

	if (!bForceMapReset && neo_score_limit.GetInt() != 0)
	{
#ifdef DEBUG
		float neoScoreLimitMin = -1.0f;
		AssertOnce(neo_score_limit.GetMin(neoScoreLimitMin));
		AssertOnce(neoScoreLimitMin >= 0);
#endif
		if (winningTeam->GetRoundsWon() >= neo_score_limit.GetInt())
		{
			V_sprintf_safe(victoryMsg, "Team %s wins the match!\n", (team == TEAM_JINRAI ? "Jinrai" : "NSF"));
			m_flNeoNextRoundStartTime = FLT_MAX;
			gotMatchWinner = true;
		}
	}

	if (!gotMatchWinner)
	{
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
	}

	EmitSound_t soundParams;
	soundParams.m_nChannel = CHAN_VOICE;
	// Referencing sounds directly because we can't override the soundscript volume level otherwise
	soundParams.m_pSoundName = (team == TEAM_JINRAI) ? "gameplay/jinrai.mp3" : (team == TEAM_NSF) ? "gameplay/nsf.mp3" : "gameplay/draw.mp3";
	soundParams.m_bWarnOnDirectWaveReference = false;

	CRecipientFilter soundFilter;
	soundFilter.AddAllPlayers();
	soundFilter.MakeReliable();

	for (int i = 1; i <= gpGlobals->maxClients; ++i)
	{
		auto player = static_cast<CNEO_Player*>(UTIL_PlayerByIndex(i));
		if (player && (!player->IsBot() || player->IsHLTV()))
		{
			engine->ClientPrintf(player->edict(), victoryMsg);
			UTIL_ClientPrintAll((gotMatchWinner ? HUD_PRINTTALK : HUD_PRINTCENTER), victoryMsg);

			float jingleVolume = atof(engine->GetClientConVarValue(i, snd_victory_volume.GetName()));
			soundParams.m_flVolume = jingleVolume;
			player->EmitSound(soundFilter, i, soundParams);
		}
	}

	if (gotMatchWinner)
	{
		GoToIntermission();
	}
}
#endif

void CNEORules::PlayerKilled(CBasePlayer *pVictim, const CTakeDamageInfo &info)
{
	BaseClass::PlayerKilled(pVictim, info);

	auto attacker = dynamic_cast<CNEO_Player*>(info.GetAttacker());
	auto victim = dynamic_cast<CNEO_Player*>(pVictim);

	if (!attacker || !pVictim)
	{
		return;
	}

	// Suicide
	if (attacker == victim)
	{
		victim->m_iXP.GetForModify() -= 1;
	}
	else
	{
		// Team kill
		if (attacker->GetTeamNumber() == victim->GetTeamNumber())
		{
			victim->m_iXP.GetForModify() -= 1;
		}
		// Enemy kill
		else
		{
			victim->m_iXP.GetForModify() += 1;
		}
	}
}

#ifdef GAME_DLL
extern ConVar falldamage;
ConVar sv_neo_falldmg_scale("sv_neo_falldmg_scale", "0.25", FCVAR_CHEAT | FCVAR_DEVELOPMENTONLY, "Scale factor for NEO fall damage.");
float CNEORules::FlPlayerFallDamage(CBasePlayer* pPlayer)
{
	// Is player fall damage disabled?
	if (!falldamage.GetBool())
	{
		return 0;
	}

	// subtract off the speed at which a player is allowed to fall without being hurt,
	// so damage will be based on speed beyond that, not the entire fall
	pPlayer->m_Local.m_flFallVelocity -= PLAYER_MAX_SAFE_FALL_SPEED;
	return pPlayer->m_Local.m_flFallVelocity * DAMAGE_FOR_FALL_SPEED * sv_neo_falldmg_scale.GetFloat();
}

const char* CNEORules::GetChatFormat(bool bTeamOnly, CBasePlayer* pPlayer)
{
	if (!pPlayer)  // dedicated server output
	{
		return NULL;
	}

#define FMT_PLAYERNAME "%s1"
#define FMT_CHATMESSAGE "%s2"
#define FMT_LOCATION "%s3"
#define FMT_TEAM_JINRAI "[Jinrai]"
#define FMT_TEAM_NSF "[NSF]"
#define FMT_TEAM_SPECTATOR "[Spectator]"
#define FMT_TEAM_UNASSIGNED "[Unassigned]"
#define FMT_DEAD "*DEAD*"

	if (pPlayer->IsAlive())
	{
		if (bTeamOnly)
		{
			switch (pPlayer->GetTeamNumber())
			{
			case TEAM_JINRAI: return FMT_TEAM_JINRAI " (team) " FMT_PLAYERNAME ": " FMT_CHATMESSAGE;
			case TEAM_NSF: return FMT_TEAM_NSF " (team) " FMT_PLAYERNAME ": " FMT_CHATMESSAGE;
			case TEAM_SPECTATOR: return FMT_TEAM_SPECTATOR " (team) " FMT_PLAYERNAME ": " FMT_CHATMESSAGE;
			default: return FMT_TEAM_UNASSIGNED " (team) " FMT_PLAYERNAME ": " FMT_CHATMESSAGE;
			}
		}
		else
		{
			switch (pPlayer->GetTeamNumber())
			{
			case TEAM_JINRAI: return FMT_TEAM_JINRAI " " FMT_PLAYERNAME ": " FMT_CHATMESSAGE;
			case TEAM_NSF: return FMT_TEAM_NSF " " FMT_PLAYERNAME ": " FMT_CHATMESSAGE;
			case TEAM_SPECTATOR: return FMT_TEAM_SPECTATOR " " FMT_PLAYERNAME ": " FMT_CHATMESSAGE;
			default: return FMT_TEAM_UNASSIGNED " " FMT_PLAYERNAME ": " FMT_CHATMESSAGE;
			}
		}
	}
	else
	{
		if (bTeamOnly)
		{
			switch (pPlayer->GetTeamNumber())
			{
			case TEAM_JINRAI: return FMT_DEAD " " FMT_TEAM_JINRAI " (team) " FMT_PLAYERNAME ": " FMT_CHATMESSAGE;
			case TEAM_NSF: return FMT_DEAD " " FMT_TEAM_NSF " (team) " FMT_PLAYERNAME ": " FMT_CHATMESSAGE;
			case TEAM_SPECTATOR: return FMT_TEAM_SPECTATOR " (team) " FMT_PLAYERNAME ": " FMT_CHATMESSAGE;
			default: return FMT_TEAM_UNASSIGNED " (team) " FMT_PLAYERNAME ": " FMT_CHATMESSAGE;
			}
		}
		else
		{
			switch (pPlayer->GetTeamNumber())
			{
			case TEAM_JINRAI: return FMT_DEAD " " FMT_TEAM_JINRAI " " FMT_PLAYERNAME ": " FMT_CHATMESSAGE;
			case TEAM_NSF: return FMT_DEAD " " FMT_TEAM_NSF " " FMT_PLAYERNAME ": " FMT_CHATMESSAGE;
			case TEAM_SPECTATOR: return FMT_TEAM_SPECTATOR " " FMT_PLAYERNAME ": " FMT_CHATMESSAGE;
			default: return FMT_TEAM_UNASSIGNED " " FMT_PLAYERNAME ": " FMT_CHATMESSAGE;
			}
		}
	}

	Assert(false); // should never fall through the switch
}
#endif

#ifdef GAME_DLL
void CNEORules::DeathNotice(CBasePlayer* pVictim, const CTakeDamageInfo& info)
{
	// Work out what killed the player, and send a message to all clients about it
	const char* killer_weapon_name = "world";		// by default, the player is killed by the world
	int killer_ID = 0;

	// Find the killer & the scorer
	CBaseEntity* pInflictor = info.GetInflictor();
	CBaseEntity* pKiller = info.GetAttacker();
	CBasePlayer* pScorer = GetDeathScorer(pKiller, pInflictor);

	bool isExplosiveKill = false;
	bool isANeoDerivedWeapon = false;

	// Custom kill type?
	if (info.GetDamageCustom())
	{
		killer_weapon_name = GetDamageCustomString(info);
		if (pScorer)
		{
			killer_ID = pScorer->GetUserID();
		}
	}
	else
	{
		// Is the killer a client?
		if (pScorer)
		{
			killer_ID = pScorer->GetUserID();

			if (pInflictor)
			{
				if (pInflictor == pScorer)
				{
					// If the inflictor is the killer,  then it must be their current weapon doing the damage
					if (pScorer->GetActiveWeapon())
					{
						killer_weapon_name = pScorer->GetActiveWeapon()->GetClassname();

						auto neoWep = dynamic_cast<CNEOBaseCombatWeapon*>(pScorer->GetActiveWeapon());
						if (neoWep)
						{
							isANeoDerivedWeapon = true;
							isExplosiveKill = (neoWep->GetNeoWepBits() & NEO_WEP_EXPLOSIVE) ? true : false;
						}
					}
				}
				else
				{
					killer_weapon_name = pInflictor->GetClassname();  // it's just that easy
				}
			}
		}
		else
		{
			killer_weapon_name = pInflictor->GetClassname();
		}

		// strip the NPC_* or weapon_* from the inflictor's classname
		if (strncmp(killer_weapon_name, "weapon_", 7) == 0)
		{
			killer_weapon_name += 7;
		}
		else if (strncmp(killer_weapon_name, "npc_", 4) == 0)
		{
			killer_weapon_name += 4;
		}
		else if (strncmp(killer_weapon_name, "func_", 5) == 0)
		{
			killer_weapon_name += 5;
		}
		else if (strstr(killer_weapon_name, "physics"))
		{
			killer_weapon_name = "physics";
		}
#if(0)
		if (strcmp(killer_weapon_name, "prop_combine_ball") == 0)
		{
			killer_weapon_name = "combine_ball";
		}
		else if (strcmp(killer_weapon_name, "grenade_ar2") == 0)
		{
			killer_weapon_name = "smg1_grenade";
		}
		else if (strcmp(killer_weapon_name, "satchel") == 0 || strcmp(killer_weapon_name, "tripmine") == 0)
		{
			killer_weapon_name = "slam";
		}
#endif
	}

	IGameEvent* event = gameeventmanager->CreateEvent("player_death");
	if (event)
	{
		event->SetInt("userid", pVictim->GetUserID());
		event->SetInt("attacker", killer_ID);
		event->SetString("weapon", killer_weapon_name);
		event->SetInt("priority", 7);

		// Which deathnotice icon to draw.
		// This value needs to be the same as original NT for plugin compatibility.
		short killfeed_icon;

		// NEO HACK/TODO (Rain):
		// Knife is not yet derived from nt base wep class, so we can't yet get its bits
		if (!isANeoDerivedWeapon)
		{
			killfeed_icon = 0; // NEO TODO (Rain): set correct icon
		}
		else
		{
			// Suicide icon
			if (pKiller == pVictim)
			{
				killfeed_icon = 0; // NEO TODO (Rain): set correct icon
			}
			// Explosion icon
			else if (isExplosiveKill)
			{
				killfeed_icon = 0; // NEO TODO (Rain): set correct icon
			}
			// Headshot icon
			else if (pVictim->LastHitGroup() == HITGROUP_HEAD)
			{
				killfeed_icon = NEO_DEATH_EVENT_ICON_HEADSHOT;
			}
			// Generic weapon kill icon
			else
			{
				killfeed_icon = 0; // NEO TODO (Rain): set correct icon
			}
		}

		event->SetInt("icon", killfeed_icon);

		gameeventmanager->FireEvent(event);
	}
}
#endif

#ifdef GAME_DLL
void CNEORules::ClientDisconnected(edict_t* pClient)
{
	auto pNeoPlayer = static_cast<CNEO_Player*>(CBaseEntity::Instance(pClient));
	Assert(pNeoPlayer);
	if (pNeoPlayer)
	{
		auto ghost = GetNeoWepWithBits(pNeoPlayer, NEO_WEP_GHOST);
		if (ghost)
		{
			ghost->Drop(vec3_origin);
			ghost->SetRemoveable(false);
			pNeoPlayer->Weapon_Detach(ghost);
		}
	}

	BaseClass::ClientDisconnected(pClient);
}
#endif

#ifdef GAME_DLL
bool CNEORules::FPlayerCanRespawn(CBasePlayer* pPlayer)
{
	auto gameType = GetGameType();

	if (gameType == NEO_GAME_TYPE_TDM)
	{
		return true;
	}
	// Some unknown game mode
	else if (gameType != NEO_GAME_TYPE_CTG)
	{
		Assert(false);
		return true;
	}

	// Mode is CTG
	auto jinrai = GetGlobalTeam(TEAM_JINRAI);
	auto nsf = GetGlobalTeam(TEAM_NSF);

	if (jinrai && nsf)
	{
		if (jinrai->GetNumPlayers() == 0 || nsf->GetNumPlayers() == 0)
		{
			return true;
		}
	}
	else
	{
		Assert(false);
	}

	// Did we make it in time to spawn for this round?
	if (GetRemainingPreRoundFreezeTime(false) + mp_neo_latespawn_max_time.GetFloat() > 0)
	{
		return true;
	}

	return false;
}
#endif

void CNEORules::SetRoundStatus(NeoRoundStatus status)
{
	if (status == NeoRoundStatus::RoundLive)
	{
		for (int i = 1; i <= gpGlobals->maxClients; ++i)
		{
			auto player = static_cast<CNEO_Player*>(UTIL_PlayerByIndex(i));
			if (player)
			{
				player->RemoveFlag(FL_GODMODE);
				player->RemoveNeoFlag(NEO_FL_FREEZETIME);
			}
		}
#ifdef GAME_DLL
		UTIL_CenterPrintAll("GO GO GO\n"); // NEO TODO (Rain): correct phrase
#endif
	}

	m_nRoundStatus = status;
}

const char* CNEORules::GetGameTypeName(void)
{
	switch (GetGameType())
	{
	case NEO_GAME_TYPE_TDM:
		return "Team Deathmatch";
	case NEO_GAME_TYPE_CTG:
		return "Capture the Ghost";
	default:
		Assert(false);
		return "Unknown";
	}
}

float CNEORules::GetRemainingPreRoundFreezeTime(const bool clampToZero) const
{
	// If there's no time left, return 0 instead of a negative value.
	if (clampToZero)
	{
		return Max(0.0f, m_flNeoRoundStartTime + mp_neo_preround_freeze_time.GetFloat() - gpGlobals->curtime);
	}
	// Or return negative value of how many seconds late we were.
	else
	{
		return m_flNeoRoundStartTime + mp_neo_preround_freeze_time.GetFloat() - gpGlobals->curtime;
	}
}