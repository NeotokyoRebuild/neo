#include "cbase.h"
#include "neo_player.h"
#include "neo_gamerules.h"
#include "team_control_point_master.h"
#include "team_train_watcher.h"
#include "neo_bot.h"
#include "neo_bot_manager.h"
#include "neo_bot_vision.h"
#include "trigger_area_capture.h"
#include "GameEventListener.h"
#include "NextBotUtil.h"
#include "tier3/tier3.h"
#include "vgui/ILocalize.h"
#include "soundenvelope.h"
#include "weapon_neobasecombatweapon.h"
#include "weapon_knife.h"
#include "nav_mesh.h"
#include "neo_penetration_resistance.h"
#include "neo_shot_manipulator.h"
#include "decals.h"
#include "neo_weapon_loadout.h"
#include "behavior/neo_bot_behavior.h"

ConVar neo_bot_notice_gunfire_range("neo_bot_notice_gunfire_range", "3000", FCVAR_GAMEDLL);
ConVar neo_bot_notice_quiet_gunfire_range("neo_bot_notice_quiet_gunfire_range", "500", FCVAR_GAMEDLL);
ConVar neo_bot_near_point_travel_distance("neo_bot_near_point_travel_distance", "750", FCVAR_CHEAT, "If within this travel distance to the current point, bot is 'near' it");

ConVar neo_bot_debug_tags("neo_bot_debug_tags", "0", FCVAR_CHEAT, "ent_text will only show tags on bots");

ConVar neo_bot_ignore_real_players("neo_bot_ignore_real_players", "0", FCVAR_CHEAT);

ConVar neo_bot_shotgunner_range("neo_bot_shotgunner_range", "320", FCVAR_NONE);
ConVar neo_bot_recon_ratio("neo_bot_recon_ratio", "0.2", FCVAR_NONE);
ConVar neo_bot_support_ratio("neo_bot_support_ratio", "0.2", FCVAR_NONE);

extern ConVar bot_class;
extern ConVar neo_bot_fire_weapon_min_time;
extern ConVar neo_bot_difficulty;
extern ConVar neo_bot_farthest_visible_theater_sample_count;
extern ConVar neo_bot_path_lookahead_range;
extern ConVar neo_bot_path_around_friendly_cooldown;



//-----------------------------------------------------------------------------------------------------
bool IsTeamName(const char* string)
{
	if (!V_stricmp(string, TEAM_STR_JINRAI))
		return true;

	if (!V_stricmp(string, TEAM_STR_NSF))
		return true;

	return false;
}



//-----------------------------------------------------------------------------------------------------
int Bot_GetTeamByName(const char* string)
{
	if (!V_stricmp(string, TEAM_STR_JINRAI))
	{
		return TEAM_JINRAI;
	}
	else if (!V_stricmp(string, TEAM_STR_NSF))
	{
		return TEAM_NSF;
	}

	return TEAM_UNASSIGNED;
}


//-----------------------------------------------------------------------------------------------------
CNEOBot::DifficultyType StringToDifficultyLevel(const char* string)
{
	if (!V_stricmp(string, "easy"))
		return CNEOBot::EASY;

	if (!V_stricmp(string, "normal"))
		return CNEOBot::NORMAL;

	if (!V_stricmp(string, "hard"))
		return CNEOBot::HARD;

	if (!V_stricmp(string, "expert"))
		return CNEOBot::EXPERT;

	return CNEOBot::UNDEFINED;
}


//-----------------------------------------------------------------------------------------------------
CON_COMMAND_F(neo_bot_add, "Add a bot.", FCVAR_GAMEDLL)
{
	// Listenserver host or rcon access only!
	if (!UTIL_IsCommandIssuedByServerAdmin())
		return;

	bool bQuotaManaged = true;
	int botCount = 1;
	const char* teamname = "auto";
	const char* pszBotNameViaArg = NULL;
	CNEOBot::DifficultyType skill = clamp((CNEOBot::DifficultyType)neo_bot_difficulty.GetInt(), CNEOBot::EASY, CNEOBot::EXPERT);

	int i;
	for (i = 1; i < args.ArgC(); ++i)
	{
		CNEOBot::DifficultyType trySkill = StringToDifficultyLevel(args.Arg(i));
		int nArgAsInteger = atoi(args.Arg(i));

		// each argument could be a classname, a team, a difficulty level, a count, or a name
		if (IsTeamName(args.Arg(i)))
		{
			teamname = args.Arg(i);
		}
		else if (!V_stricmp(args.Arg(i), "noquota"))
		{
			bQuotaManaged = false;
		}
		else if (trySkill != CNEOBot::UNDEFINED)
		{
			skill = trySkill;
		}
		else if (nArgAsInteger > 0)
		{
			botCount = nArgAsInteger;
			pszBotNameViaArg = NULL; // can't have a custom name if spawning multiple bots
		}
		else if (botCount == 1)
		{
			pszBotNameViaArg = args.Arg(i);
		}
		else
		{
			Warning("Invalid argument '%s'\n", args.Arg(i));
		}
	}

	const CNEOBotProfileFilter botFilter = {
		.flagTargetDifficulty = (1 << skill),
	};

	int iTeam = Bot_GetTeamByName(teamname);

	if (NEORules()->IsTeamplay() && iTeam == TEAM_UNASSIGNED)
	{
		CTeam* pJinrai = GetGlobalTeam(TEAM_JINRAI);
		CTeam* pNSF = GetGlobalTeam(TEAM_NSF);
		const int numJinrai = pJinrai->GetNumPlayers();
		const int numNSF = pNSF->GetNumPlayers();

		iTeam = numJinrai < numNSF ? TEAM_JINRAI : numNSF < numJinrai ? TEAM_NSF : RandomInt(TEAM_JINRAI, TEAM_NSF);
	}

	int iNumAdded = 0;
	for (i = 0; i < botCount; ++i)
	{
		CNEOBot* pBot = NULL;

		const CNEOBotProfileReturn retProfile = NEOBotProfileNextPick(botFilter);

		char szBotName[MAX_PLAYER_NAME_LENGTH];
		const auto curBotSkill = static_cast<CNEOBot::DifficultyType>(
				NEOBotProfileCreateNameRetSkill(szBotName, retProfile.profile, skill, pszBotNameViaArg));

		pBot = NextBotCreatePlayerBot< CNEOBot >(szBotName);

		if (pBot)
		{
			pBot->m_iProfileIdx = retProfile.index;
			V_memcpy(&pBot->m_profile, &retProfile.profile, sizeof(CNEOBotProfile));
			if (bQuotaManaged)
			{
				pBot->SetAttribute(CNEOBot::QUOTA_MANANGED);
			}

			engine->SetFakeClientConVarValue(pBot->edict(), "name", pBot->GetPlayerName() );
			pBot->RequestSetSkin(RandomInt(0, 2));
			pBot->HandleCommand_JoinTeam(iTeam);
			pBot->SetDifficulty(curBotSkill);

			++iNumAdded;
		}
	}

	if (bQuotaManaged)
	{
		TheNEOBots().OnForceAddedBots(iNumAdded);
	}
}
CON_COMMAND_EXTERN(bot_add, neo_bot_add, "Add a bot.");


//-----------------------------------------------------------------------------------------------------
CON_COMMAND_F(neo_bot_kick, "Remove a NEOBot by name, or all bots (\"all\").", FCVAR_GAMEDLL)
{
	// Listenserver host or rcon access only!
	if (!UTIL_IsCommandIssuedByServerAdmin())
		return;

	if (args.ArgC() < 2)
	{
		DevMsg("%s <bot name>, \"jinrai\", \"nsf\", or \"all\"> <optional: \"moveToSpectatorTeam\"> \n", args.Arg(0));
		return;
	}

	bool bMoveToSpectatorTeam = false;
	int iTeam = TEAM_UNASSIGNED;
	int i;
	const char* pPlayerName = "";
	for (i = 1; i < args.ArgC(); ++i)
	{
		// each argument could be a classname, a team, or a count
		if (FStrEq(args.Arg(i), "jinrai"))
		{
			iTeam = TEAM_JINRAI;
		}
		else if (FStrEq(args.Arg(i), "nsf"))
		{
			iTeam = TEAM_NSF;
		}
		else if (FStrEq(args.Arg(i), "all"))
		{
			iTeam = TEAM_ANY;
		}
		else if (FStrEq(args.Arg(i), "moveToSpectatorTeam"))
		{
			bMoveToSpectatorTeam = true;
		}
		else
		{
			pPlayerName = args.Arg(i);
		}
	}

	int iNumKicked = 0;
	for (int i = 1; i <= gpGlobals->maxClients; ++i)
	{
		CBasePlayer* player = static_cast<CBasePlayer*>(UTIL_PlayerByIndex(i));

		if (!player)
			continue;

		if (FNullEnt(player->edict()))
			continue;

		if (player->MyNextBotPointer())
		{
			if (iTeam == TEAM_ANY ||
				FStrEq(pPlayerName, player->GetPlayerName()) ||
				(player->GetTeamNumber() == iTeam) ||
				(player->GetTeamNumber() == iTeam))
			{
				if (bMoveToSpectatorTeam)
				{
					player->ChangeTeam(TEAM_SPECTATOR, false, true);
				}
				else
				{
					engine->ServerCommand(UTIL_VarArgs("kickid %d\n", player->GetUserID()));
				}
				CNEOBot* pBot = dynamic_cast<CNEOBot*>(player);
				if (pBot && pBot->HasAttribute(CNEOBot::QUOTA_MANANGED))
				{
					++iNumKicked;
				}
			}
		}
	}
	TheNEOBots().OnForceKickedBots(iNumKicked);
}


//-----------------------------------------------------------------------------------------------------
CON_COMMAND_F(neo_bot_kill, "Kill a NEOBot by name, or all bots (\"all\").", FCVAR_GAMEDLL)
{
	// Listenserver host or rcon access only!
	if (!UTIL_IsCommandIssuedByServerAdmin())
		return;

	if (args.ArgC() < 2)
	{
		DevMsg("%s <bot name>, \"jinrai\", \"nsf\", or \"all\"> <optional: \"moveToSpectatorTeam\"> \n", args.Arg(0));
		return;
	}

	int iTeam = TEAM_UNASSIGNED;
	int i;
	const char* pPlayerName = "";
	for (i = 1; i < args.ArgC(); ++i)
	{
		// each argument could be a classname, a team, or a count
		if (FStrEq(args.Arg(i), "jinrai"))
		{
			iTeam = TEAM_JINRAI;
		}
		else if (FStrEq(args.Arg(i), "nsf"))
		{
			iTeam = TEAM_NSF;
		}
		else if (FStrEq(args.Arg(i), "all"))
		{
			iTeam = TEAM_ANY;
		}
		else if (FStrEq(args.Arg(i), "moveToSpectatorTeam"))
		{
			// bMoveToSpectatorTeam = true;
		}
		else
		{
			pPlayerName = args.Arg(i);
		}
	}

	for (int i = 1; i <= gpGlobals->maxClients; ++i)
	{
		CBasePlayer* player = static_cast<CBasePlayer*>(UTIL_PlayerByIndex(i));

		if (!player)
			continue;

		if (FNullEnt(player->edict()))
			continue;

		if (player->MyNextBotPointer())
		{
			if (iTeam == TEAM_ANY ||
				FStrEq(pPlayerName, player->GetPlayerName()) ||
				(player->GetTeamNumber() == iTeam) ||
				(player->GetTeamNumber() == iTeam))
			{
				CTakeDamageInfo info(player, player, 9999999.9f, DMG_ENERGYBEAM);
				player->TakeDamage(info);
			}
		}
	}
}

