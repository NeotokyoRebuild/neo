#pragma once

#include "neo_gamerules.h"
#include "GameEventListener.h"

#ifdef CLIENT_DLL
	#define CNEORulesVIP C_NEORulesVIP
	#define CNEOGameRulesVIPProxy C_NEOGameRulesVIPProxy
#endif

class CNEOGameRulesVIPProxy : public CNEOGameRulesProxy
{
public:
	DECLARE_CLASS( CNEOGameRulesVIPProxy, CNEOGameRulesProxy );
	DECLARE_NETWORKCLASS();
};

class CNEORulesVIP : public CNEORules
{
public:
	DECLARE_CLASS(CNEORulesVIP, CNEORules);
	DECLARE_NETWORKCLASS_NOBASE();
	
	// IGameEventListener interface:
	virtual void FireGameEvent(IGameEvent *event) override;
	
	virtual int GetGameType() override final { return NEO_GAME_TYPE_VIP; }
	virtual const char* GetGameTypeName() override final { return "VIP"; }
	const char* GetGameDescription() override final { return "Protect or Eliminate the VIP"; }
	virtual float GetRoundRemainingTime() const override final;
	virtual bool GetTeamPlayEnabled() const override final { return true; }
	virtual bool GetCompEnabled() const override final { return true; }
	virtual bool GetCapPreventEnabled() const override final { return true; }
	virtual bool CanChangeTeamClassLoadoutWhenAlive() const override final { return false; }
	virtual bool RespawnsEnabled() const override final { return false; }

#ifdef GAME_DLL
	virtual void SetGameRelatedVars() override final;
	virtual const int GetScoreLimit() const override final;
	virtual const int GetRoundLimit() const override final;
#endif // GAME_DLL
	virtual void Think() override final;
};

inline CNEORulesVIP *NEORulesVIP()
{
	return static_cast<CNEORulesVIP*>(g_pGameRules);
}
