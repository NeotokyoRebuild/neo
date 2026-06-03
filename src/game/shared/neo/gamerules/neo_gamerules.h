#ifndef NEO_GAMERULES_H
#define NEO_GAMERULES_H
#ifdef _WIN32
#pragma once
#endif

#include "gamerules.h"
#include "teamplay_gamerules.h"
#include "gamevars_shared.h"
#include "hl2mp_gamerules.h"
#include "shareddefs.h"

#include "GameEventListener.h"
#include "neo_player_shared.h"
#include "neo_misc.h"
#include "weapon_ghost.h"
#include "neo_juggernaut.h"

#ifdef CLIENT_DLL
	#include "c_neo_player.h"
#else
	#include "neo_player.h"
	#include "neo_ghost_spawn_point.h"
	#include "utlhashtable.h"
#endif

#ifdef CLIENT_DLL
	#define CNEORules C_NEORules
	#define CNEOGameRulesProxy C_NEOGameRulesProxy
#endif

// NEO JANK (Rain): magic value for signaling a mp_restart originated from "neo_restart_this".
// This is a hack around neo_restart_this leaking edicts for some reason; for now, it just
// repurposes the mp_restartgame logic but without the HL2DM-style center print.
constexpr float MAGIC_NEO_RESTART_THIS = 0xdaff;

class CNEOGameRulesProxy : public CHL2MPGameRulesProxy
{
public:
	DECLARE_CLASS( CNEOGameRulesProxy, CHL2MPGameRulesProxy );
	DECLARE_NETWORKCLASS();

#ifdef CLIENT_DLL
	void OnDataChanged(DataUpdateType_t updateType) override;
#endif // CLIENT_DLL
};

class NEOViewVectors : public HL2MPViewVectors
{
public:
	NEOViewVectors( 
		// Same as HL2MP, passed to parent ctor
		Vector vView,
		Vector vHullMin,
		Vector vHullMax,
		Vector vDuckHullMin,
		Vector vDuckHullMax,
		Vector vDuckView,
		Vector vObsHullMin,
		Vector vObsHullMax,
		Vector vDeadViewHeight,
		Vector vCrouchTraceMin,
		Vector vCrouchTraceMax) :
			HL2MPViewVectors( 
				vView,
				vHullMin,
				vHullMax,
				vDuckHullMin,
				vDuckHullMax,
				vDuckView,
				vObsHullMin,
				vObsHullMax,
				vDeadViewHeight,
				vCrouchTraceMin,
				vCrouchTraceMax )
	{
	}
};

#ifdef GAME_DLL
class CNEOGhostCapturePoint;
class CNEO_Player;
class CWeaponGhost;
class CNEOBotSeekAndDestroy;

extern ConVar sv_neo_mirror_teamdamage_multiplier;
extern ConVar sv_neo_mirror_teamdamage_duration;
extern ConVar sv_neo_mirror_teamdamage_immunity;
extern ConVar sv_neo_teamdamage_kick;

#else
class C_NEO_Player;
#endif

extern ConVar sv_neo_player_restore;

enum NeoGameType {
	NEO_GAME_TYPE_TDM = 0,
	NEO_GAME_TYPE_CTG,
	NEO_GAME_TYPE_VIP,
	NEO_GAME_TYPE_DM,
	NEO_GAME_TYPE_EMT,
	NEO_GAME_TYPE_TUT,
	NEO_GAME_TYPE_JGR,
	NEO_GAME_TYPE_ATK,

	NEO_GAME_TYPE__TOTAL // Number of game types
};

struct NeoGameTypeSettings;

extern const SZWSZTexts NEO_GAME_TYPE_DESC_STRS[NEO_GAME_TYPE__TOTAL];

enum NeoRoundStatus {
	Idle = 0,
	Warmup,
	PreRoundFreeze,
	RoundLive,
	Overtime,
	PostRound,
	Pause,
	Countdown,

	RoundStatusTotal
};

