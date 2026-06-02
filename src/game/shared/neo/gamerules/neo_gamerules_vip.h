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
	

	CNEORulesVIP();
	virtual ~CNEORulesVIP();
	
	// IGameEventListener interface:
	virtual void FireGameEvent(IGameEvent *event) override;

	virtual void Think() override final;
};

inline CNEORulesVIP *NEORulesVIP()
{
	return static_cast<CNEORulesVIP*>(g_pGameRules);
}