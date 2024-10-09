//========= Copyright © 1996-2005, Valve Corporation, All rights reserved. ============//
// Authors: 
// Ivan Bravo Bravo (linkedin.com/in/ivanbravobravo), 2017
// NeoTokyo: Rebuild Team, 2024

#ifndef BOT_CONDITIONS_H
#define BOT_CONDITIONS_H

#ifdef _WIN32
#pragma once
#endif

#include "nav.h"
#include "nav_path.h"
#include "nav_pathfind.h"
#include "nav_area.h"
#include "nav_mesh.h"
#include "bots\interfaces\improv.h"

//================================================================================
// Source Engine
//================================================================================

#ifndef INSOURCE_DLL

#include "player.h"
#ifdef NEO
#include "neo_player.h"
#include "neo_player_shared.h"
#include "weapon_neobasecombatweapon.h"
#endif
// The Bots were developed for Source Engine modification codename "InSource". (INSOURCE_DLL)
// https://github.com/WootsMX/InSource
// Below are alternative code to the InSource code, 
// modify them according to your mod.

#ifndef MAX_CONDITIONS
#define MAX_CONDITIONS 32*8
#endif

typedef CBitVec<MAX_CONDITIONS> CFlagsBits;

// Derived classes
typedef CBasePlayer CPlayer;
typedef CBaseCombatWeapon CBaseWeapon;
#ifdef NEO
typedef CNEO_Player CNEOPlayer;
#endif

// Similar functions
#define ToInPlayer ToBasePlayer
#define ToBaseWeapon dynamic_cast<CBaseWeapon *>
#define TheGameRules GameRules()
#define GetGameDifficulty() TheGameRules->GetSkillLevel()
#define GetActiveBaseWeapon GetActiveWeapon

// Applied to a [CBaseCombatWeapon] to detect the type of weapon.
#ifdef NEO
#define IsMPN() ClassMatches("CWeaponMPN")
#define IsMPNS() ClassMatches("CWeaponMPN_S")
#define IsAA13() ClassMatches("CWeaponAA13")
#define IsGrenade() ClassMatches("CWeaponGrenade")
#define IsJitte() ClassMatches("CWeaponJitte")
#define IsJittes() ClassMatches("CWeaponJitteS")
#define IsKnife() ClassMatches("CWeaponKnife")
#define IsKyla() ClassMatches("CWeaponKyla")
#define IsM41() ClassMatches("CWeaponM41")
#define IsM41l() ClassMatches("CWeaponM41L")
#define IsM41s() ClassMatches("CWeaponM41S")
#define IsMilso() ClassMatches("CWeaponMilso")
#define IsMX() ClassMatches("CWeaponMX")
#define IsMXS() ClassMatches("CWeaponMX_S")
#define IsPZ() ClassMatches("CWeaponPZ")
#define IsSMAC() ClassMatches("CWeaponSMAC")
#define IsSmoke() ClassMatches("CWeaponSmokeGrenade")
#define IsSRM() ClassMatches("CWeaponSRM")
#define IsSRMS() ClassMatches("CWeaponSRM_S")
#define IsSRS() ClassMatches("CWeaponSRS")
#define IsSupa() ClassMatches("CWeaponSupa7")
#define IsTachi() ClassMatches("CWeaponTachi")
#define IsZRC() ClassMatches("CWeaponZR68C")
#define IsZRL() ClassMatches("CWeaponZR68L")
#define IsZRS() ClassMatches("CWeaponZR68S")

#define IsGhost() ClassMatches ("CWeaponGhost")
#define IsDetpack() ClassMatches ("CWeaponDetpack")

// NEOTODO
//#define IsPBK56S() ClassMatches ("CWeaponPBK56S")
#endif

#define GetWeaponInfo GetWpnData

// Skill Levels
#define SKILL_VERY_HARD SKILL_HARD
#define SKILL_ULTRA_HARD SKILL_HARD
#define SKILL_IMPOSIBLE SKILL_HARD
#define SKILL_EASIEST SKILL_EASY
#define SKILL_HARDEST SKILL_HARD

// Sounds
#define SOUND_CONTEXT_BULLET_IMPACT SOUND_BULLET_IMPACT

