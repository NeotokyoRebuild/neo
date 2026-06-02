#pragma once

#include "neo_gamerules.h"
#include "GameEventListener.h"

#ifdef CLIENT_DLL
	#define CNEORulesJGR C_NEORulesJGR
	#define CNEOGameRulesJGRProxy C_NEOGameRulesJGRProxy
#endif
//
//ConVar sv_neo_jgr_score_limit("sv_neo_jgr_score_limit", "1", FCVAR_REPLICATED, "JGR score limit", true, 0.0f, true, 99.0f);
//ConVar sv_neo_jgr_round_limit("sv_neo_jgr_round_limit", "0", FCVAR_REPLICATED, "JGR max amount of rounds, 0 for no limit.", true, 0.0f, false, 0.0f);
//ConVar sv_neo_jgr_round_timelimit("neo_jgr_round_timelimit", "10.25", FCVAR_REPLICATED, "JGR round timelimit, in minutes.",	true, 0.0f, false, 600.0f);

class CNEOGameRulesJGRProxy : public CNEOGameRulesProxy
{
public:
	DECLARE_CLASS( CNEOGameRulesJGRProxy, CNEOGameRulesProxy );
	DECLARE_NETWORKCLASS();
};

class CNEORulesJGR : public CNEORules, public CGameEventListener
{
public:
	DECLARE_CLASS(CNEORulesJGR, CNEORules);
	DECLARE_NETWORKCLASS_NOBASE();
	/*

	CNEORulesJGR();
	virtual ~CNEORulesJGR();
	*/
	// IGameEventListener interface:
	virtual void FireGameEvent(IGameEvent *event) override;

	virtual void Think() override final;
#ifdef GAME_DLL
	virtual void PlayerRespawnThink() override final;
#endif // GAME_DLL
};

inline CNEORulesJGR *NEORulesJGR()
{
	return static_cast<CNEORulesJGR*>(g_pGameRules);
}