//-----------------------------------------------------------------------------------------------------
void CMD_BotWarpTeamToMe(void)
{
	CBasePlayer* player = UTIL_GetListenServerHost();
	if (!player)
		return;

	CTeam* myTeam = player->GetTeam();
	for (int i = 0; i < myTeam->GetNumPlayers(); ++i)
	{
		if (!myTeam->GetPlayer(i)->IsAlive())
			continue;

		myTeam->GetPlayer(i)->SetAbsOrigin(player->GetAbsOrigin());
	}
}
static ConCommand neo_bot_warp_team_to_me("neo_bot_warp_team_to_me", CMD_BotWarpTeamToMe, "", FCVAR_GAMEDLL | FCVAR_CHEAT);


//-----------------------------------------------------------------------------------------------------
LINK_ENTITY_TO_CLASS(neo_bot, CNEOBot);

//-----------------------------------------------------------------------------------------------------
BEGIN_ENT_SCRIPTDESC( CNEOBot, CNEO_Player, "Beep boop beep boop :3" )
DEFINE_SCRIPTFUNC_NAMED( SetAttribute, "AddBotAttribute", "Sets attribute flags on this HL2MPBot" )
DEFINE_SCRIPTFUNC_NAMED( ClearAttribute, "RemoveBotAttribute", "Removes attribute flags on this HL2MPBot" )
DEFINE_SCRIPTFUNC_NAMED( ClearAllAttributes, "ClearAllBotAttributes", "Clears all attribute flags on this HL2MPBot" )
DEFINE_SCRIPTFUNC_NAMED( HasAttribute, "HasBotAttribute", "Checks if this HL2MPBot has the given attributes" )

DEFINE_SCRIPTFUNC_NAMED( AddTag, "AddBotTag", "Adds a bot tag" )
DEFINE_SCRIPTFUNC_NAMED( RemoveTag, "RemoveBotTag", "Removes a bot tag" )
DEFINE_SCRIPTFUNC_NAMED( ClearTags, "ClearAllBotTags", "Clears bot tags" )
DEFINE_SCRIPTFUNC_NAMED( HasTag, "HasBotTag", "Checks if this HL2MPBot has the given bot tag" )

DEFINE_SCRIPTFUNC_NAMED( SetWeaponRestriction, "AddWeaponRestriction", "Adds weapon restriction flags" )
DEFINE_SCRIPTFUNC_NAMED( RemoveWeaponRestriction, "RemoveWeaponRestriction", "Removes weapon restriction flags" )
DEFINE_SCRIPTFUNC_NAMED( ClearWeaponRestrictions, "ClearAllWeaponRestrictions", "Removes all weapon restriction flags" )
DEFINE_SCRIPTFUNC_NAMED( HasWeaponRestriction, "HasWeaponRestriction", "Checks if this HL2MPBot has the given weapon restriction flags" )

DEFINE_SCRIPTFUNC_WRAPPED( IsWeaponRestricted, "Checks if the given weapon is restricted for use on the bot" )

DEFINE_SCRIPTFUNC_WRAPPED( GetDifficulty, "Returns the bot's difficulty level" )
DEFINE_SCRIPTFUNC_WRAPPED( SetDifficulty, "Sets the bots difficulty level" )
DEFINE_SCRIPTFUNC_WRAPPED( IsDifficulty, "Returns true/false if the bot's difficulty level matches." )

DEFINE_SCRIPTFUNC_WRAPPED( GetHomeArea, "Sets the home nav area of the bot" )
DEFINE_SCRIPTFUNC_WRAPPED( SetHomeArea, "Returns the home nav area of the bot -- may be nil." )

DEFINE_SCRIPTFUNC_WRAPPED( DelayedThreatNotice, "" )
DEFINE_SCRIPTFUNC( UpdateDelayedThreatNotices, "" )

DEFINE_SCRIPTFUNC( SetMaxVisionRangeOverride, "Sets max vision range override for the bot" )
DEFINE_SCRIPTFUNC( GetMaxVisionRangeOverride, "Gets the max vision range override for the bot" )

DEFINE_SCRIPTFUNC( SetScaleOverride, "Sets the scale override for the bot" )

DEFINE_SCRIPTFUNC( SetAutoJump, "Sets if the bot should automatically jump" )
DEFINE_SCRIPTFUNC( ShouldAutoJump, "Returns if the bot should automatically jump" )

DEFINE_SCRIPTFUNC( IsInASquad, "Checks if we are in a squad" )
DEFINE_SCRIPTFUNC( LeaveSquad, "Makes us leave the current squad (if any)" )
DEFINE_SCRIPTFUNC( GetSquadFormationError, "Gets our formation error coefficient." )
DEFINE_SCRIPTFUNC( SetSquadFormationError, "Sets our formation error coefficient." )
DEFINE_SCRIPTFUNC_WRAPPED( DisbandCurrentSquad, "Forces the current squad to be entirely disbanded by everyone" )
DEFINE_SCRIPTFUNC_WRAPPED( FindVantagePoint, "Get the nav area of the closest vantage point (within distance)" )

DEFINE_SCRIPTFUNC_WRAPPED( SetAttentionFocus, "Sets our current attention focus to this entity" )
DEFINE_SCRIPTFUNC_WRAPPED( IsAttentionFocusedOn, "Is our attention focused on this entity" )
DEFINE_SCRIPTFUNC( ClearAttentionFocus, "Clear current focus" )
DEFINE_SCRIPTFUNC( IsAttentionFocused, "Is our attention focused right now?" )

DEFINE_SCRIPTFUNC( IsAmmoLow, "" )
DEFINE_SCRIPTFUNC( IsAmmoFull, "" )

DEFINE_SCRIPTFUNC_WRAPPED( GetSpawnArea, "Return the nav area of where we spawned" )

DEFINE_SCRIPTFUNC( PressFireButton, "" )
DEFINE_SCRIPTFUNC( PressAltFireButton, "" )
DEFINE_SCRIPTFUNC( PressSpecialFireButton, "" )

DEFINE_SCRIPTFUNC( GetBotId, "Get this bot's id" )
DEFINE_SCRIPTFUNC( FlagForUpdate, "Flag this bot for update" )
DEFINE_SCRIPTFUNC( IsFlaggedForUpdate, "Is this bot flagged for update" )
DEFINE_SCRIPTFUNC( GetTickLastUpdate, "Get last update tick" )
DEFINE_SCRIPTFUNC_WRAPPED( GetLocomotionInterface, "Get this bot's locomotion interface" )
DEFINE_SCRIPTFUNC_WRAPPED( GetBodyInterface, "Get this bot's body interface" )
DEFINE_SCRIPTFUNC_WRAPPED( GetIntentionInterface, "Get this bot's intention interface" )
DEFINE_SCRIPTFUNC_WRAPPED( GetVisionInterface, "Get this bot's vision interface" )
DEFINE_SCRIPTFUNC_WRAPPED( IsEnemy, "Return true if given entity is our enemy" )
DEFINE_SCRIPTFUNC_WRAPPED( IsFriend, "Return true if given entity is our friend" )
DEFINE_SCRIPTFUNC( IsImmobile, "Return true if we haven't moved in awhile" )
DEFINE_SCRIPTFUNC( GetImmobileDuration, "How long have we been immobile" )
DEFINE_SCRIPTFUNC( ClearImmobileStatus, "Clear immobile status" )
DEFINE_SCRIPTFUNC( GetImmobileSpeedThreshold, "Return units/second below which this actor is considered immobile" )

DEFINE_SCRIPTFUNC_NAMED( ScriptGetAllTags, "GetAllBotTags", "Get all bot tags" )

DEFINE_SCRIPTFUNC_WRAPPED( SetMission, "Set this bot's current mission to the given mission" )
DEFINE_SCRIPTFUNC_WRAPPED( SetPrevMission, "Set this bot's previous mission to the given mission" )
DEFINE_SCRIPTFUNC_WRAPPED( GetMission, "Get this bot's current mission" )
DEFINE_SCRIPTFUNC_WRAPPED( GetPrevMission, "Get this bot's previous mission" )
DEFINE_SCRIPTFUNC_WRAPPED( HasMission, "Return true if the given mission is this bot's current mission" )
DEFINE_SCRIPTFUNC( IsOnAnyMission, "Return true if this bot has a current mission" )
DEFINE_SCRIPTFUNC_WRAPPED( SetMissionTarget, "Set this bot's mission target to the given entity" )
DEFINE_SCRIPTFUNC_WRAPPED( GetMissionTarget, "Get this bot's current mission target" )

DEFINE_SCRIPTFUNC( SetBehaviorFlag, "Set the given behavior flag(s) for this bot" )
DEFINE_SCRIPTFUNC( ClearBehaviorFlag, "Clear the given behavior flag(s) for this bot" )
DEFINE_SCRIPTFUNC( IsBehaviorFlagSet, "Return true if the given behavior flag(s) are set for this bot" )

DEFINE_SCRIPTFUNC_WRAPPED( SetActionPoint, "Set the given action point for this bot" )
DEFINE_SCRIPTFUNC_WRAPPED( GetActionPoint, "Get the given action point for this bot" )

END_SCRIPTDESC();

//-----------------------------------------------------------------------------------------------------
/**
 * Allocate a bot and bind it to the edict
 */
CBasePlayer* CNEOBot::AllocatePlayerEntity(edict_t* edict, const char* playerName)
{
	CBasePlayer::s_PlayerEdict = edict;
	return static_cast<CBasePlayer*>(CreateEntityByName("neo_bot"));
}

//-----------------------------------------------------------------------------------------------------
void CNEOBot::PressFireButton(float duration)
{
	// can't fire if stunned
	// @todo Tom Bui: Eventually, we'll probably want to check the actual weapon for supress fire
	if (HasAttribute(CNEOBot::SUPPRESS_FIRE))
	{
		ReleaseFireButton();
		return;
	}

	BaseClass::PressFireButton(duration);
}


//-----------------------------------------------------------------------------------------------------
void CNEOBot::PressAltFireButton(float duration)
{
	// can't fire if stunned
	// @todo Tom Bui: Eventually, we'll probably want to check the actual weapon for supress fire
	if (HasAttribute(CNEOBot::SUPPRESS_FIRE))
	{
		ReleaseAltFireButton();
		return;
	}

	BaseClass::PressAltFireButton(duration);
}


//-----------------------------------------------------------------------------------------------------
void CNEOBot::PressSpecialFireButton(float duration)
{
	// can't fire if stunned
	// @todo Tom Bui: Eventually, we'll probably want to check the actual weapon for supress fire
	if (HasAttribute(CNEOBot::SUPPRESS_FIRE))
	{
		ReleaseAltFireButton();
		return;
	}

	BaseClass::PressSpecialFireButton(duration);
}


