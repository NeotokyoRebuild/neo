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
#include "c_playerresource.h"
#include "engine/SndInfo.h"
#include "engine/IEngineSound.h"
#include "filesystem.h"
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
#include "neo_dm_spawn.h"
#include "neo_misc.h"
#include "neo_game_config.h"
#include "nav_mesh.h"

extern ConVar weaponstay;
#endif

ConVar sv_neo_loopback_warmup_round("sv_neo_loopback_warmup_round", "0", FCVAR_REPLICATED, "Allow loopback server to do warmup rounds.", true, 0.0f, true, 1.0f);
ConVar sv_neo_botsonly_warmup_round("sv_neo_botsonly_warmup_round", "0", FCVAR_REPLICATED, "Allow bots-only match to do warmup rounds.", true, 0.0f, true, 1.0f);
ConVar sv_neo_warmup_round_time("sv_neo_warmup_round_time", "45", FCVAR_REPLICATED, "The warmup round time, in seconds.", true, 0.0f, false, 0.0f);
ConVar sv_neo_preround_freeze_time("sv_neo_preround_freeze_time", "15", FCVAR_REPLICATED, "The pre-round freeze time, in seconds.", true, 0.0, false, 0);
ConVar sv_neo_latespawn_max_time("sv_neo_latespawn_max_time", "15", FCVAR_REPLICATED, "How many seconds late are players still allowed to spawn.", true, 0.0, false, 0);

ConVar sv_neo_wep_dmg_modifier("sv_neo_wep_dmg_modifier", "1", FCVAR_REPLICATED, "Temp global weapon damage modifier.", true, 0.0, true, 100.0);
ConVar sv_neo_player_restore("sv_neo_player_restore", "1", FCVAR_REPLICATED, "If enabled, the server will save players XP and deaths per match session and restore them if they reconnect.", true, 0.0f, true, 1.0f);

ConVar sv_neo_spraydisable("sv_neo_spraydisable", "0", FCVAR_REPLICATED, "If enabled, disables the players ability to spray.", true, 0.0f, true, 1.0f);

#ifdef CLIENT_DLL
ConVar neo_name("neo_name", "", FCVAR_USERINFO | FCVAR_ARCHIVE, "The nickname to set instead of the steam profile name.");
ConVar cl_onlysteamnick("cl_onlysteamnick", "0", FCVAR_USERINFO | FCVAR_ARCHIVE, "Only show players Steam names, otherwise show player set names.", true, 0.0f, true, 1.0f);

ConVar neo_clantag("neo_clantag", "", FCVAR_USERINFO | FCVAR_ARCHIVE, "The clantag to set.");
#endif
ConVar sv_neo_clantag_allow("sv_neo_clantag_allow", "1", FCVAR_REPLICATED, "", true, 0.0f, true, 1.0f);
#ifdef DEBUG
ConVar sv_neo_dev_test_clantag("sv_neo_dev_test_clantag", "", FCVAR_REPLICATED | FCVAR_CHEAT | FCVAR_DEVELOPMENTONLY, "Debug-mode only - Override all clantags with this value.");
#endif

#define STR_GAMEOPTS "TDM=0, CTG=1, VIP=2, DM=3"
#define STR_GAMEBWOPTS "TDM=1, CTG=2, VIP=4, DM=8"
#ifdef CLIENT_DLL
ConVar neo_vote_game_mode("neo_vote_game_mode", "1", FCVAR_USERINFO, "Vote on game mode to play. " STR_GAMEOPTS, true, 0, true, NEO_GAME_TYPE__TOTAL - 1);
ConVar neo_vip_eligible("cl_neo_vip_eligible", "1", FCVAR_ARCHIVE, "Eligible for VIP", true, 0, true, 1);
#endif // CLIENT_DLL
#ifdef GAME_DLL
ConVar sv_neo_vip_ctg_on_death("sv_neo_vip_ctg_on_death", "0", FCVAR_ARCHIVE, "Spawn Ghost when VIP dies, continue the game", true, 0, true, 1);
ConVar sv_neo_jgr_max_points("sv_neo_jgr_max_points", "100", FCVAR_ARCHIVE, "Maximum points required for a team to win in JGR", true, 1, false, 0);
#endif

#ifdef GAME_DLL
// NEO TODO (nullsystem): Change how voting done from convar to menu selection
enum eGamemodeEnforcement
{
	GAMEMODE_ENFORCEMENT_MAP = 0,	// Only use the gamemode enforced by the map
	GAMEMODE_ENFORCEMENT_SINGLE,	// Only use the single gamemode enforced by the server
	GAMEMODE_ENFORCEMENT_RAND,		// Randomly choose a gamemode on each map initialization based on a list
	GAMEMODE_ENFORCEMENT_VOTE,		// Allow vote by players on pre-match

	GAMEMODE_ENFORCEMENT__TOTAL,
};
ConVar sv_neo_gamemode_enforcement("sv_neo_gamemode_enforcement", "0", FCVAR_REPLICATED,
								   "How the gamemode are determined. 0 = By map, 1 = By sv_neo_gamemode_single, 2 = Random, 3 = Pre-match voting",
								   true, 0.0f, true, GAMEMODE_ENFORCEMENT__TOTAL - 1);
ConVar sv_neo_gamemode_single("sv_neo_gamemode_single", "3", FCVAR_REPLICATED, "The gamemode that is enforced by the server. " STR_GAMEOPTS,
							  true, 0.0f, true, NEO_GAME_TYPE__TOTAL - 1);
ConVar sv_neo_gamemode_random_allow("sv_neo_gamemode_random_allow", "11", FCVAR_REPLICATED,
									"In bitwise, the gamemodes that are allowed for random selection. Default = TDM+CTG+DM. " STR_GAMEBWOPTS,
									true, 1.0f, true, (1 << NEO_GAME_TYPE__TOTAL)); // Can't be zero, minimum has to set to a bitwise value
#endif

#ifdef GAME_DLL
#ifdef DEBUG
// Debug build by default relax integrity check requirement among debug clients
static constexpr char INTEGRITY_CHECK_DBG[] = "1";
#else
static constexpr char INTEGRITY_CHECK_DBG[] = "0";
#endif // DEBUG
ConVar sv_neo_build_integrity_check("sv_neo_build_integrity_check", "1", FCVAR_GAMEDLL | FCVAR_REPLICATED,
									"If enabled, the server checks the build's Git hash between the client and"
									" the server. If it doesn't match, the server rejects and disconnects the client.",
									true, 0.0f, true, 1.0f);
ConVar sv_neo_build_integrity_check_allow_debug("sv_neo_build_integrity_check_allow_debug", INTEGRITY_CHECK_DBG, FCVAR_GAMEDLL | FCVAR_REPLICATED,
									"If enabled, when the server checks the client hashes, it'll also allow debug"
									" builds which has a given special bit to bypass the check.",
									true, 0.0f, true, 1.0f);

#ifdef DEBUG
static constexpr char TEAMDMG_MULTI[] = "0";
#else
static constexpr char TEAMDMG_MULTI[] = "2";
#endif // DEBUG
ConVar sv_neo_mirror_teamdamage_multiplier("sv_neo_mirror_teamdamage_multiplier", TEAMDMG_MULTI, FCVAR_REPLICATED, "The damage multiplier given to the friendly-firing individual. Set value to 0 to disable mirror team damage.", true, 0.0f, true, 100.0f);
ConVar sv_neo_mirror_teamdamage_duration("sv_neo_mirror_teamdamage_duration", "7", FCVAR_REPLICATED, "How long in seconds the mirror damage is active for the start of each round. Set to 0 for the entire round.", true, 0.0f, true, 10000.0f);
ConVar sv_neo_mirror_teamdamage_immunity("sv_neo_mirror_teamdamage_immunity", "1", FCVAR_REPLICATED, "If enabled, the victim will not take damage from a teammate during the mirror team damage duration.", true, 0.0f, true, 1.0f);

ConVar sv_neo_teamdamage_kick("sv_neo_teamdamage_kick", "0", FCVAR_REPLICATED, "If enabled, the friendly-firing individual will be kicked if damage is received during the sv_neo_mirror_teamdamage_duration, exceeds the sv_neo_teamdamage_kick_hp value, or executes a teammate.", true, 0.0f, true, 1.0f);
ConVar sv_neo_teamdamage_kick_hp("sv_neo_teamdamage_kick_hp", "900", FCVAR_REPLICATED, "The threshold for the amount of HP damage inflicted on teammates before the client is kicked.", true, 100.0f, false, 0.0f);
ConVar sv_neo_teamdamage_kick_kills("sv_neo_teamdamage_kick_kills", "6", FCVAR_REPLICATED, "The threshold for the amount of team kills before the client is kicked.", true, 1.0f, false, 0.0f);
ConVar sv_neo_suicide_prevent_cap_punish("sv_neo_suicide_prevent_cap_punish", "1", FCVAR_REPLICATED,
										 "If enabled, if a player suicides and is the only one alive in their team, "
										 "while the other team is holding the ghost, reward the ghost holder team "
										 "a rank up.",
										 true, 0.0f, true, 1.0f);

#define DEF_TEAMPLAYERTHRES 5
static_assert(DEF_TEAMPLAYERTHRES <= ((MAX_PLAYERS - 1) / 2));
ConVar sv_neo_readyup_teamplayersthres("sv_neo_readyup_teamplayersthres", V_STRINGIFY(DEF_TEAMPLAYERTHRES), FCVAR_REPLICATED, "The exact total players per team to be in and ready up to start a game.", true, 0.0f, true, (MAX_PLAYERS - 1) / 2);
ConVar sv_neo_readyup_skipwarmup("sv_neo_readyup_skipwarmup", "1", FCVAR_REPLICATED, "Skip the warmup round when already using ready up.", true, 0.0f, true, 1.0f);
ConVar sv_neo_readyup_autointermission("sv_neo_readyup_autointermission", "0", FCVAR_REPLICATED, "If disabled, skips the automatic intermission at the end of the match.", true, 0.0f, true, 1.0f);
#endif // GAME_DLL

// Both CLIENT_DLL + GAME_DLL, but server-side setting so it's replicated onto client to read the values
ConVar sv_neo_readyup_lobby("sv_neo_readyup_lobby", "0", FCVAR_REPLICATED, "If enabled, players would need to ready up and match the players total requirements to start a game.", true, 0.0f, true, 1.0f);
ConVar sv_neo_pausematch_enabled("sv_neo_pausematch_enabled", "0", FCVAR_REPLICATED, "If enabled, players will be able to pause the match mid-game.", true, 0.0f, true, 1.0f);
ConVar sv_neo_pausematch_unpauseimmediate("sv_neo_pausematch_unpauseimmediate", "0", FCVAR_REPLICATED | FCVAR_CHEAT, "Testing only - If enabled, unpause will be immediate.", true, 0.0f, true, 1.0f);
ConVar sv_neo_readyup_countdown("sv_neo_readyup_countdown", "5", FCVAR_REPLICATED, "Set the countdown from fully ready to start of match in seconds.", true, 0.0f, true, 120.0f);
ConVar sv_neo_ghost_spawn_bias("sv_neo_ghost_spawn_bias", "0", FCVAR_REPLICATED, "Spawn ghost in the same location as the previous round on odd-indexed rounds (Round 1 = index 0)", true, 0, true, 1);
ConVar sv_neo_juggernaut_spawn_bias("sv_neo_juggernaut_spawn_bias", "0", FCVAR_REPLICATED, "Spawn juggernaut in the same location as the previous round on odd-indexed rounds (Round 1 = index 0)", true, 0, true, 1);

