#pragma once

#include "neo_gamerules.h"
#include "GameEventListener.h"

#ifdef CLIENT_DLL
	#define CNEORulesTUT C_NEORulesTUT
	#define CNEOGameRulesTUTProxy C_NEOGameRulesTUTProxy
#endif
//
//ConVar sv_neo_tut_score_limit("sv_neo_tut_score_limit", "1", FCVAR_REPLICATED, "TUT score limit", true, 0.0f, true, 99.0f);
//ConVar sv_neo_tut_round_limit("sv_neo_tut_round_limit", "0", FCVAR_REPLICATED, "TUT max amount of rounds, 0 for no limit.", true, 0.0f, false, 0.0f);
//ConVar sv_neo_tut_round_timelimit("neo_tut_round_timelimit", "10.25", FCVAR_REPLICATED, "TUT round timelimit, in minutes.",	true, 0.0f, false, 600.0f);

class CNEOGameRulesTUTProxy : public CNEOGameRulesProxy
{
public:
	DECLARE_CLASS( CNEOGameRulesTUTProxy, CNEOGameRulesProxy );
	DECLARE_NETWORKCLASS();
};

class CNEORulesTUT : public CNEORules, public CGameEventListener
{
public:
	DECLARE_CLASS(CNEORulesTUT, CNEORules);
	DECLARE_NETWORKCLASS_NOBASE();
	

	CNEORulesTUT();
	virtual ~CNEORulesTUT();
	
	// IGameEventListener interface:
	virtual void FireGameEvent(IGameEvent *event) override;

	virtual void Think() override final;
};

inline CNEORulesTUT *NEORulesTUT()
{
	return static_cast<CNEORulesTUT*>(g_pGameRules);
}