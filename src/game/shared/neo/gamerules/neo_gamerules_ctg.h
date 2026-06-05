#pragma once

#include "neo_gamerules.h"
#include "GameEventListener.h"

#ifdef CLIENT_DLL
	#define CNEORulesCTG C_NEORulesCTG
	#define CNEOGameRulesCTGProxy C_NEOGameRulesCTGProxy
#endif
//
//ConVar sv_neo_ctg_score_limit("sv_neo_ctg_score_limit", "1", FCVAR_REPLICATED, "CTG score limit", true, 0.0f, true, 99.0f);
//ConVar sv_neo_ctg_round_limit("sv_neo_ctg_round_limit", "0", FCVAR_REPLICATED, "CTG max amount of rounds, 0 for no limit.", true, 0.0f, false, 0.0f);
//ConVar sv_neo_ctg_round_timelimit("neo_ctg_round_timelimit", "10.25", FCVAR_REPLICATED, "CTG round timelimit, in minutes.",	true, 0.0f, false, 600.0f);

class CNEOGameRulesCTGProxy : public CNEOGameRulesProxy
{
public:
	DECLARE_CLASS( CNEOGameRulesCTGProxy, CNEOGameRulesProxy );
	DECLARE_NETWORKCLASS();
};

class CNEORulesCTG : public CNEORules, public CGameEventListener
{
public:
	DECLARE_CLASS(CNEORulesCTG, CNEORules);
	DECLARE_NETWORKCLASS_NOBASE();
	
	//CNEORulesCTG();
	//virtual ~CNEORulesCTG();
	
	// IGameEventListener interface:
	virtual void FireGameEvent(IGameEvent *event) override;
	
	virtual int GetGameType() override final { return NEO_GAME_TYPE_CTG; }
	virtual const char* GetGameTypeName() override { return "CTG"; }
	const char* GetGameDescription() override { return "Capture the Ghost"; }
	virtual bool GetTeamPlayEnabled() const override { return true; }
	virtual bool GetCompEnabled() const override final { return true; }
	virtual bool GetCapPreventEnabled() const override final { return true; }
	virtual bool CanChangeTeamClassLoadoutWhenAlive() const override final { return false; }
	virtual bool CanRespawnAnyTime() const override final { return false; }

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

inline CNEORulesCTG *NEORulesCTG()
{
	return static_cast<CNEORulesCTG*>(g_pGameRules);
}