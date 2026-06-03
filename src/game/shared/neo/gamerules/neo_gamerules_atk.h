#pragma once

#include "neo_gamerules.h"
#include "GameEventListener.h"

#ifdef CLIENT_DLL
	#define CNEORulesATK C_NEORulesATK
	#define CNEOGameRulesATKProxy C_NEOGameRulesATKProxy
#endif
//
//ConVar sv_neo_atk_score_limit("sv_neo_atk_score_limit", "1", FCVAR_REPLICATED, "ATK score limit", true, 0.0f, true, 99.0f);
//ConVar sv_neo_atk_round_limit("sv_neo_atk_round_limit", "0", FCVAR_REPLICATED, "ATK max amount of rounds, 0 for no limit.", true, 0.0f, false, 0.0f);
//ConVar sv_neo_atk_round_timelimit("neo_atk_round_timelimit", "10.25", FCVAR_REPLICATED, "ATK round timelimit, in minutes.",	true, 0.0f, false, 600.0f);

class CNEOGameRulesATKProxy : public CNEOGameRulesProxy
{
public:
	DECLARE_CLASS( CNEOGameRulesATKProxy, CNEOGameRulesProxy );
	DECLARE_NETWORKCLASS();
};

class CNEORulesATK : public CNEORules, public CGameEventListener
{
public:
	DECLARE_CLASS(CNEORulesATK, CNEORules);
	DECLARE_NETWORKCLASS_NOBASE();
	
	//CNEORulesATK();
	//virtual ~CNEORulesATK();
	
	// IGameEventListener interface:
	virtual void FireGameEvent(IGameEvent *event) override;

	const char* GetGameDescription() override { return "Attack/Defend"; }
	virtual bool GetTeamPlayEnabled() const override { return true; };
	
	virtual void CheckOvertime();
	virtual float GetRoundRemainingTime() const override final;
#ifdef GAME_DLL
	virtual void SetGameRelatedVars() override final;
	virtual const int GetScoreLimit() const override final;
	virtual const int GetRoundLimit() const override final;
	virtual void RoundTimeout() override final;
#endif // GAME_DLL
	virtual void Think() override final;
};

inline CNEORulesATK *NEORulesATK()
{
	return static_cast<CNEORulesATK*>(g_pGameRules);
}