//-----------------------------------------------------------------------------------------------------
CNEOBot::CNEOBot()
{
	m_body = new CNEOBotBody(this);
	m_locomotor = new CNEOBotLocomotion(this);
	m_vision = new CNEOBotVision(this);
	m_intention = new CNEOBotIntention(this);

	m_spawnArea = NULL;
	m_weaponRestrictionFlags = 0;
	m_attributeFlags = 0;
	m_homeArea = NULL;
	m_squad = NULL;
	m_didReselectClass = false;
	m_spotWhereEnemySentryLastInjuredMe = vec3_origin;
	m_isLookingAroundForEnemies = true;
	m_behaviorFlags = 0;
	m_attentionFocusEntity = NULL;
	m_noisyTimer.Invalidate();
	m_repathAroundFriendlyTimer.Invalidate();

	m_difficulty = clamp((CNEOBot::DifficultyType)neo_bot_difficulty.GetInt(), CNEOBot::EASY, CNEOBot::EXPERT);

	m_actionPoint = NULL;
	m_proxy = NULL;
	m_spawner = NULL;

	SetMission(NO_MISSION, MISSION_DOESNT_RESET_BEHAVIOR_SYSTEM);
	SetMissionTarget(NULL);
	m_missionString.Clear();

	m_fModelScaleOverride = -1.0f;
	m_maxVisionRangeOverride = -1.0f;
	m_squadFormationError = 0.0f;

	m_bWantsRespawn = false;
	m_bRespawnCopyCorpse = false;

	SetAutoJump(0.f, 0.f);

	V_memcpy(&m_profile, &FIXED_DEFAULT_PROFILE, sizeof(CNEOBotProfile));

	// set default values for convars only present on the client
	edict_t* edict = GetEntity()->edict();
	if (edict)
	{
		constexpr struct {
			const char* name, *value;
		} convars[] = {
			{ "cl_neo_crosshair", NEO_CROSSHAIR_DEFAULT },
			{ "cl_neo_pvs_cull_roaming_observer", "0" },
			{ "cl_neo_streamermode", "0" },
			{ "cl_neo_tachi_prefer_auto", "1" },
			{ "cl_neo_taking_damage_sounds", "0" },
			{ "cl_onlysteamnick", "0" },
			{ "hap_HasDevice", "0" },
			{ "neo_clantag", "" },
			{ "neo_fov", "90" },
			{ "neo_name", "" },
		};

		for (const auto& convar : convars)
		{
			engine->SetFakeClientConVarValue(edict, convar.name, convar.value);
		}
	}
}


//-----------------------------------------------------------------------------------------------------
CNEOBot::~CNEOBot()
{
	// delete Intention first, since destruction of Actions may access other components
	if (m_intention)
		delete m_intention;

	if (m_body)
		delete m_body;

	if (m_locomotor)
		delete m_locomotor;

	if (m_vision)
		delete m_vision;

	NEOBotProfileFreePickByIdx(m_iProfileIdx);
}


//-----------------------------------------------------------------------------------------------------
void CNEOBot::Spawn()
{
	// CNEOBot do m_iNeoClass a bit earlier
	if ((m_iNextSpawnClassChoice != NEO_CLASS_RANDOM) && (m_iNeoClass != m_iNextSpawnClassChoice))
	{
		m_iNeoClass = m_iNextSpawnClassChoice;
	}

	const ENeoRank eRank = static_cast<ENeoRank>(GetRank(m_iXP) - 1);
	if (eRank == NEO_RANK_RANKLESS_DOG || (false == IN_BETWEEN_EQ(NEO_CLASS_RECON, m_iNeoClass, NEO_CLASS_VIP)))
	{
		m_iLoadoutWepChoice = 0;
	}
	else
	{
		const NEO_WEP_BITS_UNDERLYING_TYPE wepPrefsForCurRank = m_profile.flagsWepPrefs[m_iNeoClass][eRank];

		int iChosenWeps[MAX_WEAPON_LOADOUTS] = {};
		int iChosenWepsSize = 0;
		for (int i = 0; i < MAX_WEAPON_LOADOUTS; ++i)
		{
			if (wepPrefsForCurRank & CNEOWeaponLoadout::s_LoadoutWeapons[m_iNeoClass][i].info.m_iWepBit)
			{
				iChosenWeps[iChosenWepsSize++] = i;
			}
		}

		if (iChosenWepsSize == 0)
		{
			// Generally shouldn't happen, but if so, just pick from any under the XP limit
			for (int i = 0; i < MAX_WEAPON_LOADOUTS; ++i)
			{
				if (CNEOWeaponLoadout::s_LoadoutWeapons[m_iNeoClass][i].m_iWeaponPrice > m_iXP)
				{
					break;
				}
				iChosenWeps[iChosenWepsSize++] = i;
			}
		}

		if (iChosenWepsSize == 1)
		{
			m_iLoadoutWepChoice = iChosenWeps[0];
		}
		else
		{
			m_iLoadoutWepChoice = iChosenWeps[RandomInt(0, iChosenWepsSize - 1)];
		}
	}

	BaseClass::Spawn();

	m_spawnArea = NULL;
	m_justLostPointTimer.Invalidate();
	m_squad = NULL;
	m_didReselectClass = false;
	m_isLookingAroundForEnemies = true;
	m_attentionFocusEntity = NULL;
	GetLocomotionInterface()->m_bBreakBreakableInPath = false;

	m_delayedNoticeVector.RemoveAll();

	ClearTags();

	m_requiredWeaponStack.Clear();

	SetSquadFormationError(0.0f);
	SetBrokenFormation(false);

	GetVisionInterface()->ForgetAllKnownEntities();

	m_qPrevShouldAim = ANSWER_NO;
	m_flLastShouldAimTime = 0.0f;

	m_bWantsRespawn = false;
	m_bRespawnCopyCorpse = false;
}


//-----------------------------------------------------------------------------------------------------
void CNEOBot::SetMission(MissionType mission, bool resetBehaviorSystem)
{
	SetPrevMission(m_mission);
	m_mission = mission;

	if (resetBehaviorSystem)
	{
		// reset the behavior system to start the given mission
		GetIntentionInterface()->Reset();
	}

	// Temp hack - some missions play an idle loop
	if (m_mission > NO_MISSION)
	{
		StartIdleSound();
	}
}


//-----------------------------------------------------------------------------------------------------
extern void respawn(CBaseEntity* pEdict, bool fCopyCorpse);
void CNEOBot::PhysicsSimulate(void)
{
	BaseClass::PhysicsSimulate();

	if (m_bWantsRespawn)
	{
		m_bWantsRespawn = false;
		respawn(this, m_bRespawnCopyCorpse);
		// Note: respawn() triggers Reset(), which deletes the behavior.
		// Since we are outside BaseClass::PhysicsSimulate() (which calls Update()),
		// it is safe to delete the behavior now.
		return;
	}

	if (m_spawnArea == NULL)
	{
		m_spawnArea = GetLastKnownArea();
	}

	if (IsInASquad())
	{
		if (GetSquad()->GetMemberCount() <= 1 || GetSquad()->GetLeader() == NULL)
		{
			// squad has collapsed - disband it
			LeaveSquad();
		}
	}
}


//-----------------------------------------------------------------------------------------------------
void CNEOBot::Touch(CBaseEntity* pOther)
{
	BaseClass::Touch(pOther);

	CNEO_Player* them = ToNEOPlayer(pOther);
	if (them && IsEnemy(them))
	{
		// always notice if we bump an enemy
		TheNextBots().OnWeaponFired(them, them->GetActiveWeapon());
	}
}


//-----------------------------------------------------------------------------------------------------
void CNEOBot::AvoidPlayers(CUserCmd* pCmd)
{
#ifdef AVOID_PLAYERS_TEST
	Vector forward, right;
	EyeVectors(&forward, &right);

	CUtlVector< CNEO_Player* > playerVector;
	CollectPlayers(&playerVector, GetTeamNumber(), COLLECT_ONLY_LIVING_PLAYERS);

	Vector avoidVector = vec3_origin;

	float tooClose = 50.0f;

	for (int i = 0; i < playerVector.Count(); ++i)
	{
		CNEO_Player* them = playerVector[i];

		if (IsSelf(them))
		{
			continue;
		}

		if (IsInASquad())
		{
			continue;
		}

		Vector between = GetAbsOrigin() - them->GetAbsOrigin();
		if (between.IsLengthLessThan(tooClose))
		{
			float range = between.NormalizeInPlace();

			avoidVector += (1.0f - (range / tooClose)) * between;
		}
	}

	if (avoidVector.IsZero())
	{
		return;
	}

	avoidVector.NormalizeInPlace();

	const float maxSpeed = 50.0f;

	float ahead = maxSpeed * DotProduct(forward, avoidVector);
	float side = maxSpeed * DotProduct(right, avoidVector);

	pCmd->forwardmove += ahead;
	pCmd->sidemove += side;
#endif
}


//-----------------------------------------------------------------------------------------------------
void CNEOBot::UpdateOnRemove(void)
{
	StopIdleSound();

	BaseClass::UpdateOnRemove();
}


//-----------------------------------------------------------------------------------------------------
int CNEOBot::ShouldTransmit(const CCheckTransmitInfo* pInfo)
{
	return BaseClass::ShouldTransmit(pInfo);
}


//-----------------------------------------------------------------------------------------------------
void CNEOBot::ModifyMaxHealth(int nNewMaxHealth, bool bSetCurrentHealth /*= true*/)
{
	if (bSetCurrentHealth)
	{
		SetHealth(nNewMaxHealth);
	}
}

//-----------------------------------------------------------------------------------------------------
/**
 * Invoked when a game event occurs
 */
void CNEOBot::FireGameEvent(IGameEvent* event)
{
}

extern ConVar nb_update_frequency;
void CNEOBot::Update()
{
	if (!TheNavMesh->IsLoaded())
	{
		if ( IsDebugging( NEXTBOT_DEBUG_ALL ) )
		{
			CFmtStr msg;
			const float messageTime = nb_update_frequency.GetFloat() + (gpGlobals->frametime * 2);
			GetEntity()->EntityText( 0, msg.sprintf( "#%d", GetEntity()->entindex() ), messageTime );
			GetEntity()->EntityText( 1, msg.sprintf( "No navigation mesh, updates frozen" ), messageTime );
		}
		return;
	}
	BaseClass::Update();
}

//-----------------------------------------------------------------------------------------------------
void CNEOBot::Event_Killed(const CTakeDamageInfo& info)
{
	BaseClass::Event_Killed(info);

	if (HasProxy())
	{
		GetProxy()->OnKilled();
	}

	if (HasSpawner())
	{
		GetSpawner()->OnBotKilled(this);
	}

	if (IsInASquad())
	{
		LeaveSquad();
	}

	CNavArea* lastArea = (CNavArea*)GetLastKnownArea();
	if (lastArea)
	{
		// remove us from old visible set
		NavAreaCollector wasVisible;
		lastArea->ForAllPotentiallyVisibleAreas(wasVisible);
	}

	StopIdleSound();
}


//---------------------------------------------------------------------------------------------
class CCollectReachableObjects : public ISearchSurroundingAreasFunctor
{
public:
	CCollectReachableObjects(const CNEOBot* me, float maxRange, const CUtlVector< CHandle< CBaseEntity > >& potentialVector, CUtlVector< CHandle< CBaseEntity > >* collectionVector) : m_potentialVector(potentialVector)
	{
		m_me = me;
		m_maxRange = maxRange;
		m_collectionVector = collectionVector;
	}

	virtual bool operator() (CNavArea* area, CNavArea* priorArea, float travelDistanceSoFar)
	{
		// do any of the potential objects overlap this area?
		FOR_EACH_VEC(m_potentialVector, it)
		{
			CBaseEntity* obj = m_potentialVector[it];

			if (obj && area->Contains(obj->WorldSpaceCenter()))
			{
				// reachable - keep it
				if (!m_collectionVector->HasElement(obj))
				{
					m_collectionVector->AddToTail(obj);
				}
			}
		}
		return true;
	}

	virtual bool ShouldSearch(CNavArea* adjArea, CNavArea* currentArea, float travelDistanceSoFar)
	{
		if (adjArea->IsBlocked(m_me->GetTeamNumber()))
		{
			return false;
		}

		if (travelDistanceSoFar > m_maxRange)
		{
			// too far away
			return false;
		}

		return currentArea->IsContiguous(adjArea);
	}

	const CNEOBot* m_me;
	float m_maxRange;
	const CUtlVector< CHandle< CBaseEntity > >& m_potentialVector;
	CUtlVector< CHandle< CBaseEntity > >* m_collectionVector;
};


