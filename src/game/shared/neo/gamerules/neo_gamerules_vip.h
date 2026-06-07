#pragma once

#include "neo_gamerules.h"
#include "GameEventListener.h"

#ifdef CLIENT_DLL
	#define CNEORulesVIP C_NEORulesVIP
	#define CNEOGameRulesVIPProxy C_NEOGameRulesVIPProxy
#endif
//
//ConVar sv_neo_vip_score_limit("sv_neo_vip_score_limit", "1", FCVAR_REPLICATED, "VIP score limit", true, 0.0f, true, 99.0f);
//ConVar sv_neo_vip_round_limit("sv_neo_vip_round_limit", "0", FCVAR_REPLICATED, "VIP max amount of rounds, 0 for no limit.", true, 0.0f, false, 0.0f);
//ConVar sv_neo_vip_round_timelimit("neo_vip_round_timelimit", "10.25", FCVAR_REPLICATED, "VIP round timelimit, in minutes.",	true, 0.0f, false, 600.0f);

class CNEOGameRulesVIPProxy : public CNEOGameRulesProxy
{
public:
	DECLARE_CLASS( CNEOGameRulesVIPProxy, CNEOGameRulesProxy );
	DECLARE_NETWORKCLASS();
};

class CNEORulesVIP : public CNEORules, public CGameEventListener
{
public:
	DECLARE_CLASS(CNEORulesVIP, CNEORules);
	DECLARE_NETWORKCLASS_NOBASE();
	
	//CNEORulesVIP();
	//virtual ~CNEORulesVIP();
	
	// IGameEventListener interface:
	virtual void FireGameEvent(IGameEvent *event) override;
	
	virtual int GetGameType() override final { return NEO_GAME_TYPE_VIP; }
	virtual const char* GetGameTypeName() override { return "VIP"; }
	const char* GetGameDescription() override { return "Protect or Eliminate the VIP"; }
	virtual float GetRoundRemainingTime() const override final;
	virtual bool GetTeamPlayEnabled() const override { return true; }
	virtual bool GetCompEnabled() const override final { return true; }
	virtual bool GetCapPreventEnabled() const override final { return true; }
	virtual bool CanChangeTeamClassLoadoutWhenAlive() const override final { return false; }
	virtual bool CanRespawnAnyTime() const override final { return false; }

#ifdef GAME_DLL
	virtual void SetGameRelatedVars() override final;
	virtual const int GetScoreLimit() const override final;
	virtual const int GetRoundLimit() const override final;
	//virtual void RoundTimeout() override final;
#endif // GAME_DLL
	virtual void Think() override final;
};

inline CNEORulesVIP *NEORulesVIP()
{
	return static_cast<CNEORulesVIP*>(g_pGameRules);
}