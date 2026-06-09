#pragma once

#include "neo_gamerules.h"
#include "GameEventListener.h"

#ifdef CLIENT_DLL
	#define CNEORulesTDM C_NEORulesTDM
	#define CNEOGameRulesTDMProxy C_NEOGameRulesTDMProxy
#endif

class CNEOGameRulesTDMProxy : public CNEOGameRulesProxy
{
public:
	DECLARE_CLASS( CNEOGameRulesTDMProxy, CNEOGameRulesProxy );
	DECLARE_NETWORKCLASS();
};

class CNEORulesTDM : public CNEORules, public CGameEventListener
{
public:
	DECLARE_CLASS(CNEORulesTDM, CNEORules);
	DECLARE_NETWORKCLASS_NOBASE();
	
	// IGameEventListener interface:
	virtual void FireGameEvent(IGameEvent *event) override;
	
	virtual int GetGameType() override final { return NEO_GAME_TYPE_TDM; }
	virtual const char* GetGameTypeName() override final { return "TDM"; }
	const char* GetGameDescription() override final { return "Team Deathmatch"; }
	virtual bool GetTeamPlayEnabled() const override final { return true; }
	virtual bool GetCompEnabled() const override final { return false; }
	virtual bool GetCapPreventEnabled() const override final { return false; }
	virtual bool CanChangeTeamClassLoadoutWhenAlive() const override final { return false; }
	virtual bool RespawnsEnabled() const override final { return true; }

	virtual float GetRoundRemainingTime() const override final;
#ifdef GAME_DLL
	virtual bool FPlayerCanRespawn(CBasePlayer* pPlayer) override final;
	virtual bool PlayerCanChangeLoadout(CNEO_Player* pPlayer) override final;

	virtual void SetGameRelatedVars() override final;
	virtual const int GetScoreLimit() const override final;
	virtual const int GetRoundLimit() const override final;
	virtual void RoundTimeout() override final;
#endif // GAME_DLL
	virtual void Think() override final;
#ifdef GAME_DLL
	virtual void PlayerRespawnThink() override final;
#endif // GAME_DLL
};

inline CNEORulesTDM *NEORulesTDM()
{
	return static_cast<CNEORulesTDM*>(g_pGameRules);
}