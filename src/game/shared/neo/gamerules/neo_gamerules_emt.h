#pragma once

#include "neo_gamerules.h"
#include "GameEventListener.h"

#ifdef CLIENT_DLL
	#define CNEORulesEMT C_NEORulesEMT
	#define CNEOGameRulesEMTProxy C_NEOGameRulesEMTProxy
#endif
//
//ConVar sv_neo_emt_score_limit("sv_neo_emt_score_limit", "1", FCVAR_REPLICATED, "EMT score limit", true, 0.0f, true, 99.0f);
//ConVar sv_neo_emt_round_limit("sv_neo_emt_round_limit", "0", FCVAR_REPLICATED, "EMT max amount of rounds, 0 for no limit.", true, 0.0f, false, 0.0f);
//ConVar sv_neo_emt_round_timelimit("neo_emt_round_timelimit", "10.25", FCVAR_REPLICATED, "EMT round timelimit, in minutes.",	true, 0.0f, false, 600.0f);

class CNEOGameRulesEMTProxy : public CNEOGameRulesProxy
{
public:
	DECLARE_CLASS( CNEOGameRulesEMTProxy, CNEOGameRulesProxy );
	DECLARE_NETWORKCLASS();
};

class CNEORulesEMT : public CNEORules, public CGameEventListener
{
public:
	DECLARE_CLASS(CNEORulesEMT, CNEORules);
	DECLARE_NETWORKCLASS_NOBASE();
	
	//CNEORulesEMT();
	//virtual ~CNEORulesEMT();
	
	// IGameEventListener interface:
	virtual void FireGameEvent(IGameEvent *event) override;


	
	virtual float GetRoundRemainingTime() const override final;
	virtual bool GetTeamPlayEnabled() const override { return true; };

#ifdef GAME_DLL
	virtual bool FPlayerCanRespawn(CBasePlayer* pPlayer) override final;

	virtual void SetGameRelatedVars() override final {};
	virtual const int GetScoreLimit() const override final;
	virtual const int GetRoundLimit() const override final;
	virtual void RoundTimeout() override final;
#endif // GAME_DLL
	virtual void Think() override final;
};

inline CNEORulesEMT *NEORulesEMT()
{
	return static_cast<CNEORulesEMT*>(g_pGameRules);
}