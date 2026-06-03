#pragma once

#include "neo_gamerules.h"
#include "GameEventListener.h"

#ifdef CLIENT_DLL
	#define CNEORulesDM C_NEORulesDM
	#define CNEOGameRulesDMProxy C_NEOGameRulesDMProxy
#endif
//
//ConVar sv_neo_dm_score_limit("sv_neo_dm_score_limit", "1", FCVAR_REPLICATED, "DM score limit", true, 0.0f, true, 99.0f);
//ConVar sv_neo_dm_round_limit("sv_neo_dm_round_limit", "0", FCVAR_REPLICATED, "DM max amount of rounds, 0 for no limit.", true, 0.0f, false, 0.0f);
//ConVar sv_neo_dm_round_timelimit("neo_dm_round_timelimit", "10.25", FCVAR_REPLICATED, "DM round timelimit, in minutes.",	true, 0.0f, false, 600.0f);

class CNEOGameRulesDMProxy : public CNEOGameRulesProxy
{
public:
	DECLARE_CLASS( CNEOGameRulesDMProxy, CNEOGameRulesProxy );
	DECLARE_NETWORKCLASS();
};

class CNEORulesDM : public CNEORules, public CGameEventListener
{
public:
	DECLARE_CLASS(CNEORulesDM, CNEORules);
	DECLARE_NETWORKCLASS_NOBASE();
	
	//CNEORulesDM();
	//virtual ~CNEORulesDM();

	// IGameEventListener interface:
	virtual void FireGameEvent(IGameEvent *event) override;
	
	const char* GetGameDescription() override { return "Deathmatch"; }
	virtual bool GetTeamPlayEnabled() const override { return false; };

	virtual float GetRoundRemainingTime() const override final;
#ifdef GAME_DLL
	virtual bool FPlayerCanRespawn(CBasePlayer* pPlayer) override final;

	virtual void SetGameRelatedVars() override final;
	virtual const int GetScoreLimit() const override final;
	virtual const int GetRoundLimit() const override final;
	virtual void RoundTimeout() override final;
#endif // GAME_DLL
	virtual void Think() override final;
#ifdef GAME_DLL
	virtual void PlayerRespawnThink() override final;
#endif // GAME_DLL
	
	void GetDMHighestScorers(
#ifdef GAME_DLL
			CNEO_Player *(*pHighestPlayers)[MAX_PLAYERS + 1],
#endif
			int *iHighestPlayersTotal,
			int *iHighestXP) const;
#ifdef GAME_DLL
	void SetWinningDMPlayer(CNEO_Player *pWinner);
#endif // GAME_DLL
};

inline CNEORulesDM *NEORulesDM()
{
	return static_cast<CNEORulesDM*>(g_pGameRules);
}