#pragma once

#include "Player/NextBotPlayer.h"
#include "neo_bot_vision.h"
#include "neo_bot_body.h"
#include "neo_bot_locomotion.h"
#include "neo_player.h"
#include "neo_bot_squad.h"
#include "map_entities/neo_bot_generator.h"
#include "map_entities/neo_bot_proxy.h"
#include "neo_gamerules.h"
#include "weapon_neobasecombatweapon.h"
#include "nav_entities.h"
#include "utlstack.h"

#define NEO_BOT_TYPE	1338

class CNEOBotActionPoint;
class CNEOBotGenerator;
class CNEOBot;

extern ConVar hl2_normspeed;

extern void BotGenerateAndWearItem(CNEO_Player* pBot, const char* itemName);

extern int Bot_GetTeamByName(const char* string);

inline int GetEnemyTeam(int team)
{
	if (team == TEAM_JINRAI)
		return TEAM_NSF;

	if (team == TEAM_NSF)
		return TEAM_JINRAI;

	// no enemy team
	return team;
}

//----------------------------------------------------------------------------

#define NEOBOT_ALL_BEHAVIOR_FLAGS		0xFFFF

//----------------------------------------------------------------------------
class CNEOBot : public NextBotPlayer< CNEO_Player >, public CGameEventListener
{
public:
	DECLARE_CLASS(CNEOBot, NextBotPlayer< CNEO_Player >);

	DECLARE_ENT_SCRIPTDESC();

	CNEOBot();
	virtual ~CNEOBot();

	virtual void		Spawn();
	virtual void		FireGameEvent(IGameEvent* event);
	virtual void		Event_Killed(const CTakeDamageInfo& info);
	virtual void		PhysicsSimulate(void);
	virtual void		Touch(CBaseEntity* pOther);
	virtual void		AvoidPlayers(CUserCmd* pCmd);				// some game types allow players to pass through each other, this method pushes them apart
	virtual void		UpdateOnRemove(void);
	virtual int			ShouldTransmit(const CCheckTransmitInfo* pInfo) OVERRIDE;

	virtual int			DrawDebugTextOverlays(void);

	void				ModifyMaxHealth(int nNewMaxHealth, bool bSetCurrentHealth = true);

	virtual int GetBotType(void) const;				// return a unique int representing the type of bot instance this is

	virtual CNavArea* GetLastKnownArea(void) const { return static_cast<CNavArea*>(BaseClass::GetLastKnownArea()); }		// return the last nav area the player occupied - NULL if unknown

	// NextBotPlayer
	static CBasePlayer* AllocatePlayerEntity(edict_t* pEdict, const char* playerName);

	virtual void PressFireButton(float duration = -1.0f) OVERRIDE;
	virtual void PressAltFireButton(float duration = -1.0f) OVERRIDE;
	virtual void PressSpecialFireButton(float duration = -1.0f) OVERRIDE;

	// INextBot
	virtual CNEOBotLocomotion* GetLocomotionInterface(void) const { return m_locomotor; }
	virtual CNEOBotBody* GetBodyInterface(void) const { return m_body; }
	virtual CNEOBotVision* GetVisionInterface(void) const { return m_vision; }
	DECLARE_INTENTION_INTERFACE(CNEOBot);

	virtual bool IsDormantWhenDead(void) const;			// should this player-bot continue to update itself when dead (respawn logic, etc)

	virtual void OnWeaponFired(CBaseCombatCharacter* whoFired, CBaseCombatWeapon* weapon);		// when someone fires their weapon

	// search outwards from startSearchArea and collect all reachable objects from the given list that pass the given filter
	void SelectReachableObjects(const CUtlVector< CHandle< CBaseEntity > >& candidateObjectVector, CUtlVector< CHandle< CBaseEntity > >* selectedObjectVector, const INextBotFilter& filter, CNavArea* startSearchArea, float maxRange = 2000.0f) const;
	CBaseEntity* FindClosestReachableObject(const char* objectName, CNavArea* from, float maxRange = 2000.0f) const;

	CNavArea* GetSpawnArea(void) const;							// get area where we spawned in
	HSCRIPT ScriptGetSpawnArea(void) const { return ToHScript(this->GetSpawnArea()); }

	bool IsAmmoLow(void) const;
	bool IsAmmoFull(void) const;

	bool IsCloakEnabled(void) const;
	float GetCloakPower(void) const;
	void EnableCloak(float threshold);
	void DisableCloak(void);

