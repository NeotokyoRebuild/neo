#pragma once

#include "neo_gamerules.h"
#include "GameEventListener.h"

#ifdef CLIENT_DLL
	#define CNEORulesEMT C_NEORulesEMT
	#define CNEOGameRulesEMTProxy C_NEOGameRulesEMTProxy
#endif

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
	
	// IGameEventListener interface:
	virtual void FireGameEvent(IGameEvent *event) override;

	virtual int GetGameType() override final { return NEO_GAME_TYPE_EMT; }
	virtual const char* GetGameTypeName() override final { return "EMT"; }
	const char* GetGameDescription() override final { return "Empty Gamemode"; }
	virtual float GetRoundRemainingTime() const override final;
	virtual bool GetTeamPlayEnabled() const override final { return true; }
	virtual bool GetCompEnabled() const override final { return false; }
	virtual bool GetCapPreventEnabled() const override final { return false; }
	virtual bool CanChangeTeamClassLoadoutWhenAlive() const override final { return true; }
	virtual bool RespawnsEnabled() const override final { return true; }

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