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
#endif

enum
{
	TEAM_JINRAI = LAST_SHARED_TEAM + 1,
	TEAM_NSF,
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
#else
class C_NEO_Player;
#endif

enum NeoRoundStatus {
	Idle = 0,
	//Warmup,
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

	virtual void SetWinningTeam(int team, int iWinReason, bool bForceMapReset = true, bool bSwitchTeams = false, bool bDontAddScore = false, bool bFinal = false) OVERRIDE;

	virtual void ChangeLevel(void) OVERRIDE;

	virtual void ClientDisconnected(edict_t* pClient) OVERRIDE;
#endif
	virtual bool ShouldCollide( int collisionGroup0, int collisionGroup1 ) OVERRIDE;

	virtual const char* GetGameName() { return NEO_GAME_NAME; }

#ifdef GAME_DLL
	virtual bool FPlayerCanRespawn(CBasePlayer* pPlayer) OVERRIDE;
#endif

	virtual int GetGameType(void) OVERRIDE { return NEO_GAME_TYPE_CTG; /*NEO TODO (Rain): modes*/ }
	virtual const char* GetGameTypeName(void) OVERRIDE;

	virtual void Think( void ) OVERRIDE;
	virtual void CreateStandardEntities( void ) OVERRIDE;

	virtual int WeaponShouldRespawn(CBaseCombatWeapon* pWeapon) OVERRIDE;

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

	float GetRoundRemainingTime();

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

#ifdef GAME_DLL
	bool IsRoundOver() const;
	void StartNextRound();

	virtual const char* GetChatFormat(bool bTeamOnly, CBasePlayer* pPlayer) OVERRIDE;
	virtual const char* GetChatPrefix(bool bTeamOnly, CBasePlayer* pPlayer) OVERRIDE { return ""; } // handled by GetChatFormat
	virtual const char* GetChatLocation(bool bTeamOnly, CBasePlayer* pPlayer) OVERRIDE { return NULL; } // unimplemented
#endif

	void SetRoundStatus(NeoRoundStatus status);

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

public:
#ifdef GAME_DLL
	// Workaround for bot spawning. See Bot_f() for details.
	bool m_bNextClientIsFakeClient;
#endif

private:
#ifdef GAME_DLL
	CUtlVector<int> m_pGhostCaps;
#endif
	CNetworkVar(int, m_nRoundStatus); // NEO TODO (Rain): probably don't need to network this
	CNetworkVar(int, m_iRoundNumber);

	CNetworkVar(float, m_flNeoRoundStartTime);
	CNetworkVar(float, m_flNeoNextRoundStartTime);
};

inline CNEORules *NEORules()
{
	return static_cast<CNEORules*>(g_pGameRules);
}

#endif // NEO_GAMERULES_H