	void UpdateLookingAroundForEnemies(void);						// update our view to keep an eye on enemies, and where enemies come from

#define LOOK_FOR_FRIENDS false
#define LOOK_FOR_ENEMIES true
	void UpdateLookingAroundForIncomingPlayers(bool lookForEnemies);	// update our view to watch where friends or enemies will be coming from
	void StartLookingAroundForEnemies(void);						// enable updating view for enemy searching
	void StopLookingAroundForEnemies(void);						// disable updating view for enemy searching

	void SetAttentionFocus(CBaseEntity* focusOn);					// restrict bot's attention to only this entity (or radius around this entity) to the exclusion of everything else
	void ClearAttentionFocus(void);								// remove attention focus restrictions
	bool IsAttentionFocused(void) const;
	bool IsAttentionFocusedOn(CBaseEntity* who) const;
	void ScriptSetAttentionFocus(HSCRIPT hFocusOn) { this->SetAttentionFocus(ToEnt(hFocusOn)); }
	bool ScriptIsAttentionFocusedOn(HSCRIPT hWho) const { return this->IsAttentionFocusedOn(ToEnt(hWho)); }

	void DelayedThreatNotice(CHandle< CBaseEntity > who, float noticeDelay);	// notice the given threat after the given number of seconds have elapsed
	void UpdateDelayedThreatNotices(void);
	void ScriptDelayedThreatNotice(HSCRIPT hWho, float flNoticeDelay) { this->DelayedThreatNotice(ToEnt(hWho), flNoticeDelay); }

	CNavArea* FindVantagePoint(float maxTravelDistance = 2000.0f) const;	// return a nearby area where we can see a member of the enemy team
	HSCRIPT ScriptFindVantagePoint(float maxTravelDistance) { return ToHScript(this->FindVantagePoint(maxTravelDistance)); }

	float GetThreatDanger(CBaseCombatCharacter* who) const;		// return perceived danger of threat (0=none, 1=immediate deadly danger)
	float GetMaxAttackRange(void) const;							// return the max range at which we can effectively attack
	float GetDesiredAttackRange(void) const;						// return the ideal range at which we can effectively attack

	bool EquipRequiredWeapon(void);								// if we're required to equip a specific weapon, do it.
	void EquipBestWeaponForThreat(const CKnownEntity* threat);	// equip the best weapon we have to attack the given threat

	void PushRequiredWeapon(CNEOBaseCombatWeapon* weapon);				// force us to equip and use this weapon until popped off the required stack
	void PopRequiredWeapon(void);									// pop top required weapon off of stack and discard

	bool IsCombatWeapon(CNEOBaseCombatWeapon* weapon) const;				// return true if given weapon can be used to attack
	bool IsHitScanWeapon(CNEOBaseCombatWeapon* weapon) const;			// return true if given weapon is a "hitscan" weapon (scattered tracelines with instant damage)
	bool IsContinuousFireWeapon(CNEOBaseCombatWeapon* weapon) const;		// return true if given weapon "sprays" bullets/fire/etc continuously (ie: not individual rockets/etc)
	bool IsExplosiveProjectileWeapon(CNEOBaseCombatWeapon* weapon) const;// return true if given weapon launches explosive projectiles with splash damage
	bool IsBarrageAndReloadWeapon(CNEOBaseCombatWeapon* weapon) const;	// return true if given weapon has small clip and long reload cost (ie: rocket launcher, etc)
	bool IsQuietWeapon(CNEOBaseCombatWeapon* weapon) const;				// return true if given weapon doesn't make much sound when used (ie: spy knife, etc)

	bool IsEnvironmentNoisy(void) const;							// return true if there are/have been loud noises (ie: non-quiet weapons) nearby very recently

	bool IsEnemy(const CBaseEntity* them) const OVERRIDE;

	CNEOBaseCombatWeapon* GetBludgeonWeapon(void);

	static bool IsBludgeon(CNEOBaseCombatWeapon* pWeapon);
	static bool IsCloseRange(CNEOBaseCombatWeapon* pWeapon);
	static bool IsRanged(CNEOBaseCombatWeapon* pWeapon);
	static bool PrefersLongRange(CNEOBaseCombatWeapon* pWeapon);

	enum WeaponRestrictionType
	{
		ANY_WEAPON = 0,
		MELEE_ONLY = 0x0001,
	};
	void ClearWeaponRestrictions(void);
	void SetWeaponRestriction(int restrictionFlags);
	void RemoveWeaponRestriction(int restrictionFlags);
	bool HasWeaponRestriction(int restrictionFlags) const;
	bool IsWeaponRestricted(CNEOBaseCombatWeapon* weapon) const;
	bool ScriptIsWeaponRestricted(HSCRIPT script) const;