static void neoSvCompCallback(IConVar* var, const char* pOldValue, float flOldValue)
{
	const bool bCurrentValue = !(bool)flOldValue;
	sv_neo_readyup_lobby.SetValue(bCurrentValue);
	mp_forcecamera.SetValue(bCurrentValue); // 0 = OBS_ALLOWS_ALL, 1 = OBS_ALLOW_TEAM. For strictly original neotokyo spectator experience, 2 = OBS_ALLOW_NONE
	sv_neo_spraydisable.SetValue(bCurrentValue);
	sv_neo_pausematch_enabled.SetValue(bCurrentValue);
	sv_neo_ghost_spawn_bias.SetValue(bCurrentValue);
	sv_neo_juggernaut_spawn_bias.SetValue(bCurrentValue);
}

ConVar sv_neo_comp("sv_neo_comp", "0", FCVAR_REPLICATED, "Enables competitive gamerules", true, 0.f, true, 1.f
	#ifdef GAME_DLL
	, neoSvCompCallback
	#endif // GAME_DLL
);

#ifdef CLIENT_DLL
extern ConVar snd_victory_volume;
void sndVictoryVolumeChangeCallback(IConVar* cvar [[maybe_unused]], const char* pOldVal [[maybe_unused]], float flOldVal [[maybe_unused]])
{
	if (!g_pFullFileSystem)
	{
		Assert(false);
		return;
	}

#ifdef LINUX
#define DIR_SLASH "/"
#elif defined(_WIN32)
#define DIR_SLASH "\\"
#else
#error Unimplemented!
#endif
	const char* jingles[] = {
		"gameplay"	DIR_SLASH "draw.mp3",
		"gameplay"	DIR_SLASH "jinrai.mp3",
		"gameplay"	DIR_SLASH "nsf.mp3",
	};

	CUtlVector<SndInfo_t> sounds;
	enginesound->GetActiveSounds(sounds);

	// If we are playing a victory jingle, update its volume
	char filename[MAX_PATH];
	for (int i = 0; i < sounds.Size(); i++)
	{
		if (!g_pFullFileSystem->String(sounds[i].m_filenameHandle, filename, sizeof(filename)))
		{
			continue;
		}
		else if (!*filename)
		{
			continue;
		}

		for (int j = 0; j < ARRAYSIZE(jingles); ++j)
		{
			if (Q_strcmp(filename, jingles[j]) == 0)
			{
				enginesound->SetVolumeByGuid(sounds[i].m_nGuid, snd_victory_volume.GetFloat());
				return; // should only ever be playing one jingle at a time; return early
			}
		}
	}
}

ConVar snd_victory_volume("snd_victory_volume", "0.33", FCVAR_ARCHIVE | FCVAR_DONTRECORD | FCVAR_USERINFO, "Loudness of the victory jingle (0-1).", true, 0.0, true, 1.0, sndVictoryVolumeChangeCallback);
#endif // CLIENT_DLL

REGISTER_GAMERULES_CLASS( CNEORules );

BEGIN_NETWORK_TABLE_NOBASE( CNEORules, DT_NEORules )
// NEO TODO (Rain): NEO specific game modes var (CTG/TDM/...)
#ifdef CLIENT_DLL
	RecvPropFloat(RECVINFO(m_flNeoNextRoundStartTime)),
	RecvPropFloat(RECVINFO(m_flNeoRoundStartTime)),
	RecvPropFloat(RECVINFO(m_flPauseEnd)),
	RecvPropInt(RECVINFO(m_nRoundStatus)),
	RecvPropInt(RECVINFO(m_nGameTypeSelected)),
	RecvPropInt(RECVINFO(m_iRoundNumber)),
	RecvPropInt(RECVINFO(m_iHiddenHudElements)),
	RecvPropInt(RECVINFO(m_iForcedTeam)),
	RecvPropInt(RECVINFO(m_iForcedClass)),
	RecvPropInt(RECVINFO(m_iForcedSkin)),
	RecvPropInt(RECVINFO(m_iForcedWeapon)),
	RecvPropString(RECVINFO(m_szNeoJinraiClantag)),
	RecvPropString(RECVINFO(m_szNeoNSFClantag)),
	RecvPropInt(RECVINFO(m_iGhosterTeam)),
	RecvPropInt(RECVINFO(m_iGhosterPlayer)),
	RecvPropInt(RECVINFO(m_iEscortingTeam)),
	RecvPropBool(RECVINFO(m_bGhostExists)),
	RecvPropVector(RECVINFO(m_vecGhostMarkerPos)),
	RecvPropInt(RECVINFO(m_iJuggernautPlayerIndex)),
	RecvPropBool(RECVINFO(m_bJuggernautItemExists)),
	RecvPropVector(RECVINFO(m_vecJuggernautMarkerPos)),
#else
	SendPropFloat(SENDINFO(m_flNeoNextRoundStartTime)),
	SendPropFloat(SENDINFO(m_flNeoRoundStartTime)),
	SendPropFloat(SENDINFO(m_flPauseEnd)),
	SendPropInt(SENDINFO(m_nRoundStatus)),
	SendPropInt(SENDINFO(m_nGameTypeSelected)),
	SendPropInt(SENDINFO(m_iRoundNumber)),
	SendPropInt(SENDINFO(m_iHiddenHudElements)),
	SendPropInt(SENDINFO(m_iForcedTeam)),
	SendPropInt(SENDINFO(m_iForcedClass)),
	SendPropInt(SENDINFO(m_iForcedSkin)),
	SendPropInt(SENDINFO(m_iForcedWeapon)),
	SendPropString(SENDINFO(m_szNeoJinraiClantag)),
	SendPropString(SENDINFO(m_szNeoNSFClantag)),
	SendPropInt(SENDINFO(m_iGhosterTeam)),
	SendPropInt(SENDINFO(m_iGhosterPlayer)),
	SendPropInt(SENDINFO(m_iEscortingTeam)),
	SendPropBool(SENDINFO(m_bGhostExists)),
	SendPropVector(SENDINFO(m_vecGhostMarkerPos), -1, SPROP_COORD_MP_LOWPRECISION | SPROP_CHANGES_OFTEN, MIN_COORD_FLOAT, MAX_COORD_FLOAT),
	SendPropInt(SENDINFO(m_iJuggernautPlayerIndex)),
	SendPropBool(SENDINFO(m_bJuggernautItemExists)),
	SendPropVector(SENDINFO(m_vecJuggernautMarkerPos), -1, SPROP_COORD_MP_LOWPRECISION | SPROP_CHANGES_OFTEN, MIN_COORD_FLOAT, MAX_COORD_FLOAT),
#endif
END_NETWORK_TABLE()

LINK_ENTITY_TO_CLASS( neo_gamerules, CNEOGameRulesProxy );
IMPLEMENT_NETWORKCLASS_ALIASED( NEOGameRulesProxy, DT_NEOGameRulesProxy );

extern bool RespawnWithRet(CBaseEntity *pEdict, bool fCopyCorpse);

// The defaults are the numbers for Support, check macros in shareddefs.h for
// how the other classes are derived.
static NEOViewVectors g_NEOViewVectors(
	Vector( 0, 0, 60 ),	   //VEC_VIEW (m_vView)

	Vector(-16, -16, 0 ),	  //VEC_HULL_MIN (m_vHullMin)
	Vector(16, 16, 70 ),	  //VEC_HULL_MAX (m_vHullMax)

	Vector(-16, -16, 0 ),	  //VEC_DUCK_HULL_MIN (m_vDuckHullMin)
	Vector( 16,  16, 59 ),	  //VEC_DUCK_HULL_MAX	(m_vDuckHullMax)
	Vector( 0, 0, 50 ),		  //VEC_DUCK_VIEW		(m_vDuckView)

	Vector(-10, -10, -10 ),	  //VEC_OBS_HULL_MIN	(m_vObsHullMin)
	Vector( 10,  10,  10 ),	  //VEC_OBS_HULL_MAX	(m_vObsHullMax)

	Vector( 0, 0, 14 ),		  //VEC_DEAD_VIEWHEIGHT (m_vDeadViewHeight)

	Vector(-16, -16, 0 ),	  //VEC_CROUCH_TRACE_MIN (m_vCrouchTraceMin)
	Vector(16, 16, 60)	  //VEC_CROUCH_TRACE_MAX (m_vCrouchTraceMax)
);

struct NeoGameTypeSettings {
	const char* gameTypeName;
	bool respawns;
	bool neoRulesThink;
	bool changeTeamClassLoadoutWhenAlive;
	bool comp;
	bool capPrevent;
};

const NeoGameTypeSettings NEO_GAME_TYPE_SETTINGS[NEO_GAME_TYPE__TOTAL] = {
//						gametypeName	respawns	neoRulesThink	changeTeamClassLoadoutWhenAlive	comp	capPrevent
/*NEO_GAME_TYPE_TDM*/	{"TDM",			true,		true,			false,							false,	false},
/*NEO_GAME_TYPE_CTG*/	{"CTG",			false,		true,			false,							true,	true},
/*NEO_GAME_TYPE_VIP*/	{"VIP",			false,		true,			false,							true,	true},
/*NEO_GAME_TYPE_DM*/	{"DM",			true,		true,			false,							false,	false},
/*NEO_GAME_TYPE_EMT*/	{"EMT",			true,		false,			true,							false,	false},
/*NEO_GAME_TYPE_TUT*/	{"TUT",			true,		false,			false,							false,	false},
/*NEO_GAME_TYPE_JGR*/	{"JGR",			true,		true,			false,							false,	false},
};

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

ConVar neo_dm_round_timelimit("neo_dm_round_timelimit", "10.25", FCVAR_REPLICATED, "DM round timelimit, in minutes.",
	true, 0.0f, false, 600.0f);

ConVar neo_jgr_round_timelimit("neo_jgr_round_timelimit", "4.25", FCVAR_REPLICATED, "JGR round timelimit, in minutes.",
	true, 0.0f, false, 600.0f);

ConVar sv_neo_ignore_wep_xp_limit("sv_neo_ignore_wep_xp_limit", "0", FCVAR_CHEAT | FCVAR_REPLICATED, "If true, allow equipping any loadout regardless of player XP.",
	true, 0.0f, true, 1.0f);

ConVar sv_neo_dm_win_xp("sv_neo_dm_win_xp", "50", FCVAR_REPLICATED, "The XP limit to win the match.",
	true, 0.0f, true, 1000.0f);


#ifdef CLIENT_DLL
extern ConVar neo_fov;
#endif

extern CBaseEntity *g_pLastJinraiSpawn, *g_pLastNSFSpawn;