enum NeoWinReason {
	NEO_VICTORY_GHOST_CAPTURE = 0,
	NEO_VICTORY_VIP_ESCORT,
	NEO_VICTORY_VIP_ELIMINATION,
	NEO_VICTORY_TEAM_ELIMINATION,
	NEO_VICTORY_TIMEOUT_WIN_BY_NUMBERS,
	NEO_VICTORY_ATK_TIMEOUT,
	NEO_VICTORY_POINTS,
	NEO_VICTORY_FORFEIT,
	NEO_VICTORY_STALEMATE, // Not actually a victory
	NEO_VICTORY_MAPIO
};

#define NEO_HUD_BITS_UNDERLYING_TYPE int
enum NeoHudElements : NEO_HUD_BITS_UNDERLYING_TYPE {
	NEO_HUD_ELEMENT_INVALID = 0x0,

	NEO_HUD_ELEMENT_AMMO = (static_cast<NEO_HUD_BITS_UNDERLYING_TYPE>(1) << 0),
	NEO_HUD_ELEMENT_COMPASS = (static_cast<NEO_HUD_BITS_UNDERLYING_TYPE>(1) << 1),
	NEO_HUD_ELEMENT_CROSSHAIR = (static_cast<NEO_HUD_BITS_UNDERLYING_TYPE>(1) << 2),
	NEO_HUD_ELEMENT_DAMAGE_INDICATOR = (static_cast<NEO_HUD_BITS_UNDERLYING_TYPE>(1) << 3),
	NEO_HUD_ELEMENT_FRIENDLY_MARKER = (static_cast<NEO_HUD_BITS_UNDERLYING_TYPE>(1) << 4),
	NEO_HUD_ELEMENT_GAME_EVENT = (static_cast<NEO_HUD_BITS_UNDERLYING_TYPE>(1) << 5),
	NEO_HUD_ELEMENT_GHOST_BEACONS = (static_cast<NEO_HUD_BITS_UNDERLYING_TYPE>(1) << 6),
	NEO_HUD_ELEMENT_GHOST_CAP_POINTS = (static_cast<NEO_HUD_BITS_UNDERLYING_TYPE>(1) << 7),
	NEO_HUD_ELEMENT_GHOST_MARKER = (static_cast<NEO_HUD_BITS_UNDERLYING_TYPE>(1) << 8),
	NEO_HUD_ELEMENT_GHOST_UPLINK_STATE = (static_cast<NEO_HUD_BITS_UNDERLYING_TYPE>(1) << 9),
	NEO_HUD_ELEMENT_HEALTH_THERMOPTIC_AUX = (static_cast<NEO_HUD_BITS_UNDERLYING_TYPE>(1) << 10),
	NEO_HUD_ELEMENT_HINT = (static_cast<NEO_HUD_BITS_UNDERLYING_TYPE>(1) << 11),
	NEO_HUD_ELEMENT_ROUND_STATE = (static_cast<NEO_HUD_BITS_UNDERLYING_TYPE>(1) << 12),
	NEO_HUD_ELEMENT_WORLDPOS_MARKER = (static_cast<NEO_HUD_BITS_UNDERLYING_TYPE>(1) << 13),
	NEO_HUD_ELEMENT_SCOREBOARD = (static_cast<NEO_HUD_BITS_UNDERLYING_TYPE>(1) << 14),
	NEO_HUD_ELEMENT_PLAYER_PING = (static_cast<NEO_HUD_BITS_UNDERLYING_TYPE>(1) << 15),
	NEO_HUD_ELEMENT_WORLDPOS_MARKER_ENT = (static_cast<NEO_HUD_BITS_UNDERLYING_TYPE>(1) << 16),
};

enum NeoSpectateEvent {
	NEO_SPECTATE_EVENT_LAST_HURT = 0,
	NEO_SPECTATE_EVENT_LAST_SHOOTER,
	NEO_SPECTATE_EVENT_LAST_EVENT,
	NEO_SPECTATE_EVENT_LAST_ATTACKER,
	NEO_SPECTATE_EVENT_LAST_KILLER,
	NEO_SPECTATE_EVENT_LAST_GHOSTER,
};