	bool IsLineOfFireClear(const Vector& where) const;			// return true if a weapon has no obstructions along the line from our eye to the given position
	bool IsLineOfFireClear(CBaseEntity* who) const;				// return true if a weapon has no obstructions along the line from our eye to the given entity
	bool IsLineOfFireClear(const Vector& from, const Vector& to) const;			// return true if a weapon has no obstructions along the line between the given points
	bool IsLineOfFireClear(const Vector& from, CBaseEntity* who) const;			// return true if a weapon has no obstructions along the line between the given point and entity

	bool IsEntityBetweenTargetAndSelf(CBaseEntity* other, CBaseEntity* target);	// return true if "other" is positioned inbetween us and "target"

	CNEO_Player* GetClosestHumanLookingAtMe(int team = TEAM_ANY);	// return the nearest human player on the given team who is looking directly at me

	enum AttributeType
	{
		REMOVE_ON_DEATH = 1 << 0,					// kick bot from server when killed
		AGGRESSIVE = 1 << 1,					// in MvM mode, push for the cap point
		IS_NPC = 1 << 2,					// a non-player support character
		SUPPRESS_FIRE = 1 << 3,
		DISABLE_DODGE = 1 << 4,
		BECOME_SPECTATOR_ON_DEATH = 1 << 5,					// move bot to spectator team when killed
		QUOTA_MANANGED = 1 << 6,					// managed by the bot quota in CNEOBotManager 
		IGNORE_ENEMIES = 1 << 7,
		HOLD_FIRE_UNTIL_FULL_RELOAD = 1 << 8,				// don't fire our barrage weapon until it is full reloaded (rocket launcher, etc)
		PRIORITIZE_DEFENSE = 1 << 9,				// bot prioritizes defending when possible
		ALWAYS_FIRE_WEAPON = 1 << 10,				// constantly fire our weapon
		TELEPORT_TO_HINT = 1 << 11,				// bot will teleport to hint target instead of walking out from the spawn point
		AUTO_JUMP = 1 << 12,				// auto jump
	};
	void SetAttribute(int attributeFlag);
	void ClearAttribute(int attributeFlag);
	void ClearAllAttributes();
	bool HasAttribute(int attributeFlag) const;

	enum DifficultyType
	{
		UNDEFINED = -1,
		EASY = 0,
		NORMAL = 1,
		HARD = 2,
		EXPERT = 3,

		NUM_DIFFICULTY_LEVELS
	};
	DifficultyType GetDifficulty(void) const;
	void SetDifficulty(DifficultyType difficulty);
	bool IsDifficulty(DifficultyType skill) const;
	int ScriptGetDifficulty(void) const { return this->GetDifficulty(); }
	void ScriptSetDifficulty(int difficulty) { this->SetDifficulty((DifficultyType)difficulty); }
	bool ScriptIsDifficulty(int difficulty) const { return this->IsDifficulty((DifficultyType)difficulty); }

	void SetHomeArea(CNavArea* area);
	CNavArea* GetHomeArea(void) const;
	void ScriptSetHomeArea(HSCRIPT hScript) { this->SetHomeArea(ToNavArea(hScript)); }
	HSCRIPT ScriptGetHomeArea(void) { return ToHScript(this->GetHomeArea()); }

	const Vector& GetSpotWhereEnemySentryLastInjuredMe(void) const;

	void SetActionPoint(CNEOBotActionPoint* point);
	CNEOBotActionPoint* GetActionPoint(void) const;
	void ScriptSetActionPoint(HSCRIPT hPoint) { SetActionPoint(ScriptToEntClass< CNEOBotActionPoint >(hPoint)); }
	HSCRIPT ScriptGetActionPoint(void) const { return ToHScript(GetActionPoint()); }

	bool HasProxy(void) const;
	void SetProxy(CNEOBotProxy* proxy);					// attach this bot to a bot_proxy entity for map I/O communications
	CNEOBotProxy* GetProxy(void) const;

	bool HasSpawner(void) const;
	void SetSpawner(CNEOBotGenerator* spawner);
	CNEOBotGenerator* GetSpawner(void) const;