// Command Macros
#define FCVAR_SERVER            FCVAR_REPLICATED | FCVAR_DEMO
#define FCVAR_ADMIN_ONLY        FCVAR_SERVER_CAN_EXECUTE | FCVAR_SERVER

#undef DECLARE_COMMAND
#define DECLARE_COMMAND( name, value, description, flags )				ConVar name( #name, value, flags, description );
#define DECLARE_ADMIN_COMMAND( name, value, description )				DECLARE_COMMAND( name, value, description, FCVAR_ADMIN_ONLY )
#define DECLARE_REPLICATED_CHEAT_COMMAND( name, value, description )	DECLARE_COMMAND( name, value, description, FCVAR_REPLICATED | FCVAR_CHEAT )
#define DECLARE_REPLICATED_COMMAND( name, value, description )			DECLARE_COMMAND( name, value, description, FCVAR_SERVER )
#define DECLARE_DEBUG_COMMAND( name, value, description )				DECLARE_COMMAND( name, value, description, FCVAR_ADMIN_ONLY | FCVAR_CHEAT | FCVAR_NOTIFY )

#define DECLARE_CHEAT_COMMAND DECLARE_REPLICATED_CHEAT_COMMAND
#define DECLARE_NOTIFY_COMMAND( name, value, description ) DECLARE_COMMAND( name, value, description, FCVAR_SERVER | FCVAR_NOTIFY )

//================================================================================
//================================================================================
struct DebugMessage
{
    char m_string[1024];
    IntervalTimer m_age;
};

//================================================================================
//================================================================================
class CBulletsTraceFilter : public CTraceFilterSimpleList
{
public:
    CBulletsTraceFilter( int collisionGroup ) : CTraceFilterSimpleList( collisionGroup ) {}

    bool ShouldHitEntity( IHandleEntity *pHandleEntity, int contentsMask )
    {
        if ( m_PassEntities.Count() ) {
            CBaseEntity *pEntity = EntityFromEntityHandle( pHandleEntity );
            CBaseEntity *pPassEntity = EntityFromEntityHandle( m_PassEntities[0] );
            if ( pEntity && pPassEntity && pEntity->GetOwnerEntity() == pPassEntity &&
                 pPassEntity->IsSolidFlagSet( FSOLID_NOT_SOLID ) && pPassEntity->IsSolidFlagSet( FSOLID_CUSTOMBOXTEST ) &&
                 pPassEntity->IsSolidFlagSet( FSOLID_CUSTOMRAYTEST ) ) {
                // It's a bone follower of the entity to ignore (toml 8/3/2007)
                return false;
            }
        }
        return CTraceFilterSimpleList::ShouldHitEntity( pHandleEntity, contentsMask );
    }

};

//================================================================================
// Allows you to save various types of information.
//================================================================================
class CMultidata
{
public:
    DECLARE_CLASS_NOBASE( CMultidata );

    CMultidata()
    {
        Reset();
    }

    CMultidata( int value )
    {
        Reset();
        SetInt( value );
    }

    CMultidata( Vector value )
    {
        Reset();
        SetVector( value );
    }

    CMultidata( float value )
    {
        Reset();
        SetFloat( value );
    }

    CMultidata( const char *value )
    {
        Reset();
        SetString( value );
    }

    CMultidata( CBaseEntity *value )
    {
        Reset();
        SetEntity( value );
    }

    virtual void Reset() {
        vecValue.Invalidate();
        flValue = 0;
        iValue = 0;
        iszValue = NULL_STRING;
        pszValue = NULL;
        Purge();
    }

    void SetInt( int value ) {
        iValue = value;
        flValue = (float)value;
        OnSet();
    }

    void SetFloat( float value ) {
        iValue = (int)value;
        flValue = value;
        OnSet();
    }

    void SetVector( const Vector &value ) {
        vecValue = value;
        OnSet();
    }

    void SetString( const char *value ) {
        iszValue = AllocPooledString( value );
        OnSet();
    }

    void SetEntity( CBaseEntity *value ) {
        pszValue = value;
        OnSet();
    }

    virtual void OnSet() { }

    const Vector &GetVector() const {
        return vecValue;
    }

    float GetFloat() const {
        return flValue;
    }

    int GetInt() const {
        return iValue;
    }

    const char *GetString() const {
        return STRING( iszValue );
    }