//
// Search outwards from startSearchArea and collect all reachable objects from the given list that pass the given filter
// Items in selectedObjectVector will be approximately sorted in nearest-to-farthest order (because of SearchSurroundingAreas)
//
void CNEOBot::SelectReachableObjects(const CUtlVector< CHandle< CBaseEntity > >& candidateObjectVector,
	CUtlVector< CHandle< CBaseEntity > >* selectedObjectVector,
	const INextBotFilter& filter,
	CNavArea* startSearchArea,
	float maxRange) const
{
	if (startSearchArea == NULL || selectedObjectVector == NULL)
		return;

	selectedObjectVector->RemoveAll();

	// filter candidate objects
	CUtlVector< CHandle< CBaseEntity > > filteredObjectVector;
	for (int i = 0; i < candidateObjectVector.Count(); ++i)
	{
		if (filter.IsSelected(candidateObjectVector[i]))
		{
			filteredObjectVector.AddToTail(candidateObjectVector[i]);
		}
	}

	// only keep those that are reachable by us
	CCollectReachableObjects collector(this, maxRange, filteredObjectVector, selectedObjectVector);
	SearchSurroundingAreas(startSearchArea, collector);
}


//---------------------------------------------------------------------------------------------
bool CNEOBot::IsAmmoLow(void) const
{
	CNEOBaseCombatWeapon* myWeapon = static_cast<CNEOBaseCombatWeapon*>(GetActiveWeapon());
	if (!myWeapon)
	{
		return false;
	}

	if (myWeapon->GetNeoWepBits() & NEO_WEP_KNIFE) // NEO TODO (Adam) NEO_WEP_MELEE
	{
		return false;
	}

	if (myWeapon->GetPrimaryAmmoType() == -1)
	{
		return false;
	}

	if (myWeapon->m_iClip1 + myWeapon->m_iPrimaryAmmoCount < (myWeapon->GetWpnData().iDefaultClip1 * 0.2))
	{
		return true;
	}

	return false;
}


//-----------------------------------------------------------------------------------------------------
bool CNEOBot::IsAmmoFull(void) const
{
	CNEOBaseCombatWeapon* myWeapon = static_cast<CNEOBaseCombatWeapon*>(GetActiveWeapon());
	if (!myWeapon)
	{
		return false;
	}

	if (myWeapon->GetNeoWepBits() & NEO_WEP_KNIFE)
	{
		// we never run out of ammo with a melee weapon
		return true;
	}

	if (myWeapon->GetPrimaryAmmoType() == -1)
	{
		return true;
	}

	FileWeaponInfo_t weaponInfo = myWeapon->GetWpnData();
	bool isPrimaryFull = myWeapon->m_iClip1 == weaponInfo.iMaxClip1 && myWeapon->m_iPrimaryAmmoCount == weaponInfo.iDefaultClip1;
	bool isSecondaryFull = myWeapon->m_iClip2 == weaponInfo.iMaxClip2 && myWeapon->m_iSecondaryAmmoCount == weaponInfo.iDefaultClip2;

	return isPrimaryFull && isSecondaryFull;
}

bool CNEOBot::IsCloakEnabled(void) const
{
	auto myBody = GetBodyInterface();
	return myBody && myBody->IsCloakEnabled();
}

float CNEOBot::GetCloakPower(void) const
{
	auto myBody = GetBodyInterface();
	return myBody ? myBody->GetCloakPower() : 0.0f;
}

void CNEOBot::EnableCloak(float threshold)
{
	if ( (GetCloakPower() >= threshold)
		&& !IsCloakEnabled() )
	{
		PressThermopticButton();
	}
}

void CNEOBot::DisableCloak(void)
{
	if ( IsCloakEnabled() )
	{
		PressThermopticButton();
	}
}


bool CNEOBot::IsDormantWhenDead(void) const
{
	// When a bot becomes an Observer (e.g., in Juggernaut mode after death without respawn), it should become dormant.
	// This stops the NextBot update loop (INextBot::Update), preventing it from trying to run behaviors 
	// on a non-existent or spectator body, which causes crashes (use-after-free/dangling pointers).
	return IsObserver();
}


//-----------------------------------------------------------------------------------------------------
/**
 * When someone fires their weapon
 */
void CNEOBot::OnWeaponFired(CBaseCombatCharacter* whoFired, CBaseCombatWeapon* weapon)
{
	VPROF_BUDGET("CNEOBot::OnWeaponFired", "NextBot");

	BaseClass::OnWeaponFired(whoFired, weapon);

	if (!whoFired || !whoFired->IsAlive())
		return;

	if (IsRangeGreaterThan(whoFired, neo_bot_notice_gunfire_range.GetFloat()))
		return;

	CNEOBaseCombatWeapon* neoWeapon = static_cast<CNEOBaseCombatWeapon*>(weapon);
	if (!neoWeapon)
	{
		return;
	}

	int noticeChance = 100;

	if (neoWeapon->GetNeoWepBits() & NEO_WEP_SUPPRESSED)
	{
		if (IsRangeGreaterThan(whoFired, neo_bot_notice_quiet_gunfire_range.GetFloat()))
		{
			// too far away to hear in any event
			return;
		}

		switch (GetDifficulty())
		{
		case EASY:
			noticeChance = 10;
			break;

		case NORMAL:
			noticeChance = 30;
			break;

		case HARD:
			noticeChance = 60;
			break;

		default:
		case EXPERT:
			noticeChance = 90;
			break;
		}

		if (IsEnvironmentNoisy())
		{
			// less likely to notice with all the noise
			noticeChance /= 2;
		}
	}
	else if (IsRangeLessThan(whoFired, 1000.0f))
	{
		// loud gunfire in our area - it's now "noisy" for a bit
		m_noisyTimer.Start(3.0f);
	}

	if (RandomInt(1, 100) > noticeChance)
	{
		return;
	}

	// notice the gunfire
	GetVisionInterface()->AddKnownEntity(whoFired);
}


//-----------------------------------------------------------------------------------------------------
class CFindClosestPotentiallyVisibleAreaToPos
{
public:
	CFindClosestPotentiallyVisibleAreaToPos(const Vector& pos)
	{
		m_pos = pos;
		m_closeArea = NULL;
		m_closeRangeSq = FLT_MAX;
	}

	bool operator() (CNavArea* baseArea)
	{
		CNavArea* area = (CNavArea*)baseArea;

		Vector close;
		area->GetClosestPointOnArea(m_pos, &close);

		float rangeSq = (close - m_pos).LengthSqr();
		if (rangeSq < m_closeRangeSq)
		{
			m_closeArea = area;
			m_closeRangeSq = rangeSq;
		}

		return true;
	}

	Vector m_pos;
	CNavArea* m_closeArea;
	float m_closeRangeSq;
};


//-----------------------------------------------------------------------------------------------------
// Update our view to watch where members of the given team will be coming from
void CNEOBot::UpdateLookingAroundForIncomingPlayers(bool lookForEnemies)
{
	if (!m_lookAtEnemyInvasionAreasTimer.IsElapsed())
		return;

	const float maxLookInterval = 1.0f;
	m_lookAtEnemyInvasionAreasTimer.Start(RandomFloat(0.333f, maxLookInterval));
}


//-----------------------------------------------------------------------------------------------------
/**
 * Update our view to keep an eye on areas where the enemy will be coming from
 */
void CNEOBot::UpdateLookingAroundForEnemies(void)
{
	if (!m_isLookingAroundForEnemies)
		return;

	if (HasAttribute(CNEOBot::IGNORE_ENEMIES))
		return;

	const float maxLookInterval = 1.0f;

	const CKnownEntity* known = GetVisionInterface()->GetPrimaryKnownThreat();

	if (known)
	{
		if (known->IsVisibleInFOVNow())
		{
			// I see you!
			GetBodyInterface()->AimHeadTowards(known->GetEntity(), IBody::CRITICAL, 1.0f, NULL, "Aiming at a visible threat");
			return;
		}

		/*
		if ( known->WasEverVisible() && known->GetTimeSinceLastSeen() < 3.0f )
		{
			// I saw you just a moment ago...
			GetBodyInterface()->AimHeadTowards( known->GetLastKnownPosition() + Vector( 0, 0, HumanEyeHeight ), IBody::IMPORTANT, 1.0f, NULL, "Aiming at a last known threat position" );
			return;
		}
		*/

		// known but not currently visible (I know you're around here somewhere)

		// if there is unobstructed space between us, turn around
		if (IsLineOfSightClear(known->GetEntity(), IGNORE_ACTORS))
		{
			Vector toThreat = known->GetEntity()->GetAbsOrigin() - GetAbsOrigin();
			float threatRange = toThreat.NormalizeInPlace();

			float aimError = M_PI / 6.0f;

			float s, c;
			FastSinCos(aimError, &s, &c);

			float error = threatRange * s;
			Vector imperfectAimSpot = known->GetEntity()->WorldSpaceCenter();
			imperfectAimSpot.x += RandomFloat(-error, error);
			imperfectAimSpot.y += RandomFloat(-error, error);

			GetBodyInterface()->AimHeadTowards(imperfectAimSpot, IBody::IMPORTANT, 1.0f, NULL, "Turning around to find threat out of our FOV");
			return;
		}

		{
			// look toward potentially visible area nearest the last known position
			CNavArea* myArea = GetLastKnownArea();
			if (myArea)
			{
				const CNavArea* closeArea = NULL;
				CFindClosestPotentiallyVisibleAreaToPos find(known->GetLastKnownPosition());
				myArea->ForAllPotentiallyVisibleAreas(find);

				closeArea = find.m_closeArea;

				if (closeArea)
				{
					// try to not look directly at walls
					const int retryCount = 10.0f;
					for (int r = 0; r < retryCount; ++r)
					{
						Vector gazeSpot = closeArea->GetRandomPoint() + Vector(0, 0, 0.75f * HumanHeight);

						if (GetVisionInterface()->IsLineOfSightClear(gazeSpot))
						{
							// use maxLookInterval so these looks override body aiming from path following
							GetBodyInterface()->AimHeadTowards(gazeSpot, IBody::IMPORTANT, maxLookInterval, NULL, "Looking toward potentially visible area near known but hidden threat");
							return;
						}
					}

					// can't find a clear line to look along
					if (IsDebugging(NEXTBOT_VISION | NEXTBOT_ERRORS))
					{
						ConColorMsg(Color(255, 255, 0, 255), "%3.2f: %s can't find clear line to look at potentially visible near known but hidden entity %s(#%d)\n",
							gpGlobals->curtime,
							GetDebugIdentifier(),
							known->GetEntity()->GetClassname(),
							known->GetEntity()->entindex());
					}
				}
				else if (IsDebugging(NEXTBOT_VISION | NEXTBOT_ERRORS))
				{
					ConColorMsg(Color(255, 255, 0, 255), "%3.2f: %s no potentially visible area to look toward known but hidden entity %s(#%d)\n",
						gpGlobals->curtime,
						GetDebugIdentifier(),
						known->GetEntity()->GetClassname(),
						known->GetEntity()->entindex());
				}
			}

			return;
		}
	}

	// no known threat - look toward where enemies will come from
	UpdateLookingAroundForIncomingPlayers(LOOK_FOR_ENEMIES);
}


//---------------------------------------------------------------------------------------------
class CFindVantagePoint : public ISearchSurroundingAreasFunctor
{
public:
	CFindVantagePoint(int enemyTeamIndex)
	{
		m_enemyTeamIndex = enemyTeamIndex;
		m_vantageArea = NULL;
	}

	virtual bool operator() (CNavArea* baseArea, CNavArea* priorArea, float travelDistanceSoFar)
	{
		CNavArea* area = (CNavArea*)baseArea;

		CTeam* enemyTeam = GetGlobalTeam(m_enemyTeamIndex);
		for (int i = 0; i < enemyTeam->GetNumPlayers(); ++i)
		{
			CNEO_Player* enemy = (CNEO_Player*)enemyTeam->GetPlayer(i);

			if (!enemy->IsAlive() || !enemy->GetLastKnownArea())
				continue;

			CNavArea* enemyArea = (CNavArea*)enemy->GetLastKnownArea();
			if (enemyArea->IsCompletelyVisible(area))
			{
				// nearby area from which we can see the enemy team
				m_vantageArea = area;
				return false;
			}
		}

		return true;
	}