	void JoinSquad(CNEOBotSquad* squad);					// become a member of the given squad
	void LeaveSquad(void);								// leave our current squad
	void DeleteSquad(void);
	bool IsInASquad(void) const;
	bool IsSquadmate(CNEO_Player* who) const;				// return true if given bot is in my squad
	CNEOBotSquad* GetSquad(void) const;					// return squad we are in, or NULL
	float GetSquadFormationError(void) const;				// return normalized error term where 0 = in formation position and 1 = completely out of position
	void SetSquadFormationError(float error);
	bool HasBrokenFormation(void) const;					// return true if this bot is far out of formation, or has no path back
	void SetBrokenFormation(bool state);

	void ScriptDisbandCurrentSquad(void) { if (GetSquad()) GetSquad()->DisbandAndDeleteSquad(); }

	float TransientlyConsistentRandomValue(float period = 10.0f, int seedValue = 0) const;		// compute a pseudo random value (0-1) that stays consistent for the given period of time, but changes unpredictably each period

	void SetBehaviorFlag(unsigned int flags);
	void ClearBehaviorFlag(unsigned int flags);
	bool IsBehaviorFlagSet(unsigned int flags) const;

	bool FindSplashTarget(CBaseEntity* target, float maxSplashRadius, Vector* splashTarget) const;

	enum MissionType
	{
		NO_MISSION = 0,
		MISSION_SEEK_AND_DESTROY,		// focus on finding and killing enemy players
		MISSION_DESTROY_SENTRIES,		// focus on finding and destroying enemy sentry guns (and buildings)
		MISSION_SNIPER,					// maintain teams of snipers harassing the enemy
		MISSION_SPY,					// maintain teams of spies harassing the enemy
		MISSION_ENGINEER,				// maintain engineer nests for harassing the enemy
		MISSION_REPROGRAMMED,			// MvM: robot has been hacked and will do bad things to their team
	};
#define MISSION_DOESNT_RESET_BEHAVIOR_SYSTEM false
	void SetMission(MissionType mission, bool resetBehaviorSystem = true);
	void SetPrevMission(MissionType mission);
	MissionType GetMission(void) const;
	MissionType GetPrevMission(void) const;
	bool HasMission(MissionType mission) const;
	bool IsOnAnyMission(void) const;
	void SetMissionTarget(CBaseEntity* target);
	CBaseEntity* GetMissionTarget(void) const;
	void SetMissionString(CUtlString string);
	CUtlString* GetMissionString(void);
	void ScriptSetMission(unsigned int mission, bool resetBehaviorSystem = true) { this->SetMission((MissionType)mission, resetBehaviorSystem); }
	void ScriptSetPrevMission(unsigned int mission) { this->SetPrevMission((MissionType)mission); }
	unsigned int ScriptGetMission(void) const { return (unsigned int)this->GetMission(); }
	unsigned int ScriptGetPrevMission(void) const { return (unsigned int)this->GetPrevMission(); }
	bool ScriptHasMission(unsigned int mission) const { return this->HasMission((MissionType)mission); }
	void ScriptSetMissionTarget(HSCRIPT hTarget) { this->SetMissionTarget(ToEnt(hTarget)); }
	HSCRIPT ScriptGetMissionTarget(void) const { return ToHScript(this->GetMissionTarget()); }

	void SetTeleportWhere(const CUtlStringList& teleportWhereName);
	const CUtlStringList& GetTeleportWhere();
	void ClearTeleportWhere();

	void SetScaleOverride(float fScale);

	void SetMaxVisionRangeOverride(float range);
	float GetMaxVisionRangeOverride(void) const;

	void ClearTags(void);
	void AddTag(const char* tag);
	void RemoveTag(const char* tag);
	bool HasTag(const char* tag);
	void ScriptGetAllTags(HSCRIPT hTable);

	Action< CNEOBot >* OpportunisticallyUseWeaponAbilities(void);

	CNEO_Player* SelectRandomReachableEnemy(void);	// mostly for MvM - pick a random enemy player that is not in their spawn room

	float GetDesiredPathLookAheadRange(void) const;	// different sized bots used different lookahead distances

	void StartIdleSound(void);
	void StopIdleSound(void);

	void SetAutoJump(float flAutoJumpMin, float flAutoJumpMax) { m_flAutoJumpMin = flAutoJumpMin; m_flAutoJumpMax = flAutoJumpMax; }
	bool ShouldAutoJump();

	void MarkPhyscannonPickupTime() { m_flPhyscannonPickupTime = gpGlobals->curtime; }
	float GetPhyscannonPickupTime() { return m_flPhyscannonPickupTime; }


	struct EventChangeAttributes_t
	{
		EventChangeAttributes_t()
		{
			Reset();
		}