    CBaseEntity *GetEntity() const {
        return pszValue.Get();
    }

    int Add( CMultidata *data ) {
        int index = list.AddToTail( data );
        OnSet();
        return index;
    }

    bool Remove( CMultidata *data ) {
        return list.FindAndRemove( data );
    }

    void Purge() {
        list.PurgeAndDeleteElements();
    }

    Vector vecValue;
    float flValue;
    int iValue;
    string_t iszValue;
    EHANDLE pszValue;
    CUtlVector<CMultidata *> list;
};
#endif

//================================================================================
//================================================================================

#define MEMORY_BLOCK_LOOK_AROUND "BlockLookAround"
#define MEMORY_SPAWN_POSITION "SpawnPosition"
#define MEMORY_BEST_WEAPON "BestWeapon"
#define MEMORY_DEJECTED_FRIEND "DejectedFriend"

#define GET_COVER_RADIUS 1500.0f

//================================================================================
// Bot Names
// TODO: One way to change these names without editing the code. (Script file for example)
//================================================================================

#ifdef INSOURCE_DLL
static const char *m_botNames[] =
{
    "Abigail",
    "Mariana",
    "Alfonso",
    "Chell",
    "Adan",
    "Gordon",
    "Freeman",
    "Kolesias",
    "Jennifer",
    "Valeria",
    "Alejandra",
    "Andrea",
    "Mario",
    "Maria",
    "Steam",
    "Hoxton",
    "Alex",
    "Sing"
};
#else
// http://www.behindthename.com/random/random.php?number=1&gender=both&surname=&norare=yes&nodiminutives=yes&all=no&usage_eng=1
static const char *m_botNames[] =
{
    "Elias",
    "Ronda",
    "Theodora",
    "Alger",
    "Barbara",
    "Shelley",
    "Crystal",
    "Gordon",
    "Alyx"
};
#endif

//================================================================================
// Priorities
//================================================================================
enum
{
    PRIORITY_INVALID = 0,
    PRIORITY_VERY_LOW,
    PRIORITY_LOW,
    PRIORITY_FOLLOWING,
    PRIORITY_NORMAL,
    PRIORITY_HIGH,
    PRIORITY_VERY_HIGH,
    PRIORITY_CRITICAL,
    PRIORITY_UNINTERRUPTABLE,

    LAST_PRIORITY
};

static const char *g_PriorityNames[LAST_PRIORITY] =
{
    "INVALID",
    "VERY LOW",
    "LOW",
    "FOLLOWING",
    "NORMAL",
    "HIGH",
    "VERY HIGH",
    "CRITICAL",
    "UNINTERRUPTABLE"
};

//================================================================================
// Speeds when aiming
//================================================================================
enum 
{
    AIM_SPEED_VERYLOW = 0,
    AIM_SPEED_LOW,
    AIM_SPEED_NORMAL,
    AIM_SPEED_FAST,
    AIM_SPEED_VERYFAST,
    AIM_SPEED_INSTANT,

    LAST_AIM_SPEED
};

//================================================================================
// ID Components
//================================================================================
enum
{
    BOT_COMPONENT_VISION = 0,
    BOT_COMPONENT_LOCOMOTION,
    BOT_COMPONENT_FOLLOW,
    BOT_COMPONENT_MEMORY,
    BOT_COMPONENT_ATTACK,
    BOT_COMPONENT_DECISION,

    LAST_COMPONENT
};

//================================================================================
// Levels of desire
//================================================================================

#define BOT_DESIRE_NONE		0.0f
#define BOT_DESIRE_VERYLOW	0.1f
#define BOT_DESIRE_LOW		0.2f
#define BOT_DESIRE_MODERATE 0.5f
#define BOT_DESIRE_HIGH		0.7f
#define BOT_DESIRE_VERYHIGH 0.9f
#define BOT_DESIRE_FORCED   0.99f
#define BOT_DESIRE_ABSOLUTE 1.0f

//================================================================================
// Schedules
//================================================================================
enum
{
    SCHEDULE_NONE = 0,
    SCHEDULE_INVESTIGATE_LOCATION,
    SCHEDULE_INVESTIGATE_SOUND,
    SCHEDULE_HUNT_ENEMY,