	int m_enemyTeamIndex;
	CNavArea* m_vantageArea;
};


//-----------------------------------------------------------------------------------------------------
// Return a nearby area where we can see a member of the enemy team
CNavArea* CNEOBot::FindVantagePoint(float maxTravelDistance) const
{
	CFindVantagePoint find(GetTeamNumber() == TEAM_JINRAI ? TEAM_NSF : TEAM_JINRAI);
	SearchSurroundingAreas(GetLastKnownArea(), find, maxTravelDistance);
	return find.m_vantageArea;
}


//-----------------------------------------------------------------------------------------------------
/**
 * Return perceived danger of threat (0=none, 1=immediate deadly danger)
 * @todo: Move this to contextual query
 * @todo: Differentiate between potential threats (that sentry up ahead along our route) and immediate threats (the sentry I'm in range of)
 */
float CNEOBot::GetThreatDanger(CBaseCombatCharacter* who) const
{
	if (who == NULL)
		return 0.0f;

	if (who->IsPlayer())
	{
		CNEO_Player* player = static_cast<CNEO_Player*>(who);

#if 1
		CNEOBaseCombatWeapon *whoWeapon = static_cast<CNEOBaseCombatWeapon *>(player->GetActiveWeapon());
		if (whoWeapon && (whoWeapon->GetNeoWepBits() & (NEO_WEP_AA13 | NEO_WEP_SUPA7))) // NEO TODO (Adam) NEO_WEP_SHOTGUN
		{
			// Shotgunners are scary at close range
			if (IsRangeLessThan(player, neo_bot_shotgunner_range.GetFloat()))
			{
				return 0.7f;
			}
		}

		return 0.4f;
#else
		// misyl: I tried this, but it results in weird, unintuitive bot behavior.
		// Wasn't good!

		// A little fudge, make it so players with high K/D are more feared by bots.
		if (player->FragCount() == 0)
			return 0.0f;

		if (player->FragCount() < 2 && player->DeathCount() == 0)
			return 0.5f;

		if (player->DeathCount() == 0)
			return 1.0f;

		const float flKdRatio = player->FragCount() / (float)player->DeathCount();

		return flKdRatio;
#endif
	}

	return 0.0f;
}


//-----------------------------------------------------------------------------------------------------
/**
 * Return the max range at which we can effectively attack
 */
float CNEOBot::GetMaxAttackRange(void) const
{
	CNEOBaseCombatWeapon* myWeapon = static_cast<CNEOBaseCombatWeapon *>(GetActiveWeapon());
	if (!myWeapon)
	{
		return 0.0f;
	}

	if (myWeapon->GetNeoWepBits() & NEO_WEP_KNIFE)
	{
		return NEO_WEP_KNIFE_RANGE;
	}

	// bullet spray weapons, grenades, etc
	// for now, default to infinite so bot always returns fire and doesn't look dumb
	return FLT_MAX;
}


//-----------------------------------------------------------------------------------------------------
/**
 * Return the ideal range at which we can effectively attack
 */
float CNEOBot::GetDesiredAttackRange(void) const
{
	CNEOBaseCombatWeapon* myWeapon = static_cast<CNEOBaseCombatWeapon*>(GetActiveWeapon());
	if (!myWeapon)
	{
		return 0.0f;
	}

	if (myWeapon->GetNeoWepBits() & NEO_WEP_KNIFE)
	{
		return 32.0f;
	}

	if (myWeapon->GetNeoWepBits() & (NEO_WEP_AA13 | NEO_WEP_SUPA7))
	{
		return 100.0f;
	}

	if (myWeapon->GetNeoWepBits() & (NEO_WEP_JITTE | NEO_WEP_JITTE_S | NEO_WEP_KYLA | NEO_WEP_MILSO | NEO_WEP_MPN | NEO_WEP_MPN_S | NEO_WEP_SRM | NEO_WEP_SRM_S | NEO_WEP_TACHI))
	{
		return 250.0f;
	}

	if (myWeapon->GetNeoWepBits() & NEO_WEP_SCOPEDWEAPON)
	{
		return 1000.0f;
	}

	// bullet spray weapons, grenades, etc
	return 500.0f;
}


//-----------------------------------------------------------------------------------------------------
// If we're required to equip a specific weapon, do it.
bool CNEOBot::EquipRequiredWeapon(void)
{
	// if we have a required weapon on our stack, it takes precedence (items, etc)
	if (m_requiredWeaponStack.Count())
	{
		CBaseCombatWeapon* pWeapon = m_requiredWeaponStack.Top().Get();
		return Weapon_Switch(pWeapon);
	}

	if (TheNEOBots().IsMeleeOnly() || HasWeaponRestriction(MELEE_ONLY))
	{
		// force use of melee weapons
		Weapon_Switch(GetBludgeonWeapon());
		return true;
	}

	return false;
}


//-----------------------------------------------------------------------------------------------------
// Equip the best weapon we have to attack the given threat
void CNEOBot::EquipBestWeaponForThreat(const CKnownEntity* threat, const bool bNotPrimary)
{
	if (EquipRequiredWeapon())
		return;

	// -------------------------------------------------------------------------------

	CNEOBaseCombatWeapon* primaryWeapon = static_cast<CNEOBaseCombatWeapon *>(Weapon_GetSlot(0));
	CNEOBaseCombatWeapon* secondaryWeapon = static_cast<CNEOBaseCombatWeapon *>(Weapon_GetSlot(1));
	CNEOBaseCombatWeapon* knife = static_cast<CNEOBaseCombatWeapon *>(Weapon_GetSlot(2));
	CNEOBaseCombatWeapon* throwable = static_cast<CNEOBaseCombatWeapon *>(Weapon_GetSlot(3));

	// --------------------------------------------------------------------------------
	// Don't consider weapons that we have no ammo for (or filter out primary like shotgun + glass scenario)
	// We do not care about slugs
	if (bNotPrimary ||
			(primaryWeapon &&
			 	((primaryWeapon->GetNeoWepBits() & NEO_WEP_GHOST) ||
			 	 !primaryWeapon->m_iPrimaryAmmoType ||
				 (primaryWeapon->Clip1() + primaryWeapon->m_iPrimaryAmmoCount) <= 0)))
	{
		primaryWeapon = NULL;
	}

	if (secondaryWeapon && (secondaryWeapon->Clip1() + secondaryWeapon->m_iPrimaryAmmoCount <= 0))
		secondaryWeapon = NULL;

	// -------------------------------------------------------------------------------

	if (!threat || !threat->WasEverVisible() || threat->GetTimeSinceLastSeen() > 5.0f)
	{
		CNEOBaseCombatWeapon* pChosen = NULL;

		// Pull out weapons we want to reload.
		if (!pChosen && primaryWeapon && !primaryWeapon->Clip1())
			pChosen = primaryWeapon;

		if (!pChosen && secondaryWeapon && !secondaryWeapon->Clip1())
			pChosen = secondaryWeapon;

		// Go back to something neutral if they all have a clip.
		if (!pChosen && primaryWeapon)
			pChosen = primaryWeapon;

		if (!pChosen && secondaryWeapon)
			pChosen = secondaryWeapon;

		if (!pChosen && throwable)
			pChosen = throwable;

		if (!pChosen && knife) // NEO TODO (Adam) Can we give supports a knife?
			pChosen = knife;

		// No threat - go back to a gun, either SMG or something like that... Anything we need to reload.
		if (pChosen)
		{
			Weapon_Switch(pChosen);
		}

		return;
	}

	CNEOBaseCombatWeapon* pChosen = NULL;

#if 0
	bool bCanSeeTarget = threat->GetTimeSinceLastSeen() < 0.2f;
	// Don't stay in melee if they are far away, or we don't know where they are right now.
	bool bInMeleeRange = !IsRangeGreaterThan(threat->GetLastKnownPosition(), 127.0f) && bCanSeeTarget;
#endif

	if (throwable)
	{
		pChosen = throwable;
	}
	if (knife)
	{
		pChosen = knife;
	}
	if (secondaryWeapon)
	{
		pChosen = secondaryWeapon;
	}

	// When to set aside primary in special situations
	if (!primaryWeapon)
	{
		// passthrough
	}
	else if (secondaryWeapon
		&& primaryWeapon->GetNeoWepBits() & (NEO_WEP_AA13 | NEO_WEP_SUPA7)
		&& IsRangeGreaterThan(threat->GetLastKnownPosition(), 1000.0f))
	{
		// passthrough
	}
	else if (secondaryWeapon
		&& primaryWeapon->GetNeoWepBits() & (NEO_WEP_SCOPEDWEAPON)
		&& IsRangeLessThan(threat->GetLastKnownPosition(), 250.0f))
	{
		// passthrough
	}
	// Ideally for close range empty primary reaction
	else if ( secondaryWeapon
		&& primaryWeapon->Clip1() <= 0
		&& (secondaryWeapon->Clip1() > 0)
		&& threat->IsVisibleInFOVNow()
		&& (IsRangeLessThan(threat->GetLastKnownPosition(), 250.0f)) )
	{
		// passthrough
	}
	else
	{
		pChosen = primaryWeapon;
	}

	if (pChosen)
	{
		Weapon_Switch(pChosen);
	}
}


//-----------------------------------------------------------------------------------------------------
// Reload the active weapon if it makes sense for the situation 
void CNEOBot::ReloadIfLowClip(void)
{
	CNEOBaseCombatWeapon* myWeapon = static_cast<CNEOBaseCombatWeapon*>(GetActiveWeapon());
	if (myWeapon && myWeapon->GetPrimaryAmmoCount() > 0)
	{
		bool shouldReload = false;
		// SUPA7 reload doesn't discard ammo
		if ((myWeapon->GetNeoWepBits() & NEO_WEP_SUPA7) && (myWeapon->Clip1() < myWeapon->GetMaxClip1()))
		{
			shouldReload = true;
		}
		else
		{
			int maxClip = myWeapon->GetMaxClip1();
			bool isBarrage = IsBarrageAndReloadWeapon(myWeapon);

			int baseThreshold = isBarrage ? (maxClip / 3) : (maxClip / 2);

			float aggressionFactor = 1.0f - HealthFraction();

			float dynamicThreshold = baseThreshold + aggressionFactor * (maxClip - baseThreshold);

			if (myWeapon->Clip1() < static_cast<int>(dynamicThreshold))
			{
				shouldReload = true;
			}
		}

		if (shouldReload)
		{
			ReleaseFireButton();
			PressReloadButton();
		}
	}
}


//-----------------------------------------------------------------------------------------------------
// Force us to equip and use this weapon until popped off the required stack
void CNEOBot::PushRequiredWeapon(CNEOBaseCombatWeapon* weapon)
{
	m_requiredWeaponStack.Push(weapon);
}


//-----------------------------------------------------------------------------------------------------
// Pop top required weapon off of stack and discard
void CNEOBot::PopRequiredWeapon(void)
{
	m_requiredWeaponStack.Pop();
}


//-----------------------------------------------------------------------------------------------------
// return true if given weapon can be used to attack
bool CNEOBot::IsCombatWeapon(CNEOBaseCombatWeapon* weapon) const
{
	if (!weapon)
	{
		return false;
	}

	if (weapon->GetNeoWepBits() & NEO_WEP_GHOST)
	{
		return false;
	}

	return true;
}


//-----------------------------------------------------------------------------------------------------
// return true if given weapon is a "hitscan" weapon
bool CNEOBot::IsHitScanWeapon(CNEOBaseCombatWeapon* weapon) const
{
	if (!weapon)
	{
		return true;
	}

	if (weapon->GetNeoWepBits() & (NEO_WEP_KNIFE | NEO_WEP_GHOST | NEO_WEP_THROWABLE))
	{
		return false;
	}

	return true;
}