		EventChangeAttributes_t(const EventChangeAttributes_t& copy)
		{
			Reset();

			m_eventName = copy.m_eventName;

			m_skill = copy.m_skill;
			m_weaponRestriction = copy.m_weaponRestriction;
			m_mission = copy.m_mission;
			m_prevMission = copy.m_prevMission;
			m_attributeFlags = copy.m_attributeFlags;
			m_maxVisionRange = copy.m_maxVisionRange;

			for (int i = 0; i < copy.m_items.Count(); ++i)
			{
				m_items.CopyAndAddToTail(copy.m_items[i]);
			}

			for (int i = 0; i < copy.m_tags.Count(); ++i)
			{
				m_tags.CopyAndAddToTail(copy.m_tags[i]);
			}
		}

		void Reset()
		{
			m_eventName = "default";

			m_skill = CNEOBot::EASY;
			m_weaponRestriction = CNEOBot::ANY_WEAPON;
			m_mission = CNEOBot::NO_MISSION;
			m_prevMission = m_mission;
			m_attributeFlags = 0;
			m_maxVisionRange = -1.f;

			m_items.RemoveAll();

			m_tags.RemoveAll();
		}

		CUtlString m_eventName;

		DifficultyType m_skill;
		WeaponRestrictionType m_weaponRestriction;
		MissionType m_mission;
		MissionType m_prevMission;
		int m_attributeFlags;
		float m_maxVisionRange;

		CUtlStringList m_items;
		CUtlStringList m_tags;
	};
	void ClearEventChangeAttributes() { m_eventChangeAttributes.RemoveAll(); }
	void AddEventChangeAttributes(const EventChangeAttributes_t* newEvent);
	const EventChangeAttributes_t* GetEventChangeAttributes(const char* pszEventName) const;
	void OnEventChangeAttributes(const CNEOBot::EventChangeAttributes_t* pEvent);

private:
	CNEOBotLocomotion* m_locomotor;
	CNEOBotBody* m_body;
	CNEOBotVision* m_vision;

	CountdownTimer m_lookAtEnemyInvasionAreasTimer;

	CNavArea* m_spawnArea;			// where we spawned
	CountdownTimer m_justLostPointTimer;

	int m_weaponRestrictionFlags;
	int m_attributeFlags;
	DifficultyType m_difficulty;

	CNavArea* m_homeArea;

	CHandle< CNEOBotActionPoint > m_actionPoint;
	CHandle< CNEOBotProxy > m_proxy;
	CHandle< CNEOBotGenerator > m_spawner;

	CNEOBotSquad* m_squad;
	bool m_didReselectClass;

	Vector m_spotWhereEnemySentryLastInjuredMe;			// the last position where I was injured by an enemy sentry

	bool m_isLookingAroundForEnemies;

	unsigned int m_behaviorFlags;						// spawnflags from the bot_generator that spawned us
	CUtlVector< CFmtStr > m_tags;

	CHandle< CBaseEntity > m_attentionFocusEntity;

	float m_fModelScaleOverride;

	MissionType m_mission;
	MissionType m_prevMission;

	CHandle< CBaseEntity > m_missionTarget;
	CUtlString m_missionString;

	CUtlStack< CHandle<CBaseHL2MPCombatWeapon> > m_requiredWeaponStack;	// if non-empty, bot must equip the weapon on top of the stack

	CountdownTimer m_noisyTimer;

	struct DelayedNoticeInfo
	{
		CHandle< CBaseEntity > m_who;
		float m_when;
	};
	CUtlVector< DelayedNoticeInfo > m_delayedNoticeVector;

	float m_maxVisionRangeOverride;

	CountdownTimer m_opportunisticTimer;

	CSoundPatch* m_pIdleSound;

	float m_squadFormationError;
	bool m_hasBrokenFormation;

	CUtlStringList m_teleportWhereName;	// spawn name an engineer mission teleporter will override
	bool m_bForceQuickBuild;

	float m_flAutoJumpMin;
	float m_flAutoJumpMax;
	CountdownTimer m_autoJumpTimer;

	float m_flPhyscannonPickupTime = 0.0f;

	CUtlVector< const EventChangeAttributes_t* > m_eventChangeAttributes;
};


inline void CNEOBot::SetTeleportWhere(const CUtlStringList& teleportWhereName)
{
	// deep copy strings
	for (int i = 0; i < teleportWhereName.Count(); ++i)
	{
		m_teleportWhereName.CopyAndAddToTail(teleportWhereName[i]);
	}
}

