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

#ifndef CLIENT_DLL
	#include "neo_player.h"
	#include "utlhashtable.h"
#endif

enum
{
	TEAM_JINRAI = LAST_SHARED_TEAM + 1,
	TEAM_NSF,

	TEAM__TOTAL, // Always last enum in here
};

#define TEAM_STR_JINRAI "Jinrai"
#define TEAM_STR_NSF "NSF"
#define TEAM_STR_SPEC "Spectator"

#define NEO_GAME_NAME "Neotokyo: Revamp"

#define NEO_GAME_TYPE_TDM 0
#define NEO_GAME_TYPE_CTG 1
#define NEO_GAME_TYPE_VIP 2

#ifdef CLIENT_DLL
	#define CNEORules C_NEORules
	#define CNEOGameRulesProxy C_NEOGameRulesProxy
#endif

class CNEOGameRulesProxy : public CHL2MPGameRulesProxy
{
public:
	DECLARE_CLASS( CNEOGameRulesProxy, CHL2MPGameRulesProxy );
	DECLARE_NETWORKCLASS();
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

extern ConVar neo_sv_mirror_teamdamage_multiplier;
extern ConVar neo_sv_mirror_teamdamage_duration;
extern ConVar neo_sv_mirror_teamdamage_immunity;
extern ConVar neo_sv_teamdamage_kick;
#else
class C_NEO_Player;
#endif

extern ConVar neo_sv_player_restore;

enum NeoRoundStatus {
	Idle = 0,
	Warmup,
	PreRoundFreeze,
	RoundLive,
	PostRound,
};

class CNEORules : public CHL2MPRules, public CGameEventListener
{
public:
	DECLARE_CLASS( CNEORules, CHL2MPRules );

// This makes datatables able to access our private vars.
#ifdef CLIENT_DLL
	DECLARE_CLIENTCLASS_NOBASE();
#else
	DECLARE_SERVERCLASS_NOBASE();
#endif

	CNEORules();
	virtual ~CNEORules();

#ifdef GAME_DLL
	virtual void Precache() OVERRIDE;

	virtual bool ClientConnected(edict_t *pEntity, const char *pszName, const char *pszAddress, char *reject, int maxrejectlen) OVERRIDE;

	virtual bool ClientCommand(CBaseEntity* pEdict, const CCommand& args) OVERRIDE;

	virtual void SetWinningTeam(int team, int iWinReason, bool bForceMapReset = true, bool bSwitchTeams = false, bool bDontAddScore = false, bool bFinal = false) OVERRIDE;

	virtual void ChangeLevel(void) OVERRIDE;

	virtual void ClientDisconnected(edict_t* pClient) OVERRIDE;
#endif
	bool ShouldCollide(const CBaseEntity *ent0, const CBaseEntity *ent1) const;

	virtual const char* GetGameName() { return NEO_GAME_NAME; }

#ifdef GAME_DLL
	virtual bool FPlayerCanRespawn(CBasePlayer* pPlayer) OVERRIDE;
#endif

	virtual int GetGameType(void) OVERRIDE { return NEO_GAME_TYPE_CTG; /*NEO TODO (Rain): modes*/ }
	virtual const char* GetGameTypeName(void) OVERRIDE;

	virtual void Think( void ) OVERRIDE;
	virtual void CreateStandardEntities( void ) OVERRIDE;

	virtual const char *GetGameDescription( void ) OVERRIDE;
	virtual const CViewVectors* GetViewVectors() const OVERRIDE;

	const NEOViewVectors* GetNEOViewVectors() const;

	virtual void ClientSettingsChanged(CBasePlayer *pPlayer) OVERRIDE;

	virtual void ClientSpawned(edict_t* pPlayer) OVERRIDE;

	virtual void DeathNotice(CBasePlayer* pVictim, const CTakeDamageInfo& info) OVERRIDE
#ifdef CLIENT_DLL
	{ }
#else
	;
#endif

	bool RoundIsInSuddenDeath() const;
	bool RoundIsMatchPoint() const;

	virtual int DefaultFOV(void) override;

	float GetRemainingPreRoundFreezeTime(const bool clampToZero) const;

	float GetMapRemainingTime();

	void ResetGhostCapPoints();

	void CheckRestartGame();

	void AwardRankUp(int client);
#ifdef CLIENT_DLL
	void AwardRankUp(C_NEO_Player *pClient);
#else
	void AwardRankUp(CNEO_Player *pClient);
#endif

	virtual bool CheckGameOver(void) OVERRIDE;

