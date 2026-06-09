#pragma once

#include "neo_gamerules.h"
#include "GameEventListener.h"

#ifdef CLIENT_DLL
	#define CNEORulesCTG C_NEORulesCTG
	#define CNEOGameRulesCTGProxy C_NEOGameRulesCTGProxy
#endif

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
	
	// IGameEventListener interface:
	virtual void FireGameEvent(IGameEvent *event) override;
	
	virtual int GetGameType() override final { return NEO_GAME_TYPE_CTG; }
	virtual const char* GetGameTypeName() override { return "CTG"; }
	const char* GetGameDescription() override { return "Capture the Ghost"; }
	virtual bool GetTeamPlayEnabled() const override { return true; }
	virtual bool GetCompEnabled() const override final { return true; }
	virtual bool GetCapPreventEnabled() const override final { return true; }
	virtual bool CanChangeTeamClassLoadoutWhenAlive() const override final { return false; }
	virtual bool RespawnsEnabled() const override final { return false; }

	virtual float GetRoundRemainingTime() const override final;
#ifdef GAME_DLL
	virtual void SetGameRelatedVars() override final;
	virtual const int GetScoreLimit() const override final;
	virtual const int GetRoundLimit() const override final;
	virtual void CheckOvertime();
	virtual void RoundTimeout() override final;
#endif // GAME_DLL
	virtual void Think() override final;
};

inline CNEORulesCTG *NEORulesCTG()
{
	return static_cast<CNEORulesCTG*>(g_pGameRules);
}