inline const CUtlStringList& CNEOBot::GetTeleportWhere()
{
	return m_teleportWhereName;
}

inline void CNEOBot::ClearTeleportWhere()
{
	m_teleportWhereName.RemoveAll();
}

inline void CNEOBot::SetMissionString(CUtlString string)
{
	m_missionString = string;
}

inline CUtlString* CNEOBot::GetMissionString(void)
{
	return &m_missionString;
}

inline void CNEOBot::SetMissionTarget(CBaseEntity* target)
{
	m_missionTarget = target;
}

inline CBaseEntity* CNEOBot::GetMissionTarget(void) const
{
	return m_missionTarget;
}

inline float CNEOBot::GetSquadFormationError(void) const
{
	return m_squadFormationError;
}

inline void CNEOBot::SetSquadFormationError(float error)
{
	m_squadFormationError = error;
}

inline bool CNEOBot::HasBrokenFormation(void) const
{
	return m_hasBrokenFormation;
}

inline void CNEOBot::SetBrokenFormation(bool state)
{
	m_hasBrokenFormation = state;
}

inline void CNEOBot::SetMaxVisionRangeOverride(float range)
{
	m_maxVisionRangeOverride = range;
}

inline float CNEOBot::GetMaxVisionRangeOverride(void) const
{
	return m_maxVisionRangeOverride;
}

inline void CNEOBot::SetBehaviorFlag(unsigned int flags)
{
	m_behaviorFlags |= flags;
}

inline void CNEOBot::ClearBehaviorFlag(unsigned int flags)
{
	m_behaviorFlags &= ~flags;
}

inline bool CNEOBot::IsBehaviorFlagSet(unsigned int flags) const
{
	return (m_behaviorFlags & flags) ? true : false;
}

inline void CNEOBot::StartLookingAroundForEnemies(void)
{
	m_isLookingAroundForEnemies = true;
}

inline void CNEOBot::StopLookingAroundForEnemies(void)
{
	m_isLookingAroundForEnemies = false;
}

inline int CNEOBot::GetBotType(void) const
{
	return NEO_BOT_TYPE;
}

inline const Vector& CNEOBot::GetSpotWhereEnemySentryLastInjuredMe(void) const
{
	return m_spotWhereEnemySentryLastInjuredMe;
}

inline CNEOBot::DifficultyType CNEOBot::GetDifficulty(void) const
{
	return m_difficulty;
}

inline void CNEOBot::SetDifficulty(CNEOBot::DifficultyType difficulty)
{
	m_difficulty = difficulty;
}

inline bool CNEOBot::IsDifficulty(DifficultyType skill) const
{
	return skill == m_difficulty;
}

inline bool CNEOBot::HasProxy(void) const
{
	return m_proxy == NULL ? false : true;
}

inline void CNEOBot::SetProxy(CNEOBotProxy* proxy)
{
	m_proxy = proxy;
}

inline CNEOBotProxy* CNEOBot::GetProxy(void) const
{
	return m_proxy;
}

inline bool CNEOBot::HasSpawner(void) const
{
	return m_spawner == NULL ? false : true;
}

inline void CNEOBot::SetSpawner(CNEOBotGenerator* spawner)
{
	m_spawner = spawner;
}

inline CNEOBotGenerator* CNEOBot::GetSpawner(void) const
{
	return m_spawner;
}

inline void CNEOBot::SetActionPoint(CNEOBotActionPoint* point)
{
	m_actionPoint = point;
}

inline CNEOBotActionPoint* CNEOBot::GetActionPoint(void) const
{
	return m_actionPoint;
}

inline bool CNEOBot::IsInASquad(void) const
{
	return m_squad == NULL ? false : true;
}

inline CNEOBotSquad* CNEOBot::GetSquad(void) const
{
	return m_squad;
}

inline void CNEOBot::SetHomeArea(CNavArea* area)
{
	m_homeArea = area;
}

inline CNavArea* CNEOBot::GetHomeArea(void) const
{
	return m_homeArea;
}

inline void CNEOBot::ClearWeaponRestrictions(void)
{
	m_weaponRestrictionFlags = 0;
}

inline void CNEOBot::SetWeaponRestriction(int restrictionFlags)
{
	m_weaponRestrictionFlags |= restrictionFlags;
}

inline void CNEOBot::RemoveWeaponRestriction(int restrictionFlags)
{
	m_weaponRestrictionFlags &= ~restrictionFlags;
}