    SCHEDULE_ATTACK,
    SCHEDULE_RELOAD,
    SCHEDULE_COVER,
    SCHEDULE_HIDE,

    SCHEDULE_CHANGE_WEAPON,
    SCHEDULE_HIDE_AND_HEAL,
    SCHEDULE_HIDE_AND_RELOAD,

    SCHEDULE_HELP_DEJECTED_FRIEND,
    SCHEDULE_MOVE_ASIDE,
    SCHEDULE_CALL_FOR_BACKUP,
    SCHEDULE_DEFEND_SPAWN,

#ifdef APOCALYPSE
    SCHEDULE_SOLDIER_COVER,
	SCHEDULE_SEARCH_RESOURCES,
	SCHEDULE_WANDER,
    SCHEDULE_CLEAN_BUILDING,
#elif HL2MP
    SCHEDULE_WANDER,
#endif

    LAST_BOT_SCHEDULE
};

static const char *g_BotSchedules[LAST_BOT_SCHEDULE] =
{
    "NONE",
    "INVESTIGATE_LOCATION",
    "INVESTIGATE_SOUND",
    "HUNT_ENEMY",
    
    "ATTACK",
    "RELOAD",
    "COVER",
    "HIDE",

    "CHANGE_WEAPON",
    "HIDE_AND_HEAL",
    "HIDE_AND_RELOAD",

    "HELP_DEJECTED_FRIEND",
    "MOVE_ASIDE",
    "CALL_FOR_BACKUP",
    "DEFEND_SPAWN",

#ifdef APOCALYPSE
    "SOLDIER_COVER",
	"SEARCH_RESOURCES",
	"WANDER",
    "CLEAN_BUILDING",
#elif HL2MP
    "WANDER",
#endif
};

//================================================================================
// Tasks
//================================================================================

enum
{
	BTASK_INVALID = 0,
	BTASK_WAIT,                         // Wait a certain amount of seconds
    BTASK_SET_TOLERANCE,                // Sets the tolerance in distance for the tasks of moving to a destination.

    BTASK_PLAY_ANIMATION,
    BTASK_PLAY_GESTURE,
    BTASK_PLAY_SEQUENCE,

	BTASK_SAVE_POSITION,                // Save our current position
	BTASK_RESTORE_POSITION,             // Move to the saved position

	BTASK_MOVE_DESTINATION,             // Move to specified destination
	BTASK_MOVE_LIVE_DESTINATION,        // Move to a destination/entity even if it is moving continuously.

	BTASK_HUNT_ENEMY,                   // Hunt our enemy

    BTASK_SAVE_SPAWN_POSITION,          // Save the position where we spawn
	BTASK_SAVE_ASIDE_SPOT,              // Find and save a random near position. (In the same NavArea where we are)
	BTASK_SAVE_COVER_SPOT,              // Find and save a cover position.
	BTASK_SAVE_FAR_COVER_SPOT,          // Finding and storing a distant cover position.

	BTASK_AIM,                          // Aim to a specific spot

	BTASK_USE,                          // +use
	BTASK_JUMP,                         // +jump
	BTASK_CROUCH,                       // +duck
	BTASK_STANDUP,                      // -duck
	BTASK_RUN,                          // +speed
    BTASK_SNEAK,                        // +walk
	BTASK_WALK,                         // -walk & -speed
	BTASK_RELOAD,                       // Reload our weapon and wait for it to end.
    BTASK_RELOAD_SAFE,                  // Reload our weapon and wait for it to end. (Only if there are no enemies nearby)
	BTASK_HEAL,                         // Heal (TODO: Only gives health to itself)

	BTASK_CALL_FOR_BACKUP,              // Request reinforcements (TODO)

    BTASK_SET_FAIL_SCHEDULE,            // If the schedule fails, the indicated schedule will run
    BTASK_SET_SCHEDULE,                 // Running the specified schedule at the end of the active

	BLAST_TASK,

	BCUSTOM_TASK = 999
};

