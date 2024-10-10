#include "cbase.h"
#include "neo_gamerules.h"
#include "neo_version_info.h"
#include "in_buttons.h"
#include "ammodef.h"

#include "takedamageinfo.h"
#include "basegrenade_shared.h"

#ifdef CLIENT_DLL
	#include "c_neo_player.h"
	#include "c_team.h"
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
	#include "inetchannelinfo.h"

extern ConVar weaponstay;
#endif

ConVar mp_neo_loopback_warmup_round("mp_neo_loopback_warmup_round", "0", FCVAR_REPLICATED, "Allow loopback server to do warmup rounds.", true, 0.0f, true, 1.0f);
ConVar mp_neo_warmup_round_time("mp_neo_warmup_round_time", "45", FCVAR_REPLICATED, "The warmup round time, in seconds.", true, 0.0f, false, 0.0f);
ConVar mp_neo_preround_freeze_time("mp_neo_preround_freeze_time", "15", FCVAR_REPLICATED, "The pre-round freeze time, in seconds.", true, 0.0, false, 0);
ConVar mp_neo_latespawn_max_time("mp_neo_latespawn_max_time", "15", FCVAR_REPLICATED, "How many seconds late are players still allowed to spawn.", true, 0.0, false, 0);

ConVar sv_neo_wep_dmg_modifier("sv_neo_wep_dmg_modifier", "1", FCVAR_REPLICATED, "Temp global weapon damage modifier.", true, 0.0, true, 100.0);
ConVar neo_sv_player_restore("neo_sv_player_restore", "1", FCVAR_REPLICATED, "If enabled, the server will save players XP and deaths per match session and restore them if they reconnect.", true, 0.0f, true, 1.0f);

#ifdef CLIENT_DLL
ConVar neo_name("neo_name", "", FCVAR_USERINFO | FCVAR_ARCHIVE, "The nickname to set instead of the steam profile name.");
ConVar cl_onlysteamnick("cl_onlysteamnick", "0", FCVAR_USERINFO | FCVAR_ARCHIVE, "Only show players Steam names, otherwise show player set names.", true, 0.0f, true, 1.0f);
#endif

ConVar neo_vote_game_mode("neo_vote_game_mode", "1", FCVAR_USERINFO, "Vote on game mode to play. TDM=0, CTG=1, VIP=2", true, 0, true, 2);
ConVar neo_vip_eligible("neo_cl_vip_eligible", "1", FCVAR_ARCHIVE, "Eligible for VIP", true, 0, true, 1);
#ifdef GAME_DLL
ConVar sv_neo_vip_ctg_on_death("sv_neo_vip_ctg_on_death", "0", FCVAR_ARCHIVE, "Spawn Ghost when VIP dies, continue the game", true, 0, true, 1);
#endif

#ifdef GAME_DLL
ConVar sv_neo_change_game_type_mid_round("sv_neo_change_game_type_mid_round", "1", FCVAR_REPLICATED, "Allow game type change mid-match");
#endif

#ifdef GAME_DLL
#ifdef DEBUG
// Debug build by default relax integrity check requirement among debug clients
static constexpr char INTEGRITY_CHECK_DBG[] = "1";
#else
static constexpr char INTEGRITY_CHECK_DBG[] = "0";
#endif
ConVar neo_sv_build_integrity_check("neo_sv_build_integrity_check", "1", FCVAR_GAMEDLL | FCVAR_REPLICATED,
									"If enabled, the server checks the build's Git hash between the client and"
									" the server. If it doesn't match, the server rejects and disconnects the client.",
									true, 0.0f, true, 1.0f);
ConVar neo_sv_build_integrity_check_allow_debug("neo_sv_build_integrity_check_allow_debug", INTEGRITY_CHECK_DBG, FCVAR_GAMEDLL | FCVAR_REPLICATED,
									"If enabled, when the server checks the client hashes, it'll also allow debug"
									" builds which has a given special bit to bypass the check.",
									true, 0.0f, true, 1.0f);

#ifdef DEBUG
static constexpr char TEAMDMG_MULTI[] = "0";
#else
static constexpr char TEAMDMG_MULTI[] = "2";
#endif
ConVar neo_sv_mirror_teamdamage_multiplier("neo_sv_mirror_teamdamage_multiplier", TEAMDMG_MULTI, FCVAR_REPLICATED, "The damage multiplier given to the friendly-firing individual. Set value to 0 to disable mirror team damage.", true, 0.0f, true, 100.0f);
ConVar neo_sv_mirror_teamdamage_duration("neo_sv_mirror_teamdamage_duration", "7", FCVAR_REPLICATED, "How long in seconds the mirror damage is active for the start of each round. Set to 0 for the entire round.", true, 0.0f, true, 10000.0f);
ConVar neo_sv_mirror_teamdamage_immunity("neo_sv_mirror_teamdamage_immunity", "1", FCVAR_REPLICATED, "If enabled, the victim will not take damage from a teammate during the mirror team damage duration.", true, 0.0f, true, 1.0f);

ConVar neo_sv_teamdamage_kick("neo_sv_teamdamage_kick", "0", FCVAR_REPLICATED, "If enabled, the friendly-firing individual will be kicked if damage is received during the neo_sv_mirror_teamdamage_duration, exceeds the neo_sv_teamdamage_kick_hp value, or executes a teammate.", true, 0.0f, true, 1.0f);
ConVar neo_sv_teamdamage_kick_hp("neo_sv_teamdamage_kick_hp", "900", FCVAR_REPLICATED, "The threshold for the amount of HP damage inflicted on teammates before the client is kicked.", true, 100.0f, false, 0.0f);
ConVar neo_sv_teamdamage_kick_kills("neo_sv_teamdamage_kick_kills", "6", FCVAR_REPLICATED, "The threshold for the amount of team kills before the client is kicked.", true, 1.0f, false, 0.0f);
ConVar neo_sv_suicide_prevent_cap_punish("neo_sv_suicide_prevent_cap_punish", "1", FCVAR_REPLICATED,
										 "If enabled, if a player suicides and is the only one alive in their team, "
										 "while the other team is holding the ghost, reward the ghost holder team "
										 "a rank up.",
										 true, 0.0f, true, 1.0f);
#endif

enum ENeoCollision
{
	NEOCOLLISION_NONE = 0,
	NEOCOLLISION_TEAM,
	NEOCOLLISION_ALL,

	NEOCOLLISION__TOTAL,
};

ConVar neo_sv_collision("neo_sv_collision", "0", FCVAR_REPLICATED, "0 = No collision (default), 1 = Team-based collision, 2 = All player collision", true, 0.0f, true, NEOCOLLISION__TOTAL - 1);

REGISTER_GAMERULES_CLASS( CNEORules );

BEGIN_NETWORK_TABLE_NOBASE( CNEORules, DT_NEORules )
// NEO TODO (Rain): NEO specific game modes var (CTG/TDM/...)
#ifdef CLIENT_DLL
	RecvPropFloat(RECVINFO(m_flNeoNextRoundStartTime)),
	RecvPropFloat(RECVINFO(m_flNeoRoundStartTime)),
	RecvPropInt(RECVINFO(m_nRoundStatus)),
	RecvPropInt(RECVINFO(m_nGameTypeSelected)),
	RecvPropInt(RECVINFO(m_iRoundNumber)),
	RecvPropInt(RECVINFO(m_iGhosterTeam)),
	RecvPropInt(RECVINFO(m_iGhosterPlayer)),
	RecvPropInt(RECVINFO(m_iEscortingTeam)),
	RecvPropBool(RECVINFO(m_bGhostExists)),
	RecvPropVector(RECVINFO(m_vecGhostMarkerPos)),
#else
	SendPropFloat(SENDINFO(m_flNeoNextRoundStartTime)),
	SendPropFloat(SENDINFO(m_flNeoRoundStartTime)),
	SendPropInt(SENDINFO(m_nRoundStatus)),
	SendPropInt(SENDINFO(m_nGameTypeSelected)),
	SendPropInt(SENDINFO(m_iRoundNumber)),
	SendPropInt(SENDINFO(m_iGhosterTeam)),
	SendPropInt(SENDINFO(m_iGhosterPlayer)),
	SendPropInt(SENDINFO(m_iEscortingTeam)),
	SendPropBool(SENDINFO(m_bGhostExists)),
	SendPropVector(SENDINFO(m_vecGhostMarkerPos), -1, SPROP_COORD_MP_LOWPRECISION | SPROP_CHANGES_OFTEN, MIN_COORD_FLOAT, MAX_COORD_FLOAT),
#endif
END_NETWORK_TABLE()

LINK_ENTITY_TO_CLASS( neo_gamerules, CNEOGameRulesProxy );
IMPLEMENT_NETWORKCLASS_ALIASED( NEOGameRulesProxy, DT_NEOGameRulesProxy );