inline bool CNEOBot::HasWeaponRestriction(int restrictionFlags) const
{
	return m_weaponRestrictionFlags & restrictionFlags ? true : false;
}

inline void CNEOBot::SetAttribute(int attributeFlag)
{
	m_attributeFlags |= attributeFlag;
}

inline void CNEOBot::ClearAttribute(int attributeFlag)
{
	m_attributeFlags &= ~attributeFlag;
}

inline void CNEOBot::ClearAllAttributes()
{
	m_attributeFlags = 0;
}

inline bool CNEOBot::HasAttribute(int attributeFlag) const
{
	return m_attributeFlags & attributeFlag ? true : false;
}

inline CNavArea* CNEOBot::GetSpawnArea(void) const
{
	return m_spawnArea;
}

inline CNEOBot::MissionType CNEOBot::GetMission(void) const
{
	return m_mission;
}

inline void CNEOBot::SetPrevMission(MissionType mission)
{
	m_prevMission = mission;
}

inline CNEOBot::MissionType CNEOBot::GetPrevMission(void) const
{
	return m_prevMission;
}

inline bool CNEOBot::HasMission(MissionType mission) const
{
	return m_mission == mission ? true : false;
}

inline bool CNEOBot::IsOnAnyMission(void) const
{
	return m_mission == NO_MISSION ? false : true;
}

inline void CNEOBot::SetScaleOverride(float fScale)
{
	m_fModelScaleOverride = fScale;

	SetModelScale(m_fModelScaleOverride > 0.0f ? m_fModelScaleOverride : 1.0f);
}

inline bool CNEOBot::IsEnvironmentNoisy(void) const
{
	return !m_noisyTimer.IsElapsed();
}

//---------------------------------------------------------------------------------------------
inline CNEOBot* ToNEOBot(CBaseEntity* pEntity)
{
	if (!pEntity || !pEntity->IsPlayer() || !ToNEOPlayer(pEntity)->IsBotOfType(NEO_BOT_TYPE))
		return NULL;

	Assert("***IMPORTANT!!! DONT IGNORE ME!!!***" && dynamic_cast<CNEOBot*>(pEntity) != 0);

	return static_cast<CNEOBot*>(pEntity);
}


//---------------------------------------------------------------------------------------------
inline const CNEOBot* ToNEOBot(const CBaseEntity* pEntity)
{
	if (!pEntity || !pEntity->IsPlayer() || !ToNEOPlayer(const_cast<CBaseEntity*>(pEntity))->IsBotOfType(NEO_BOT_TYPE))
		return NULL;

	Assert("***IMPORTANT!!! DONT IGNORE ME!!!***" && dynamic_cast<const CNEOBot*>(pEntity) != 0);

	return static_cast<const CNEOBot*>(pEntity);
}


//--------------------------------------------------------------------------------------------------------------
/**
 * Functor used with NavAreaBuildPath()
 */
class CNEOBotPathCost : public IPathCost
{
public:
	CNEOBotPathCost(CNEOBot* me, RouteType routeType)
	{
		m_me = me;
		m_routeType = routeType;
		m_stepHeight = me->GetLocomotionInterface()->GetStepHeight();
		m_maxJumpHeight = me->GetLocomotionInterface()->GetMaxJumpHeight();
		m_maxDropHeight = me->GetLocomotionInterface()->GetDeathDropHeight();
	}

