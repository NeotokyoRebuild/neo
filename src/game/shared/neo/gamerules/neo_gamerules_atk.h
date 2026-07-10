#pragma once

#include "neo_gamerules.h"
#include "GameEventListener.h"

#ifdef CLIENT_DLL
	#define CNEORulesATK C_NEORulesATK
	#define CNEOGameRulesATKProxy C_NEOGameRulesATKProxy
#endif

class CNEOGameRulesATKProxy : public CNEOGameRulesProxy
{
public:
	DECLARE_CLASS( CNEOGameRulesATKProxy, CNEOGameRulesProxy );
	DECLARE_NETWORKCLASS();
};

class CNEORulesATK : public CNEORules
{
public:
	DECLARE_CLASS(CNEORulesATK, CNEORules);
	DECLARE_NETWORKCLASS_NOBASE();
	
	// IGameEventListener interface:
	virtual void FireGameEvent(IGameEvent *event) override;

	virtual int GetGameType() override final { return NEO_GAME_TYPE_ATK; }
	virtual const char* GetGameTypeName() override final { return "ATK"; }
	const char* GetGameDescription() override final { return "Attack / Defend the ghost"; }
	virtual bool GetTeamPlayEnabled() const override final { return true; }
	virtual bool GetCompEnabled() const override final { return true; }
	virtual bool GetCapPreventEnabled() const override final { return false; }
	virtual bool CanChangeTeamClassLoadoutWhenAlive() const override final { return false; }
	virtual bool RespawnsEnabled() const override final { return false; }
	
	virtual float GetRoundRemainingTime() const override final;
#ifdef GAME_DLL
	virtual void SetGameRelatedVars() override final;
	virtual const int GetScoreLimit() const override final;
	virtual const int GetRoundLimit() const override final;
	virtual void CheckOvertime() override final;
	virtual void RoundTimeout() override final;
#endif // GAME_DLL
	virtual void Think() override final;
};

inline CNEORulesATK *NEORulesATK()
{
	return static_cast<CNEORulesATK*>(g_pGameRules);
}
