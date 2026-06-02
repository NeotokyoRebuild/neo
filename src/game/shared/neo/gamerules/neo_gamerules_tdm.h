#pragma once

#include "neo_gamerules.h"
#include "GameEventListener.h"

#ifdef CLIENT_DLL
	#define CNEORulesTDM C_NEORulesTDM
	#define CNEOGameRulesTDMProxy C_NEOGameRulesTDMProxy
#endif
//
//ConVar sv_neo_tdm_score_limit("sv_neo_tdm_score_limit", "1", FCVAR_REPLICATED, "TDM score limit", true, 0.0f, true, 99.0f);
//ConVar sv_neo_tdm_round_limit("sv_neo_tdm_round_limit", "0", FCVAR_REPLICATED, "TDM max amount of rounds, 0 for no limit.", true, 0.0f, false, 0.0f);
//ConVar sv_neo_tdm_round_timelimit("neo_tdm_round_timelimit", "10.25", FCVAR_REPLICATED, "TDM round timelimit, in minutes.",	true, 0.0f, false, 600.0f);

class CNEOGameRulesTDMProxy : public CNEOGameRulesProxy
{
public:
	DECLARE_CLASS( CNEOGameRulesTDMProxy, CNEOGameRulesProxy );
	DECLARE_NETWORKCLASS();
};

class CNEORulesTDM : public CNEORules, public CGameEventListener
{
public:
	DECLARE_CLASS(CNEORulesTDM, CNEORules);
	DECLARE_NETWORKCLASS_NOBASE();
	

	CNEORulesTDM();
	virtual ~CNEORulesTDM();
	
	// IGameEventListener interface:
	virtual void FireGameEvent(IGameEvent *event) override;

	virtual void Think() override final;
#ifdef GAME_DLL
	virtual void PlayerRespawnThink() override final;
#endif // GAME_DLL
};

inline CNEORulesTDM *NEORulesTDM()
{
	return static_cast<CNEORulesTDM*>(g_pGameRules);
}