static const char *s_NeoPreserveEnts[] =
{
	"neo_gamerules",
	"neo_game_config",
	"info_player_attacker",
	"info_player_defender",
	"info_player_start",
	"neo_predicted_viewmodel",
	"neo_ghost_retrieval_point",
	"neo_ghostspawnpoint",
	"neo_juggernautspawnpoint",

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

	m_nGameTypeSelected = NEO_GAME_TYPE_CTG;
#endif
	m_iHiddenHudElements = 0;
	m_iForcedTeam = -1;
	m_iForcedClass = -1;
	m_iForcedSkin = -1;
	m_iForcedWeapon = -1;

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

bool CNEORules::ShouldCollide(int collisionGroup0, int collisionGroup1)
{
	if (collisionGroup0 > collisionGroup1)
	{
		// swap so that lowest is always first
		V_swap(collisionGroup0, collisionGroup1);
	}

	if ((collisionGroup0 == COLLISION_GROUP_PLAYER || collisionGroup0 == COLLISION_GROUP_PLAYER_MOVEMENT) &&
		(collisionGroup1 == COLLISION_GROUP_PROJECTILE || collisionGroup1 == COLLISION_GROUP_WEAPON ||
		 collisionGroup1 == COLLISION_GROUP_PLAYER || collisionGroup1 == COLLISION_GROUP_PLAYER_MOVEMENT))
	{
		return false;
	}

	return CTeamplayRules::ShouldCollide(collisionGroup0, collisionGroup1);
}

extern ConVar mp_chattime;

void CNEORules::ResetMapSessionCommon()
{
	SetRoundStatus(NeoRoundStatus::Idle);
	m_iRoundNumber = 0;
	V_memset(m_szNeoJinraiClantag.GetForModify(), 0, NEO_MAX_CLANTAG_LENGTH);
	V_memset(m_szNeoNSFClantag.GetForModify(), 0, NEO_MAX_CLANTAG_LENGTH);
	m_iGhosterTeam = TEAM_UNASSIGNED;
	m_iGhosterPlayer = 0;
	m_bGhostExists = false;
	m_vecGhostMarkerPos = vec3_origin;
	m_bJuggernautItemExists = false;
	m_iJuggernautPlayerIndex = 0;
	m_vecJuggernautMarkerPos = vec3_origin;
	m_flNeoRoundStartTime = 0.0f;
	m_flNeoNextRoundStartTime = 0.0f;
#ifdef GAME_DLL
	m_pRestoredInfos.Purge();
	m_readyAccIDs.Purge();
	m_bIgnoreOverThreshold = false;

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
	DMSpawnComCallbackLoad();
	m_vecPreviousGhostSpawn = vec3_origin;
	m_vecPreviousJuggernautSpawn = vec3_origin;
	m_pJuggernautItem = nullptr;
	m_pJuggernautPlayer = nullptr;
#endif
}

#ifdef GAME_DLL
void CNEORules::ChangeLevel(void)
{
	ResetMapSessionCommon();
	if (sv_neo_readyup_lobby.GetBool() && !sv_neo_readyup_autointermission.GetBool())
	{
		m_bChangelevelDone = false;
	}
	else
	{
		BaseClass::ChangeLevel();
	}
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

void CNEORules::GetDMHighestScorers(
#ifdef GAME_DLL
		CNEO_Player *(*pHighestPlayers)[MAX_PLAYERS + 1],
#endif
		int *iHighestPlayersTotal,
		int *iHighestXP) const
{
	*iHighestPlayersTotal = 0;
	*iHighestXP = 0;
#ifdef GAME_DLL
	for (int i = 1; i <= gpGlobals->maxClients; ++i)
#else
	if (!g_PR)
	{
		return;
	}

	for (int i = 0; i < (MAX_PLAYERS + 1); ++i)
#endif
	{
		int iXP = 0;

#ifdef GAME_DLL
		auto pCmpPlayer = static_cast<CNEO_Player *>(UTIL_PlayerByIndex(i));
		if (!pCmpPlayer)
		{
			continue;
		}
		iXP = pCmpPlayer->m_iXP;
#else
		if (!g_PR->IsConnected(i))
		{
			continue;
		}
		iXP = g_PR->GetXP(i);
#endif

		if (iXP == *iHighestXP)
		{
#ifdef GAME_DLL
			(*pHighestPlayers)[(*iHighestPlayersTotal)++] = pCmpPlayer;
#else
			(*iHighestPlayersTotal)++;
#endif
		}
		else if (iXP > *iHighestXP)
		{
			*iHighestPlayersTotal = 0;
			*iHighestXP = iXP;
#ifdef GAME_DLL
			(*pHighestPlayers)[(*iHighestPlayersTotal)++] = pCmpPlayer;
#else
			(*iHighestPlayersTotal)++;
#endif
		}
	}
}

#ifdef GAME_DLL
void CNEORules::CheckGameType()
{
	// Static as CNEORules doesn't persists through map changes
	static int iStaticInitOnCmd = -1;
	static int iStaticInitOnRandAllow = -1;
	static bool staticGamemodesCanPick[NEO_GAME_TYPE__TOTAL] = {};
	static int iStaticLastPick = -1; // Mostly so it doesn't repeat on array refresh

	const int iGamemodeEnforce = sv_neo_gamemode_enforcement.GetInt();
	const int iGamemodeRandAllow = sv_neo_gamemode_random_allow.GetInt();
	// Update on what to select on first map load or server operator changes sv_neo_gamemode_enforcement
	const bool bCheckOnGameType = (!m_bGamemodeTypeBeenInitialized || iGamemodeEnforce != iStaticInitOnCmd ||
			iGamemodeRandAllow != iStaticInitOnRandAllow);
	if (!bCheckOnGameType)
	{
		return;
	}

	// NEO NOTE (nullsystem): CNEORules always recreated on map change, yet entities properly found
	// happens later. So checking and init on game type will execute here once.
	switch (iGamemodeEnforce)
	{
	case GAMEMODE_ENFORCEMENT_SINGLE:
	{
		m_nGameTypeSelected = sv_neo_gamemode_single.GetInt();
	} break;
	case GAMEMODE_ENFORCEMENT_RAND:
	{
		const int iBWAllow = sv_neo_gamemode_random_allow.GetInt(); // Min of 1, cannot be zero
		Assert(iBWAllow > 0);

		// Check if all are used up
		{
			int iAllowsPicks = 0;
			for (int i = 0; i < NEO_GAME_TYPE__TOTAL; ++i)
			{
				iAllowsPicks += staticGamemodesCanPick[i];
			}
			if (iAllowsPicks == 0 || iGamemodeRandAllow != iStaticInitOnRandAllow)
			{
#ifdef DEBUG
				DevMsg("Array reset!\n");
#endif
				// Preset true to those not-allowed, preset false to those allowed
				int iTotalPicks = 0;
				for (int i = 0; i < NEO_GAME_TYPE__TOTAL; ++i)
				{
					const bool bCanPick = (iBWAllow & (1 << i));
					iTotalPicks += bCanPick;
					staticGamemodesCanPick[i] = bCanPick;
				}
				if (iTotalPicks <= 1)
				{
					iStaticLastPick = -1;
				}
			}
		}

		m_nGameTypeSelected = RandomInt(0, NEO_GAME_TYPE__TOTAL - 1);
		for (int iWalk = 0;
			 (!staticGamemodesCanPick[m_nGameTypeSelected] || m_nGameTypeSelected == iStaticLastPick) &&
			 iWalk < NEO_GAME_TYPE__TOTAL;
			 ++iWalk)
		{
			m_nGameTypeSelected = LoopAroundInArray(m_nGameTypeSelected + 1, NEO_GAME_TYPE__TOTAL);
		}

#ifdef DEBUG
		for (int i = 0; i < NEO_GAME_TYPE__TOTAL; ++i)
		{
			DevMsg("%d | %s: %s\n", i, NEO_GAME_TYPE_DESC_STRS[i].szStr, staticGamemodesCanPick[i] ? "Allowed" : "Not allowed");
		}
		DevMsg("Pick: %d | Prev: %d\n", m_nGameTypeSelected.Get(), iStaticLastPick);
#endif

		staticGamemodesCanPick[m_nGameTypeSelected] = false;
		iStaticLastPick = m_nGameTypeSelected;
	} break;
	default:
	{
		const auto pEntGameCfg = static_cast<CNEOGameConfig *>(gEntList.FindEntityByClassname(nullptr, "neo_game_config"));
		m_nGameTypeSelected = (pEntGameCfg) ? pEntGameCfg->m_GameType : NEO_GAME_TYPE_CTG;
	} break;
	}
	m_bGamemodeTypeBeenInitialized = true;
	iStaticInitOnCmd = iGamemodeEnforce;
	iStaticInitOnRandAllow = iGamemodeRandAllow;

	if (CNEOGameConfig::s_pGameRulesToConfig && sv_neo_comp.GetBool())
	{
		CNEOGameConfig::s_pGameRulesToConfig->OutputCompetitive();
	}
}

void CNEORules::CheckHiddenHudElements()
{
	const auto pEntGameCfg = static_cast<CNEOGameConfig*>(gEntList.FindEntityByClassname(nullptr, "neo_game_config"));
	m_iHiddenHudElements = (pEntGameCfg) ? pEntGameCfg->m_HiddenHudElements : 0;
}

void CNEORules::CheckPlayerForced()
{
	const auto pEntGameCfg = static_cast<CNEOGameConfig*>(gEntList.FindEntityByClassname(nullptr, "neo_game_config"));
	m_iForcedTeam = (pEntGameCfg) ? pEntGameCfg->m_ForcedTeam : -1;
	m_iForcedClass = (pEntGameCfg) ? pEntGameCfg->m_ForcedClass : -1;
	m_iForcedSkin = (pEntGameCfg) ? pEntGameCfg->m_ForcedSkin : -1;
	m_iForcedWeapon = (pEntGameCfg) ? pEntGameCfg->m_ForcedWeapon : -1;
}
#endif

bool CNEORules::CheckShouldNotThink()
{
#ifdef GAME_DLL
	if (gpGlobals->eLoadType == MapLoad_Background || !NEO_GAME_TYPE_SETTINGS[m_nGameTypeSelected].neoRulesThink)
#else // CLIENT_DLL
	if (engine->IsLevelMainMenuBackground() || !NEO_GAME_TYPE_SETTINGS[m_nGameTypeSelected].neoRulesThink)
#endif // GAME_DLL || CLIENT_DLL
	{
		return true;
	}
	return false;
}

void CNEORules::Think(void)
{
#ifdef GAME_DLL
	CheckHiddenHudElements();
	CheckGameType();
	CheckPlayerForced();
	if (CheckShouldNotThink())
	{
		return;
	}

	const bool bIsIdleState = m_nRoundStatus == NeoRoundStatus::Idle
			|| m_nRoundStatus == NeoRoundStatus::Warmup
			|| m_nRoundStatus == NeoRoundStatus::Countdown;
	bool bIsPause = m_nRoundStatus == NeoRoundStatus::Pause;
	if (bIsIdleState && gpGlobals->curtime > m_flNeoNextRoundStartTime)
	{
		StartNextRound();
		return;
	}

	// Make the pause instant if we're still in freeze time
	if (m_nRoundStatus == NeoRoundStatus::PreRoundFreeze && m_flPauseDur > 0.0f &&
			m_iRoundNumber == (m_iPausingRound - 1))
	{
		SetRoundStatus(NeoRoundStatus::Pause);
		bIsPause = true;
		m_bPausedByPreRoundFreeze = true;
		m_flNeoNextRoundStartTime = 0.0f;
		m_flPauseEnd = gpGlobals->curtime + m_flPauseDur;
	}

	if (bIsPause)
	{
		if (gpGlobals->curtime >= m_flPauseEnd)
		{
			// NEO NOTE (nullsystem): If paused during freezetime, start on the round the freezetime was on
			// otherwise start on the next since going into pause state do not update round number since.
			if (m_bPausedByPreRoundFreeze)
			{
				--m_iRoundNumber;
			}
			m_bPausedByPreRoundFreeze = false;
			m_bPausingTeamRequestedUnpause = false;
			m_iPausingTeam = 0;
			m_iPausingRound = 0;
			m_flPauseDur = 0.0f;
			m_flPauseEnd = 0.0f;
			StartNextRound();
			return;
		}
		else if (gpGlobals->curtime > m_flNeoNextRoundStartTime)
		{
			m_flNeoNextRoundStartTime = gpGlobals->curtime + 5.0f;
			UTIL_CenterPrintAll("- MATCH IS CURRENTLY PAUSED -\n");
		}
	}

	// Allow respawn if it's an idle, warmup round, pausing, or deathmatch-type gamemode
	const bool bIsDMType = (m_nGameTypeSelected == NEO_GAME_TYPE_DM || m_nGameTypeSelected == NEO_GAME_TYPE_TDM);
	if (bIsDMType || bIsIdleState || bIsPause)
	{
		CRecipientFilter filter;
		filter.MakeReliable();

		for (int i = 1; i <= gpGlobals->maxClients; i++)
		{
			auto player = static_cast<CNEO_Player *>(UTIL_PlayerByIndex(i));
			if (player && player->IsDead() && (bIsPause || player->DeathCount() > 0))
			{
				const int playerTeam = player->GetTeamNumber();
				if ((playerTeam == TEAM_JINRAI || playerTeam == TEAM_NSF) && RespawnWithRet(player, false))
				{
					player->m_bInAim = false;
					player->m_bCarryingGhost = false;
					player->m_bInThermOpticCamo = false;
					player->m_bInVision = false;
					player->m_bIneligibleForLoadoutPick = false;
					player->SetTestMessageVisible(false);

					if (!bIsIdleState && !bIsPause && bIsDMType)
					{
						engine->ClientCommand(player->edict(), "loadoutmenu");
					}
					else
					{
						filter.AddRecipient(player);
					}
				}
			}
		}

		if (filter.GetRecipientCount() > 0 && bIsIdleState)
		{
			UserMessageBegin(filter, "IdleRespawnShowMenu");
			MessageEnd();
		}
	}

	if (m_bThinkCheckClantags)
	{
		m_bThinkCheckClantags = false;
		int iHasClantags[TEAM__TOTAL] = {};
		bool bClantagSet[TEAM__TOTAL] = {};
		char szTeamClantags[TEAM__TOTAL][NEO_MAX_CLANTAG_LENGTH] = {};
		for (int i = 1; i <= gpGlobals->maxClients; ++i)
		{
			auto pNeoPlayer = static_cast<CNEO_Player*>(UTIL_PlayerByIndex(i));
			if (pNeoPlayer)
			{
				const int iTeam = pNeoPlayer->GetTeamNumber();
				if (!bClantagSet[iTeam])
				{
					bClantagSet[iTeam] = true;
					V_strcpy_safe(szTeamClantags[iTeam], pNeoPlayer->GetNeoClantag()),
					++iHasClantags[iTeam];
				}
				else
				{
					iHasClantags[iTeam] += (V_strcmp(szTeamClantags[iTeam], pNeoPlayer->GetNeoClantag()) == 0);
				}
			}
		}

		char *pszClantagMod[TEAM__TOTAL] = {};
		pszClantagMod[TEAM_JINRAI] = m_szNeoJinraiClantag.GetForModify();
		pszClantagMod[TEAM_NSF] = m_szNeoNSFClantag.GetForModify();
		for (const int i : {TEAM_JINRAI, TEAM_NSF})
		{
			V_strncpy(pszClantagMod[i], (iHasClantags[i] == GetGlobalTeam(i)->GetNumPlayers()) ?
						  szTeamClantags[i] : "", NEO_MAX_CLANTAG_LENGTH);
		}
	}

	if (bIsPause)
	{
		return;
	}

	if (g_fGameOver)   // someone else quit the game already
	{
		// check to see if we should change levels now
		if (m_flIntermissionEndTime < gpGlobals->curtime)
		{
			if (!m_bChangelevelDone)
			{
				m_bChangelevelDone = true;
				ChangeLevel(); // intermission is over
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

	if (sv_neo_teamdamage_kick.GetBool() && m_nRoundStatus == NeoRoundStatus::RoundLive &&
			gpGlobals->curtime > (m_flPrevThinkKick + 0.5f))
	{
		const int iThresKickHp = sv_neo_teamdamage_kick_hp.GetInt();
		const int iThresKickKills = sv_neo_teamdamage_kick_kills.GetInt();

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
			m_pJuggernautItem = nullptr;
			m_pJuggernautPlayer = nullptr;
			m_iJuggernautPlayerIndex = 0;
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
		if (GetGameType() == NEO_GAME_TYPE_TDM || GetGameType() == NEO_GAME_TYPE_JGR)
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
		else if (GetGameType() == NEO_GAME_TYPE_DM)
		{
			// Winning player
			CNEO_Player *pWinners[MAX_PLAYERS + 1] = {};
			int iWinnersTotal = 0;
			int iWinnerXP = 0;
			GetDMHighestScorers(&pWinners, &iWinnersTotal, &iWinnerXP);
			if (iWinnersTotal == 1)
			{
				SetWinningDMPlayer(pWinners[0]);
				return;
			}
			// Otherwise go into overtime
		}

		if (IsTeamplay())
		{
			SetWinningTeam(TEAM_SPECTATOR, NEO_VICTORY_STALEMATE, false, false, true, false);
		}
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

				IGameEvent* event = gameeventmanager->CreateEvent("ghost_capture");
				if (event)
				{
					event->SetInt("userid", UTIL_PlayerByIndex(m_iGhosterPlayer)->GetUserID());
					gameeventmanager->FireEvent(event);
				}

				if (m_iEscortingTeam && m_iEscortingTeam == captorTeam)
				{
					break;
				}

				for (int i = 1; i <= gpGlobals->maxClients; i++)
				{
					auto player = UTIL_PlayerByIndex(i);
					if (player && player->GetTeamNumber() == captorTeam)
					{
						auto* neoPlayer = static_cast<CNEO_Player*>(player);
						if (player->IsAlive())
						{
							auto playerPossessedByMe = neoPlayer->m_hSpectatorTakeoverPlayerTarget.Get();
							if (playerPossessedByMe)
							{
								// I already died and am possessing another player that should instead get credit
								neoPlayer->AddPoints(1, false);
								AwardRankUp(playerPossessedByMe);
							}
							else
							{
								// I am myself and should get full credit for being alive
								AwardRankUp(i);
							}
						}
						else
						{
							auto playerControllingMe = neoPlayer->m_hSpectatorTakeoverPlayerImpersonatingMe.Get();
							if (!playerControllingMe)
							{
								// I didn't get credit for another player possessing me, so I get a single team point
								neoPlayer->AddPoints(1, false);
							}
						}
					}
				}

				break;
			}
		}
	}
	else if (m_pJuggernautItem)
	{
		if (m_pJuggernautItem->GetAbsOrigin().IsValid())
		{
			m_vecJuggernautMarkerPos = m_pJuggernautItem->WorldSpaceCenter();
		}
		else
		{
			Assert(false);
		}

		m_iJuggernautPlayerIndex = 0;
		m_bJuggernautItemExists = true;
	}
	else
	{
		m_bJuggernautItemExists = false;
	}

#define JGR_POINT_INTERVAL 1.5f

	if (GetGameType() == NEO_GAME_TYPE_JGR && m_nRoundStatus == NeoRoundStatus::RoundLive && m_pJuggernautPlayer)
	{
		if (gpGlobals->curtime >= m_flLastPointTime)
		{
			m_flLastPointTime = gpGlobals->curtime + JGR_POINT_INTERVAL;

			if (m_pJuggernautPlayer->GetTeamNumber() == TEAM_JINRAI)
			{
				GetGlobalTeam(TEAM_JINRAI)->AddScore(1);
			}
			else if (m_pJuggernautPlayer->GetTeamNumber() == TEAM_NSF)
			{
				GetGlobalTeam(TEAM_NSF)->AddScore(1);
			}
		}

		if (GetGlobalTeam(TEAM_JINRAI)->GetScore() >= sv_neo_jgr_max_points.GetInt())
		{
			SetWinningTeam(TEAM_JINRAI, NEO_VICTORY_POINTS, false, true, false, false);
			return;
		}

		if (GetGlobalTeam(TEAM_NSF)->GetScore() >= sv_neo_jgr_max_points.GetInt())
		{
			SetWinningTeam(TEAM_NSF, NEO_VICTORY_POINTS, false, true, false, false);
			return;
		}
	}

	if (GetGameType() == NEO_GAME_TYPE_VIP && m_nRoundStatus == NeoRoundStatus::RoundLive && !m_pGhost)
	{
		if (!m_pVIP)
		{
			if (sv_neo_vip_ctg_on_death.GetBool())
			{
				UTIL_CenterPrintAll("- HVT DOWN - RECOVER THE GHOST -\n");
				SpawnTheGhost();
			}
			else
			{
				// Assume vip player disconnected, forfeit round
				SetWinningTeam(GetOpposingTeam(m_iEscortingTeam), NEO_VICTORY_FORFEIT, false, true, false, false);
			}
			IGameEvent* event = gameeventmanager->CreateEvent("vip_death");
			if (event)
			{
				gameeventmanager->FireEvent(event);
			}
		}

		if (!m_pVIP->IsAlive())
		{
			if (sv_neo_vip_ctg_on_death.GetBool())
			{
				UTIL_CenterPrintAll("- HVT DOWN - RECOVER THE GHOST -\n");
				SpawnTheGhost(&m_pVIP->GetAbsOrigin());
			}
			else
			{
				// VIP was killed, end round
				SetWinningTeam(GetOpposingTeam(m_iEscortingTeam), NEO_VICTORY_VIP_ELIMINATION, false, true, false, false);
			}
			IGameEvent* event = gameeventmanager->CreateEvent("vip_death");
			if (event)
			{
				event->SetInt("userid", m_pVIP->GetUserID());
				gameeventmanager->FireEvent(event);
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

				IGameEvent* event = gameeventmanager->CreateEvent("vip_extract");
				if (event)
				{
					event->SetInt("userid", m_pVIP->GetUserID());
					gameeventmanager->FireEvent(event);
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
							neoPlayer->AddPoints(1, false);
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
			if (gpGlobals->curtime > m_flNeoRoundStartTime + sv_neo_preround_freeze_time.GetFloat())
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
			if (GetGameType() != NEO_GAME_TYPE_TDM && GetGameType() != NEO_GAME_TYPE_DM && GetGameType() != NEO_GAME_TYPE_JGR)
			{
				for (int team = TEAM_JINRAI; team <= TEAM_NSF; ++team)
				{
					if (GetGlobalTeam(team)->GetAliveMembers() == 0)
					{
						SetWinningTeam(GetOpposingTeam(team), NEO_VICTORY_TEAM_ELIMINATION, false, true, false, false);
					}
				}
			}
			if (GetGameType() == NEO_GAME_TYPE_DM && sv_neo_dm_win_xp.GetInt() > 0)
			{
				// End game early if there's already a player past the winning XP
				CNEO_Player *pHighestPlayers[MAX_PLAYERS + 1] = {};
				int iWinningTotal = 0;
				int iWinningXP = 0;
				GetDMHighestScorers(&pHighestPlayers, &iWinningTotal, &iWinningXP);
				if (iWinningXP >= sv_neo_dm_win_xp.GetInt() && iWinningTotal == 1)
				{
					SetWinningDMPlayer(pHighestPlayers[0]);
				}
			}
		}
	}
#endif
}

#ifdef GAME_DLL
void CNEORules::SetWinningDMPlayer(CNEO_Player *pWinner)
{
	if (IsRoundOver())
	{
		return;
	}

	if (CNEOGameConfig::s_pGameRulesToConfig)
	{
		CNEOGameConfig::s_pGameRulesToConfig->OutputRoundEnd();
	}

	SetRoundStatus(NeoRoundStatus::PostRound);
	char victoryMsg[128];
	// TODO: Per client since client has neo_name settings
	V_sprintf_safe(victoryMsg, "%s is the winner of the deathmatch!\n", pWinner->GetNeoPlayerName());

	CRecipientFilter filter;
	filter.AddAllPlayers();
	UserMessageBegin(filter, "RoundResult");
	WRITE_STRING("tie");
	WRITE_FLOAT(gpGlobals->curtime);
	WRITE_STRING(victoryMsg);
	MessageEnd();

	EmitSound_t soundParams;
	soundParams.m_nChannel = CHAN_AUTO;
	soundParams.m_SoundLevel = SNDLVL_NONE;
	soundParams.m_flVolume = 0.33f;
	// Differing between Jinrai/NSF only as a sound cosmetic (no affect on DM)
	const int team = pWinner->GetTeamNumber();
	soundParams.m_pSoundName = (team == TEAM_JINRAI) ? "gameplay/jinrai.mp3" : (team == TEAM_NSF) ? "gameplay/nsf.mp3" : "gameplay/draw.mp3";
	soundParams.m_bWarnOnDirectWaveReference = false;
	soundParams.m_bEmitCloseCaption = false;

	for (int i = 1; i <= gpGlobals->maxClients; ++i)
	{
		CBasePlayer* basePlayer = UTIL_PlayerByIndex(i);
		auto player = static_cast<CNEO_Player*>(basePlayer);
		if (player)
		{
			if (!player->IsBot() || player->IsHLTV())
			{
				const char* volStr = engine->GetClientConVarValue(i, "snd_victory_volume");
				const float jingleVolume = volStr ? atof(volStr) : 0.33f;
				soundParams.m_flVolume = jingleVolume;

				CRecipientFilter soundFilter;
				soundFilter.AddRecipient(basePlayer);
				soundFilter.MakeReliable();
				player->EmitSound(soundFilter, i, soundParams);
			}
		}
	}

	GoToIntermission();
}
#endif

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
	Assert(m_nRoundStatus != NeoRoundStatus::Pause);
	if (m_nRoundStatus == NeoRoundStatus::Pause)
	{
		return;
	}

	if (!pClient)
	{
		return;
	}

	const int ranks[] = { 0, 4, 10, 20 };
	for (int i = 0; i < ARRAYSIZE(ranks); i++)
	{
		if (pClient->m_iXP.Get() < ranks[i])
		{
			pClient->AddPoints(ranks[i] - pClient->m_iXP, false);
			return;
		}
	}

	// If we're beyond max rank, just award +1 point.
	pClient->AddPoints(1, false);
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
		roundTimeLimit = sv_neo_warmup_round_time.GetFloat();
	}
	else if (m_nRoundStatus == NeoRoundStatus::Countdown)
	{
		roundTimeLimit = sv_neo_readyup_countdown.GetFloat();
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
			case NEO_GAME_TYPE_DM:
				roundTimeLimit = neo_dm_round_timelimit.GetFloat() * 60.f;
				break;
			case NEO_GAME_TYPE_JGR:
				roundTimeLimit = neo_jgr_round_timelimit.GetFloat() * 60.f;
				break;
			default:
				break;
		}
	}

	return (m_flNeoRoundStartTime + roundTimeLimit) - gpGlobals->curtime;
}

float CNEORules::GetRoundAccumulatedTime() const
{
	return gpGlobals->curtime - (m_flNeoRoundStartTime + sv_neo_preround_freeze_time.GetFloat());
}

#ifdef GAME_DLL
float CNEORules::MirrorDamageMultiplier() const
{
	if (m_nRoundStatus != NeoRoundStatus::RoundLive)
	{
		return 0.0f;
	}
	const float flAccTime = GetRoundAccumulatedTime();
	const float flMirrorMult = sv_neo_mirror_teamdamage_multiplier.GetFloat();
	const float flMirrorDur = sv_neo_mirror_teamdamage_duration.GetFloat();
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
		m_pGhost->Drop(vec3_origin);
	}
	// We didn't have any spawns, spawn ghost at origin
	else if (numGhostSpawns == 0)
	{
		Warning("No ghost spawns found! Spawning ghost at map origin, instead.\n");
		m_pGhost->SetAbsOrigin(vec3_origin);
		m_pGhost->Drop(vec3_origin);
	}
	else if (sv_neo_ghost_spawn_bias.GetBool() == true && roundAlternate())
	{
		m_pGhost->SetAbsOrigin(m_vecPreviousGhostSpawn);
		m_pGhost->Drop(vec3_origin);
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
						m_pGhost->Drop(vec3_origin);
						Warning("Failed to get ghost spawn coords; spawning ghost at map origin instead!\n");
						Assert(false);
					}
					else
					{
						m_pGhost->SetAbsOrigin(ghostSpawn->GetAbsOrigin());
						m_pGhost->Drop(vec3_origin);
					}

					break;
				}
			}

			pEnt = gEntList.NextEnt(pEnt);
		}
	}

	m_vecPreviousGhostSpawn = m_pGhost->GetAbsOrigin();
	DevMsg("%s ghost at coords:\n\t%.1f %.1f %.1f\n",
			spawnedGhostNow ? "Spawned" : "Moved",
			m_vecPreviousGhostSpawn.x,
			m_vecPreviousGhostSpawn.y,
			m_vecPreviousGhostSpawn.z);
}