class CNEORules : public CHL2MPRules, public CGameEventListener
{

#ifdef GAME_DLL
friend class CNEORulesATK;
friend class CNEORulesCTG;
friend class CNEORulesDM;
friend class CNEORulesEMT;
friend class CNEORulesJGR;
friend class CNEORulesTDM;
friend class CNEORulesTUT;
friend class CNEORulesVIP;
#else
friend class C_NEORulesATK;
friend class C_NEORulesCTG;
friend class C_NEORulesDM;
friend class C_NEORulesEMT;
friend class C_NEORulesJGR;
friend class C_NEORulesTDM;
friend class C_NEORulesTUT;
friend class C_NEORulesVIP;
#endif // GAME_DLL

public:
	DECLARE_CLASS( CNEORules, CHL2MPRules );
	// This makes datatables able to access our private vars.
	DECLARE_NETWORKCLASS_NOBASE();
	

	CNEORules();
	virtual ~CNEORules();
	
	// IGameEventListener interface:
	virtual void FireGameEvent(IGameEvent *event) OVERRIDE;
	
	virtual void Think() OVERRIDE;

	// This is the supposed encrypt key on NT, although it has its issues.
	// See https://steamcommunity.com/groups/ANPA/discussions/0/1482109512299590948/
	// (and NT Discord) for discussions.
	virtual const unsigned char* GetEncryptionKey(void) OVERRIDE { return (unsigned char*)"tBA%-ygc"; }

#ifdef GAME_DLL
	bool RoundStartFromIdleOrPausedThink();
	virtual void PlayerRespawnThink();
	void CheckClantagsThink();
	bool GameOverThink();
	void TeamDamageThink();
	bool RoundOverThink();
	void RoundStatusThink();
	void CheckWinByElimination();

	virtual void Precache() override;
	virtual void InitDefaultAIRelationships(void); // NEO NOTE (Adam) override?


	virtual bool ClientConnected(edict_t *pEntity, const char *pszName, const char *pszAddress, char *reject, int maxrejectlen) override;
	virtual bool ClientCommand(CBaseEntity* pEdict, const CCommand& args) override;
	virtual void ClientDisconnected(edict_t* pClient) override;
	virtual const char* GetChatFormat(bool bTeamOnly, CBasePlayer* pPlayer) OVERRIDE;
	virtual const char* GetChatPrefix(bool bTeamOnly, CBasePlayer* pPlayer) OVERRIDE { return ""; } // handled by GetChatFormat
	virtual const char* GetChatLocation(bool bTeamOnly, CBasePlayer* pPlayer) OVERRIDE { return NULL; } // unimplemented

	virtual bool FPlayerCanRespawn(CBasePlayer* pPlayer) override;
	virtual float FlPlayerFallDamage(CBasePlayer* pPlayer) OVERRIDE;


	virtual void SetWinningTeam(int team, int iWinReason, bool bForceMapReset = true, bool bSwitchTeams = false, bool bDontAddScore = false, bool bFinal = false) OVERRIDE;
	void AwardRankUp(CNEO_Player *pClient);
	void SetRoundStatus(NeoRoundStatus status);


	// NEOGameConfig is a logic entity, which is a server only entity. To access config values client side, we need to copy values to a networked entity
	void UpdateFromGameConfig();
	void StartNextRound();
	virtual void RoundTimeout() {};


	struct ReadyPlayers
	{
		int array[TEAM__TOTAL];
	};
	void CheckChatCommand(CNEO_Player *pNeoPlayer, const char *pSzChat);
	ReadyPlayers FetchReadyPlayers() const; // NEO TODO (Adam) This needs to be shown in the ui somewhere instead, ready up button
	CUtlHashtable<AccountID_t> m_readyAccIDs; // NEO NOTE (Adam) What's wrong with player ent index
	bool m_bIgnoreOverThreshold = false;
	bool ReadyUpPlayerIsReady(CNEO_Player *pNeoPlayer) const;


	virtual void CleanUpMap() OVERRIDE;
	virtual void RestartGame() OVERRIDE;
	virtual void ChangeLevel(void) OVERRIDE;


	virtual bool IsOfficialMap(void) override;
	virtual void MarkAchievement ( IRecipientFilter& filter, char const *pchAchievementName ) override;
#endif

#ifdef CLIENT_DLL
#endif // CLIENT_DLL

	virtual void CreateStandardEntities( void ) OVERRIDE;
	virtual const CViewVectors* GetViewVectors() const OVERRIDE;
	const NEOViewVectors* GetNEOViewVectors() const;