//-----------------------------------------------------------------------------------------------------
// return true if given weapon "sprays" bullets/fire/etc continuously (ie: not individual rockets/etc)
bool CNEOBot::IsContinuousFireWeapon(CNEOBaseCombatWeapon* weapon) const
{
	if (!weapon)
	{
		return false;
	}

	if (!IsCombatWeapon(weapon))
		return false;

	if (weapon && weapon->GetNeoWepBits() & (NEO_WEP_THROWABLE | NEO_WEP_KYLA | NEO_WEP_SRS | NEO_WEP_M41 | NEO_WEP_M41_L | NEO_WEP_M41_S | NEO_WEP_MILSO | NEO_WEP_TACHI | NEO_WEP_ZR68_L))
	{
		return false;
	}

	return true;
}


//-----------------------------------------------------------------------------------------------------
// return true if given weapon launches explosive projectiles with splash damage
bool CNEOBot::IsExplosiveProjectileWeapon(CNEOBaseCombatWeapon* weapon) const
{
	return false;
}


//-----------------------------------------------------------------------------------------------------
// return true if given weapon has small clip and long reload cost (ie: rocket launcher, etc)
bool CNEOBot::IsBarrageAndReloadWeapon(CNEOBaseCombatWeapon* weapon) const
{
	if (!weapon)
	{
		return false;
	}

	if (weapon && weapon->GetNeoWepBits() & (NEO_WEP_SUPA7 | NEO_WEP_PZ | NEO_WEP_SRS))
	{
		return true;
	}

	return false;
}


//-----------------------------------------------------------------------------------------------------
// Return true if given weapon doesn't make much sound when used (ie: spy knife, etc)
bool CNEOBot::IsQuietWeapon(CNEOBaseCombatWeapon* weapon) const
{
	if (!IsCombatWeapon(weapon))
	{
		return false;
	}

	if (weapon->GetNeoWepBits() & (NEO_WEP_SUPPRESSED | NEO_WEP_KNIFE))
	{
		return true;
	}

	return false;
}

unsigned int CNEOBot::LineOfFireMask(const LineOfFireFlags flags)
{
	// Flag set by outside which should already know if using shotgun or not
	// Shotgun cannot deal with windows at all
	if (flags & LINE_OF_FIRE_FLAGS_SHOTGUN)
	{
		return MASK_SHOT;
	}
	return MASK_SHOT & ~CONTENTS_WINDOW;
}

// A very basic penetration material check, it doesn't do the full penetration
// check and just simply base it on the material that it traced in front of the bot
// and a short distance.
bool CNEOBot::IsLineOfFirePenetrationClear(const trace_t &tr, const Vector &from, const Vector &to,
		const ELineOfFirePenetrationMode eMode) const
{
	// Min is making sure at least the distance under it is considered
	// Max is scaling up to penetration value of 100.0f which no weapon have, but
	// scaling works out in general
	static constexpr const float FL_METERS_TRY_PENETRATE_MIN = 9.0f;
	static constexpr const float FL_METERS_TRY_PENETRATE_MAX = 36.0f;

	int material = physprops->GetSurfaceData(tr.surface.surfaceProps)->game.material;
	if (material == CHAR_TEX_BLOCKBULLETS)
	{
		return false;
	}

	// Only bother with fire penetration in short distance
	auto *neoWeapon = static_cast<CNEOBaseCombatWeapon *>(GetActiveWeapon());
	if (!neoWeapon)
	{
		return false;
	}

	// Glass mode can just skip the distance check
	bool bAttemptPenTest = (eMode == LINE_OF_FIRE_PENETRATION_MODE_GLASS);
	if (!bAttemptPenTest)
	{
		const float flMaxTryDist = Clamp(
				(neoWeapon->GetPenetration() / 100.0f) * FL_METERS_TRY_PENETRATE_MAX,
				FL_METERS_TRY_PENETRATE_MIN, FL_METERS_TRY_PENETRATE_MAX);

		const float flMeters = METERS_PER_INCH * from.DistTo(to);
		bAttemptPenTest = (flMeters <= flMaxTryDist);
	}
	if (bAttemptPenTest)
	{
		material -= 'A';
		if (IN_BETWEEN_AR(0, material, MATERIALS_NUM))
		{
			// Just for simplicity, only try to fire against materials that can be penetrated
			if (PENETRATION_RESISTANCE[material] < 1.0f)
			{
				CNEOShotManipulator manipulator(0,
						const_cast<CNEOBot *>(this)->GetAutoaimVector(AUTOAIM_SCALE_DEFAULT),
						const_cast<CNEOBot *>(this),
						neoWeapon);

				Vector vecDir = manipulator.ApplySpread(const_cast<CNEOBot *>(this)->GetAttackSpread(neoWeapon));

				trace_t	penetrationTrace;
				TestPenetrationTrace(penetrationTrace, tr, vecDir, nullptr);

				// See if we found the surface again
				const bool bFoundSurface = (penetrationTrace.startsolid || tr.fraction == 0.0f || penetrationTrace.fraction == 1.0f);
				return !bFoundSurface;
			}
		}
	}
	return false;
}

bool CNEOBot::IsLineOfSightClear(CBaseEntity *entity, LineOfSightCheckType checkType) const
{
	if (auto *neoPlayer = ToNEOPlayer(entity); neoPlayer && neoPlayer->IsCarryingGhost())
	{
		return true;
	}
	return BaseClass::IsLineOfSightClear(entity, checkType);
}

//-----------------------------------------------------------------------------------------------------
// Return true if a weapon has no obstructions along the line between the given points
bool CNEOBot::IsLineOfFireClear(const Vector& from, const Vector& to, const LineOfFireFlags flags) const
{
	trace_t trace;
	NextBotTraceFilterIgnoreActors botFilter(NULL, COLLISION_GROUP_NONE);
	CTraceFilterIgnoreFriendlyCombatItems ignoreFriendlyCombatFilter(this, COLLISION_GROUP_NONE, GetTeamNumber());
	CTraceFilterChain filter(&botFilter, &ignoreFriendlyCombatFilter);

	const auto lofMask = LineOfFireMask(flags);
	UTIL_TraceLine(from, to, lofMask, &filter, &trace);

	const bool bIsClear = !trace.DidHit() || IsAbleToBreak(trace.m_pEnt);

	if (bIsClear && !(lofMask & CONTENTS_WINDOW))
	{
		// Do a second trace with the window mask added in, if this hits but not the first,
		// then it's a window and need to go to IsLineOfFirePenetrationClear so we can
		// differentiate between penetratable vs non-penetratable windows
		trace_t withWindowTrace;
		UTIL_TraceLine(from, to, MASK_SHOT, &filter, &withWindowTrace);
		if (withWindowTrace.DidHit())
		{
			return IsLineOfFirePenetrationClear(withWindowTrace, from, to, LINE_OF_FIRE_PENETRATION_MODE_GLASS);
		}
	}

	if (!bIsClear && !(flags & LINE_OF_FIRE_FLAGS_SHOTGUN) && (flags & LINE_OF_FIRE_FLAGS_PENETRATION))
	{
		return IsLineOfFirePenetrationClear(trace, from, to, LINE_OF_FIRE_PENETRATION_MODE_DEFAULT);
	}

	return bIsClear;
}


//-----------------------------------------------------------------------------------------------------
// Return true if a weapon has no obstructions along the line from our eye to the given position
bool CNEOBot::IsLineOfFireClear(const Vector& where, const LineOfFireFlags flags) const
{
	return IsLineOfFireClear(const_cast<CNEOBot*>(this)->EyePosition(), where, flags);
}


//-----------------------------------------------------------------------------------------------------
// Return whether there is a friendly player blocking the line of fire
bool CNEOBot::IsLineOfFireClearOfFriendlies(const Vector& from, CBaseEntity* who) const
{
	if (NEORules()->IsTeamplay())
	{
		trace_t playerTrace;
		CTraceFilterSimple playerFilter(this, COLLISION_GROUP_NONE);
		const Vector to = who->WorldSpaceCenter();
		UTIL_TraceLine(from, to, MASK_SHOT_HULL, &playerFilter, &playerTrace);

		if (playerTrace.DidHit())
		{
			if (playerTrace.m_pEnt && playerTrace.m_pEnt->IsPlayer())
			{
				// If it's a friendly player, line of fire is NOT clear
				if (playerTrace.m_pEnt->GetTeamNumber() == GetTeamNumber())
				{
					return false;
				}
			}
		}
	}
	return true;
}


//-----------------------------------------------------------------------------------------------------
// Return whether there is a friendly player blocking the line of fire
bool CNEOBot::IsLineOfFireClearOfFriendlies(const Vector& from, const Vector& to) const
{
	if (NEORules()->IsTeamplay())
	{
		trace_t playerTrace;
		CTraceFilterSimple playerFilter(this, COLLISION_GROUP_NONE);
		UTIL_TraceLine(from, to, MASK_SHOT_HULL, &playerFilter, &playerTrace);

		if (playerTrace.DidHit())
		{
			CBaseEntity *pEnt = playerTrace.m_pEnt;
			if (pEnt && pEnt->IsPlayer())
			{
				if (IsFriend(pEnt) && pEnt != this)
				{
					return false;
				}
			}
		}
	}
	return true;
}

//-----------------------------------------------------------------------------------------------------
// Return true if a weapon has no obstructions along the line between the given point and entity
bool CNEOBot::IsLineOfFireClear(const Vector& from, CBaseEntity* who, const LineOfFireFlags flags) const
{
	if (!IsLineOfFireClearOfFriendlies(from, who))
	{
		return false; // Friendly player blocking line of fire
	}

	trace_t trace;
	NextBotTraceFilterIgnoreActors botFilter(NULL, COLLISION_GROUP_NONE);
	CTraceFilterIgnoreFriendlyCombatItems ignoreFriendlyCombatFilter(this, COLLISION_GROUP_NONE, GetTeamNumber());
	CTraceFilterChain filter(&botFilter, &ignoreFriendlyCombatFilter);

	const Vector to = who->WorldSpaceCenter();
	const auto lofMask = LineOfFireMask(flags);
	UTIL_TraceLine(from, to, lofMask, &filter, &trace);

	const bool bIsClear = !trace.DidHit() || trace.m_pEnt == who || IsAbleToBreak(trace.m_pEnt);

	if (bIsClear && !(lofMask & CONTENTS_WINDOW))
	{
		// Do a second trace with the window mask added in, if this hits but not the first,
		// then it's a window and need to go to IsLineOfFirePenetrationClear so we can
		// differentiate between penetratable vs non-penetratable windows
		trace_t withWindowTrace;
		UTIL_TraceLine(from, to, MASK_SHOT, &filter, &withWindowTrace);
		if (withWindowTrace.DidHit())
		{
			return IsLineOfFirePenetrationClear(withWindowTrace, from, to, LINE_OF_FIRE_PENETRATION_MODE_GLASS);
		}
	}

	if (!bIsClear && !(flags & LINE_OF_FIRE_FLAGS_SHOTGUN) && (flags & LINE_OF_FIRE_FLAGS_PENETRATION))
	{
		return IsLineOfFirePenetrationClear(trace, from, to, LINE_OF_FIRE_PENETRATION_MODE_DEFAULT);
	}

	return bIsClear;
}


//-----------------------------------------------------------------------------------------------------
// Return true if a weapon has no obstructions along the line from our eye to the given entity
bool CNEOBot::IsLineOfFireClear(CBaseEntity* who, const LineOfFireFlags flags) const
{
	return IsLineOfFireClear(const_cast<CNEOBot*>(this)->EyePosition(), who, flags);
}