// Very similar to above.
void CNEORules::SpawnTheJuggernaut(const Vector* origin)
{
	CBaseEntity* pEnt;

	// Get the amount of juggernaut spawns available to us
	int numJgrSpawns = 0;

	pEnt = gEntList.FirstEnt();
	while (pEnt)
	{
		if (dynamic_cast<CNEOJuggernautSpawnPoint*>(pEnt))
		{
			numJgrSpawns++;
		}
		else if (auto* jgr = dynamic_cast<CNEO_Juggernaut*>(pEnt))
		{
			m_pJuggernautItem = jgr;
		}

		pEnt = gEntList.NextEnt(pEnt);
	}

	// No juggernaut spawns
	if (numJgrSpawns == 0)
	{
		m_pJuggernautItem = nullptr;
		return;
	}

	bool spawnedJuggernautNow = false;
	if (!m_pJuggernautItem)
	{
		m_pJuggernautItem = dynamic_cast<CNEO_Juggernaut*>(CreateEntityByName("neo_juggernaut", -1));
		if (!m_pJuggernautItem)
		{
			Assert(false);
			Warning("Failed to spawn a juggernaut\n");
			return;
		}

		const int dispatchRes = DispatchSpawn(m_pJuggernautItem);
		if (dispatchRes != 0)
		{
			Assert(false);
			return;
		}

		spawnedJuggernautNow = true;
	}
	m_bJuggernautItemExists = true;

	Assert(UTIL_IsValidEntity(m_pJuggernautItem));

	if (origin)
	{
		m_pJuggernautItem->SetAbsOrigin(*origin);
	}
	// We didn't have any spawns, spawn jgr at origin
	else if (numJgrSpawns == 0)
	{
		Warning("No juggernaut spawns found! Spawning juggernaut at map origin, instead.\n");
		m_pJuggernautItem->SetAbsOrigin(vec3_origin);
	}
	else if (sv_neo_juggernaut_spawn_bias.GetBool() == true && roundAlternate())
	{
		m_pJuggernautItem->SetAbsOrigin(m_vecPreviousJuggernautSpawn);
	}
	else
	{
		// Randomly decide on a juggernaut spawn point we want this time
		const int desiredSpawn = RandomInt(1, numJgrSpawns);
		int jgrSpawnIteration = 1;

		pEnt = gEntList.FirstEnt();
		// Second iteration, we pick the ghost spawn we want
		while (pEnt)
		{
			auto jgrSpawn = dynamic_cast<CNEOJuggernautSpawnPoint*>(pEnt);

			if (jgrSpawn)
			{
				if (jgrSpawnIteration++ == desiredSpawn)
				{
					if (!jgrSpawn->GetAbsOrigin().IsValid())
					{
						m_pJuggernautItem->SetAbsOrigin(vec3_origin);
						Warning("Failed to get ghost spawn coords; spawning juggernaut at map origin instead!\n");
						Assert(false);
					}
					else
					{
						m_pJuggernautItem->SetAbsOrigin(jgrSpawn->GetAbsOrigin());
						m_pJuggernautItem->SetAbsAngles(QAngle(0, jgrSpawn->GetAbsAngles().y, 0));
					}

					break;
				}
			}

			pEnt = gEntList.NextEnt(pEnt);
		}
	}

	m_vecPreviousJuggernautSpawn = m_pJuggernautItem->GetAbsOrigin();
	DevMsg("%s juggernaut at coords:\n\t%.1f %.1f %.1f\n",
		spawnedJuggernautNow ? "Spawned" : "Moved",
		m_vecPreviousJuggernautSpawn.x,
		m_vecPreviousJuggernautSpawn.y,
		m_vecPreviousJuggernautSpawn.z);
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

			const bool clientWantsToBeVIP = engine->GetClientConVarValue(sameTeamAsVIP[sameTeamAsVIPTop], "cl_neo_vip_eligible");
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

void CNEORules::JuggernautActivated(CNEO_Player* pPlayer)
{
	if (GetGameType() == NEO_GAME_TYPE_JGR)
	{
		m_pJuggernautPlayer = pPlayer;
		m_iJuggernautPlayerIndex = pPlayer->entindex();
		m_pJuggernautItem = nullptr;
		m_vecJuggernautMarkerPos = vec3_origin;
	}
}

void CNEORules::GatherGameTypeVotes()
{
	int gameTypes[NEO_GAME_TYPE__TOTAL] = {};

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
	for (int i = 1; i < NEO_GAME_TYPE__TOTAL; i++)
	{
		if (gameTypes[i] > mostVotes) // NEOTODO (Adam) Handle draws
		{
			mostVotes = gameTypes[i];
			mostPopularGameType = i;
		}
	}

	m_nGameTypeSelected = mostPopularGameType;
}

bool CNEORules::ReadyUpPlayerIsReady(CNEO_Player *pNeoPlayer) const
{
	if (!pNeoPlayer) return false;

	const CSteamID steamID = GetSteamIDForPlayerIndex(pNeoPlayer->entindex());
	return pNeoPlayer->IsBot() || (steamID.IsValid() && m_readyAccIDs.HasElement(steamID.GetAccountID()));
}

void CNEORules::CheckChatCommand(CNEO_Player *pNeoCmdPlayer, const char *pSzChat)
{
	const bool bHasCmds = sv_neo_readyup_lobby.GetBool() || sv_neo_pausematch_enabled.GetBool();
	if (!bHasCmds || !pNeoCmdPlayer || !pSzChat || pSzChat[0] != '.') return;
	++pSzChat;

	if (V_strcmp(pSzChat, "help") == 0)
	{
		char szHelpText[256];
		V_strcpy_safe(szHelpText, "Available commands:\n");
		ClientPrint(pNeoCmdPlayer, HUD_PRINTTALK, szHelpText);
		if (sv_neo_readyup_lobby.GetBool())
		{
			V_strcpy_safe(szHelpText,
					"Ready up commands (only available while waiting for players):\n"
					".ready - Ready up yourself\n"
					".unready - Unready yourself\n"
					".start - Override players amount restriction\n"
					".readylist - List players that are not ready\n");
			ClientPrint(pNeoCmdPlayer, HUD_PRINTTALK, szHelpText);
		}
		if (sv_neo_pausematch_enabled.GetBool())
		{
			V_strcpy_safe(szHelpText,
					"Pause commands (only available during a match):\n"
					".pause - Pause the match\n"
					".unpause - Unpause the match\n");
			ClientPrint(pNeoCmdPlayer, HUD_PRINTTALK, szHelpText);
		}
		return;
	}

	const bool bNonCmdGameType = !NEO_GAME_TYPE_SETTINGS[GetGameType()].comp;

	if (sv_neo_readyup_lobby.GetBool() && (bNonCmdGameType || m_nRoundStatus != NeoRoundStatus::Idle))
	{
		for (const auto pSzCheck : {"ready", "unready", "start", "readylist"})
		{
			if (V_strcmp(pSzChat, pSzCheck) == 0)
			{
				ClientPrint(pNeoCmdPlayer, HUD_PRINTTALK,
							(bNonCmdGameType) ?
								"You cannot use this command in DM/TDM." :
								"You cannot use this command during a match.");
				break;
			}
		}
	}
	else if (sv_neo_readyup_lobby.GetBool() && m_nRoundStatus == NeoRoundStatus::Idle)
	{
		const CSteamID steamID = GetSteamIDForPlayerIndex(pNeoCmdPlayer->entindex());
		if (steamID.IsValid())
		{
			const int iThres = sv_neo_readyup_teamplayersthres.GetInt();
			if (V_strcmp(pSzChat, "ready") == 0)
			{
				m_readyAccIDs.Insert(steamID.GetAccountID());
				ClientPrint(pNeoCmdPlayer, HUD_PRINTTALK, "You are now marked as ready.");
				const auto readyPlayers = FetchReadyPlayers();
				if (readyPlayers.array[TEAM_JINRAI] == iThres && readyPlayers.array[TEAM_NSF] == iThres)
				{
					UTIL_ClientPrintAll(HUD_PRINTTALK, "All players are ready! Starting soon...");
				}
			}
			else if (V_strcmp(pSzChat, "unready") == 0)
			{
				m_readyAccIDs.Remove(steamID.GetAccountID());
				ClientPrint(pNeoCmdPlayer, HUD_PRINTTALK, "You are now marked as unready.");
			}
			else if (V_strcmp(pSzChat, "start") == 0)
			{
				const auto readyPlayers = FetchReadyPlayers();
				if (readyPlayers.array[TEAM_JINRAI] > iThres && readyPlayers.array[TEAM_NSF] > iThres)
				{
					m_bIgnoreOverThreshold = true;
					ClientPrint(pNeoCmdPlayer, HUD_PRINTTALK, "Overriding threshold, allowing more players.");
				}
				else
				{
					ClientPrint(pNeoCmdPlayer, HUD_PRINTTALK, "You must go past the threshold in order to set override.");
				}
			}
			else if (V_strcmp(pSzChat, "readylist") == 0)
			{
				bool bHasPlayersInList = false;
				bool bHasUnreadyPlayers = false;
				const bool bIsStreamer = pNeoCmdPlayer->m_bClientStreamermode;
				char szPrintText[((MAX_PLAYER_NAME_LENGTH + 1) * MAX_PLAYERS) + 32];
				szPrintText[0] = '\0';
				ReadyPlayers readyPlayers = {};
				for (int i = 1; i <= gpGlobals->maxClients; i++)
				{
					auto *pNeoOtherPlayer = static_cast<CNEO_Player *>(UTIL_PlayerByIndex(i));
					if (pNeoOtherPlayer &&
							(pNeoOtherPlayer->GetTeamNumber() == TEAM_JINRAI ||
							 pNeoOtherPlayer->GetTeamNumber() == TEAM_NSF) &&
							!pNeoOtherPlayer->IsHLTV())
					{
						const bool bPlayerReady = ReadyUpPlayerIsReady(pNeoOtherPlayer);
						readyPlayers.array[pNeoOtherPlayer->GetTeamNumber()] += bPlayerReady;
						if (!bHasPlayersInList)
						{
							if (!bIsStreamer) V_strcat_safe(szPrintText, "Ready list:\n");
							bHasPlayersInList = true;
						}

						V_strcat_safe(szPrintText, pNeoOtherPlayer->GetNeoPlayerName(pNeoCmdPlayer));
						if (bPlayerReady)
						{
							if (!bIsStreamer) V_strcat_safe(szPrintText, " [READY]");
						}
						else
						{
							if (!bIsStreamer) V_strcat_safe(szPrintText, " [NOT READY]");
							bHasUnreadyPlayers = true;
						}
						if (!bIsStreamer) V_strcat_safe(szPrintText, "\n");
					}
				}
				if (!bIsStreamer && bHasPlayersInList)
				{
					ClientPrint(pNeoCmdPlayer, HUD_PRINTTALK, szPrintText);
				}

				if (bIsStreamer || !bHasUnreadyPlayers)
				{
					if (!bHasUnreadyPlayers)
					{
						ClientPrint(pNeoCmdPlayer, HUD_PRINTTALK, "All players are ready.");
					}
					if (readyPlayers.array[TEAM_JINRAI] < iThres || readyPlayers.array[TEAM_NSF] < iThres)
					{
						const int iNeedJin = max(0, iThres - readyPlayers.array[TEAM_JINRAI]);
						const int iNeedNSF = max(0, iThres - readyPlayers.array[TEAM_NSF]);
						char szPrintNeed[100];
						V_sprintf_safe(szPrintNeed, "Jinrai need %d players and NSF need %d players "
													"to ready up to start.", iNeedJin, iNeedNSF);
						ClientPrint(pNeoCmdPlayer, HUD_PRINTTALK, szPrintNeed);
					}
					else if (readyPlayers.array[TEAM_JINRAI] > iThres || readyPlayers.array[TEAM_NSF] > iThres)
					{
						const int iExtraJin = max(0, readyPlayers.array[TEAM_JINRAI] - iThres);
						const int iExtraNSF = max(0, readyPlayers.array[TEAM_NSF] - iThres);
						char szPrintNeed[100];
						V_sprintf_safe(szPrintNeed, "Jinrai have %d extra players and NSF have %d extra players "
													"over the %d per team threshold.", iExtraJin, iExtraNSF, iThres);
						ClientPrint(pNeoCmdPlayer, HUD_PRINTTALK, szPrintNeed);
					}
				}
			}
		}
	}

	if (sv_neo_pausematch_enabled.GetBool())
	{
		if (bNonCmdGameType || m_nRoundStatus == Idle || m_nRoundStatus == Warmup)
		{
			for (const auto pSzCheck : {"pause", "unpause"})
			{
				if (V_strcmp(pSzChat, pSzCheck) == 0)
				{
					ClientPrint(pNeoCmdPlayer, HUD_PRINTTALK,
								(bNonCmdGameType) ?
									"You cannot use this command in DM/TDM." :
									"You cannot use this command outside a match.");
					break;
				}
			}
		}
		else
		{
			const bool bIsPause = (m_nRoundStatus == Pause);
			if (V_strcmp(pSzChat, "unpause") == 0)
			{
				if (bIsPause)
				{
					if (m_bPausingTeamRequestedUnpause)
					{
						// Check if this is from the non-pausing team, if so do the unpause
						if (m_iPausingTeam != pNeoCmdPlayer->GetTeamNumber())
						{
							// Unpause the game
							m_flPauseEnd = gpGlobals->curtime;
							UTIL_ClientPrintAll(HUD_PRINTTALK, "The game is now unpaused.");
						}
						else
						{
							ClientPrint(pNeoCmdPlayer, HUD_PRINTTALK, "Already started unpause request, waiting for non-pausing team.");
						}
					}
					else
					{
						// Check if this is from the pausing team, if so, then wait for the
						// non-pausing team to .unpause to accept unpause
						if (m_iPausingTeam == pNeoCmdPlayer->GetTeamNumber())
						{
							// sv_neo_pausematch_unpauseimmediate is locked behind cheat flag, so generally shouldn't happen
							if (sv_neo_pausematch_unpauseimmediate.GetBool())
							{
								m_flPauseEnd = gpGlobals->curtime;
								UTIL_ClientPrintAll(HUD_PRINTTALK, "The game is now unpaused.");
							}
							else
							{
								m_bPausingTeamRequestedUnpause = true;
								UTIL_ClientPrintAll(HUD_PRINTTALK, "An unpause request has started, waiting for non-pausing team to respond.");
							}
						}
						else
						{
							ClientPrint(pNeoCmdPlayer, HUD_PRINTTALK, "Non-pausing team cannot start unpause request.");
						}
					}
				}
				else
				{
					ClientPrint(pNeoCmdPlayer, HUD_PRINTTALK, "The match is already live.");
				}
			}
			else if (V_strcmp(pSzChat, "pause") == 0)
			{
				if (bIsPause)
				{
					if (m_bPausingTeamRequestedUnpause)
					{
						if (m_iPausingTeam == pNeoCmdPlayer->GetTeamNumber())
						{
							m_bPausingTeamRequestedUnpause = false;
							UTIL_ClientPrintAll(HUD_PRINTTALK, "Pausing team cancelled unpause request.");
						}
						else
						{
							ClientPrint(pNeoCmdPlayer, HUD_PRINTTALK, "Non-pausing team cannot cancel unpause request.");
						}
					}
					else
					{
						ClientPrint(pNeoCmdPlayer, HUD_PRINTTALK, "The match is already paused.");
					}
				}
				else
				{
					// Present a pause menu, giving short (1m) vs long (3m)
					pNeoCmdPlayer->m_eMenuSelectType = MENU_SELECT_TYPE_PAUSE;
					CSingleUserRecipientFilter filter(pNeoCmdPlayer);
					filter.MakeReliable();
					UserMessageBegin(filter, "ShowMenu");
					{
						// The key options available in bitwise (EX: 1 -> 1 << 0, 9 -> 1 << 8)
						const short sBitwiseOpts =
								  1 << (PAUSE_MENU_SELECT_SHORT - 1)
								| 1 << (PAUSE_MENU_SELECT_LONG - 1)
								| 1 << (PAUSE_MENU_SELECT_DISMISS - 1);
						WRITE_SHORT(sBitwiseOpts);
						WRITE_CHAR(static_cast<char>(15)); // 15s timeout
						WRITE_BYTE(static_cast<unsigned int>(0));
						WRITE_STRING("Pause match:\n"
									 "\n"
									 "->1. Short pause (1 minute)\n"
									 "->2. Long pause (3 minutes)\n"
									 "->3. Dismiss\n");
					}
					MessageEnd();
				}
			}
		}
	}
}

CNEORules::ReadyPlayers CNEORules::FetchReadyPlayers() const
{
	ReadyPlayers readyPlayers = {};
	if (!sv_neo_readyup_lobby.GetBool())
	{
		return readyPlayers;
	}

	for (int i = 1; i <= gpGlobals->maxClients; i++)
	{
		if (auto *pNeoPlayer = static_cast<CNEO_Player *>(UTIL_PlayerByIndex(i)))
		{
			readyPlayers.array[pNeoPlayer->GetTeamNumber()] += ReadyUpPlayerIsReady(pNeoPlayer);
		}
	}

	return readyPlayers;
}

void CNEORules::StartNextRound()
{
	// Only check ready-up on idle state
	const bool bLobby = sv_neo_readyup_lobby.GetBool() && m_nRoundStatus == NeoRoundStatus::Idle;
	const int iThres = sv_neo_readyup_teamplayersthres.GetInt();
	const bool bEqualThres = (iThres == GetGlobalTeam(TEAM_JINRAI)->GetNumPlayers()) && (iThres == GetGlobalTeam(TEAM_NSF)->GetNumPlayers());
	const auto readyPlayers = FetchReadyPlayers();
	// Do not start if: Non-ready-up mode, no players in either teams
	if ((!bLobby && (GetGlobalTeam(TEAM_JINRAI)->GetNumPlayers() == 0 || GetGlobalTeam(TEAM_NSF)->GetNumPlayers() == 0))
			// If ready-up mode and doesn't exactly match the threshold on ready-up or players
			|| (bLobby && !m_bIgnoreOverThreshold && (!bEqualThres || (readyPlayers.array[TEAM_JINRAI] != iThres || readyPlayers.array[TEAM_NSF] != iThres)))
			// If ready-up mode, allows over threshold and is lower than threshold or not equal teams
			|| (bLobby && m_bIgnoreOverThreshold &&
				((readyPlayers.array[TEAM_JINRAI] < iThres || readyPlayers.array[TEAM_NSF] < iThres)
				 || GetGlobalTeam(TEAM_JINRAI)->GetNumPlayers() != GetGlobalTeam(TEAM_NSF)->GetNumPlayers()))
			)
	{
		if (sv_neo_readyup_lobby.GetBool())
		{
			bool bPrintHelpInfo = (m_iPrintHelpCounter == 0);
			if (!m_bIgnoreOverThreshold && (readyPlayers.array[TEAM_JINRAI] > iThres || readyPlayers.array[TEAM_NSF] > iThres))
			{
				char szPrint[128];
				V_sprintf_safe(szPrint, "More players than %dv%d! Type \".start\" to allow more players to start!",
							   iThres, iThres);
				UTIL_ClientPrintAll(HUD_PRINTTALK, szPrint);
				bPrintHelpInfo = false;
			}

			// Untoggle the overrider if there's suddenly less players than threshold
			if (m_bIgnoreOverThreshold && (readyPlayers.array[TEAM_JINRAI] < iThres || readyPlayers.array[TEAM_NSF] < iThres))
			{
				m_bIgnoreOverThreshold = false;
			}

			char szPrint[512];
			V_sprintf_safe(szPrint, "- WAITING FOR %dv%d: %d JINRAI, %d NSF PLAYERS READY -\n",
						   iThres, iThres,
						   readyPlayers.array[TEAM_JINRAI], readyPlayers.array[TEAM_NSF]);
			UTIL_CenterPrintAll(szPrint);
			if (bPrintHelpInfo)
			{
				V_sprintf_safe(szPrint, "Ready up lobby is on - Type \".help\" for list of commands.");
				UTIL_ClientPrintAll(HUD_PRINTTALK, szPrint);
			}
			static constexpr int HELP_COUNT_NEXT_PRINT = 3;
			m_iPrintHelpCounter = LoopAroundInArray(m_iPrintHelpCounter + 1, HELP_COUNT_NEXT_PRINT);
		}
		else
		{
			UTIL_CenterPrintAll("- NEW ROUND START DELAYED - ONE OR BOTH TEAMS HAS NO PLAYERS -\n");
		}
		SetRoundStatus(NeoRoundStatus::Idle);
		m_flNeoNextRoundStartTime = gpGlobals->curtime + 10.0f;
		return;
	}

	if (m_nRoundStatus == NeoRoundStatus::Idle)
	{
		if (bLobby)
		{
			SetRoundStatus(NeoRoundStatus::Countdown);
			m_iRoundNumber = 0;
			m_flNeoRoundStartTime = gpGlobals->curtime;
			m_flNeoNextRoundStartTime = gpGlobals->curtime + sv_neo_readyup_countdown.GetFloat();
			UTIL_CenterPrintAll("- ALL PLAYERS READY: MATCH STARTING SOON... -\n");
			return;
		}
		else
		{
			// NOTE (nullsystem): If it's a loopback server or bots only, then go straight in.
			// Mostly ease for testing other stuff.
			bool loopbackSkipWarmup = false;
			const bool bLoopbackDontDoWarmup = !sv_neo_loopback_warmup_round.GetBool();
			const bool bBotsonlyDontDoWarmup = !sv_neo_botsonly_warmup_round.GetBool();
			if (bLoopbackDontDoWarmup || bBotsonlyDontDoWarmup)
			{
				int iCountBots = 0;
				int iCountHumans = 0;
				int iCountLoopback = 0;

				for (int i = 1; i <= gpGlobals->maxClients; i++)
				{
					if (auto* pPlayer = static_cast<CNEO_Player*>(UTIL_PlayerByIndex(i)))
					{
						const int teamNum = pPlayer->GetTeamNumber();
						if (teamNum == TEAM_JINRAI || teamNum == TEAM_NSF)
						{
							const bool bIsBot = pPlayer->IsBot();
							const bool bIsHuman = (!bIsBot && !pPlayer->IsHLTV());
							iCountBots += bIsBot;
							iCountHumans += bIsHuman;
							if (bIsHuman)
							{
								INetChannelInfo* nci = engine->GetPlayerNetInfo(i);
								iCountLoopback += nci->IsLoopback();
							}
						}
					}
				}
				if (bLoopbackDontDoWarmup)
				{
					loopbackSkipWarmup = (iCountLoopback > 0 && iCountLoopback == iCountHumans);
				}
				if (bBotsonlyDontDoWarmup && !loopbackSkipWarmup)
				{
					loopbackSkipWarmup = (iCountBots > 0 && iCountHumans == 0);
				}
			}

			SetRoundStatus(NeoRoundStatus::Warmup);
			m_iRoundNumber = 0;
			if (!loopbackSkipWarmup && !(bLobby && sv_neo_readyup_skipwarmup.GetBool()))
			{
				// Moving from 0 players from either team to playable at team state
				UTIL_CenterPrintAll("- WARMUP COUNTDOWN STARTED -\n");
				m_flNeoRoundStartTime = gpGlobals->curtime;
				m_flNeoNextRoundStartTime = gpGlobals->curtime + sv_neo_warmup_round_time.GetFloat();
				return;
			}
		}
	}

	if (m_flPauseDur > 0.0f && (m_iRoundNumber + 1) == NEORules()->m_iPausingRound)
	{
		SetRoundStatus(NeoRoundStatus::Pause);
		m_flNeoNextRoundStartTime = gpGlobals->curtime + 5.0f;
		m_bPausedByPreRoundFreeze = false;
		UTIL_CenterPrintAll("- MATCH IS CURRENTLY PAUSED -\n");
		m_flPauseEnd = gpGlobals->curtime + m_flPauseDur;
		return;
	}

	m_flNeoRoundStartTime = gpGlobals->curtime;
	m_flNeoNextRoundStartTime = 0;

	CleanUpMap();
	const bool bFromStarting = (m_nRoundStatus == NeoRoundStatus::Warmup
								|| m_nRoundStatus == NeoRoundStatus::Countdown);

	if (sv_neo_gamemode_enforcement.GetInt() == GAMEMODE_ENFORCEMENT_VOTE && bFromStarting)
	{
		GatherGameTypeVotes();
	}

	// NEO TODO (nullsystem): There should be a more sophisticated logic to be able to restore XP
	// for when moving from idle to preroundfreeze, or in the future, competitive with whatever
	// extra stuff in there. But to keep it simple: just clear if it was a warmup.
	SetRoundStatus(NeoRoundStatus::PreRoundFreeze);
	++m_iRoundNumber;

	for (int i = 1; i <= gpGlobals->maxClients; i++)
	{
		CNEO_Player *pPlayer = (CNEO_Player*)UTIL_PlayerByIndex(i);

		if (!pPlayer)
		{
			continue;
		}

		if (pPlayer->m_hSpectatorTakeoverPlayerTarget.Get())
		{
			pPlayer->RestorePlayerFromSpectatorTakeover();
		}

		if (pPlayer->GetTeamNumber() == TEAM_SPECTATOR)
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

		pPlayer->m_bInAim = false;
		pPlayer->m_bCarryingGhost = false;
		pPlayer->m_bInThermOpticCamo = false;
		pPlayer->m_bInVision = false;
		pPlayer->m_bIneligibleForLoadoutPick = false;

		if (bFromStarting)
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
	if (bFromStarting)
	{
		m_pRestoredInfos.Purge();
		// If game was in warmup then also decide on game mode here

		CTeam *pJinrai = GetGlobalTeam(TEAM_JINRAI);
		CTeam *pNSF = GetGlobalTeam(TEAM_NSF);
		Assert(pJinrai && pNSF);
		pJinrai->SetScore(0);
		pJinrai->SetRoundsWon(0);
		pNSF->SetScore(0);
		pNSF->SetRoundsWon(0);
	}

	FireLegacyEvent_NeoRoundEnd();

	char RoundMsg[27];
	static_assert(sizeof(RoundMsg) == sizeof("- CTG ROUND 99 STARTED -\n\0"), "RoundMsg requires to fit round numbers up to 2 digits");
	V_sprintf_safe(RoundMsg, "- %s ROUND %d STARTED -\n", GetGameTypeName(), Min(99, m_iRoundNumber.Get()));
	UTIL_CenterPrintAll(RoundMsg);

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

bool CNEORules::IsRoundOn() const
{
	return (m_nRoundStatus == NeoRoundStatus::PreRoundFreeze) || (m_nRoundStatus == NeoRoundStatus::RoundLive) || (m_nRoundStatus == NeoRoundStatus::PostRound);
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

const SZWSZTexts NEO_GAME_TYPE_DESC_STRS[NEO_GAME_TYPE__TOTAL] = {
	SZWSZ_INIT("Team Deathmatch"),
	SZWSZ_INIT("Capture the Ghost"),
	SZWSZ_INIT("Extract or Kill the VIP"),
	SZWSZ_INIT("Deathmatch"),
	SZWSZ_INIT("Free Roam"),
	SZWSZ_INIT("Training"),
};

const char *CNEORules::GetGameDescription(void)
{
	return NEO_GAME_TYPE_DESC_STRS[GetGameType()].szStr;
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

	if (CNEOGameConfig::s_pGameRulesToConfig && sv_neo_comp.GetBool())
	{
		CNEOGameConfig::s_pGameRulesToConfig->OutputCompetitive();
	}
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

	if (GetGameType() == NEO_GAME_TYPE_DM)
	{
		for (int i = 1; i <= gpGlobals->maxClients; ++i)
		{
			auto pPlayer = static_cast<CNEO_Player *>(UTIL_PlayerByIndex(i));
			if (pPlayer)
			{
				pPlayer->m_iXP.GetForModify() = 0;
			}
		}
	}

	if (GetGameType() == NEO_GAME_TYPE_JGR)
	{
		ResetJGR();
		SpawnTheJuggernaut();
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

void CNEORules::ResetJGR()
{
	m_pJuggernautItem = nullptr;
	m_pJuggernautPlayer = nullptr;
	m_flLastPointTime = 0.0f;
	m_iJuggernautPlayerIndex = 0;
	m_bJuggernautItemExists = false;
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
	if (sv_neo_build_integrity_check.GetBool())
	{
		const char *clientGitHash = engine->GetClientConVarValue(engine->IndexOfEdict(pEntity), "__cl_neo_git_hash");
		const bool dbgBuildSkip = (sv_neo_build_integrity_check_allow_debug.GetBool()) ? (clientGitHash[0] & 0b1000'0000) : false;
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
	const auto optClStreamerMode = StrToInt(engine->GetClientConVarValue(engine->IndexOfEdict(pNEOPlayer->edict()), "cl_neo_streamermode"));
	pNEOPlayer->m_bClientStreamermode = (optClStreamerMode && *optClStreamerMode);

	const char *pszNeoClantag = engine->GetClientConVarValue(pNEOPlayer->entindex(), "neo_clantag");
	const char *pszOldNeoClantag = pNEOPlayer->GetNeoClantag();
	if (V_strcmp(pszOldNeoClantag, pszNeoClantag) != 0)
	{
		V_strncpy(pNEOPlayer->m_szNeoClantag.GetForModify(), pszNeoClantag, NEO_MAX_CLANTAG_LENGTH);
		m_bThinkCheckClantags = true;
	}

	const char *pszClNeoCrosshair = engine->GetClientConVarValue(pNEOPlayer->entindex(), "cl_neo_crosshair");
	const char *pszOldClNeoCrosshair = pNEOPlayer->m_szNeoCrosshair.Get();
	if (V_strcmp(pszOldClNeoCrosshair, pszClNeoCrosshair) != 0)
	{
		V_strncpy(pNEOPlayer->m_szNeoCrosshair.GetForModify(), pszClNeoCrosshair, NEO_XHAIR_SEQMAX);
	}

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

				CRecipientFilter filterNonStreamers;
				filterNonStreamers.MakeReliable();
				for (int i = 1; i <= gpGlobals->maxClients; ++i)
				{
					auto neoPlayer = static_cast<CNEO_Player *>(UTIL_PlayerByIndex(i));
					if (neoPlayer && !neoPlayer->m_bClientStreamermode)
					{
						filterNonStreamers.AddRecipient(neoPlayer);
					}
				}
				UTIL_ClientPrintFilter(filterNonStreamers, HUD_PRINTTALK, text);

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
		const int roundDiff = neo_round_limit.GetInt() - m_iRoundNumber;
		if ((teamJinrai->GetRoundsWon() + 1) > (teamNSF->GetRoundsWon() + roundDiff)) return true;
		if ((teamNSF->GetRoundsWon() + 1) > (teamJinrai->GetRoundsWon() + roundDiff)) return true;
		return false;
	}
	return false;
}

#ifdef GAME_DLL
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

	if (CNEOGameConfig::s_pGameRulesToConfig)
	{
		CNEOGameConfig::s_pGameRulesToConfig->OutputRoundEnd();
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
			IGameEvent* event = gameeventmanager->CreateEvent("team_score");
			if (event)
			{
				event->SetInt("teamid", winningTeam->GetTeamNumber());
				event->SetInt("score", winningTeam->GetRoundsWon());
				gameeventmanager->FireEvent(event);
			}
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
				const int roundDiff = neo_round_limit.GetInt() - m_iRoundNumber;
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
		case NEO_VICTORY_MAPIO:
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
	if(iWinReason != NEO_VICTORY_MAPIO)
	{
		WRITE_STRING(victoryMsg);															// extra message (who capped or last kill or who got the most points or whatever)
	}
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

				const char* volStr = engine->GetClientConVarValue(i, "snd_victory_volume");
				const float jingleVolume = volStr ? atof(volStr) : 0.33f;
				soundParams.m_flVolume = jingleVolume;

				CRecipientFilter soundFilter;
				soundFilter.AddRecipient(basePlayer);
				soundFilter.MakeReliable();
				player->EmitSound(soundFilter, i, soundParams);
			}

			// Ghost-caps and VIP-escorts are handled separately
			if (winningTeamNum != TEAM_SPECTATOR && iWinReason != NEO_VICTORY_GHOST_CAPTURE && iWinReason != NEO_VICTORY_VIP_ESCORT && player->GetTeamNumber() == winningTeamNum)
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
				player->AddPoints(xpAward, false);
			}

			// Any human player still alive, show them damage stats in round end
			if (!player->IsBot() && !player->IsHLTV() && player->IsAlive())
			{
				player->StartShowDmgStats(nullptr);
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
		if (sv_neo_readyup_lobby.GetBool() && !sv_neo_readyup_autointermission.GetBool())
		{
			ResetMapSessionCommon();
		}
		else
		{
			GoToIntermission();
		}
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
	if (!NEO_GAME_TYPE_SETTINGS[GetGameType()].capPrevent)
	{
		return;
	}

	// If this is the only player alive left before the suicide/disconnect and the other team was holding
	// the ghost, reward the other team an XP to the next rank as a ghost cap was prevented.
	const bool bShouldCheck = (sv_neo_suicide_prevent_cap_punish.GetBool()
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

	if (m_nRoundStatus == NeoRoundStatus::Pause)
	{
#ifdef GAME_DLL
		// Counter-act the death count for pausing state
		victim->IncrementDeathCount(-1);
#endif
		return;
	}

	// Suicide or suicide by environment (non-grenade as grenade is likely from a player)
	if (attacker == victim || (!attacker && !grenade))
	{
		victim->AddPoints(-1, true);
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
		if (IsTeamplay() && attacker->GetTeamNumber() == victim->GetTeamNumber())
		{
			attacker->AddPoints(-1, true);
#ifdef GAME_DLL
			if (sv_neo_teamdamage_kick.GetBool() && m_nRoundStatus == NeoRoundStatus::RoundLive)
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
			attacker->AddPoints(1, false);
		}

		if (auto *assister = FetchAssists(attacker, victim))
		{
			// Team kill assist
			if (assister->GetTeamNumber() == victim->GetTeamNumber())
			{
				assister->AddPoints(-1, true);
			}
			// Enemy kill assist
			else
			{
				assister->AddPoints(1, false);
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

	bool isGrenade = false;
	bool isRemoteDetpack = false;
	CNEOBaseCombatWeapon* neoWep = pScorer ? static_cast<CNEOBaseCombatWeapon*>(pScorer->GetActiveWeapon()) : nullptr;

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
					}
				}
				else
				{
					killer_weapon_name = pInflictor->GetClassname();  // it's just that easy
					isGrenade = !Q_strnicmp(killer_weapon_name, "neo_gre", 7);
					isRemoteDetpack = !Q_strnicmp(killer_weapon_name, "neo_dep", 7);
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
		event->SetBool("headshot", pVictim->LastHitGroup() == HITGROUP_HEAD);
		event->SetBool("suicide", pKiller == pVictim || !pKiller->IsPlayer());
		
		if (isGrenade)
		{
			event->SetString("deathIcon", "2"); // NEO TODO (Adam) get from enum
		}
		else if (isRemoteDetpack)
		{
			event->SetString("deathIcon", "A"); // NEO TODO (Adam) get from enum
		}
		else if (neoWep)
		{
			event->SetString("deathIcon", neoWep->GetWpnData().szDeathIcon);
		}
		else
		{
			event->SetString("deathIcon", "");
		}

		event->SetBool("explosive", isGrenade || isRemoteDetpack);
		event->SetBool("ghoster", m_iGhosterPlayer == pVictim->entindex());

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
		// If the disconnecting player was controlling a bot, restore the bot now.
		if (pNeoPlayer->m_hSpectatorTakeoverPlayerTarget.Get())
		{
		    pNeoPlayer->RestorePlayerFromSpectatorTakeover();
		}

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

		if (pNeoPlayer->GetClass() == NEO_CLASS_JUGGERNAUT && pNeoPlayer->IsAlive())
		{
			pNeoPlayer->SpawnJuggernautPostDeath();
		}

		// Save XP/death counts
		if (sv_neo_player_restore.GetBool())
		{
			const CSteamID steamID = GetSteamIDForPlayerIndex(pNeoPlayer->entindex());
			if (steamID.IsValid())
			{
				const auto accountID = steamID.GetAccountID();
				const RestoreInfo restoreInfo{
					.xp = pNeoPlayer->m_iXP.Get(),
					.deaths = pNeoPlayer->DeathCount(),
					.spawnedThisRound = pNeoPlayer->m_bSpawnedThisRound,
					.deathTime = pNeoPlayer->IsAlive() ? gpGlobals->curtime : pNeoPlayer->GetDeathTime(), // NEOTODO (Adam) prevent players abusing retry command to save themselves in games with respawns. Should award xp to whoever did most damage to disconnecting player, for now simply ensure they can't respawn too quickly
				};

				if (restoreInfo.xp == 0 && restoreInfo.deaths == 0 && restoreInfo.deathTime == 0.f && restoreInfo.spawnedThisRound == false)
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

bool CNEORules::GetTeamPlayEnabled() const
{
	return m_nGameTypeSelected != NEO_GAME_TYPE_DM;
}

#ifdef GAME_DLL
bool CNEORules::FPlayerCanRespawn(CBasePlayer* pPlayer)
{
	// Special case for spectator player takeover
	CNEO_Player* pNeoPlayer = ToNEOPlayer(pPlayer);
	if (pNeoPlayer && pNeoPlayer->GetSpectatorTakeoverPlayerPending())
	{
		return true;
	}

	auto gameType = GetGameType();

	if ((gameType == NEO_GAME_TYPE_JGR) && m_pJuggernautPlayer)
	{
		if (pPlayer->GetTeamNumber() == m_pJuggernautPlayer->GetTeamNumber())
		{
			return false;
		}
	}

	if (CanRespawnAnyTime())
	{
		return true;
	}

	else if (gameType == NEO_GAME_TYPE_TUT)
	{
		if (pPlayer->IsAlive())
		{
			return false;
		}
		return true;
	}

	auto jinrai = GetGlobalTeam(TEAM_JINRAI);
	auto nsf = GetGlobalTeam(TEAM_NSF);

	if (jinrai && nsf)
	{
		if (!IsRoundOn())
		{
			return true;
		}

		CNEO_Player* pNeoPlayer = ToNEOPlayer(pPlayer);
		if (pNeoPlayer->m_bSpawnedThisRound)
		{
			return false;
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
	if (GetRemainingPreRoundFreezeTime(false) + sv_neo_latespawn_max_time.GetFloat() > 0)
	{
		return true;
	}

	return false;
}

CBaseEntity *CNEORules::GetPlayerSpawnSpot(CBasePlayer *pPlayer)
{
	// NEO NOTE (nullsystem): If available + DM, instead of by entity, player spawn
	// by set position. It doesn't seem anything utilizes what returned anyway.
	if (m_nGameTypeSelected == NEO_GAME_TYPE_DM && DMSpawn::HasDMSpawn())
	{
		const auto spawn = DMSpawn::GiveNextSpawn();
		const QAngle spawnAngle{0, spawn.lookY, 0};
		pPlayer->SetLocalOrigin(spawn.pos + Vector(0,0,1));
		pPlayer->SetAbsVelocity(vec3_origin);
		pPlayer->SetLocalAngles(spawnAngle);
		pPlayer->m_Local.m_vecPunchAngle = vec3_angle;
		pPlayer->m_Local.m_vecPunchAngleVel = vec3_angle;
		pPlayer->SnapEyeAngles(spawnAngle);
		return nullptr;
	}

	return BaseClass::GetPlayerSpawnSpot(pPlayer);
}

#endif

void CNEORules::SetRoundStatus(NeoRoundStatus status)
{
	if (status != NeoRoundStatus::PreRoundFreeze && status != NeoRoundStatus::PostRound)
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
			UTIL_CenterPrintAll("- GO! GO! GO! -\n");

			if (CNEOGameConfig::s_pGameRulesToConfig)
			{
				CNEOGameConfig::s_pGameRulesToConfig->OutputRoundStart();
			}
		}
#endif
	}

#ifdef GAME_DLL
	if (status == NeoRoundStatus::PreRoundFreeze)
	{ // we clear these so people who rejoin on a different round to the round when they left aren't prevented from spawning. This is done before all players are 
	  // spawned on the new round so these values will be overwritten for those players who are still in the game
		auto currentHandle = m_pRestoredInfos.FirstHandle();
		while (m_pRestoredInfos.IsValidHandle(currentHandle))
		{
			m_pRestoredInfos[currentHandle].spawnedThisRound = false;
			m_pRestoredInfos[currentHandle].deathTime = 0.f;
			currentHandle = m_pRestoredInfos.NextHandle(currentHandle);
		}
	}
#endif // GAME_DLL

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

int CNEORules::GetHiddenHudElements(void)
{
	return m_iHiddenHudElements;
}

int CNEORules::GetForcedTeam(void)
{
	return m_iForcedTeam.Get();
}

int CNEORules::GetForcedClass(void)
{
	return m_iForcedClass;
}

int CNEORules::GetForcedSkin(void)
{
	return m_iForcedSkin;
}

int CNEORules::GetForcedWeapon(void)
{
	return m_iForcedWeapon;
}

inline const char* CNEORules::GetGameTypeName(void)
{
	return NEO_GAME_TYPE_SETTINGS[GetGameType()].gameTypeName;
}

inline const bool CNEORules::CanChangeTeamClassLoadoutWhenAlive()
{
	return NEO_GAME_TYPE_SETTINGS[GetGameType()].changeTeamClassLoadoutWhenAlive;
}

inline const bool CNEORules::CanRespawnAnyTime()
{
	return NEO_GAME_TYPE_SETTINGS[GetGameType()].respawns;
}

float CNEORules::GetRemainingPreRoundFreezeTime(const bool clampToZero) const
{
	// If there's no time left, return 0 instead of a negative value.
	if (clampToZero)
	{
		return Max(0.0f, m_flNeoRoundStartTime + sv_neo_preround_freeze_time.GetFloat() - gpGlobals->curtime);
	}
	// Or return negative value of how many seconds late we were.
	else
	{
		return m_flNeoRoundStartTime + sv_neo_preround_freeze_time.GetFloat() - gpGlobals->curtime;
	}
}

const char *CNEORules::GetTeamClantag(const int iTeamNum) const
{
	switch (iTeamNum)
	{
	case TEAM_JINRAI: return m_szNeoJinraiClantag.Get();
	case TEAM_NSF: return m_szNeoNSFClantag.Get();
	default: return "";
	}
}

#ifdef GAME_DLL
void CNEORules::OnNavMeshLoad(void)
{
	TheNavMesh->SetPlayerSpawnName("info_player_defender");
}
#endif