extern bool RespawnWithRet(CBaseEntity *pEdict, bool fCopyCorpse);

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
ConVar neo_round_limit("neo_round_limit", "0", FCVAR_REPLICATED, "Max amount of rounds, 0 for no limit.", true, 0.0f, false, 0.0f);
ConVar neo_round_sudden_death("neo_round_sudden_death", "1", FCVAR_REPLICATED, "If neo_round_limit is not 0 and round is past "
	"neo_round_limit, go into sudden death where match won't end until a team won.", true, 0.0f, true, 1.0f);

ConVar neo_tdm_round_timelimit("neo_tdm_round_timelimit", "10.25", FCVAR_REPLICATED, "TDM round timelimit, in minutes.",
	true, 0.0f, false, 600.0f);

ConVar neo_ctg_round_timelimit("neo_ctg_round_timelimit", "3.25", FCVAR_REPLICATED, "CTG round timelimit, in minutes.",
	true, 0.0f, false, 600.0f);

ConVar neo_vip_round_timelimit("neo_vip_round_timelimit", "3.25", FCVAR_REPLICATED, "VIP round timelimit, in minutes.",
	true, 0.0f, false, 600.0f);

ConVar neo_sv_ignore_wep_xp_limit("neo_sv_ignore_wep_xp_limit", "0", FCVAR_CHEAT | FCVAR_REPLICATED, "If true, allow equipping any loadout regardless of player XP.",
	true, 0.0f, true, 1.0f);

#ifdef CLIENT_DLL
extern ConVar neo_fov;
#endif

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

#ifdef GAME_DLL
static void CvarChanged_WeaponStay(IConVar* convar, const char* pOldVal, float flOldVal)
{
	auto wep = gEntList.NextEntByClass((CNEOBaseCombatWeapon*)NULL);
	while (wep)
	{
		// Don't schedule removal for guns currently held by someone
		if (!wep->GetOwner())
		{
			wep->SetPickupTouch();
		}
		wep = gEntList.NextEntByClass(wep);
	}
}
#endif

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

	if (GetGameType() == NEO_GAME_TYPE_CTG || GetGameType() == NEO_GAME_TYPE_VIP)
	{
		ResetGhostCapPoints();
	}
#endif

	ResetMapSessionCommon();
	ListenForGameEvent("round_start");

#ifdef GAME_DLL
	weaponstay.InstallChangeCallback(CvarChanged_WeaponStay);
#endif
}