//-----------------------------------------------------------------------------------------------------
// If I am aiming at a friendly, recalculate my path to get around them
void CNEOBot::RepathIfFriendlyBlockingLineOfFire()
{
	if (!IsAlive())
	{
		return;
	}

	const PathFollower* pPath = GetCurrentPath();
	if (!pPath)
	{
		return;
	}

	if (!m_repathAroundFriendlyTimer.IsElapsed())
	{
		return;
	}

	float jitter = RandomFloat(0.0f, 0.5f);
	m_repathAroundFriendlyTimer.Start(neo_bot_path_around_friendly_cooldown.GetFloat() + jitter);

	Vector eyePos = EyePosition();
	Vector forward;
	EyeVectors(&forward);

	const float checkDistance = 10000.0f;
	Vector targetPos = eyePos + forward * checkDistance;

	if (!IsLineOfFireClearOfFriendlies(eyePos, targetPos))
	{
		Vector goal = pPath->GetEndPosition();

		CNEOBotPathCost cost(this, DEFAULT_ROUTE);
		if (m_repathAroundFriendlyFollower.Compute(this, goal, cost))
		{
			if (m_repathAroundFriendlyFollower.IsValid())
			{
				SetCurrentPath(&m_repathAroundFriendlyFollower);
			}
		}
	}
}


//-----------------------------------------------------------------------------------------------------
bool CNEOBot::IsEntityBetweenTargetAndSelf(CBaseEntity* other, CBaseEntity* target)
{
	Vector toTarget = target->GetAbsOrigin() - GetAbsOrigin();
	float rangeToTarget = toTarget.NormalizeInPlace();

	Vector toOther = other->GetAbsOrigin() - GetAbsOrigin();
	float rangeToOther = toOther.NormalizeInPlace();

	return rangeToOther < rangeToTarget && DotProduct(toTarget, toOther) > 0.7071f;
}


//-----------------------------------------------------------------------------------------------------
// Return the nearest human player on the given team who is looking directly at me
CNEO_Player* CNEOBot::GetClosestHumanLookingAtMe(int team)
{
	CUtlVector< CNEO_Player* > otherVector;
	CollectPlayers(&otherVector, team, COLLECT_ONLY_LIVING_PLAYERS);

	float closeRange = FLT_MAX;
	CNEO_Player* close = NULL;

	for (int i = 0; i < otherVector.Count(); ++i)
	{
		CNEO_Player* other = otherVector[i];

		if (other->IsBot())
			continue;

		Vector otherEye, otherForward;
		other->EyePositionAndVectors(&otherEye, &otherForward, NULL, NULL);

		Vector toMe = EyePosition() - otherEye;
		float range = toMe.NormalizeInPlace();

		if (range < closeRange)
		{
			const float cosTolerance = 0.98f;
			if (DotProduct(toMe, otherForward) > cosTolerance)
			{
				// a human is looking toward me - check LOS
				if (IsLineOfSightClear(otherEye, IGNORE_NOTHING, other))
				{
					close = other;
					closeRange = range;
				}
			}
		}
	}

	return close;
}


//-----------------------------------------------------------------------------------------------------
// become a member of the given squad
void CNEOBot::JoinSquad(CNEOBotSquad* squad)
{
	if (squad)
	{
		squad->Join(this);
		m_squad = squad;
	}
}


//-----------------------------------------------------------------------------------------------------
// leave our current squad
void CNEOBot::LeaveSquad(void)
{
	if (m_squad)
	{
		m_squad->Leave(this);
		m_squad = NULL;
	}
}

//-----------------------------------------------------------------------------------------------------
// leave our current squad
void CNEOBot::DeleteSquad(void)
{
	if (m_squad)
	{
		m_squad = NULL;
	}
}

//---------------------------------------------------------------------------------------------
bool CNEOBot::IsWeaponRestricted(CNEOBaseCombatWeapon* weapon) const
{
	if (!weapon)
	{
		return false;
	}

	if (HasWeaponRestriction(MELEE_ONLY))
	{
		return !IsBludgeon(weapon);
	}

	return false;
}

bool CNEOBot::ScriptIsWeaponRestricted(HSCRIPT script) const
{
	CBaseEntity* pEntity = ToEnt(script);
	if (!pEntity)
		return true;

	CNEOBaseCombatWeapon* pWeapon = dynamic_cast<CNEOBaseCombatWeapon*>(pEntity);
	if (!pWeapon)
		return true;

	return IsWeaponRestricted(pWeapon);
}


//---------------------------------------------------------------------------------------------
// Compute a pseudo random value (0-1) that stays consistent for the 
// given period of time, but changes unpredictably each period.
float CNEOBot::TransientlyConsistentRandomValue(float period, int seedValue) const
{
	CNavArea* area = GetLastKnownArea();
	if (!area)
	{
		return 0.0f;
	}

	// this term stays stable for 'period' seconds, then changes in an unpredictable way
	int timeMod = (int)(gpGlobals->curtime / period) + 1;
	return fabs(FastCos((float)(seedValue + (entindex() * area->GetID() * timeMod))));
}


//---------------------------------------------------------------------------------------------
// Given a target entity, find a target within 'maxSplashRadius' that has clear line of fire
// to both the target entity and to me.
bool CNEOBot::FindSplashTarget(CBaseEntity* target, float maxSplashRadius, Vector* splashTarget) const
{
	if (!target || !splashTarget)
		return false;

	*splashTarget = target->WorldSpaceCenter();

	const int retryCount = 50;
	for (int i = 0; i < retryCount; ++i)
	{
		Vector probe = target->WorldSpaceCenter() + RandomVector(-maxSplashRadius, maxSplashRadius);

		trace_t trace;
		NextBotTraceFilterIgnoreActors filter(NULL, COLLISION_GROUP_NONE);

		auto *myWeapon = static_cast<const CNEOBaseCombatWeapon *>(GetActiveWeapon());
		const bool bIsShotgun = (myWeapon && (myWeapon->GetNeoWepBits() & (NEO_WEP_AA13 | NEO_WEP_SUPA7)));
		const LineOfFireFlags flags = bIsShotgun ? LINE_OF_FIRE_FLAGS_SHOTGUN : LINE_OF_FIRE_FLAGS_DEFAULT;
		UTIL_TraceLine(target->WorldSpaceCenter(), probe, LineOfFireMask(flags), &filter, &trace);
		if (trace.DidHitWorld())
		{
			// can we shoot this spot?
			if (IsLineOfFireClear(trace.endpos, flags))
			{
				// yes, found a corner-sticky target
				*splashTarget = trace.endpos;

				NDebugOverlay::Line(target->WorldSpaceCenter(), trace.endpos, 255, 0, 0, true, 60.0f);
				NDebugOverlay::Cross3D(trace.endpos, 5.0f, 255, 255, 0, true, 60.0f);

				return true;
			}
		}
	}

	return false;
}


//---------------------------------------------------------------------------------------------
// Restrict bot's attention to only this entity (or radius around this entity) to the exclusion of everything else
void CNEOBot::SetAttentionFocus(CBaseEntity* focusOn)
{
	m_attentionFocusEntity = focusOn;
}


//---------------------------------------------------------------------------------------------
// Remove attention focus restrictions
void CNEOBot::ClearAttentionFocus(void)
{
	m_attentionFocusEntity = NULL;
}


//---------------------------------------------------------------------------------------------
bool CNEOBot::IsAttentionFocused(void) const
{
	return m_attentionFocusEntity != NULL;
}


//---------------------------------------------------------------------------------------------
bool CNEOBot::IsAttentionFocusedOn(CBaseEntity* who) const
{
	if (m_attentionFocusEntity == NULL || who == NULL)
	{
		return false;
	}

	if (m_attentionFocusEntity->entindex() == who->entindex())
	{
		// specifically focused on this entity
		return true;
	}

	CNEOBotActionPoint* actionPoint = dynamic_cast<CNEOBotActionPoint*>(m_attentionFocusEntity.Get());
	if (actionPoint)
	{
		// we attend to everything within the action point's radius
		return actionPoint->IsWithinRange(who);
	}

	return false;
}


//---------------------------------------------------------------------------------------------
// Notice the given threat after the given number of seconds have elapsed
void CNEOBot::DelayedThreatNotice(CHandle< CBaseEntity > who, float noticeDelay)
{
	float when = gpGlobals->curtime + noticeDelay;

	// if we already have a delayed notice for this threat, ignore the new one unless the delay is less
	for (int i = 0; i < m_delayedNoticeVector.Count(); ++i)
	{
		if (m_delayedNoticeVector[i].m_who == who)
		{
			if (m_delayedNoticeVector[i].m_when > when)
			{
				// update delay to shorter time
				m_delayedNoticeVector[i].m_when = when;
			}
			return;
		}
	}

	// new notice
	DelayedNoticeInfo delay;
	delay.m_who = who;
	delay.m_when = when;
	m_delayedNoticeVector.AddToTail(delay);
}


//---------------------------------------------------------------------------------------------
void CNEOBot::UpdateDelayedThreatNotices(void)
{
	for (int i = 0; i < m_delayedNoticeVector.Count(); ++i)
	{
		if (m_delayedNoticeVector[i].m_when <= gpGlobals->curtime)
		{
			// delay is up - notice this threat
			CBaseEntity* who = m_delayedNoticeVector[i].m_who;

			if (who)
			{
				GetVisionInterface()->AddKnownEntity(who);
			}

			m_delayedNoticeVector.Remove(i);
			--i;
		}
	}
}


//---------------------------------------------------------------------------------------------
bool CNEOBot::IsSquadmate(CNEO_Player* who) const
{
	if (!m_squad || !who || !who->IsBotOfType(NEO_BOT_TYPE))
		return false;

	return GetSquad() == ToNEOBot(who)->GetSquad();
}


//---------------------------------------------------------------------------------------------
void CNEOBot::ClearTags(void)
{
	m_tags.RemoveAll();
}


//---------------------------------------------------------------------------------------------
void CNEOBot::AddTag(const char* tag)
{
	if (!HasTag(tag))
	{
		m_tags.AddToTail(CFmtStr("%s", tag));
	}
}


//---------------------------------------------------------------------------------------------
void CNEOBot::RemoveTag(const char* tag)
{
	for (int i = 0; i < m_tags.Count(); ++i)
	{
		if (FStrEq(tag, m_tags[i]))
		{
			m_tags.Remove(i);
			return;
		}
	}
}


//---------------------------------------------------------------------------------------------
// TODO: Make this an efficient lookup/match
bool CNEOBot::HasTag(const char* tag)
{
	for (int i = 0; i < m_tags.Count(); ++i)
	{
		if (FStrEq(tag, m_tags[i]))
		{
			return true;
		}
	}

	return false;
}


//---------------------------------------------------------------------------------------------
void CNEOBot::ScriptGetAllTags(HSCRIPT hTable)
{
	for (int i = 0; i < m_tags.Count(); i++)
	{
		g_pScriptVM->SetValue(hTable, CFmtStr("%d", i), m_tags[i]);
	}
}


//-----------------------------------------------------------------------------------------
Action< CNEOBot >* CNEOBot::OpportunisticallyUseWeaponAbilities(void)
{
	if (!m_opportunisticTimer.IsElapsed())
	{
		return NULL;
	}

	m_opportunisticTimer.Start(RandomFloat(0.1f, 0.2f));

	for (int w = 0; w < MAX_WEAPONS; ++w)
	{
		CBaseHL2MPCombatWeapon* weapon = (CBaseHL2MPCombatWeapon*)GetWeapon(w);
		if (!weapon)
			continue;

		// TODO(misyl): SMG1, AR2, Grenades here?
	}

	return NULL;
}


