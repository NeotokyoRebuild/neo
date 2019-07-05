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
#endif

class CNEORules : public CHL2MPRules
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
	virtual void Precache();

	virtual bool ClientConnected(edict_t *pEntity, const char *pszName, const char *pszAddress, char *reject, int maxrejectlen);

	virtual void SetWinningTeam(int team, int iWinReason, bool bForceMapReset = true, bool bSwitchTeams = false, bool bDontAddScore = false, bool bFinal = false);
#endif
	virtual bool ShouldCollide( int collisionGroup0, int collisionGroup1 );

	virtual void Think( void );
	virtual void CreateStandardEntities( void );

	virtual int WeaponShouldRespawn(CBaseCombatWeapon* pWeapon);

	virtual const char *GetGameDescription( void );
	virtual const CViewVectors* GetViewVectors() const;

	const NEOViewVectors* GetNEOViewVectors() const;

	virtual void ClientSettingsChanged(CBasePlayer *pPlayer);

	float GetMapRemainingTime();

	void CleanUpMap();
	void CheckRestartGame();
#ifdef CLIENT_DLL
	void RestartGame();
#else
	virtual void RestartGame();
#endif

	enum
	{
		NEO_VICTORY_GHOST_CAPTURE = 0,
		NEO_VICTORY_TEAM_ELIMINATION,
		NEO_VICTORY_TIMEOUT_WIN_BY_NUMBERS,
		NEO_VICTORY_FORFEIT
	};

#ifdef GAME_DLL
private:
	CUtlVector<int> m_pGhostCaps;
#endif
};

inline CNEORules *NEORules()
{
	return static_cast<CNEORules*>(g_pGameRules);
}

#endif // NEO_GAMERULES_H