static const char *g_BotTasks[BLAST_TASK] =
{
    "INVALID",
	"WAIT",
    "SET_TOLERANCE",

    "PLAY_ANIMATION",
    "PLAY_GESTURE",
    "PLAY_SEQUENCE",

	"SAVE_POSITION",
	"RESTORE_POSITION",

	"MOVE_DESTINATION",
	"MOVE_LIVE_DESTINATION",

	"HUNT_ENEMY",

    "GET_SPAWN",
	"GET_SPOT_ASIDE",
	"GET_COVER",
	"GET_FAR_COVER",

    "AIM",

    "USE",
    "JUMP",
    "CROUCH",
    "STANDUP",
    "RUN",
    "SNEAK",
    "WALK",
    "RELOAD",
    "RELOAD_SAFE",
    "HEAL",

    "CALL_FOR_BACKUP"
};

//================================================================================
// State
//================================================================================
enum BotState
{
    STATE_IDLE,
    STATE_ALERT,
    STATE_COMBAT,
    STATE_PANIC,

    LAST_STATE
};

//================================================================================
// Team strategy
//================================================================================
enum BotStrategie
{
    ENDURE_UNTIL_DEATH = 0,   // Fight to death
    COWARDS,                  // Coward but safe
    LAST_CALL_FOR_BACKUP,     // The last remaining member of the team must leave and request reinforcements

    LAST_STRATEGIE
};

static const char *g_BotStrategie[LAST_STRATEGIE] =
{
    "ENDURE_UNTIL_DEATH",
    "COWARDS",
    "LAST_CALL_FOR_BACKUP"
};

//================================================================================
// Sleep State
//================================================================================
enum BotPerformance
{
    BOT_PERFORMANCE_AWAKE = 0,
    BOT_PERFORMANCE_PVS,

    LAST_PERFORMANCE
};

//================================================================================
// Stores the assigned Hitbox number for each body part
//================================================================================
struct HitboxBones
{
    int head;
    int chest;
    int leftLeg;
    int rightLeg;
};

typedef int HitboxType;

struct HitboxPositions
{
    void Reset() {
        head.Invalidate();
        chest.Invalidate();
        leftLeg.Invalidate();
        rightLeg.Invalidate();
    }

    bool IsValid() {
        return (head.IsValid() && chest.IsValid());
    }

    Vector head;
    Vector chest;
    Vector leftLeg;
    Vector rightLeg;
};

//================================================================================
// Conditions
//================================================================================
enum BCOND
{
    BCOND_NONE = 0,

    BCOND_EMPTY_PRIMARY_AMMO,
    BCOND_LOW_PRIMARY_AMMO,

    BCOND_EMPTY_CLIP1_AMMO,
    BCOND_LOW_CLIP1_AMMO,

    BCOND_EMPTY_SECONDARY_AMMO,
    BCOND_LOW_SECONDARY_AMMO,

    BCOND_EMPTY_CLIP2_AMMO,
    BCOND_LOW_CLIP2_AMMO,

	BCOND_HELPLESS,

    BCOND_SEE_HATE,
    BCOND_SEE_FEAR,
    BCOND_SEE_DISLIKE,
    BCOND_SEE_ENEMY,
    BCOND_SEE_FRIEND,
    BCOND_SEE_DEJECTED_FRIEND,

    BCOND_WITHOUT_ENEMY,

    BCOND_ENEMY_LOST,
    BCOND_ENEMY_OCCLUDED,
    BCOND_ENEMY_OCCLUDED_BY_FRIEND,
    BCOND_ENEMY_LAST_POSITION_VISIBLE,
    BCOND_ENEMY_DEAD,

    BCOND_ENEMY_TOO_NEAR,
    BCOND_ENEMY_NEAR,
    BCOND_ENEMY_FAR,
    BCOND_ENEMY_TOO_FAR,

    BCOND_LIGHT_DAMAGE,
    BCOND_HEAVY_DAMAGE,
    BCOND_REPEATED_DAMAGE,
    BCOND_LOW_HEALTH,
    BCOND_DEJECTED,

    BCOND_CAN_RANGE_ATTACK1,
    BCOND_CAN_RANGE_ATTACK2,
    BCOND_CAN_MELEE_ATTACK1,
    BCOND_CAN_MELEE_ATTACK2,

    BCOND_NEW_ENEMY,

    BCOND_TOO_CLOSE_TO_ATTACK,
    BCOND_TOO_FAR_TO_ATTACK,
    BCOND_NOT_FACING_ATTACK,

    BCOND_TASK_FAILED,
    BCOND_TASK_DONE,
    BCOND_SCHEDULE_FAILED,
    BCOND_SCHEDULE_DONE,