CNEORules::~CNEORules()
{
#ifdef GAME_DLL
	weaponstay.InstallChangeCallback(nullptr);
#endif
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
        // NEO TODO (brekiy/Rain): add specific ammo types
		ADD_NEO_AMMO_TYPE(AMMO_10G_SHELL, DMG_BULLET | DMG_BUCKSHOT, TRACER_NONE, BULLET_IMPULSE(400, 1200));
		ADD_NEO_AMMO_TYPE(AMMO_10G_SLUG, DMG_BULLET, TRACER_NONE, BULLET_IMPULSE(400, 1500));
		ADD_NEO_AMMO_TYPE(AMMO_GRENADE, DMG_BLAST, TRACER_NONE, 0);
		ADD_NEO_AMMO_TYPE(AMMO_SMOKEGRENADE, DMG_BLAST, TRACER_NONE, 0);
		ADD_NEO_AMMO_TYPE(AMMO_DETPACK, DMG_BLAST, TRACER_NONE, 0);
		ADD_NEO_AMMO_TYPE(AMMO_PRI, DMG_BULLET, TRACER_LINE_AND_WHIZ, BULLET_IMPULSE(400, 1200));
		ADD_NEO_AMMO_TYPE(AMMO_PZ, DMG_BULLET, TRACER_LINE_AND_WHIZ, BULLET_IMPULSE(400, 1200));
		ADD_NEO_AMMO_TYPE(AMMO_SNIPER, DMG_BULLET, TRACER_LINE_AND_WHIZ, BULLET_IMPULSE(400, 1200));
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

int CNEORules::DefaultFOV(void)
{
#ifdef CLIENT_DLL
	return neo_fov.GetInt();
#else
	return DEFAULT_FOV;
#endif
}

bool CNEORules::ShouldCollide(const CBaseEntity *ent0, const CBaseEntity *ent1) const
{
	const int ent0Group = ent0->GetCollisionGroup();
	const int ent1Group = ent1->GetCollisionGroup();
	const int iNeoCol = neo_sv_collision.GetInt();
	if (iNeoCol != NEOCOLLISION_ALL &&
			(ent0Group == COLLISION_GROUP_PLAYER || ent0Group == COLLISION_GROUP_PLAYER_MOVEMENT) &&
			(ent1Group == COLLISION_GROUP_PLAYER || ent1Group == COLLISION_GROUP_PLAYER_MOVEMENT))
	{
		return (iNeoCol == NEOCOLLISION_TEAM) ?
					(static_cast<const CNEO_Player *>(ent0)->GetTeamNumber() != static_cast<const CNEO_Player *>(ent1)->GetTeamNumber()) :
					false;
	}
	return const_cast<CNEORules *>(this)->CTeamplayRules::ShouldCollide(ent0Group, ent1Group);
}

extern ConVar mp_chattime;

void CNEORules::ResetMapSessionCommon()
{
	SetRoundStatus(NeoRoundStatus::Idle);
	m_iRoundNumber = 0;
	m_iGhosterTeam = TEAM_UNASSIGNED;
	m_iGhosterPlayer = 0;
	m_bGhostExists = false;
	m_vecGhostMarkerPos = vec3_origin;
	m_flNeoRoundStartTime = 0.0f;
	m_flNeoNextRoundStartTime = 0.0f;
#ifdef GAME_DLL
	m_pRestoredInfos.Purge();

	for (int i = 1; i <= gpGlobals->maxClients; i++)
	{
		auto *pPlayer = static_cast<CNEO_Player *>(UTIL_PlayerByIndex(i));
		if (pPlayer)
		{
			pPlayer->m_iTeamDamageInflicted = 0;
			pPlayer->m_iTeamKillsInflicted = 0;
			pPlayer->m_bIsPendingTKKick = false;
			pPlayer->m_bKilledInflicted = false;
		}
	}
	m_flPrevThinkKick = 0.0f;
	m_flPrevThinkMirrorDmg = 0.0f;
	m_bTeamBeenAwardedDueToCapPrevent = false;
	V_memset(m_arrayiEntPrevCap, 0, sizeof(m_arrayiEntPrevCap));
	m_iEntPrevCapSize = 0;
#endif
}

#ifdef GAME_DLL
void CNEORules::ChangeLevel(void)
{
	ResetMapSessionCommon();
	BaseClass::ChangeLevel();
}

#endif

bool CNEORules::CheckGameOver(void)
{
	// Note that this changes the level as side effect
	const bool gameOver = BaseClass::CheckGameOver();

	if (gameOver)
	{
		ResetMapSessionCommon();
	}

	return gameOver;
}

void CNEORules::Think(void)
{
#ifdef GAME_DLL
	if ((m_nRoundStatus == NeoRoundStatus::Idle || m_nRoundStatus == NeoRoundStatus::Warmup) && gpGlobals->curtime > m_flNeoNextRoundStartTime)
	{
		StartNextRound();
		return;
	}

	// Allow respawn if it's an idle or warmup round
	if (m_nRoundStatus == NeoRoundStatus::Idle || m_nRoundStatus == NeoRoundStatus::Warmup)
	{
		CRecipientFilter filter;
		filter.MakeReliable();

		for (int i = 1; i <= gpGlobals->maxClients; i++)
		{
			auto player = static_cast<CNEO_Player *>(UTIL_PlayerByIndex(i));
			if (player && player->IsDead() && player->DeathCount() > 0)
			{
				const int playerTeam = player->GetTeamNumber();
				if ((playerTeam == TEAM_JINRAI || playerTeam == TEAM_NSF) && RespawnWithRet(player, false))
				{
					filter.AddRecipient(player);

					player->m_bInAim = false;
					player->m_bInThermOpticCamo = false;
					player->m_bInVision = false;
					player->m_bIneligibleForLoadoutPick = false;
					player->SetTestMessageVisible(false);
				}
			}
		}

		if (filter.GetRecipientCount() > 0)
		{
			UserMessageBegin(filter, "IdleRespawnShowMenu");
			MessageEnd();
		}
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
#endif

	BaseClass::Think();

#ifdef GAME_DLL
	if (MirrorDamageMultiplier() > 0.0f &&
			gpGlobals->curtime > (m_flPrevThinkMirrorDmg + 0.25f))
	{
		for (int i = 1; i <= gpGlobals->maxClients; ++i)
		{
			auto player = static_cast<CNEO_Player*>(UTIL_PlayerByIndex(i));
			if (player && player->IsAlive() && player->m_bKilledInflicted && player->m_iHealth <= 0)
			{
				player->CommitSuicide(false, true);
			}
		}

		m_flPrevThinkMirrorDmg = gpGlobals->curtime;
	}

	if (neo_sv_teamdamage_kick.GetBool() && m_nRoundStatus == NeoRoundStatus::RoundLive &&
			gpGlobals->curtime > (m_flPrevThinkKick + 0.5f))
	{
		const int iThresKickHp = neo_sv_teamdamage_kick_hp.GetInt();
		const int iThresKickKills = neo_sv_teamdamage_kick_kills.GetInt();

		// Separate command from check so kick not affected by player index
		int userIDsToKick[MAX_PLAYERS + 1] = {};
		int userIDsToKickSize = 0;
		for (int i = 1; i <= gpGlobals->maxClients; ++i)
		{
			auto player = static_cast<CNEO_Player*>(UTIL_PlayerByIndex(i));
			if (player && (player->m_iTeamDamageInflicted >= iThresKickHp ||
						   player->m_iTeamKillsInflicted >= iThresKickKills) &&
					!player->m_bIsPendingTKKick)
			{
				userIDsToKick[userIDsToKickSize++] = player->GetUserID();
				player->m_bIsPendingTKKick = true;
			}
		}

		for (int i = 0; i < userIDsToKickSize; ++i)
		{
			engine->ServerCommand(UTIL_VarArgs("kickid %d \"%s\"\n", userIDsToKick[i],
											   "Too much friendly-fire damage inflicted."));
		}

		m_flPrevThinkKick = gpGlobals->curtime;
	}

	if (IsRoundOver())
	{
		// If the next round was not scheduled yet
		if (m_flNeoNextRoundStartTime == 0)
		{
			m_flNeoNextRoundStartTime = gpGlobals->curtime + mp_chattime.GetFloat();
			DevMsg("Round is over\n");

			m_pGhost = nullptr;
			m_iGhosterTeam = TEAM_UNASSIGNED;
			m_iGhosterPlayer = 0;
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
		if (GetGameType() == NEO_GAME_TYPE_TDM)
		{
			if (GetGlobalTeam(TEAM_JINRAI)->GetScore() > GetGlobalTeam(TEAM_NSF)->GetScore())
			{
				SetWinningTeam(TEAM_JINRAI, NEO_VICTORY_POINTS, false, true, false, false);
				return;
			}

			if (GetGlobalTeam(TEAM_NSF)->GetScore() > GetGlobalTeam(TEAM_JINRAI)->GetScore())
			{
				SetWinningTeam(TEAM_NSF, NEO_VICTORY_POINTS, false, true, false, false);
				return;
			}
		}
		SetWinningTeam(TEAM_SPECTATOR, NEO_VICTORY_STALEMATE, false, false, true, false);
	}

	if (m_pGhost)
	{
		// Update ghosting team info
		int nextGhosterTeam = TEAM_UNASSIGNED;
		int nextGhosterPlayerIdx = 0;
		CNEO_Player *pGhosterPlayer = static_cast<CNEO_Player *>(m_pGhost->GetOwner());
		if (pGhosterPlayer)
		{
			nextGhosterTeam = pGhosterPlayer->GetTeamNumber();
			nextGhosterPlayerIdx = pGhosterPlayer->entindex();
			Assert(nextGhosterTeam == TEAM_JINRAI || nextGhosterTeam == TEAM_NSF);
		}
		m_iGhosterTeam = nextGhosterTeam;
		m_iGhosterPlayer = nextGhosterPlayerIdx;

		Assert(UTIL_IsValidEntity(m_pGhost));

		if (m_pGhost->GetAbsOrigin().IsValid())
		{
			// Someone's carrying it, center at their body
			m_vecGhostMarkerPos = (pGhosterPlayer && (nextGhosterTeam == TEAM_JINRAI || nextGhosterTeam == TEAM_NSF)) ?
						pGhosterPlayer->EyePosition() : m_pGhost->GetAbsOrigin();
		}
		else
		{
			Assert(false);
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

				if (m_iEscortingTeam && m_iEscortingTeam == captorTeam)
				{
					break;
				}

				for (int i = 1; i <= gpGlobals->maxClients; i++)
				{
					if (i == captorClient)
					{
						AwardRankUp(i);
						continue;
					}

					auto player = UTIL_PlayerByIndex(i);
					if (player && player->GetTeamNumber() == captorTeam)
					{
						if (player->IsAlive())
						{
							AwardRankUp(i);
						}
						else
						{
							auto* neoPlayer = static_cast<CNEO_Player*>(player);
							neoPlayer->m_iXP.GetForModify()++;
						}
					}
				}

				break;
			}
		}
	}

	if (GetGameType() == NEO_GAME_TYPE_VIP && m_nRoundStatus == NeoRoundStatus::RoundLive && !m_pGhost)
	{
		if (!m_pVIP)
		{
			if (sv_neo_vip_ctg_on_death.GetBool())
			{
				UTIL_CenterPrintAll("HVT down, recover the Ghost");
				SpawnTheGhost();
			}
			else
			{
				// Assume vip player disconnected, forfeit round
				SetWinningTeam(GetOpposingTeam(m_iEscortingTeam), NEO_VICTORY_FORFEIT, false, true, false, false);
			}
		}

		if (!m_pVIP->IsAlive())
		{
			if (sv_neo_vip_ctg_on_death.GetBool())
			{
				UTIL_CenterPrintAll("HVT down, recover the Ghost");
				SpawnTheGhost(&m_pVIP->GetAbsOrigin());
			}
			else
			{
				// VIP was killed, end round
				SetWinningTeam(GetOpposingTeam(m_iEscortingTeam), NEO_VICTORY_VIP_ELIMINATION, false, true, false, false);
			}
		}

		// Check if the vip was escorted during this Think
		int captorTeam, captorClient;
		for (int i = 0; i < m_pGhostCaps.Count(); i++)
		{
			auto pGhostCap = dynamic_cast<CNEOGhostCapturePoint*>(UTIL_EntityByIndex(m_pGhostCaps[i]));
			if (!pGhostCap)
			{
				Assert(false);
				continue;
			}

			// If vip was escorted
			if (pGhostCap->IsGhostCaptured(captorTeam, captorClient))
			{
				// Turn off all capzones
				for (int i = 0; i < m_pGhostCaps.Count(); i++)
				{
					auto pGhostCap = dynamic_cast<CNEOGhostCapturePoint*>(UTIL_EntityByIndex(m_pGhostCaps[i]));
					pGhostCap->SetActive(false);
				}

				// And then announce team victory
				SetWinningTeam(captorTeam, NEO_VICTORY_VIP_ESCORT, false, true, false, false);

				for (int i = 1; i <= gpGlobals->maxClients; i++)
				{
					if (i == captorClient)
					{
						AwardRankUp(i);
						continue;
					}

					auto player = UTIL_PlayerByIndex(i);
					if (player && player->GetTeamNumber() == captorTeam)
					{
						if (player->IsAlive())
						{
							AwardRankUp(i);
						}
						else
						{
							auto* neoPlayer = static_cast<CNEO_Player*>(player);
							neoPlayer->m_iXP.GetForModify()++;
						}
					}
				}

				break;
			}
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
			if (GetGameType() != NEO_GAME_TYPE_TDM)
			{
				for (int team = TEAM_JINRAI; team <= TEAM_NSF; ++team)
				{
					if (GetGlobalTeam(team)->GetAliveMembers() == 0)
					{
						SetWinningTeam(GetOpposingTeam(team), NEO_VICTORY_TEAM_ELIMINATION, false, true, false, false);
					}
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
float CNEORules::GetRoundRemainingTime() const
{
	if (m_nRoundStatus == NeoRoundStatus::Idle)
	{
		return 0;
	}

	float roundTimeLimit = 0.f;
	if (m_nRoundStatus == NeoRoundStatus::Warmup)
	{
		roundTimeLimit = mp_neo_warmup_round_time.GetFloat();
	}
	else
	{
		switch (m_nGameTypeSelected) {
			case NEO_GAME_TYPE_TDM:
				roundTimeLimit = neo_tdm_round_timelimit.GetFloat() * 60.f;
				break;
			case NEO_GAME_TYPE_CTG:
				roundTimeLimit = neo_ctg_round_timelimit.GetFloat() * 60.f;
				break;
			case NEO_GAME_TYPE_VIP:
				roundTimeLimit = neo_vip_round_timelimit.GetFloat() * 60.f;
				break;
			default:
				break;
		}
	}

	return (m_flNeoRoundStartTime + roundTimeLimit) - gpGlobals->curtime;
}

float CNEORules::GetRoundAccumulatedTime() const
{
	return gpGlobals->curtime - (m_flNeoRoundStartTime + mp_neo_preround_freeze_time.GetFloat());
}

#ifdef GAME_DLL
float CNEORules::MirrorDamageMultiplier() const
{
	if (m_nRoundStatus != NeoRoundStatus::RoundLive)
	{
		return 0.0f;
	}
	const float flAccTime = GetRoundAccumulatedTime();
	const float flMirrorMult = neo_sv_mirror_teamdamage_multiplier.GetFloat();
	const float flMirrorDur = neo_sv_mirror_teamdamage_duration.GetFloat();
	return (flMirrorDur == 0.0f || (0.0f <= flAccTime && flAccTime < flMirrorDur)) ? flMirrorMult : 0.0f;
}
#endif

void CNEORules::FireGameEvent(IGameEvent* event)
{
	const char *type = event->GetName();

	if (Q_strcmp(type, "round_start") == 0)
	{
#ifdef CLIENT_DLL
		engine->ClientCmd("r_cleardecals");
		engine->ClientCmd("classmenu");
#endif
		m_flNeoRoundStartTime = gpGlobals->curtime;
		m_flNeoNextRoundStartTime = 0;
	}
}

#ifdef GAME_DLL
// Purpose: Spawns one ghost at a randomly chosen Neo ghost spawn point.
void CNEORules::SpawnTheGhost(const Vector *origin)
{
	CBaseEntity* pEnt;

	// Get the amount of ghost spawns available to us
	int numGhostSpawns = 0;

	pEnt = gEntList.FirstEnt();
	while (pEnt)
	{
		if (dynamic_cast<CNEOGhostSpawnPoint *>(pEnt))
		{
			numGhostSpawns++;
		}
		else if (auto *ghost = dynamic_cast<CWeaponGhost *>(pEnt))
		{
			m_pGhost = ghost;
		}

		pEnt = gEntList.NextEnt(pEnt);
	}

	// No ghost spawns and this map isn't named "_ctg". Probably not a CTG map.
	if (numGhostSpawns == 0 && (V_stristr(GameRules()->MapName(), "_ctg") == 0))
	{
		m_pGhost = nullptr;
		return;
	}

	bool spawnedGhostNow = false;
	if (!m_pGhost)
	{
		m_pGhost = dynamic_cast<CWeaponGhost *>(CreateEntityByName("weapon_ghost", -1));
		if (!m_pGhost)
		{
			Assert(false);
			Warning("Failed to spawn a new ghost\n");
			return;
		}

		const int dispatchRes = DispatchSpawn(m_pGhost);
		if (dispatchRes != 0)
		{
			Assert(false);
			return;
		}

		m_pGhost->NetworkStateChanged();
		spawnedGhostNow = true;
	}
	m_bGhostExists = true;

	Assert(UTIL_IsValidEntity(m_pGhost));

	if (origin)
	{
		if (m_pGhost->GetOwner())
		{
			Assert(false);
			m_pGhost->GetOwner()->Weapon_Detach(m_pGhost);
		}

		m_pGhost->SetAbsOrigin(*origin);
		m_pGhost->Drop(Vector{ 0.0f, 0.0f, 0.0f });
	}
	// We didn't have any spawns, spawn ghost at origin
	else if (numGhostSpawns == 0)
	{
		Warning("No ghost spawns found! Spawning ghost at map origin, instead.\n");
		m_pGhost->SetAbsOrigin(vec3_origin);
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
					if (m_pGhost->GetOwner())
					{
						Assert(false);
						m_pGhost->GetOwner()->Weapon_Detach(m_pGhost);
					}

					if (!ghostSpawn->GetAbsOrigin().IsValid())
					{
						m_pGhost->SetAbsOrigin(vec3_origin);
						Warning("Failed to get ghost spawn coords; spawning ghost at map origin instead!\n");
						Assert(false);
					}
					else
					{
						m_pGhost->SetAbsOrigin(ghostSpawn->GetAbsOrigin());
						m_pGhost->Drop(Vector{0.0f, 0.0f, 0.0f});
					}

					break;
				}
			}

			pEnt = gEntList.NextEnt(pEnt);
		}
	}

	DevMsg("%s ghost at coords:\n\t%.1f %.1f %.1f\n",
		   spawnedGhostNow ? "Spawned" : "Moved",
		   m_pGhost->GetAbsOrigin().x,
		   m_pGhost->GetAbsOrigin().y,
		   m_pGhost->GetAbsOrigin().z);
}

void CNEORules::SelectTheVIP()
{
	int eligibleForVIP[MAX_PLAYERS];
	int eligibleForVIPTop = -1;
	int sameTeamAsVIP[MAX_PLAYERS];
	int sameTeamAsVIPTop = -1;

	for (int i = 1; i <= gpGlobals->maxClients; i++)
	{
		if (CBasePlayer* pPlayer = static_cast<CBasePlayer*>(UTIL_EntityByIndex(i)))
		{
			if (pPlayer->GetTeamNumber() != m_iEscortingTeam)
				continue;
			sameTeamAsVIPTop++;
			sameTeamAsVIP[sameTeamAsVIPTop] = pPlayer->entindex();
			
			const bool clientWantsToBeVIP = engine->GetClientConVarValue(sameTeamAsVIP[sameTeamAsVIPTop], "neo_cl_vip_eligible");
			if (!clientWantsToBeVIP)
				continue;
			eligibleForVIPTop++;
			eligibleForVIP[eligibleForVIPTop] = pPlayer->entindex();
		}
	}

	if (eligibleForVIPTop >= 0)
	{
		m_pVIP = static_cast<CNEO_Player *>(UTIL_PlayerByIndex(eligibleForVIP[RandomInt(0, eligibleForVIPTop)]));
	}
	else if (sameTeamAsVIPTop >= 0)
	{
		m_pVIP = static_cast<CNEO_Player*>(UTIL_PlayerByIndex(sameTeamAsVIP[RandomInt(0, sameTeamAsVIPTop)]));
	}

	if (m_pVIP)
	{
		m_iVIPPreviousClass = m_pVIP->m_iNextSpawnClassChoice.Get() >= 0 ? m_pVIP->m_iNextSpawnClassChoice.Get() : m_pVIP->m_iNeoClass.Get();
		m_pVIP->m_iNeoClass.Set(NEO_CLASS_VIP);
		m_pVIP->m_iNextSpawnClassChoice.Set(NEO_CLASS_VIP);
		m_pVIP->RequestSetClass(NEO_CLASS_VIP);
		engine->ClientCommand(m_pVIP->edict(), "loadoutmenu");
		if (m_pVIP->IsFakeClient())
			m_pVIP->Respawn();
		return;
	}
	else
		Assert(false);
}

void CNEORules::GatherGameTypeVotes()
{
	int gameTypes[NEO_GAME_TYPE_TOTAL] = {};

	for (int i = 1; i <= gpGlobals->maxClients; i++)
	{
		if (CBasePlayer* pPlayer = static_cast<CBasePlayer*>(UTIL_PlayerByIndex(i)))
		{
			if (pPlayer->IsBot())
				continue;
			const char *clientGameTypeVote = engine->GetClientConVarValue(i, "neo_vote_game_mode");
			if (!clientGameTypeVote)
				continue;
			if (!clientGameTypeVote[0])
				continue;
			int gameType = atoi(clientGameTypeVote);
			gameTypes[gameType]++;
		}
	}

	int mostVotes = gameTypes[0];
	int mostPopularGameType = 0;
	for (int i = 1; i < NEO_GAME_TYPE_TOTAL; i++)
	{
		if (gameTypes[i] > mostVotes) // NEOTODO (Adam) Handle draws
		{
			mostVotes = gameTypes[i];
			mostPopularGameType = i;
		}
	}

	m_nGameTypeSelected = mostPopularGameType;
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

	if (m_nRoundStatus == NeoRoundStatus::Idle)
	{
		// NOTE (nullsystem): If it's a loopback server, then go straight in. Mostly ease for testing other stuff.
		bool loopbackSkipWarmup = false;
		if (!mp_neo_loopback_warmup_round.GetBool())
		{
			for (int i = 1; i <= gpGlobals->maxClients; i++)
			{
				if (auto* pPlayer = static_cast<CNEO_Player*>(UTIL_PlayerByIndex(i)))
				{
					const int teamNum = pPlayer->GetTeamNumber();
					if (!pPlayer->IsBot() && (teamNum == TEAM_JINRAI || teamNum == TEAM_NSF))
					{
						INetChannelInfo* nci = engine->GetPlayerNetInfo(i);
						loopbackSkipWarmup = nci->IsLoopback();
						if (!loopbackSkipWarmup)
						{
							break;
						}
					}
				}
			}
		}

		if (!loopbackSkipWarmup)
		{
			// Moving from 0 players from either team to playable at team state
			UTIL_CenterPrintAll("Warmup countdown started.\n");
			SetRoundStatus(NeoRoundStatus::Warmup);
			m_flNeoRoundStartTime = gpGlobals->curtime;
			m_flNeoNextRoundStartTime = gpGlobals->curtime + mp_neo_warmup_round_time.GetFloat();
			return;
		}
	}

	m_flNeoRoundStartTime = gpGlobals->curtime;
	m_flNeoNextRoundStartTime = 0;

	CleanUpMap();

	// NEO TODO (nullsystem): There should be a more sophisticated logic to be able to restore XP
	// for when moving from idle to preroundfreeze, or in the future, competitive with whatever
	// extra stuff in there. But to keep it simple: just clear if it was a warmup.
	const bool clearXP = (m_nRoundStatus == NeoRoundStatus::Warmup);
	SetRoundStatus(NeoRoundStatus::PreRoundFreeze);

	char RoundMsg[11];
	static_assert(sizeof(RoundMsg) == sizeof("Round 99\n\0"), "RoundMsg requires to fit round numbers up to 2 digits");
	V_sprintf_safe(RoundMsg, "Round %d\n", Min(99, ++m_iRoundNumber));
	UTIL_CenterPrintAll(RoundMsg);

	for (int i = 1; i <= gpGlobals->maxClients; i++)
	{
		CNEO_Player *pPlayer = (CNEO_Player*)UTIL_PlayerByIndex(i);

		if (!pPlayer)
		{
			continue;
		}

		pPlayer->m_bKilledInflicted = false;
		if (pPlayer->GetActiveWeapon())
		{
			pPlayer->GetActiveWeapon()->Holster();
		}
		pPlayer->RemoveAllItems(true);
		pPlayer->Spawn();
		if (gpGlobals->curtime < m_flNeoRoundStartTime + mp_neo_preround_freeze_time.GetFloat())
		{
			pPlayer->AddFlag(FL_GODMODE);
			pPlayer->AddNeoFlag(NEO_FL_FREEZETIME);
		}

		pPlayer->m_bInAim = false;
		pPlayer->m_bInThermOpticCamo = false;
		pPlayer->m_bInVision = false;
		pPlayer->m_bIneligibleForLoadoutPick = false;

		if (clearXP)
		{
			pPlayer->Reset();
			pPlayer->m_iXP.Set(0);
			pPlayer->m_iTeamDamageInflicted = 0;
			pPlayer->m_iTeamKillsInflicted = 0;
		}
		pPlayer->m_bIsPendingTKKick = false;

		pPlayer->SetTestMessageVisible(false);
	}

	m_flPrevThinkKick = 0.0f;
	m_flPrevThinkMirrorDmg = 0.0f;
	m_flIntermissionEndTime = 0;
	m_flRestartGameTime = 0;
	m_bCompleteReset = false;
	m_bTeamBeenAwardedDueToCapPrevent = false;
	V_memset(m_arrayiEntPrevCap, 0, sizeof(m_arrayiEntPrevCap));
	m_iEntPrevCapSize = 0;
	if (clearXP)
	{
		m_pRestoredInfos.Purge();
		// If game was in warmup then also decide on game mode here
	}

	FireLegacyEvent_NeoRoundEnd();

	if (!GetGameType() || sv_neo_change_game_type_mid_round.GetBool())
	{
		GatherGameTypeVotes();
	}

	SetGameRelatedVars();

	IGameEvent *event = gameeventmanager->CreateEvent("round_start");
	if (event)
	{
		event->SetInt("fraglimit", 0);
		event->SetInt("priority", 6); // HLTV event priority, not transmitted

		event->SetString("objective", "DEATHMATCH");

		gameeventmanager->FireEvent(event);
	}
	FireLegacyEvent_NeoRoundStart();

	DevMsg("New round start here!\n");
}
#endif

bool CNEORules::IsRoundPreRoundFreeze() const
{
	return m_nRoundStatus == NeoRoundStatus::PreRoundFreeze;
}

bool CNEORules::IsRoundOver() const
{
#ifdef GAME_DLL
	// We don't want to start preparing for a new round
	// if the game has ended for the current map.
	if (g_fGameOver)
	{
		return false;
	}
#endif

	// Next round start has been scheduled, so current round must be over.
	if (m_flNeoNextRoundStartTime != 0)
	{
		Assert((m_flNeoNextRoundStartTime < 0) == false);
		return true;
	}

	return false;
}

bool CNEORules::IsRoundLive() const
{
	return m_nRoundStatus == NeoRoundStatus::RoundLive;
}

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

const char *CNEORules::GetGameDescription(void)
{
	//DevMsg("Querying CNEORules game description\n");

	// NEO TODO (Rain): get a neo_game_config so we can specify better
	switch(GetGameType()) {
		case NEO_GAME_TYPE_TDM:
			return "Team Deathmatch";
		case NEO_GAME_TYPE_CTG:
			return "Capture the Ghost";
		case NEO_GAME_TYPE_VIP:
			return "Extract or Kill the VIP";
		default:
			return BaseClass::GetGameDescription();
	}
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

	ResetGhostCapPoints();
}

void CNEORules::CheckRestartGame()
{
	BaseClass::CheckRestartGame();
}

void CNEORules::PurgeGhostCapPoints()
{
	m_pGhostCaps.Purge();
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

void CNEORules::SetGameRelatedVars()
{
	ResetTDM();

	ResetGhost();
	if (GetGameType() == NEO_GAME_TYPE_CTG)
	{
		SpawnTheGhost();
	}

	ResetVIP();
	if (GetGameType() == NEO_GAME_TYPE_VIP)
	{
		if (!m_iEscortingTeam)
		{
			m_iEscortingTeam.Set(RandomInt(TEAM_JINRAI, TEAM_NSF));
		}
		else
		{
			m_iEscortingTeam.Set(m_iEscortingTeam.Get() == TEAM_JINRAI ? TEAM_NSF : TEAM_JINRAI);
		}

		SelectTheVIP();
	}
	else
	{
		m_iEscortingTeam.Set(0);
	}

	if (GetGameType() == NEO_GAME_TYPE_TDM)
	{
		for (int i = 0; i < GetNumberOfTeams(); i++)
		{
			GetGlobalTeam(i)->SetScore(0);
		}
	}
}

void CNEORules::ResetTDM()
{
	for (int i = 0; i < GetNumberOfTeams(); i++)
	{
		GetGlobalTeam(i)->SetScore(0);
	}
}

void CNEORules::ResetGhost()
{
	m_pGhost = nullptr;
	m_bGhostExists = false;
	m_iGhosterTeam = TEAM_UNASSIGNED;
	m_iGhosterPlayer = 0;
}

void CNEORules::ResetVIP()
{
	if (!m_pVIP)
		return;
	
	const int nextClass = m_iVIPPreviousClass ? m_iVIPPreviousClass : NEO_CLASS_ASSAULT;
	m_pVIP->m_iNeoClass.Set(nextClass);
	m_pVIP->m_iNextSpawnClassChoice.Set(nextClass);
	m_pVIP->RequestSetClass(nextClass);

	engine->ClientCommand(m_pVIP->edict(), "classmenu");
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
		pPlayer->Spawn();
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

	ResetMapSessionCommon();

	GatherGameTypeVotes();

	SetGameRelatedVars();

	IGameEvent * event = gameeventmanager->CreateEvent("round_start");
	if (event)
	{
		event->SetInt("fraglimit", 0);
		event->SetInt("priority", 6); // HLTV event priority, not transmitted

		event->SetString("objective", "DEATHMATCH");

		gameeventmanager->FireEvent(event);
	}
	FireLegacyEvent_NeoRoundStart();
}
#endif

#ifdef GAME_DLL
bool CNEORules::ClientConnected(edict_t *pEntity, const char *pszName, const char *pszAddress, char *reject, int maxrejectlen)
{
	if (neo_sv_build_integrity_check.GetBool())
	{
		const char *clientGitHash = engine->GetClientConVarValue(engine->IndexOfEdict(pEntity), "__neo_cl_git_hash");
		const bool dbgBuildSkip = (neo_sv_build_integrity_check_allow_debug.GetBool()) ? (clientGitHash[0] & 0b1000'0000) : false;
		if (dbgBuildSkip)
		{
			DevWarning("Client debug build integrity check bypass! Client: %s | Server: %s\n", clientGitHash, GIT_LONGHASH);
		}
		// NEO NOTE (nullsystem): Due to debug builds, if we're to match exactly, we'll have to mask out final bit first
		char cmpClientGitHash[GIT_LONGHASH_SIZE + 1];
		V_strcpy_safe(cmpClientGitHash, clientGitHash);
		cmpClientGitHash[0] &= 0b0111'1111;
		if (!dbgBuildSkip && V_strcmp(cmpClientGitHash, GIT_LONGHASH))
		{
			// Truncate the git hash so it's short hash and doesn't make message too long
			static constexpr int MAX_GITHASH_SHOW = 7;
			V_snprintf(reject, maxrejectlen, "Build integrity failed! Client vs server mis-match: Check your neo_version. "
											 "Client: %.*s | Server: %.*s",
					   MAX_GITHASH_SHOW, cmpClientGitHash, MAX_GITHASH_SHOW, GIT_LONGHASH);
			return false;
		}
	}
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

bool CNEORules::ClientCommand(CBaseEntity* pEdict, const CCommand& args)
{
	if (auto* neoPlayer = dynamic_cast<CNEO_Player*>(pEdict))
	{
		if (neoPlayer->ClientCommand(args))
		{
			return true;
		}
	}

	if (BaseClass::ClientCommand(pEdict, args))
	{
		return true;
	}

	return false;
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
	if (auto fovOpt = StrToInt(engine->GetClientConVarValue(engine->IndexOfEdict(pNEOPlayer->edict()), "neo_fov")))
	{
		pNEOPlayer->SetDefaultFOV(*fovOpt);
		pNEOPlayer->Weapon_SetZoom(pNEOPlayer->m_bInAim);
	}

	const char *pszSteamName = engine->GetClientConVarValue(pPlayer->entindex(), "name");

	const bool clientAllowsNeoName = (0 == StrToInt(engine->GetClientConVarValue(engine->IndexOfEdict(pNEOPlayer->edict()), "cl_onlysteamnick")));
	const char *pszNeoName = engine->GetClientConVarValue(pNEOPlayer->entindex(), "neo_name");
	const char *pszOldNeoName = pNEOPlayer->GetNeoPlayerNameDirect();
	bool updateDupeCheck = false;

	// NEO NOTE (nullsystem): Dont notify if client is new (pszOldNeoName is NULL, server-only check)
	// only set a name, otherwise message everyone if someone changes their neo_name
	if (pszOldNeoName == NULL || Q_strcmp(pszOldNeoName, pszNeoName))
	{
		if (pszOldNeoName != NULL && clientAllowsNeoName)
		{
			// This is basically player_changename but allows for client to filter it out with cl_onlysteamnick toggle
			IGameEvent *event = gameeventmanager->CreateEvent("player_changeneoname");
			if (event)
			{
				event->SetInt("userid", pNEOPlayer->GetUserID());
				event->SetString("oldname", (pszOldNeoName[0] == '\0') ? pszSteamName : pszOldNeoName);
				event->SetString("newname", (pszNeoName[0] == '\0') ? pszSteamName : pszNeoName);
				gameeventmanager->FireEvent(event);
			}
		}

		pNEOPlayer->SetNeoPlayerName(pszNeoName);
		updateDupeCheck = true;
	}
	pNEOPlayer->SetClientWantNeoName(clientAllowsNeoName);

	const char *pszName = pszSteamName;
	const char *pszOldName = pPlayer->GetPlayerName();

	// msg everyone if someone changes their name,  and it isn't the first time (changing no name to current name)
	// Note, not using FStrEq so that this is case sensitive
	if (Q_strcmp(pszOldName, pszName))
	{
		if (pszOldName[0] != 0)
		{
			if (!pszNeoName || pszNeoName[0] == '\0')
			{
				char text[256];
				Q_snprintf(text, sizeof(text), "%s changed name to %s\n", pszOldName, pszName);

				UTIL_ClientPrintAll(HUD_PRINTTALK, text);

				IGameEvent *event = gameeventmanager->CreateEvent("player_changename");
				if (event)
				{
					event->SetInt("userid", pPlayer->GetUserID());
					event->SetString("oldname", pszOldName);
					event->SetString("newname", pszName);
					gameeventmanager->FireEvent(event);
				}
			}

			pPlayer->SetPlayerName(pszName);
		}
		updateDupeCheck = true;
	}

	if (updateDupeCheck)
	{
		// Update name duplication checker (only used if cl_onlysteamnick=0/neo_name is used, but always set)
		KeyValues *dupeData = new KeyValues("dupeData");
		for (int i = 1; i <= gpGlobals->maxClients; ++i)
		{
			auto neoPlayer = static_cast<CNEO_Player *>(UTIL_PlayerByIndex(i));
			if (neoPlayer && !neoPlayer->IsHLTV())
			{
				int dupePos = 0;
				const char *neoPlayerName = neoPlayer->GetNeoPlayerName();

				if (neoPlayerName && neoPlayerName[0] != '\0')
				{
					const int namePos = dupeData->GetInt(neoPlayerName);
					dupePos = namePos;
					dupeData->SetInt(neoPlayerName, namePos + 1);
				}
				neoPlayer->SetNameDupePos(dupePos);
			}
		}
		dupeData->deleteThis();
	}

	// We're skipping calling the base CHL2MPRules method here
	CTeamplayRules::ClientSettingsChanged(pPlayer);
#endif
}

bool CNEORules::RoundIsInSuddenDeath() const
{
	auto teamJinrai = GetGlobalTeam(TEAM_JINRAI);
	auto teamNSF = GetGlobalTeam(TEAM_NSF);
	if (teamJinrai && teamNSF)
	{
		return (neo_round_limit.GetInt() != 0 && (m_iRoundNumber > neo_round_limit.GetInt()) && teamJinrai->GetRoundsWon() == teamNSF->GetRoundsWon());
	}
	return false;
}

bool CNEORules::RoundIsMatchPoint() const
{
	auto teamJinrai = GetGlobalTeam(TEAM_JINRAI);
	auto teamNSF = GetGlobalTeam(TEAM_NSF);
	if (teamJinrai && teamNSF && neo_round_limit.GetInt() != 0)
	{
		if (RoundIsInSuddenDeath()) return false;
		const int roundDiff = neo_round_limit.GetInt() - (m_iRoundNumber - 1);
		if ((teamJinrai->GetRoundsWon() + 1) > (teamNSF->GetRoundsWon() + roundDiff)) return true;
		if ((teamNSF->GetRoundsWon() + 1) > (teamJinrai->GetRoundsWon() + roundDiff)) return true;
		return false;
	}
	return false;
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
	if (!winningTeam)
	{
		Assert(false);
		Warning("Tried to SetWinningTeam for NULL team (%d)\n", team);
		return;
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
			winningTeam->IncrementRoundsWon();
		}
	}

	char victoryMsg[128];
	bool gotMatchWinner = false;
	bool isSuddenDeath = false;

	if (!bForceMapReset)
	{
		auto teamJinrai = GetGlobalTeam(TEAM_JINRAI);
		auto teamNSF = GetGlobalTeam(TEAM_NSF);

		if (neo_score_limit.GetInt() != 0)
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

		// If a hard round limit is set, end the game and show the team
		// that won with the most score, sudden death, or tie out
		if (neo_round_limit.GetInt() != 0 && teamJinrai && teamNSF)
		{
			// If there's a round limit and the other team cannot really catch up with the
			// winning team, then end the match early.
			bool earlyWin = false;
			if (!RoundIsInSuddenDeath())
			{
				const int roundDiff = neo_round_limit.GetInt() - (m_iRoundNumber - 1);
				earlyWin = (earlyWin || (teamJinrai->GetRoundsWon() > (teamNSF->GetRoundsWon() + roundDiff)));
				earlyWin = (earlyWin || (teamNSF->GetRoundsWon() > (teamJinrai->GetRoundsWon() + roundDiff)));
			}
			if (earlyWin)
			{
				V_sprintf_safe(victoryMsg, "Team %s wins the match!\n",
						((teamJinrai->GetRoundsWon() > teamNSF->GetRoundsWon()) ? "Jinrai" : "NSF"));
				gotMatchWinner = true;
			}
			else if (m_iRoundNumber >= neo_round_limit.GetInt())
			{
				if (teamJinrai->GetRoundsWon() == teamNSF->GetRoundsWon())
				{
					// Sudden death - Don't end the match until we get a winning team
					if (neo_round_sudden_death.GetBool())
					{
						V_sprintf_safe(victoryMsg, "Next round: Sudden death!\n");
						isSuddenDeath = true;
					}
					else
					{
						V_sprintf_safe(victoryMsg, "The match is tied!\n");
						gotMatchWinner = true;
					}
				}
				else
				{
					V_sprintf_safe(victoryMsg, "Team %s wins the match!\n", ((teamJinrai->GetRoundsWon() > teamNSF->GetRoundsWon()) ? "Jinrai" : "NSF"));
					gotMatchWinner = true;
				}
			}
		}
	}

	if (!gotMatchWinner && !isSuddenDeath)
	{
		switch (iWinReason) {
		case NEO_VICTORY_GHOST_CAPTURE:
			V_sprintf_safe(victoryMsg, "Team %s wins by capturing the ghost!\n", (team == TEAM_JINRAI ? "Jinrai" : "NSF"));
			break;
		case NEO_VICTORY_VIP_ESCORT:
			V_sprintf_safe(victoryMsg, "Team %s wins by escorting the vip!\n", (team == TEAM_JINRAI ? "Jinrai" : "NSF"));
			break;
		case NEO_VICTORY_VIP_ELIMINATION:
			V_sprintf_safe(victoryMsg, "Team %s wins by eliminating the vip!\n", (team == TEAM_JINRAI ? "Jinrai" : "NSF"));
			break;
		case NEO_VICTORY_TEAM_ELIMINATION:
			V_sprintf_safe(victoryMsg, "Team %s wins by eliminating the other team!\n", (team == TEAM_JINRAI ? "Jinrai" : "NSF"));
			break;
		case NEO_VICTORY_TIMEOUT_WIN_BY_NUMBERS:
			V_sprintf_safe(victoryMsg, "Team %s wins by numbers!\n", (team == TEAM_JINRAI ? "Jinrai" : "NSF"));
			break;
		case NEO_VICTORY_POINTS:
			V_sprintf_safe(victoryMsg, "Team %s wins by highest score!\n", (team == TEAM_JINRAI ? "Jinrai" : "NSF"));
			break;
		case NEO_VICTORY_FORFEIT:
			V_sprintf_safe(victoryMsg, "Team %s wins by forfeit!\n", (team == TEAM_JINRAI ? "Jinrai" : "NSF"));
			break;
		case NEO_VICTORY_STALEMATE:
			V_sprintf_safe(victoryMsg, "TIE\n");
			break;
		default:
			V_sprintf_safe(victoryMsg, "Unknown Neotokyo victory reason %i\n", iWinReason);
			Warning("%s", victoryMsg);
			Assert(false);
			break;
		}
	}

	CRecipientFilter filter;
	filter.AddAllPlayers();
	UserMessageBegin(filter, "RoundResult");
	WRITE_STRING(team == TEAM_JINRAI ? "jinrai" : team == TEAM_NSF ? "nsf" : "tie");	// which team won
	WRITE_FLOAT(gpGlobals->curtime);													// when did they win
	WRITE_STRING(victoryMsg);															// extra message (who capped or last kill or who got the most points or whatever)
	MessageEnd();

	EmitSound_t soundParams;
	soundParams.m_nChannel = CHAN_AUTO;
	soundParams.m_SoundLevel = SNDLVL_NONE;
	soundParams.m_flVolume = 0.33f;
	// Referencing sounds directly because we can't override the soundscript volume level otherwise
	soundParams.m_pSoundName = (team == TEAM_JINRAI) ? "gameplay/jinrai.mp3" : (team == TEAM_NSF) ? "gameplay/nsf.mp3" : "gameplay/draw.mp3";
	soundParams.m_bWarnOnDirectWaveReference = false;
	soundParams.m_bEmitCloseCaption = false;

	const int winningTeamNum = winningTeam->GetTeamNumber();
	int iRankupCapPrev = 0;

	for (int i = 1; i <= gpGlobals->maxClients; ++i)
	{
		CBasePlayer* basePlayer = UTIL_PlayerByIndex(i);
		auto player = static_cast<CNEO_Player*>(basePlayer);
		if (player)
		{
			if (!player->IsBot() || player->IsHLTV())
			{
				/*engine->ClientPrintf(player->edict(), victoryMsg);
				UTIL_ClientPrintAll((gotMatchWinner ? HUD_PRINTTALK : HUD_PRINTCENTER), victoryMsg);*/

				const char* volStr = engine->GetClientConVarValue(i, snd_victory_volume.GetName());
				const float jingleVolume = volStr ? atof(volStr) : 0.33f;
				soundParams.m_flVolume = jingleVolume;

				CRecipientFilter soundFilter;
				soundFilter.AddRecipient(basePlayer);
				soundFilter.MakeReliable();
				player->EmitSound(soundFilter, i, soundParams);
			}

			// Ghost-caps and VIP-escorts are handled separately
			if (iWinReason != NEO_VICTORY_GHOST_CAPTURE && iWinReason != NEO_VICTORY_VIP_ESCORT && player->GetTeamNumber() == winningTeamNum)
			{
				int xpAward = 1;	// Base reward for being on winning team
				if (player->IsAlive())
				{
					if (m_bTeamBeenAwardedDueToCapPrevent)
					{
						AwardRankUp(player);
						xpAward = 0; // Already been rewarded rank-up XPs
						++iRankupCapPrev;
					}
					else
					{
						++xpAward;
						xpAward += static_cast<int>(player->IsCarryingGhost());
					}
				}
				player->m_iXP.GetForModify() += xpAward;
			}

			// Any human player still alive, show them damage stats in round end
			if (!player->IsBot() && !player->IsHLTV() && player->IsAlive())
			{
				player->StartShowDmgStats(NULL);
			}
		}
	}

	if (m_bTeamBeenAwardedDueToCapPrevent && iWinReason != NEO_VICTORY_GHOST_CAPTURE)
	{
		UTIL_ClientPrintAll(HUD_PRINTTALK, "Last player of %s1 suicided vs. ghost carrier; awarding capture to team %s2.",
							(team == TEAM_JINRAI ? "NSF" : "Jinrai"), (team == TEAM_JINRAI ? "Jinrai" : "NSF"));
		char szHudChatPrint[42];
		V_sprintf_safe(szHudChatPrint, "Awarding capture rank-up to %d player%s.",
					   iRankupCapPrev, iRankupCapPrev == 1 ? "" : "s");
		UTIL_ClientPrintAll(HUD_PRINTTALK, szHudChatPrint);
	}

	if (gotMatchWinner)
	{
		GoToIntermission();
	}
}
#endif

// NEO JANK (nullsystem): Dont like how this is fetched twice (PlayerKilled, DeathNotice),
// but blame the structure of the base classes
static CNEO_Player* FetchAssists(CNEO_Player* attacker, CNEO_Player* victim)
{
	// Non-CNEO_Player, return NULL
	if (!attacker || !victim)
	{
		return NULL;
	}

	// Check for assistance (> 50 dmg, not final attacker)
	const int attackerIdx = attacker->entindex();
	for (int assistIdx = 1; assistIdx <= gpGlobals->maxClients; ++assistIdx)
	{
		if (assistIdx == attackerIdx)
		{
			continue;
		}

		const int assistDmg = victim->GetAttackersScores(assistIdx);
		static const int MIN_DMG_QUALIFY_ASSIST = 50;
		if (assistDmg >= MIN_DMG_QUALIFY_ASSIST)
		{
			return static_cast<CNEO_Player*>(UTIL_PlayerByIndex(assistIdx));
		}
	}
	return NULL;
}

#ifdef GAME_DLL
void CNEORules::CheckIfCapPrevent(CNEO_Player *capPreventerPlayer)
{
	// If this is the only player alive left before the suicide/disconnect and the other team was holding
	// the ghost, reward the other team an XP to the next rank as a ghost cap was prevented.
	const bool bShouldCheck = (neo_sv_suicide_prevent_cap_punish.GetBool()
							   && m_nRoundStatus == NeoRoundStatus::RoundLive
							   && !m_bTeamBeenAwardedDueToCapPrevent);
	if (!bShouldCheck)
	{
		return;
	}

	bool bOtherTeamPlayingGhost = false;
	int iTallyAlive[TEAM__TOTAL] = {};
	const int iPreventerTeam = capPreventerPlayer->GetTeamNumber();
	// Sanity check: Make sure it's only Jinrai/NSF players
	const bool bValidTeam = iPreventerTeam == TEAM_JINRAI || iPreventerTeam == TEAM_NSF;
	Assert(bValidTeam);
	if (!bValidTeam)
	{
		return;
	}

	const int iCapPreventerEntIdx = capPreventerPlayer->entindex();

	// Sanity check: Prevent duplication just in-case
	bool bContainsEntIdx = false;
	for (int i = 0; !bContainsEntIdx && i < m_iEntPrevCapSize; ++i)
	{
		bContainsEntIdx = (m_arrayiEntPrevCap[i] == iCapPreventerEntIdx);
	}
	if (!bContainsEntIdx) m_arrayiEntPrevCap[m_iEntPrevCapSize++] = iCapPreventerEntIdx;

	for (int i = 1; i <= gpGlobals->maxClients; ++i)
	{
		auto *player = static_cast<CNEO_Player*>(UTIL_PlayerByIndex(i));
		if (!player || player->entindex() == iCapPreventerEntIdx)
		{
			continue;
		}

		const int iPlayerTeam = player->GetTeamNumber();
		iTallyAlive[iPlayerTeam] += player->IsAlive();
		if (iPlayerTeam != iPreventerTeam && player->IsCarryingGhost())
		{
			bOtherTeamPlayingGhost = true;
		}
	}

	const int iOppositeTeam = (iPreventerTeam == TEAM_JINRAI) ? TEAM_NSF : TEAM_JINRAI;
	m_bTeamBeenAwardedDueToCapPrevent = (bOtherTeamPlayingGhost &&
										 iTallyAlive[iPreventerTeam] == 0 && iTallyAlive[iOppositeTeam] > 0);
}
#endif

void CNEORules::PlayerKilled(CBasePlayer *pVictim, const CTakeDamageInfo &info)
{
	BaseClass::PlayerKilled(pVictim, info);

	auto attacker = dynamic_cast<CNEO_Player*>(info.GetAttacker());
	auto victim = dynamic_cast<CNEO_Player*>(pVictim);
	auto grenade = dynamic_cast<CBaseGrenade *>(info.GetInflictor());

	if (!victim)
	{
		return;
	}

	// Suicide or suicide by environment (non-grenade as grenade is likely from a player)
	if (attacker == victim || (!attacker && !grenade))
	{
		victim->m_iXP.GetForModify() -= 1;
#ifdef GAME_DLL
		CheckIfCapPrevent(victim);
#endif
	}
#ifdef GAME_DLL
	else if (!attacker && grenade && grenade->GetTeamNumber() == victim->GetTeamNumber())
	{
		// Death by own team's grenade, but the player is already disconnected. Check for cap prevent.
		CheckIfCapPrevent(victim);
	}
#endif
	else if (attacker)
	{
		// Team kill
		if (attacker->GetTeamNumber() == victim->GetTeamNumber())
		{
			attacker->m_iXP.GetForModify() -= 1;
#ifdef GAME_DLL
			if (neo_sv_teamdamage_kick.GetBool() && m_nRoundStatus == NeoRoundStatus::RoundLive)
			{
				++attacker->m_iTeamKillsInflicted;
			}

			for (int i = 0; i < m_iEntPrevCapSize; ++i)
			{
				if (m_arrayiEntPrevCap[i] == attacker->entindex())
				{
					// Posthumous teamkill to prevent ghost cap scenario:
					// Player-A throws nade at Player-B, Player-A suicides right after,
					// Player-B gets killed from the nade - This dodges the general case
					// as Player-A is not the final player, but it was Player-A's intention
					// to prevent the ghost cap.
					CheckIfCapPrevent(victim);
					break;
				}
			}
#endif
		}
		// Enemy kill
		else
		{
			attacker->m_iXP.GetForModify() += 1;
		}

		if (auto *assister = FetchAssists(attacker, victim))
		{
			// Team kill assist
			if (assister->GetTeamNumber() == victim->GetTeamNumber())
			{
				assister->m_iXP.GetForModify() -= 1;
			}
			// Enemy kill assist
			else
			{
				assister->m_iXP.GetForModify() += 1;
			}
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
			case TEAM_JINRAI: return FMT_TEAM_JINRAI " " FMT_PLAYERNAME ": " FMT_CHATMESSAGE;
			case TEAM_NSF: return FMT_TEAM_NSF " " FMT_PLAYERNAME ": " FMT_CHATMESSAGE;
			case TEAM_SPECTATOR: return FMT_TEAM_SPECTATOR " " FMT_PLAYERNAME ": " FMT_CHATMESSAGE;
			default: return FMT_TEAM_UNASSIGNED " " FMT_PLAYERNAME ": " FMT_CHATMESSAGE;
			}
		}
		else
		{
			switch (pPlayer->GetTeamNumber())
			{
			case TEAM_JINRAI: return FMT_PLAYERNAME ": " FMT_CHATMESSAGE;
			case TEAM_NSF: return FMT_PLAYERNAME ": " FMT_CHATMESSAGE;
			case TEAM_SPECTATOR: return FMT_PLAYERNAME ": " FMT_CHATMESSAGE;
			default: return FMT_PLAYERNAME ": " FMT_CHATMESSAGE;
			}
		}
	}
	else
	{
		if (bTeamOnly)
		{
			switch (pPlayer->GetTeamNumber())
			{
			case TEAM_JINRAI: return FMT_DEAD FMT_TEAM_JINRAI " " FMT_PLAYERNAME ": " FMT_CHATMESSAGE;
			case TEAM_NSF: return FMT_DEAD FMT_TEAM_NSF " " FMT_PLAYERNAME ": " FMT_CHATMESSAGE;
			case TEAM_SPECTATOR: return FMT_TEAM_SPECTATOR " " FMT_PLAYERNAME ": " FMT_CHATMESSAGE;
			default: return FMT_TEAM_UNASSIGNED " " FMT_PLAYERNAME ": " FMT_CHATMESSAGE;
			}
		}
		else
		{
			switch (pPlayer->GetTeamNumber())
			{
			case TEAM_JINRAI: return FMT_DEAD " " FMT_PLAYERNAME ": " FMT_CHATMESSAGE;
			case TEAM_NSF: return FMT_DEAD " " FMT_PLAYERNAME ": " FMT_CHATMESSAGE;
			case TEAM_SPECTATOR: return FMT_PLAYERNAME ": " FMT_CHATMESSAGE;
			default: return FMT_PLAYERNAME ": " FMT_CHATMESSAGE;
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
	CBasePlayer* pAssister = FetchAssists(dynamic_cast<CNEO_Player*>(pKiller), dynamic_cast<CNEO_Player*>(pVictim));
	const int assists_ID = (pAssister) ? pAssister->GetUserID() : 0;

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
		event->SetInt("assists", assists_ID);
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
			// NEO JANK (nullsystem): Teleport so that disconnected player don't noclips the ghost?
			Vector stillVec{0.0f, 0.0f, 0.0f};
			QAngle angles;
			ghost->Drop(stillVec);
			pNeoPlayer->Weapon_Detach(ghost);
			Vector origin = ghost->GetAbsOrigin();
			ghost->Teleport(&origin, &angles, NULL);
			ghost->SetMoveType(MOVETYPE_FLYGRAVITY);
		}
		pNeoPlayer->RemoveAllWeapons();

		// Save XP/death counts
		if (neo_sv_player_restore.GetBool())
		{
			const CSteamID steamID = GetSteamIDForPlayerIndex(pNeoPlayer->entindex());
			if (steamID.IsValid())
			{
				const auto accountID = steamID.GetAccountID();
				const RestoreInfo restoreInfo{
					.xp = pNeoPlayer->m_iXP.Get(),
					.deaths = pNeoPlayer->DeathCount(),
				};

				if (restoreInfo.xp == 0 && restoreInfo.deaths == 0)
				{
					m_pRestoredInfos.Remove(accountID);
				}
				else
				{
					bool didInsert = false;
					const auto hdl = m_pRestoredInfos.Insert(accountID, restoreInfo, &didInsert);
					if (!didInsert)
					{
						// It already exists, so assign it instead
						m_pRestoredInfos[hdl] = restoreInfo;
					}
				}
			}
		}

		// Check if this is done to prevent ghost cap
		CheckIfCapPrevent(pNeoPlayer);
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
	else if (gameType != NEO_GAME_TYPE_CTG && gameType != NEO_GAME_TYPE_VIP)
	{
		Assert(false);
		return true;
	}

	// Mode is CTG
	auto jinrai = GetGlobalTeam(TEAM_JINRAI);
	auto nsf = GetGlobalTeam(TEAM_NSF);

	if (jinrai && nsf)
	{
		if (m_nRoundStatus == NeoRoundStatus::Warmup || m_nRoundStatus == NeoRoundStatus::Idle)
		{
			return true;
		}
	}
	else
	{
		Assert(false);
	}

	// Do not let anyone who tried to team-kill during mirror damage + live round to respawn
	if (static_cast<CNEO_Player *>(pPlayer)->m_bKilledInflicted)
	{
		return false;
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
	if (status == NeoRoundStatus::RoundLive || status == NeoRoundStatus::Idle || status == NeoRoundStatus::Warmup)
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
		if (status == NeoRoundStatus::RoundLive)
		{
			UTIL_CenterPrintAll("GO! GO! GO!\n");
		}
#endif
	}

	m_nRoundStatus = status;
}

NeoRoundStatus CNEORules::GetRoundStatus() const
{
	return static_cast<NeoRoundStatus>(m_nRoundStatus.Get());
}

int CNEORules::GetGameType(void)
{
	return m_nGameTypeSelected;
}

const char* CNEORules::GetGameTypeName(void)
{
	switch (GetGameType())
	{
	case NEO_GAME_TYPE_TDM:
		return "TDM";
	case NEO_GAME_TYPE_CTG:
		return "CTG";
	case NEO_GAME_TYPE_VIP:
		return "VIP";
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
