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
	

	CNEORulesCTG();
	virtual ~CNEORulesCTG();
	
	// IGameEventListener interface:
	virtual void FireGameEvent(IGameEvent *event) override;

	virtual void CheckOvertime();
	virtual void Think() override final;
};

inline CNEORulesCTG *NEORulesCTG()
{
	return static_cast<CNEORulesCTG*>(g_pGameRules);
}