    BCOND_BETTER_WEAPON_AVAILABLE,
    BCOND_MOBBED_BY_ENEMIES, // TODO
    BCOND_PLAYER_PUSHING, // TODO
    BCOND_GOAL_UNREACHABLE,

    BCOND_SMELL_MEAT,
    BCOND_SMELL_CARCASS,
    BCOND_SMELL_GARBAGE,

    BCOND_HEAR_COMBAT,
    BCOND_HEAR_WORLD,
    BCOND_HEAR_ENEMY,
    BCOND_HEAR_ENEMY_FOOTSTEP,
    BCOND_HEAR_BULLET_IMPACT,
	BCOND_HEAR_BULLET_IMPACT_SNIPER,
    BCOND_HEAR_DANGER,
    BCOND_HEAR_MOVE_AWAY,
    BCOND_HEAR_SPOOKY,

    LAST_BCONDITION,
};

static const char *g_Conditions[LAST_BCONDITION] = {
    "NONE",

    "EMPTY_PRIMARY_AMMO",
    "LOW_PRIMARY_AMMO",

    "EMPTY_CLIP1_AMMO",
    "LOW_CLIP1_AMMO",

    "EMPTY_SECONDARY_AMMO",
    "LOW_SECONDARY_AMMO",

    "EMPTY_CLIP2_AMMO",
    "LOW_CLIP2_AMMO",

	"HELPLESS",

    "SEE_HATE",
    "SEE_FEAR",
    "SEE_DISLIKE",
    "SEE_ENEMY",
    "SEE_FRIEND",
    "SEE_DEJECTED_FRIEND",

    "WITHOUT_ENEMY",

    "ENEMY_LOST",
    "ENEMY_OCCLUDED",
    "ENEMY_OCCLUDED_BY_FRIEND",
    "ENEMY_LAST_POSITION_VISIBLE",
    "ENEMY_DEAD",

    "ENEMY_TOO_NEAR",
    "ENEMY_NEAR",
    "ENEMY_FAR",
    "ENEMY_TOO_FAR",

    "LIGHT_DAMAGE",
    "HEAVY_DAMAGE",
    "REPEATED_DAMAGE",
    "LOW_HEALTH",
    "DEJECTED",

    "CAN_RANGE_ATTACK1",
    "CAN_RANGE_ATTACK2",
    "CAN_MELEE_ATTACK1",
    "CAN_MELEE_ATTACK2",

    "NEW_ENEMY",

    "TOO_CLOSE_TO_ATTACK",
    "TOO_FAR_TO_ATTACK",
    "NOT_FACING_ATTACK",

    "TASK_FAILED",
    "TASK_DONE",
    "SCHEDULE_FAILED",
    "SCHEDULE_DONE",

    "BETTER_WEAPON_AVAILABLE",
    "MOBBED_BY_ENEMIES",
    "PLAYER_PUSHING",
    "GOAL_UNREACHABLE",

    "SMELL_MEAT",
    "SMELL_CARCASS",
    "SMELL_GARBAGE",

    "HEAR_COMBAT",
    "HEAR_WORLD",
    "HEAR_ENEMY",
    "HEAR_ENEMY_FOOTSTEP",
    "HEAR_BULLET_IMPACT",
	"HEAR_BULLET_IMPACT_SNIPER",
    "HEAR_DANGER",
    "HEAR_MOVE_AWAY",
    "HEAR_SPOOKY",
};

//================================================================================
// Tactical mode
//================================================================================
enum
{
    TACTICAL_MODE_INVALID = 0,
    TACTICAL_MODE_NONE,        // No preference
    TACTICAL_MODE_STEALTH,     // Less noise and possible attention
    TACTICAL_MODE_ASSAULT,     // Organized and careful
    TACTICAL_MODE_DEFENSIVE,   // Defend a goal at all costs
    TACTICAL_MODE_AGGRESSIVE,  // Fight without fear

    LAST_TACTICAL_MODE
};

static const char *g_TacticalModes[LAST_TACTICAL_MODE] =
{
    "INVALID",
    "NONE",
    "STEALTH",
    "ASSAULT",
    "DEFENSIVE",
    "AGGRESSIVE"
};

#endif // BOT_CONDITIONS_H