	virtual float operator()(CNavArea* baseArea, CNavArea* fromArea, const CNavLadder* ladder, const CFuncElevator* elevator, float length) const
	{
		VPROF_BUDGET("CNEOBotPathCost::operator()", "NextBot");

		CNavArea* area = (CNavArea*)baseArea;

		if (fromArea == NULL)
		{
			// first area in path, no cost
			return 0.0f;
		}
		else
		{
			if (!m_me->GetLocomotionInterface()->IsAreaTraversable(area))
			{
				return -1.0f;
			}

			// compute distance traveled along path so far
			float dist;

			if (ladder)
			{
				dist = ladder->m_length;
			}
			else if (length > 0.0)
			{
				dist = length;
			}
			else
			{
				dist = (area->GetCenter() - fromArea->GetCenter()).Length();
			}


			// check height change
			float deltaZ = fromArea->ComputeAdjacentConnectionHeightChange(area);

			if (deltaZ >= m_stepHeight)
			{
				if (deltaZ >= m_maxJumpHeight)
				{
					// too high to reach
					return -1.0f;
				}

				// jumping is slower than flat ground
				const float jumpPenalty = 2.0f;
				dist *= jumpPenalty;
			}
			else if (deltaZ < -m_maxDropHeight)
			{
				// too far to drop
				return -1.0f;
			}

			// add a random penalty unique to this character so they choose different routes to the same place
			float preference = 1.0f;

			if (m_routeType == DEFAULT_ROUTE)
			{
				// this term causes the same bot to choose different routes over time,
				// but keep the same route for a period in case of repaths
				int timeMod = (int)(gpGlobals->curtime / 10.0f) + 1;
				preference = 1.0f + 50.0f * (1.0f + FastCos((float)(m_me->GetEntity()->entindex() * area->GetID() * timeMod)));
			}

			if (m_routeType == SAFEST_ROUTE)
			{
				// misyl: combat areas.
#if 0
				// avoid combat areas
				if (area->IsInCombat())
				{
					const float combatDangerCost = 4.0f;
					dist *= combatDangerCost * area->GetCombatIntensity();
				}
#endif
			}

			float cost = (dist * preference);

			if (area->HasAttributes(NAV_MESH_FUNC_COST))
			{
				cost *= area->ComputeFuncNavCost(m_me);
				DebuggerBreakOnNaN_StagingOnly(cost);
			}

			return cost + fromArea->GetCostSoFar();
		}
	}

	CNEOBot* m_me;
	RouteType m_routeType;
	float m_stepHeight;
	float m_maxJumpHeight;
	float m_maxDropHeight;
};


//---------------------------------------------------------------------------------------------
class CClosestNEOPlayer
{
public:
	CClosestNEOPlayer(const Vector& where, int team = TEAM_ANY)
	{
		m_where = where;
		m_closeRangeSq = FLT_MAX;
		m_closePlayer = NULL;
		m_team = team;
	}

	CClosestNEOPlayer(CBaseEntity* entity, int team = TEAM_ANY)
	{
		m_where = entity->WorldSpaceCenter();
		m_closeRangeSq = FLT_MAX;
		m_closePlayer = NULL;
		m_team = team;
	}

	bool operator() (CBasePlayer* player)
	{
		if (!player->IsAlive())
			return true;

		if (player->GetTeamNumber() != TEAM_JINRAI && player->GetTeamNumber() != TEAM_NSF && player->GetTeamNumber() != TEAM_UNASSIGNED)
			return true;

		if (m_team != TEAM_ANY && player->GetTeamNumber() != m_team)
			return true;

		CNEOBot* bot = ToNEOBot(player);
		if (bot && bot->HasAttribute(CNEOBot::IS_NPC))
			return true;

		float rangeSq = (m_where - player->GetAbsOrigin()).LengthSqr();
		if (rangeSq < m_closeRangeSq)
		{
			m_closeRangeSq = rangeSq;
			m_closePlayer = player;
		}
		return true;
	}

	Vector m_where;
	float m_closeRangeSq;
	CBasePlayer* m_closePlayer;
	int m_team;
};


class CTraceFilterIgnoreFriendlyCombatItems : public CTraceFilterSimple
{
public:
	DECLARE_CLASS(CTraceFilterIgnoreFriendlyCombatItems, CTraceFilterSimple);

	CTraceFilterIgnoreFriendlyCombatItems(const CNEOBot* player, int collisionGroup, int iIgnoreTeam, bool bIsProjectile = false)
		: CTraceFilterSimple(player, collisionGroup), m_iIgnoreTeam(iIgnoreTeam), m_pPlayer(player)
	{
		m_bCallerIsProjectile = bIsProjectile;
	}

	virtual bool ShouldHitEntity(IHandleEntity* pServerEntity, int contentsMask)
	{
		CBaseEntity* pEntity = EntityFromEntityHandle(pServerEntity);

		if (pEntity->IsCombatItem())
		{
			if (pEntity->GetTeamNumber() == m_iIgnoreTeam)
				return false;

			// If source is a enemy projectile, be explicit, otherwise we fail a "IsTransparent" test downstream
			if (m_bCallerIsProjectile)
				return true;
		}

		// misyl: Ignore any held props, as if we can see through them...
		if (!pEntity->BlocksLOS())
			return false;

		IPhysicsObject* pObject = pEntity->VPhysicsGetObject();
		if (pObject && (pObject->GetGameFlags() & FVPHYSICS_PLAYER_HELD))
			return false;

		return BaseClass::ShouldHitEntity(pServerEntity, contentsMask);
	}

	const CNEOBot* m_pPlayer;
	int m_iIgnoreTeam;
	bool m_bCallerIsProjectile;
};
