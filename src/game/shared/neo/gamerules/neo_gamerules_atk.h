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
	
	virtual void CheckOvertime();
	virtual void Think() override final;
};

inline CNEORulesATK *CNEORulesATK()
{
	return static_cast<CNEORulesATK*>(g_pGameRules);
}