	virtual void ClientSettingsChanged(CBasePlayer *pPlayer) OVERRIDE;
	virtual void ClientSpawned(edict_t* pPlayer) OVERRIDE;
	
	virtual void PlayerKilled(CBasePlayer *pVictim, const CTakeDamageInfo &info) override;
	virtual void EnemyPlayerKilled(CNEO_Player* pVictim, CNEO_Player* pAttacker, const CTakeDamageInfo& info) {};
	virtual void DeathNotice(CBasePlayer* pVictim, const CTakeDamageInfo& info) OVERRIDE;


	virtual const char* GetGameName() { return NEO_GAME_NAME; }
	virtual const char* GetGameTypeName(void) override;
	virtual int GetGameType(void) override;
	virtual const char* GetGameDescription(void) override { return NEO_GAME_NAME; };
	virtual bool GetTeamPlayEnabled() const override;
	int GetHiddenHudElements();
	int GetForcedTeam();
	int GetForcedClass();
	int GetForcedSkin();
	int GetForcedWeapon();
	bool IsCyberspace();
	bool CanChangeTeamClassLoadoutWhenAlive(); // NEO TODO (Adam) Replace with gamemode specific check wherever used
	bool CanRespawnAnyTime(); // NEO TODO (Adam) Replace with gamemode specific check wherever used
	virtual int DefaultFOV(void) override;
	
	bool CheckShouldNotThink(); // NEO TODO (Adam) Remove
	
	const char *GetTeamClantag(const int iTeamNum) const;
    inline int roundNumber() const { return m_iRoundNumber; }
    inline bool roundNumberIsEven() const { return (roundNumber() % 2 == 0); }
	NeoRoundStatus GetRoundStatus() const;
	int GetAttackingTeam() const;
	int GetDefendingTeam() const;
	float GetRemainingPreRoundFreezeTime(const bool clampToZero) const;
	float GetMapRemainingTime();
	virtual float GetRoundRemainingTime() const;
	float GetRoundRemainingTime(float flGameTypeRoundTimeLimit) const;
	float GetOverTime(float flRoundTimeLimit, float flOvertimeBaseAmount, float flOvertimeGrace, float flGraceDecay) const;
	virtual void CheckOvertime();
	virtual bool CheckGameOver(void) OVERRIDE; // NEO TODO (Adam) this changes map as a side effect, better name? Also is this called client side anywhere?
	float GetRoundAccumulatedTime() const;
#ifdef GAME_DLL
	float MirrorDamageMultiplier() const;
#endif
	bool RoundIsInSuddenDeath() const;
	bool RoundIsMatchPoint() const;
	bool RoundIsDoOrDie() const;
	bool IsRoundPreRoundFreeze() const;
	bool IsRoundLive() const;
	bool IsRoundOn() const;
	bool IsRoundOver() const;
	bool IsRoundIdle() const;
	inline bool IsRoundPaused() const;
	int GetOpposingTeam(const int team) const // NEO TODO (Adam) move to .cpp and / or gamemode specific gamerules
	{
		if (team == TEAM_JINRAI) { return TEAM_NSF; }
		if (team == TEAM_NSF) { return TEAM_JINRAI; }
		Assert(false);
		return TEAM_SPECTATOR;
	}

	int GetOpposingTeam(const CBaseCombatCharacter* player) const  // NEO TODO (Adam) move to .cpp and / or gamemode specific gamerules
	{
		if (!player)
		{
			Assert(false);
			return TEAM_SPECTATOR;
		}

		return GetOpposingTeam(player->GetTeamNumber());
	}
#ifdef GLOWS_ENABLE
	void GetTeamGlowColor(int teamNumber, float &r, float &g, float &b)
{
	if (teamNumber == TEAM_JINRAI)
	{
		r = static_cast<float>(COLOR_JINRAI.r()/255.f);
		g = static_cast<float>(COLOR_JINRAI.g()/255.f);
		b = static_cast<float>(COLOR_JINRAI.b()/255.f);
	}
	else if (teamNumber == TEAM_NSF)
	{
		r = static_cast<float>(COLOR_NSF.r()/255.f);
		g = static_cast<float>(COLOR_NSF.g()/255.f);
		b = static_cast<float>(COLOR_NSF.b()/255.f);
	}
	else
	{
		r = 0.76f;
		g = 0.76f;
		b = 0.76f;
	}
}
#endif