	float GetRoundRemainingTime() const;
	float GetRoundAccumulatedTime() const;
#ifdef GAME_DLL
	float MirrorDamageMultiplier() const;
#endif

#ifdef GAME_DLL
	void CheckIfCapPrevent(CNEO_Player *capPreventerPlayer);
#endif
	virtual void PlayerKilled(CBasePlayer *pVictim, const CTakeDamageInfo &info) OVERRIDE;

	// IGameEventListener interface:
	virtual void FireGameEvent(IGameEvent *event) OVERRIDE;

#ifdef CLIENT_DLL
	void CleanUpMap();
	void RestartGame();
#else
	virtual void CleanUpMap() OVERRIDE;
	virtual void RestartGame() OVERRIDE;

	virtual float FlPlayerFallDamage(CBasePlayer* pPlayer) OVERRIDE;
#endif

	bool IsRoundOver() const;
#ifdef GAME_DLL
	void StartNextRound();

	virtual const char* GetChatFormat(bool bTeamOnly, CBasePlayer* pPlayer) OVERRIDE;
	virtual const char* GetChatPrefix(bool bTeamOnly, CBasePlayer* pPlayer) OVERRIDE { return ""; } // handled by GetChatFormat
	virtual const char* GetChatLocation(bool bTeamOnly, CBasePlayer* pPlayer) OVERRIDE { return NULL; } // unimplemented
#endif

	void SetRoundStatus(NeoRoundStatus status);
	NeoRoundStatus GetRoundStatus() const;

	// This is the supposed encrypt key on NT, although it has its issues.
	// See https://steamcommunity.com/groups/ANPA/discussions/0/1482109512299590948/
	// (and NT Discord) for discussions.
	virtual const unsigned char* GetEncryptionKey(void) OVERRIDE { return (unsigned char*)"tBA%-ygc"; }

	enum
	{
		NEO_VICTORY_GHOST_CAPTURE = 0,
		NEO_VICTORY_TEAM_ELIMINATION,
		NEO_VICTORY_TIMEOUT_WIN_BY_NUMBERS,
		NEO_VICTORY_FORFEIT,
		NEO_VICTORY_STALEMATE // Not actually a victory
	};

	int GetGhosterTeam() const { return m_iGhosterTeam; }
	int GetGhosterPlayer() const { return m_iGhosterPlayer; }
	bool GhostExists() const { return m_bGhostExists; }
	Vector GetGhostPos() const { return m_vecGhostMarkerPos; }

	int GetOpposingTeam(const int team) const
	{
		if (team == TEAM_JINRAI) { return TEAM_NSF; }
		if (team == TEAM_NSF) { return TEAM_JINRAI; }
		Assert(false);
		return TEAM_SPECTATOR;
	}

	int GetOpposingTeam(const CBaseCombatCharacter* player) const
	{
		if (!player)
		{
			Assert(false);
			return TEAM_SPECTATOR;
		}

		return GetOpposingTeam(player->GetTeamNumber());
	}

        int roundNumber() const { return m_iRoundNumber; }
        bool roundAlternate() const { return static_cast<bool>(m_iRoundNumber % 2 == 0); }

public:
#ifdef GAME_DLL
	// Workaround for bot spawning. See Bot_f() for details.
	bool m_bNextClientIsFakeClient;
	struct RestoreInfo
	{
		int xp;
		int deaths;
	};
	// AccountID_t <- CSteamID::GetAccountID
	CUtlHashtable<AccountID_t, RestoreInfo> m_pRestoredInfos;
#endif

private:
	void ResetMapSessionCommon();

#ifdef GAME_DLL
	void SpawnTheGhost();

	CUtlVector<int> m_pGhostCaps;
	CWeaponGhost *m_pGhost = nullptr;
	float m_flPrevThinkKick = 0.0f;
	float m_flPrevThinkMirrorDmg = 0.0f;
	bool m_bTeamBeenAwardedDueToCapPrevent = false;
	int m_arrayiEntPrevCap[MAX_PLAYERS + 1]; // This is to check for cap-prevention workaround attempts
	int m_iEntPrevCapSize = 0;
#endif
	CNetworkVar(int, m_nRoundStatus); // NEO TODO (Rain): probably don't need to network this
	CNetworkVar(int, m_iRoundNumber);

	// Ghost networked variables
	CNetworkVar(int, m_iGhosterTeam);
	CNetworkVar(int, m_iGhosterPlayer);
	CNetworkVector(m_vecGhostMarkerPos);
	CNetworkVar(bool, m_bGhostExists);

	CNetworkVar(float, m_flNeoRoundStartTime);
	CNetworkVar(float, m_flNeoNextRoundStartTime);
};

inline CNEORules *NEORules()
{
	return static_cast<CNEORules*>(g_pGameRules);
}

#endif // NEO_GAMERULES_H