//-----------------------------------------------------------------------------------------
// mostly for MvM - pick a random enemy player that is not in their spawn room
CNEO_Player* CNEOBot::SelectRandomReachableEnemy(void)
{
	CUtlVector< CNEO_Player* > livePlayerVector;
	CollectPlayers(&livePlayerVector, GetEnemyTeam(GetTeamNumber()), COLLECT_ONLY_LIVING_PLAYERS);

	CUtlVector< CNEO_Player* > playerVector;
	for (int i = 0; i < livePlayerVector.Count(); ++i)
	{
		CNEO_Player* player = livePlayerVector[i];

		if (neo_bot_ignore_real_players.GetBool())
		{
			if (!player->IsBot())
			{
				continue;
			}
		}

		playerVector.AddToTail(player);
	}

	if (playerVector.Count() > 0)
	{
		return playerVector[RandomInt(0, playerVector.Count() - 1)];
	}

	return NULL;
}


//-----------------------------------------------------------------------------------------
// Different sized bots used different lookahead distances
float CNEOBot::GetDesiredPathLookAheadRange(void) const
{
	return neo_bot_path_lookahead_range.GetFloat() * GetModelScale();
}

//-----------------------------------------------------------------------------------------
// Hack to apply idle loop sounds in MvM
void CNEOBot::StartIdleSound(void)
{
	StopIdleSound();
}

//-----------------------------------------------------------------------------------------
void CNEOBot::StopIdleSound(void)
{
	if (m_pIdleSound)
	{
		CSoundEnvelopeController::GetController().SoundDestroy(m_pIdleSound);
		m_pIdleSound = NULL;
	}
}

bool CNEOBot::ShouldAutoJump()
{
	if (!HasAttribute(CNEOBot::AUTO_JUMP))
		return false;

	if (!m_autoJumpTimer.HasStarted())
	{
		m_autoJumpTimer.Start(RandomFloat(m_flAutoJumpMin, m_flAutoJumpMax));
		return true;
	}
	else if (m_autoJumpTimer.IsElapsed())
	{
		m_autoJumpTimer.Start(RandomFloat(m_flAutoJumpMin, m_flAutoJumpMax));
		return true;
	}

	return false;
}


int CNEOBot::DrawDebugTextOverlays(void)
{
	int offset = neo_bot_debug_tags.GetBool() ? 1 : BaseClass::DrawDebugTextOverlays();

	CUtlString strTags = "Tags : ";
	for (int i = 0; i < m_tags.Count(); ++i)
	{
		strTags.Append(m_tags[i]);
		strTags.Append(" ");
	}

	EntityText(offset, strTags.Get(), 0);
	offset++;

	return offset;
}


void CNEOBot::AddEventChangeAttributes(const CNEOBot::EventChangeAttributes_t* newEvent)
{
	m_eventChangeAttributes.AddToTail(newEvent);
}


const CNEOBot::EventChangeAttributes_t* CNEOBot::GetEventChangeAttributes(const char* pszEventName) const
{
	for (int i = 0; i < m_eventChangeAttributes.Count(); ++i)
	{
		if (FStrEq(m_eventChangeAttributes[i]->m_eventName, pszEventName))
		{
			return m_eventChangeAttributes[i];
		}
	}
	return NULL;
}


void CNEOBot::OnEventChangeAttributes(const CNEOBot::EventChangeAttributes_t* pEvent)
{
	if (pEvent)
	{
		SetDifficulty(pEvent->m_skill);

		ClearWeaponRestrictions();
		SetWeaponRestriction(pEvent->m_weaponRestriction);

		SetMission(pEvent->m_mission);

		ClearAllAttributes();
		SetAttribute(pEvent->m_attributeFlags);

		SetMaxVisionRangeOverride(pEvent->m_maxVisionRange);

		// cache off health value before we clear attribute because ModifyMaxHealth adds new attribute and reset the health
		int nHealth = GetHealth();
		int nMaxHealth = GetMaxHealth();

		NetworkStateChanged();

		// set health back to what it was before we clear bot's attributes
		ModifyMaxHealth(nMaxHealth);
		SetHealth(nHealth);

		// tags
		ClearTags();
		for (int g = 0; g < pEvent->m_tags.Count(); ++g)
		{
			AddTag(pEvent->m_tags[g]);
		}
	}
}

bool CNEOBot::IsEnemy(const CBaseEntity* them) const
{
	if (them == NULL)
		return false;

	if (them == this)
		return false;

	if (!them->IsPlayer())
		return false;

	if (neo_bot_ignore_real_players.GetBool())
	{
		CBasePlayer* pPlayer = (CBasePlayer*)them;
		if (!pPlayer->IsBot())
			return false;
	}

	if (NEORules() && NEORules()->IsTeamplay())
	{
		if (them->GetTeamNumber() == TEAM_JINRAI && this->GetTeamNumber() == TEAM_NSF)
			return true;

		if (them->GetTeamNumber() == TEAM_NSF && this->GetTeamNumber() == TEAM_JINRAI)
			return true;

		return false;
	}
	else
	{
		// Since we are now forcing bots into JINRAI/NSF teams even in DM to avoid the TEAM_UNASSIGNED crash,
		// we must explicitly tell them to treat everyone as an enemy, otherwise they will treat their "teammates" as friends and not fight.
		// In non-Teamplay modes (like DM), everyone else is an enemy.
		return true;
	}
}

CNEOBaseCombatWeapon* CNEOBot::GetBludgeonWeapon(void)
{
	return static_cast<CNEOBaseCombatWeapon *>(Weapon_GetSlot(2));
}

/*static*/ bool CNEOBot::IsCloseRange(CNEOBaseCombatWeapon* pWeapon)
{
	if (!pWeapon)
		return false;

	return (pWeapon->IsMeleeWeapon() || pWeapon->GetNeoWepBits() & (NEO_WEP_AA13 | NEO_WEP_SUPA7));
}

/*static*/ bool CNEOBot::IsBludgeon(CNEOBaseCombatWeapon* pWeapon)
{
	if (!pWeapon)
		return false;

	return pWeapon->IsMeleeWeapon();
}

/*static*/ bool CNEOBot::IsRanged(CNEOBaseCombatWeapon* pWeapon)
{
	if (!pWeapon)
		return false;

	return (!(pWeapon->GetNeoWepBits() & NEO_WEP_KNIFE));
}

bool CNEOBot::PrefersLongRange(CNEOBaseCombatWeapon* pWeapon)
{
	if (!pWeapon)
		return false;

	return (pWeapon->GetNeoWepBits() & (NEO_WEP_SRS | NEO_WEP_ZR68_L | NEO_WEP_M41 | NEO_WEP_M41_L | NEO_WEP_M41_S));
}

bool CNEOBot::IsFiring() const
{
	return m_nButtons & IN_ATTACK || m_afButtonPressed & IN_ATTACK || m_afButtonLast & IN_ATTACK;
}

NeoClass CNEOBot::ChooseRandomClass() const
{
	{
		const auto forcedClass = bot_class.GetInt();
		if (forcedClass != NEO_CLASS_RANDOM)
		{
			return static_cast<NeoClass>(forcedClass);
		}
	}

	bool bValidClasses[NEO_CLASS__ENUM_COUNT] = {};
	int iClassCounts = 0;
	for (int i = 0; i <= NEO_CLASS_SUPPORT; ++i)
	{
		bValidClasses[i] = (m_profile.flagClass & (1 << i));
		if (bValidClasses[i])
		{
			++iClassCounts;
		}
	}

	if (iClassCounts == 0)
	{
		for (int i = 0; i <= NEO_CLASS_SUPPORT; ++i)
		{
			bValidClasses[i] = true;
			++iClassCounts;
		}
	}

	bool bHasPicked = false;
	if (iClassCounts > 1)
	{
		for (int iRollCount = 0;
				iRollCount < 3 && !bHasPicked;
				++iRollCount)
		{
			float flDice = RandomFloat();
			if (flDice <= neo_bot_recon_ratio.GetFloat())
			{
				if ((bHasPicked = bValidClasses[NEO_CLASS_RECON]))
				{
					return NEO_CLASS_RECON;
				}
			}
			else if (flDice >= (1.0f - neo_bot_support_ratio.GetFloat()))
			{
				if ((bHasPicked = bValidClasses[NEO_CLASS_SUPPORT]))
				{
					return NEO_CLASS_SUPPORT;
				}
			}
			else
			{
				if ((bHasPicked = bValidClasses[NEO_CLASS_ASSAULT]))
				{
					return NEO_CLASS_ASSAULT;
				}
			}
		}
	}

	if (!bHasPicked)
	{
		for (int i = 0; i <= NEO_CLASS_SUPPORT; ++i)
		{
			if (bValidClasses[i])
			{
				return static_cast<NeoClass>(i);
			}
		}
	}

	AssertMsg(false, "fell through the logic");
	return NEO_CLASS_RECON;
}

CNEOBotIntention::CNEOBotIntention(CNEOBot *bot)
	: IIntention(bot)
{
	m_behavior = new CNEOBotBehavior(new CNEOBotMainAction);
}

CNEOBotIntention::~CNEOBotIntention()
{
	delete m_behavior;
}

void CNEOBotIntention::Reset()
{
	delete m_behavior;
	m_behavior = new CNEOBotBehavior(new CNEOBotMainAction);
}

void CNEOBotIntention::Update()
{
	m_behavior->Update(static_cast<CNEOBot *>(GetBot()), GetUpdateInterval());
}

INextBotEventResponder *CNEOBotIntention::FirstContainedResponder() const
{
	return m_behavior;
}

INextBotEventResponder *CNEOBotIntention::NextContainedResponder(INextBotEventResponder *current) const
{
	return nullptr;
}

QueryResultType CNEOBotIntention::ShouldWalk(const CNEOBot *me, const QueryResultType qShouldAimQuery) const
{
	return m_behavior->ShouldWalk(me, qShouldAimQuery);
}

QueryResultType CNEOBotBehavior::ShouldWalk(const CNEOBot *me, const QueryResultType qShouldAimQuery) const
{
	QueryResultType result = ANSWER_UNDEFINED;

	auto *neoAction = static_cast<CNEOBotMainAction *>(m_action);
	if ( neoAction )
	{
		// find innermost child action
		CNEOBotMainAction *action;
		for( action = neoAction; action->m_child; action = static_cast<CNEOBotMainAction *>(action->m_child) )
			;

		// work our way through our containers
		while( action && result == ANSWER_UNDEFINED )
		{
			CNEOBotMainAction *containingAction = static_cast<CNEOBotMainAction *>(action->m_parent);

			// work our way up the stack
			while( action && result == ANSWER_UNDEFINED )
			{
				result = action->ShouldWalk(me, qShouldAimQuery);
				action = static_cast<CNEOBotMainAction *>(action->GetActionBuriedUnderMe());
			}

			action = containingAction;
		}
	}

	return result;
}

QueryResultType CNEOBotIntention::ShouldAim(const CNEOBot *me, const bool bWepHasClip) const
{
	return m_behavior->ShouldAim( me, bWepHasClip );
}

QueryResultType CNEOBotBehavior::ShouldAim(const CNEOBot *me, const bool bWepHasClip) const
{
	QueryResultType result = ANSWER_UNDEFINED;

	auto *neoAction = static_cast<CNEOBotMainAction *>(m_action);
	if ( neoAction )
	{
		// find innermost child action
		CNEOBotMainAction *action;
		for( action = neoAction; action->m_child; action = static_cast<CNEOBotMainAction *>(action->m_child) )
			;

		// work our way through our containers
		while( action && result == ANSWER_UNDEFINED )
		{
			CNEOBotMainAction *containingAction = static_cast<CNEOBotMainAction *>(action->m_parent);

			// work our way up the stack
			while( action && result == ANSWER_UNDEFINED )
			{
				result = action->ShouldAim(me, bWepHasClip);
				action = static_cast<CNEOBotMainAction *>(action->GetActionBuriedUnderMe());
			}

			action = containingAction;
		}
	}

	return result;
}