	virtual bool ShouldCollide( int collisionGroup0, int collisionGroup1 ) OVERRIDE;

	void PurgeGhostCapPoints(); // NEO TODO (Adam) move to ctg / vip gamerules
	void ResetGhostCapPoints(); // NEO TODO (Adam) move to ctg / vip gamerules

#ifdef GAME_DLL
	virtual void SetGameRelatedVars() {};
	void ResetTeamScores();
#endif // GAME_DLL
	void ResetGhost(); // NEO TODO (Adam) move to related gamerules
	void ResetVIP(); // NEO TODO (Adam) move to related gamerules
	void ResetJGR(); // NEO TODO (Adam) move to related gamerules


#ifdef GAME_DLL
	void CheckIfCapPrevent(CNEO_Player *capPreventerPlayer); // NEO TODO (Adam) move to related gamerules
#endif

	// NEO TODO (Adam) Move to gamemode specific gamerules
	int GetGhosterTeam() const { return m_iGhosterTeam; }
	int GetGhosterPlayer() const { return m_iGhosterPlayer; }
	bool GhostExists() const { return m_bGhostExists; }
	const Vector& GetGhostPos() const;
	Vector GetGhostMarkerPos() const;

	// NEO TODO (Adam) Move to gamemode specific gamerules
	int GetJuggernautPlayer() const { return m_iJuggernautPlayerIndex; }
	bool JuggernautItemExists() const;
	const Vector& GetJuggernautMarkerPos() const;
	bool IsJuggernautLocked() const;


#ifdef GAME_DLL
	void OnNavMeshLoad() override;
#endif // GAME_DL:

public:
#ifdef GAME_DLL
	// Workaround for bot spawning. See Bot_f() for details.
	bool m_bNextClientIsFakeClient;
	struct RestoreInfo
	{
		int xp;
		int deaths;
		bool spawnedThisRound;
		float deathTime;
	};
	// AccountID_t <- CSteamID::GetAccountID
	CUtlHashtable<AccountID_t, RestoreInfo> m_pRestoredInfos;

	float m_flPauseDur = 0.0f;
	int m_iPausingTeam = 0;
	int m_iPausingRound = 0;
	bool m_bPausedByPreRoundFreeze = false;
	bool m_bPausingTeamRequestedUnpause = false;
	bool m_bThinkCheckClantags = false;
	bool m_bRotatingMapRightNow = false;
#endif
	CNetworkVar(float, m_flPauseEnd);

private:
	void ResetMapSessionCommon(); // NEO TODO (Adam) Is this needed client side

#ifdef GAME_DLL
	virtual const int GetScoreLimit() const { Assert(false); return 0; };
	virtual const int GetRoundLimit() const { Assert(false); return 0; };

	void SpawnTheGhost(const Vector *origin = nullptr);
	void SpawnTheJuggernaut(const Vector *origin = nullptr);
	void SelectTheVIP();
public:
	void JuggernautActivated(CNEO_Player *pPlayer);
	void JuggernautDeactivated(CNEO_Juggernaut *pJuggernaut);
	void JuggernautTotalRemoval(CNEO_Juggernaut *pJuggernaut);

	void SetLastHurt(const int index) { m_iLastHurt = index; }
	void SetLastShooter(const int index) { m_iLastShooter = index; }
	void SetLastAttacker(const int index) { m_iLastAttacker = m_iLastEvent = index; }
	void SetLastKiller(const int index) { m_iLastKiller = m_iLastEvent = index; }
	void SetLastGhoster(const int index) { m_iLastGhoster = m_iLastEvent = index; }
#endif // GAME_DLL
public:
	const int GetLastHurt() const { return m_iLastHurt; }
	const int GetLastShooter() const { return m_iLastShooter; }
	const int GetLastEvent() const { return m_iLastEvent; }
	const int GetLastAttacker() const { return m_iLastAttacker; }
	const int GetLastKiller() const { return m_iLastKiller; }
	const int GetLastGhoster() const { return m_iLastGhoster; }
#ifdef GAME_DLL
private:
	CNEO_Juggernaut *m_pJuggernautItem = nullptr;
	CNEO_Player *m_pJuggernautPlayer = nullptr;
	float m_flJuggernautDeathTime = 0.0f;
	int m_iLastJuggernautTeam = TEAM_INVALID;
	
