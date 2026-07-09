#pragma once

#include "neo_gamerules.h"
#include "GameEventListener.h"

#ifdef CLIENT_DLL
	#define CNEORulesTUT C_NEORulesTUT
	#define CNEOGameRulesTUTProxy C_NEOGameRulesTUTProxy
#endif

class CNEOGameRulesTUTProxy : public CNEOGameRulesProxy
{
public:
	DECLARE_CLASS( CNEOGameRulesTUTProxy, CNEOGameRulesProxy );
	DECLARE_NETWORKCLASS();
};

class CNEORulesTUT : public CNEORules
{
public:
	DECLARE_CLASS(CNEORulesTUT, CNEORules);
	DECLARE_NETWORKCLASS_NOBASE();

	// IGameEventListener interface:
	virtual void FireGameEvent(IGameEvent *event) override;
	
	virtual int GetGameType() override final { return NEO_GAME_TYPE_TUT; }
	virtual const char* GetGameTypeName() override final { return "TUT"; }
	const char* GetGameDescription() override final { return "Training"; }
	virtual bool GetTeamPlayEnabled() const override final { return true; }
	virtual bool GetCompEnabled() const override final { return false; }
	virtual bool GetCapPreventEnabled() const override final { return false; }
	virtual bool CanChangeTeamClassLoadoutWhenAlive() const override final { return false; }
	virtual bool RespawnsEnabled() const override final { return true; }

	virtual float GetRoundRemainingTime() const override final;
#ifdef GAME_DLL
	virtual bool FPlayerCanRespawn(CBasePlayer* pPlayer) override final;

	virtual void SetGameRelatedVars() override final {};
	virtual const int GetScoreLimit() const override final;
	virtual const int GetRoundLimit() const override final;
	virtual void RoundTimeout() override final;
#endif // GAME_DLL
	virtual void Think() override final;
};

inline CNEORulesTUT *NEORulesTUT()
{
	return static_cast<CNEORulesTUT*>(g_pGameRules);
}