	// For looking up capture zone locations
	friend class CNEOBotCtgCarrier;
	friend class CNEOBotCtgEscort;
	friend class CNEOBotCtgLoneWolf;

	friend class CNEOBotSeekAndDestroy;
	CUtlVector<int> m_pGhostCaps;
	CWeaponGhost *m_pGhost = nullptr;
	CNEO_Player *m_pVIP = nullptr;
	int m_iVIPPreviousClass = 0;

	float m_flPrevThinkKick = 0.0f;
	float m_flPrevThinkMirrorDmg = 0.0f;
	bool m_bTeamBeenAwardedDueToCapPrevent = false;
	int m_arrayiEntPrevCap[MAX_PLAYERS + 1]; // This is to check for cap-prevention workaround attempts
	int m_iEntPrevCapSize = 0;
	int m_iPrintHelpCounter = 0;
	bool m_bGamemodeTypeBeenInitialized = false;
	bool m_bServerIsCurrentlyAutoRecording = false;
	friend class CNEO_GhostBoundary;
	friend class CNEOGhostSpawnPoint;
	friend class CNEOJuggernautSpawnPoint;
	friend class CMultiplayRules;
	CUtlVector<CHandle<CNEOGhostSpawnPoint>> m_ghostSpawns;
	CUtlVector<CHandle<CNEOJuggernautSpawnPoint>> m_jgrSpawns;
	Vector m_vecPreviousGhostSpawn = vec3_origin;
	Vector m_vecPreviousJuggernautSpawn = vec3_origin;
	bool m_bGotMatchWinner = false;
	int m_iMatchWinner = TEAM_UNASSIGNED;
#endif
	CNetworkVar(int, m_nRoundStatus);
	CNetworkVar(int, m_iHiddenHudElements);
	CNetworkVar(int, m_iForcedTeam);
	CNetworkVar(int, m_iForcedClass);
	CNetworkVar(int, m_iForcedSkin);
	CNetworkVar(int, m_iForcedWeapon);
	CNetworkVar(bool, m_bCyberspaceLevel);
	CNetworkVar(int, m_nGameTypeSelected);
	CNetworkVar(int, m_iRoundNumber);
	CNetworkVar(bool, m_bIsMatchPoint);
	CNetworkVar(bool, m_bIsDoOrDie);
	CNetworkVar(bool, m_bIsInSuddenDeath);
	CNetworkString(m_szNeoJinraiClantag, NEO_MAX_CLANTAG_LENGTH);
	CNetworkString(m_szNeoNSFClantag, NEO_MAX_CLANTAG_LENGTH);

	// Ghost networked variables
	CNetworkVar(int, m_iGhosterTeam);
	CNetworkVar(int, m_iGhosterPlayer);
	CNetworkVector(m_vecGhostMarkerPos);
	CNetworkVar(bool, m_bGhostExists);
	CNetworkVar(float, m_flGhostLastHeld);
	CNetworkHandle( CWeaponGhost, m_hGhost );

	// Juggernaut networked variables
	CNetworkVar(int, m_iJuggernautPlayerIndex);
	CNetworkVar(bool, m_bJuggernautItemExists);
	CNetworkHandle( CBaseEntity, m_hJuggernaut );

	CNetworkVar(float, m_flNeoRoundStartTime);
	CNetworkVar(float, m_flNeoNextRoundStartTime);

	// For spectator commands. Networked so can be saved in demos for hltv
	CNetworkVar(int, m_iLastHurt);
	CNetworkVar(int, m_iLastShooter);
	CNetworkVar(int, m_iLastEvent);
	CNetworkVar(int, m_iLastAttacker);
	CNetworkVar(int, m_iLastKiller);
	CNetworkVar(int, m_iLastGhoster);

public:
	// VIP networked variables
	CNetworkVar(int, m_iEscortingTeam);
};

inline CNEORules *NEORules()
{
	return static_cast<CNEORules*>(g_pGameRules);
}

#endif // NEO_GAMERULES_H
